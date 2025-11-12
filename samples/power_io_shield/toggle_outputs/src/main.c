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

int main() {
  if (!device_is_ready(power_io_shield)) {
    LOG_ERR("HV Shield device not ready");
    return 1;
  }

  LOG_INF("Initializing input GPIOs...");
  for (size_t i = 0; i < ARRAY_SIZE(input_gpios); i++) {
    int ret = gpio_pin_configure_dt(&input_gpios[i], GPIO_INPUT);
    if (ret != 0) {
      LOG_ERR("Failed to configure input GPIO pin %d: %d", input_gpios[i].pin,
              ret);
      return 1;
    }
  }

  LOG_INF("Initializing output GPIOs...");
  for (size_t i = 0; i < ARRAY_SIZE(output_gpios); i++) {
    int ret = gpio_pin_configure_dt(&output_gpios[i], GPIO_OUTPUT_LOW);
    if (ret != 0) {
      LOG_ERR("Failed to configure output GPIO pin %d: %d", output_gpios[i].pin,
              ret);
      return 1;
    }
  }

  LOG_INF("Initializing fault GPIOs...");
  for (size_t i = 0; i < ARRAY_SIZE(fault_gpios); i++) {
    int ret = gpio_pin_configure_dt(&fault_gpios[i], GPIO_INPUT);
    if (ret != 0) {
      LOG_ERR("Failed to configure fault GPIO pin %d: %d", fault_gpios[i].pin,
              ret);
      return 1;
    }
  }

  LOG_INF("GPIOs initialized successfully.");

  LOG_INF("Entering main loop, toggling output GPIOs and logging inputs");

  uint8_t output_value = 0;
  for (;;) {
    printk("Output GPIO target: ");
    for (size_t i = 0; i < ARRAY_SIZE(output_gpios); i++) {
      bool value = (output_value >> i) & 1;
      printk("%d ", value);

      int ret = gpio_pin_set_dt(&output_gpios[i], value);
      if (ret != 0) {
        LOG_ERR("Failed to set output GPIO pin %d: %d", output_gpios[i].pin,
                ret);
      }
    }
    output_value++;
    printk("\n");

    printk("Input GPIO states:  ");
    for (size_t i = 0; i < ARRAY_SIZE(input_gpios); i++) {
      int ret = gpio_pin_get_dt(&input_gpios[i]);
      if (ret < 0) {
        LOG_ERR("Failed to read input GPIO pin %d: %d", input_gpios[i].pin,
                ret);
        continue;
      }
      printk("%d ", ret);
    }
    printk("\n");

    printk("Fault GPIO states:  ");
    for (size_t i = 0; i < ARRAY_SIZE(fault_gpios); i++) {
      int ret = gpio_pin_get_dt(&fault_gpios[i]);
      if (ret < 0) {
        LOG_ERR("Failed to read fault GPIO pin %d: %d", fault_gpios[i].pin,
                ret);
        continue;
      }
      printk("%d ", ret);
    }
    printk("\n");
    printk("\n");

    k_msleep(1000);
  }
}
