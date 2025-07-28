/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT can_termination_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(can_termination_gpio, CONFIG_CAN_LOG_LEVEL);

struct can_termination_gpio_config {
  struct gpio_dt_spec enable_gpio;
  bool enable_on_boot;
};

static int can_termination_gpio_init(const struct device *dev) {
  const struct can_termination_gpio_config *config = dev->config;
  int err;

  if (config->enable_gpio.port != NULL) {
    if (!gpio_is_ready_dt(&config->enable_gpio)) {
      LOG_ERR("enable pin GPIO device not ready");
      return -EINVAL;
    }

    err = gpio_pin_configure_dt(
        &config->enable_gpio,
        config->enable_on_boot ? GPIO_OUTPUT_ACTIVE : GPIO_OUTPUT_INACTIVE);
    if (err != 0) {
      LOG_ERR("failed to configure enable GPIO pin (err %d)", err);
      return err;
    }

    if (config->enable_on_boot) {
      LOG_INF("Enabled termination %s", dev->name);
    }
  }
  return 0;
}

#define CAN_TERMINATION_GPIO_INIT(inst)                                   \
  static const struct can_termination_gpio_config                         \
      can_termination_gpio_config_##inst = {                              \
        .enable_gpio = GPIO_DT_SPEC_INST_GET(inst, enable_gpios),         \
        .enable_on_boot = DT_INST_PROP_OR(inst, enable_on_boot, false),   \
  };                                                                      \
                                                                          \
  DEVICE_DT_INST_DEFINE(inst, &can_termination_gpio_init, NULL, NULL,     \
                        &can_termination_gpio_config_##inst, POST_KERNEL, \
                        CONFIG_CAN_TERMINATION_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(CAN_TERMINATION_GPIO_INIT)
