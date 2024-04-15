/*
 * Copyright (c) 2024 Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/devicetree.h"
#include "zephyr/drivers/gpio.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys/printk.h"

#include <stdio.h>

#include <zephyr/kernel.h>

#define LOG_MODULE_NAME tester
LOG_MODULE_DECLARE(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct gpio_dt_spec gpios[] = {DT_FOREACH_PROP_ELEM_SEP(
    ZEPHYR_USER_NODE, gpios, GPIO_DT_SPEC_GET_BY_IDX, (, ))};

static void configure_gpios() {
  // all pins as output
  for (int i = 0; i < ARRAY_SIZE(gpios); i++) {
    int ret = gpio_pin_configure_dt(&gpios[i], GPIO_INPUT);
    if (ret != 0) {
      LOG_ERR("failed to configure pin %d of device %s", gpios[i].pin,
              gpios[i].port->name);
    }
  }
}

static void execute_test() {
  for (int i = 0; i < ARRAY_SIZE(gpios); i++) {
    // save start timestamp
    uint32_t start = k_uptime_get_32();

    // wait for pin to get active
    while (gpio_pin_get(gpios[i].port, gpios[i].pin) == 0) {
      k_msleep(1);

      // timeout after 600ms
      if (k_uptime_get_32() - start > 600) {
        LOG_ERR("pin %d of device %s did not get active", gpios[i].pin,
                gpios[i].port->name);
        break;
      }
    }

    k_msleep(3);

    // validate that all other pins are inactive
    for (int j = 0; j < ARRAY_SIZE(gpios); j++) {
      if (i == j) {
        continue;
      }

      if (gpio_pin_get(gpios[j].port, gpios[j].pin) != 0) {
        LOG_ERR("pin %d of device %s is active", gpios[j].pin,
                gpios[j].port->name);
      }
    }
  }
}

void gpio_test(void) {
  k_msleep(100);
  configure_gpios();
  execute_test();
}
