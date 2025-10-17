/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME app
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

#define CANBUS_1_NODE DT_NODELABEL(can_a)
#define CANBUS_2_NODE DT_NODELABEL(can_b)

#define CAN_FRAME_ID CONFIG_CAN_EXAMPLE_FRAME_ID

static void can_tx_callback(const struct device *dev,
                            int error,
                            void *user_data) {
  struct k_sem *tx_queue_sem = user_data;

  k_sem_give(tx_queue_sem);
}

int init_can(const struct device *can_dev) {
  int err;

  if (!device_is_ready(can_dev)) {
    LOG_ERR("CAN device not ready");
    return -ENODEV;
  }

#if CONFIG_CAN_EXAMPLE_USE_CAN_FD
  err = can_set_mode(can_dev, CAN_MODE_FD);
  if (err != 0) {
    LOG_ERR("Error setting CAN-FD mode (err %d)", err);
    return err;
  }
#endif

  err = can_start(can_dev);
  if (err != 0) {
    LOG_ERR("Error starting CAN controller (err %d)", err);
    return err;
  }

  return 0;
}

struct k_sem tx_queue_sem;
int send_frame(const struct device *can_dev) {
  struct can_frame frame = {0};

  frame.id = CAN_FRAME_ID;

  static uint8_t data = 0;
  frame.data[0] = data++;
  frame.dlc = 1;
#if CONFIG_CAN_EXAMPLE_USE_CAN_FD
  memset(frame.data, data, sizeof(frame.data));
  frame.dlc = 15;
  frame.flags |= CAN_FRAME_FDF;
#endif

  return can_send(can_dev, &frame, K_NO_WAIT, can_tx_callback, &tx_queue_sem);
}

const struct can_filter receive_frame_filter = {
  .flags = 0,
  .id = CAN_FRAME_ID,
  .mask = CAN_STD_ID_MASK,
};

void rx_callback(const struct device *dev,
                 struct can_frame *frame,
                 void *user_data) {
  LOG_INF("Received CAN frame with ID %d \n", frame->id);
}

int main(void) {
  const struct device *can_dev_1 = DEVICE_DT_GET(CANBUS_1_NODE);
  const struct device *can_dev_2 = DEVICE_DT_GET(CANBUS_2_NODE);

  int err;

  k_sem_init(&tx_queue_sem, 1, 1);

  err = init_can(can_dev_1);

  if (err != 0) {
    LOG_ERR("Error initializing CAN controller 1 (err %d)", err);
    return err;
  }

  err = init_can(can_dev_2);

  if (err != 0) {
    LOG_ERR("Error initializing CAN controller 2 (err %d)", err);
    return err;
  }

  int filter_id =
      can_add_rx_filter(can_dev_2, rx_callback, NULL, &receive_frame_filter);

  if (filter_id < 0) {
    LOG_ERR("Error adding CAN filter (err %d)", filter_id);
    return filter_id;
  }

  LOG_INF("CAN example started");
#if CONFIG_CAN_EXAMPLE_USE_CAN_FD
  LOG_INF("Using CAN FD");
#endif

  LOG_INF("Sending via CAN-A: %s", can_dev_1->name);
  LOG_INF("Receiving via CAN-B: %s", can_dev_2->name);

  while (true) {
    if (k_sem_take(&tx_queue_sem, K_MSEC(100)) == 0) {
      err = send_frame(can_dev_1);

      if (err != 0) {
        LOG_ERR("failed to enqueue CAN frame (err %d)\n", err);
      }
    }

    k_msleep(100);
  }
}
