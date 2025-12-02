/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

#include <ardep/dt-bindings/power-io-shield.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

const struct device* console_uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

static const struct device* power_io_shield =
    DEVICE_DT_GET(DT_NODELABEL(power_io_shield0));

#define DEFINE_GPIO_ARRAY(node, prop)                                    \
  static const struct gpio_dt_spec prop[] = {                            \
    DT_FOREACH_PROP_ELEM_SEP(node, prop, GPIO_DT_SPEC_GET_BY_IDX, (, )), \
  };

DEFINE_GPIO_ARRAY(DT_PATH(zephyr_user), output_gpios);
DEFINE_GPIO_ARRAY(DT_PATH(zephyr_user), input_gpios);
DEFINE_GPIO_ARRAY(DT_PATH(zephyr_user), fault_gpios);

BUILD_ASSERT(ARRAY_SIZE(output_gpios) == ARRAY_SIZE(input_gpios),
             "Number of output and input GPIOs must match");

int assert_input_pins(const bool* expected_states) {
  for (size_t i = 0; i < ARRAY_SIZE(input_gpios); i++) {
    int val = gpio_pin_get(input_gpios[i].port, input_gpios[i].pin);
    if (val < 0) {
      LOG_ERR("Failed to read input GPIO %d: %d", i, val);
      return val;
    }

    bool expected = expected_states[i];
    if (val != (int)expected) {
      LOG_ERR("Input GPIO %d state %d does not match expected %d", i, val,
              expected);
      return -1;
    }
  }

  return 0;
}

int assert_fault_pins_low() {
  for (size_t i = 0; i < ARRAY_SIZE(fault_gpios); i++) {
    int val = gpio_pin_get(fault_gpios[i].port, fault_gpios[i].pin);
    if (val < 0) {
      LOG_ERR("Failed to read fault GPIO %d: %d", i, val);
      return val;
    }

    if (val != 0) {
      LOG_ERR("Fault GPIO %d is high, expected low", i);
      return -1;
    }
  }

  return 0;
}

int wait_for_uart_msg(const char* msg) {
  size_t msg_len = strlen(msg);
  size_t received = 0;

  while (received < msg_len) {
    uint8_t byte;
    int ret = uart_poll_in(console_uart, &byte);
    if (ret == 0) {
      if (byte == (uint8_t)msg[received]) {
        received++;
      } else {
        received = 0;  // reset on mismatch
      }
    }
    k_msleep(10);
  }

  return 0;
}

int main() {
  if (!device_is_ready(power_io_shield)) {
    LOG_ERR("HV Shield device not ready");
    return 1;
  }

  bool expected_input_states[ARRAY_SIZE(input_gpios)] = {0};

  LOG_INF("Waiting for host to say \"START\"");

  wait_for_uart_msg("START");

  LOG_INF("Starting end of line test");

  // setup all pins as low
  for (size_t i = 0; i < ARRAY_SIZE(output_gpios); i++) {
    int err = gpio_pin_configure_dt(&output_gpios[i], GPIO_OUTPUT_INACTIVE);
    if (err) {
      LOG_ERR("Failed to configure output GPIO %d: %d", i, err);
      return err;
    }
  }
  k_msleep(1);
  int err = assert_input_pins(expected_input_states);
  if (err) {
    LOG_ERR("Initial input pin state check failed");
    return err;
  }
  err = assert_fault_pins_low();
  if (err) {
    LOG_ERR("Initial fault pin state check failed");
    return err;
  }

  // Go through all output patterns and validate inputs and fault pins
  for (uint8_t pattern = 0; pattern < ((1 << 6) - 1); pattern++) {
    for (int i = 0; i < ARRAY_SIZE(output_gpios); i++) {
      bool state = (pattern & (1 << i)) != 0;
      expected_input_states[i] = state;

      int err = gpio_pin_set(output_gpios[i].port, output_gpios[i].pin, state);
      if (err) {
        LOG_ERR("Failed to set output GPIO %d to %d: %d", i, state, err);
        return err;
      }
    }
    k_msleep(1);
    err = assert_input_pins(expected_input_states);
    if (err) {
      LOG_ERR("Input pin state check failed for pattern 0x%02X", pattern);
      return err;
    }
    err = assert_fault_pins_low();
    if (err) {
      LOG_ERR("Fault pin state check failed for pattern 0x%02X", pattern);
      return err;
    }

    k_msleep(5);
  }

  // reset all outputs to low
  for (size_t i = 0; i < ARRAY_SIZE(output_gpios); i++) {
    int err = gpio_pin_set(output_gpios[i].port, output_gpios[i].pin, 0);
    if (err) {
      LOG_ERR("Failed to reset output GPIO %d: %d", i, err);
      return err;
    }
  }

  LOG_INF("End of line tester completed successfully");

  LOG_INF("Rebooting system in 5 seconds...");
  k_msleep(5000);
  sys_reboot(SYS_REBOOT_COLD);
}
