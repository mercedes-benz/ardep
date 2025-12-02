/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ardep/dt-bindings/power-io-shield.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

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

int main() {
  if (!device_is_ready(power_io_shield)) {
    LOG_ERR("HV Shield device not ready");
    return 1;
  }

  bool expected_input_states[ARRAY_SIZE(input_gpios)] = {0};

  LOG_INF("Starting End of line tester in 1s");
  k_sleep(K_SECONDS(1));
  // setup all pins as low
  for (size_t i = 0; i < ARRAY_SIZE(output_gpios); i++) {
    int err = gpio_pin_configure_dt(&output_gpios[i], GPIO_OUTPUT_INACTIVE);
    if (err) {
      LOG_ERR("Failed to configure output GPIO %d: %d", i, err);
      return err;
    }
  }
  int err = assert_input_pins(expected_input_states);
  if (err) {
    LOG_ERR("Initial input pin state check failed");
    return err;
  }
}
