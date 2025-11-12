/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME app
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

#define CANBUS_NODE DT_CHOSEN(zephyr_canbus)

#define GPIO_CONFIG_FRAME_ID CONFIG_GPIO_EXPANDER_CONFIG_FRAME_ID
#define GPIO_STATE_FRAME_ID CONFIG_GPIO_EXPANDER_STATE_FRAME_ID
#define GPIO_WRITE_FRAME_ID CONFIG_GPIO_EXPANDER_WRITE_FRAME_ID

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct gpio_dt_spec gpios[] = {
  // all gpios defined in the zephyr_user node
  DT_FOREACH_PROP_ELEM_SEP(
      ZEPHYR_USER_NODE, gpios, GPIO_DT_SPEC_GET_BY_IDX, (, ))};

static uint8_t config_frame_data[8] = {0};
struct k_work configure_gpios_work;

bool is_gpio_output(int pin) {
  return (config_frame_data[pin / 8] >> (pin % 8)) & 0x01;
}

static uint8_t write_frame_data[8] = {0};
struct k_work write_gpios_work;

int output_level(int pin) {
  return (write_frame_data[pin / 8] >> (pin % 8)) & 0x01;
}

int init_can(const struct device *can_dev) {
  int err;

  if (!device_is_ready(can_dev)) {
    LOG_ERR("CAN device not ready");
    return -ENODEV;
  }

  err = can_start(can_dev);
  if (err != 0) {
    LOG_ERR("Error starting CAN controller (err %d)", err);
    return err;
  }

  return 0;
}

struct k_sem tx_queue_sem;

static void can_tx_callback(const struct device *dev,
                            int error,
                            void *user_data) {
  struct k_sem *tx_queue_sem = user_data;

  k_sem_give(tx_queue_sem);
}

int send_state_update_frame(const struct device *can_dev) {
  struct can_frame frame = {0};

  frame.id = GPIO_STATE_FRAME_ID;

  memset(frame.data, 0, sizeof(frame.data));

  for (int i = 0; i < ARRAY_SIZE(gpios); i++) {
    if (is_gpio_output(i)) {
      frame.data[i / 8] |= (output_level(i) << (i % 8));
    } else {
      int value = gpio_pin_get_dt(&gpios[i]);

      if (value < 0) {
        LOG_ERR("Error reading GPIO pin (err %d)", value);
        return value;
      }

      frame.data[i / 8] |= (value << (i % 8));
    }
  }

  frame.dlc = 8;

  return can_send(can_dev, &frame, K_NO_WAIT, can_tx_callback, &tx_queue_sem);
}

const struct can_filter receive_config_filter = {
  .flags = 0,
  .id = GPIO_CONFIG_FRAME_ID,
  .mask = CAN_STD_ID_MASK,
};

void receive_config_callback(const struct device *dev,
                             struct can_frame *frame,
                             void *user_data) {
  memcpy(config_frame_data, frame->data, frame->dlc);
  k_work_submit(&configure_gpios_work);
}

void configure_gpios(struct k_work *item) {
  LOG_INF("Configuring GPIOs");

  for (int i = 0; i < ARRAY_SIZE(gpios); i++) {
    int err = gpio_pin_configure_dt(
        &gpios[i], (is_gpio_output(i) ? GPIO_OUTPUT : GPIO_INPUT));
    if (err != 0) {
      LOG_ERR("Error configuring GPIO pin (err %d)", err);
      return;
    }
  }
}

const struct can_filter receive_write_filter = {
  .flags = 0,
  .id = GPIO_WRITE_FRAME_ID,
  .mask = CAN_STD_ID_MASK,
};

void receive_write_callback(const struct device *dev,
                            struct can_frame *frame,
                            void *user_data) {
  memcpy(write_frame_data, frame->data, frame->dlc);
  k_work_submit(&write_gpios_work);
}

void write_gpios(struct k_work *item) {
  LOG_INF("Writing GPIOs");

  for (int i = 0; i < ARRAY_SIZE(gpios); i++) {
    if (!is_gpio_output(i)) {
      continue;
    }

    int err = gpio_pin_set_dt(&gpios[i], output_level(i));
    if (err != 0) {
      LOG_ERR("Error writing GPIO pin (err %d)", err);
      return;
    }
  }
}

int configure_receive_filters(const struct device *can_dev) {
  int filter_id;

  filter_id = can_add_rx_filter(can_dev, receive_config_callback, NULL,
                                &receive_config_filter);
  if (filter_id < 0) {
    LOG_ERR("Error adding receive filter (err %d)", filter_id);
    return filter_id;
  }

  filter_id = can_add_rx_filter(can_dev, receive_write_callback, NULL,
                                &receive_write_filter);
  if (filter_id < 0) {
    LOG_ERR("Error adding receive filter (err %d)", filter_id);
    return filter_id;
  }

  return 0;
}

int init(const struct device *can_dev) {
  k_sem_init(&tx_queue_sem, 1, 1);
  k_work_init(&configure_gpios_work, configure_gpios);
  k_work_init(&write_gpios_work, write_gpios);

  configure_gpios(NULL);

  int err;
  err = init_can(can_dev);

  if (err != 0) {
    LOG_ERR("Error initializing CAN controller 1 (err %d)", err);
    return err;
  }

  err = configure_receive_filters(can_dev);

  if (err != 0) {
    LOG_ERR("Error configuring CAN receive filters (err %d)", err);
    return err;
  }

  return 0;
}

int main(void) {
  const struct device *can_dev = DEVICE_DT_GET(CANBUS_NODE);
  int err = init(can_dev);

  if (err != 0) {
    LOG_ERR("Error initializing application (err %d)", err);
  }

  while (1) {
    send_state_update_frame(can_dev);
    k_sleep(K_MSEC(100));
  }
}
