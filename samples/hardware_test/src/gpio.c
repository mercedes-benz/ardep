/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/devicetree.h"
#include "zephyr/drivers/gpio.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys/printk.h"

#include <zephyr/kernel.h>

#define LOG_MODULE_NAME sut
LOG_MODULE_DECLARE(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct gpio_dt_spec gpios[] = {DT_FOREACH_PROP_ELEM_SEP(
    ZEPHYR_USER_NODE, gpios, GPIO_DT_SPEC_GET_BY_IDX, (, ))};

static void gpio_ready_check() {
  LOG_INF("gpio ready check");
  for (int i = 0; i < ARRAY_SIZE(gpios); i++) {
    if (!gpio_is_ready_dt(&gpios[i])) {
      LOG_ERR("gpio device not ready: %s", gpios[i].port->name);
    }
  }
  LOG_INF("gpio ready check finished");
}

static void gpio_initialize() {
  LOG_INF("gpio configure");
  for (int i = 0; i < ARRAY_SIZE(gpios); i++) {
    int ret = gpio_pin_configure_dt(&gpios[i], GPIO_OUTPUT_INACTIVE);

    if (ret < 0) {
      LOG_ERR("gpio configure %s pin %d with error %d", gpios[i].port->name,
              gpios[i].pin, ret);
    }
  }

  LOG_INF("gpio configure finished");
}
static void gpio_toggle() {
  for (int i = 0; i < ARRAY_SIZE(gpios); i++) {
    LOG_INF("%s pin %d on", gpios[i].port->name, gpios[i].pin);
    gpio_pin_set_dt(&gpios[i], 1);
    k_msleep(10);
    gpio_pin_set_dt(&gpios[i], 0);
    k_msleep(10);
  }
}

static void disable_gpios() {
  for (int i = 0; i < ARRAY_SIZE(gpios); i++) {
    gpio_pin_set_dt(&gpios[i], 0);
  }
}

void gpio_test(void) {
  gpio_ready_check();
  gpio_initialize();
  k_msleep(300);
  gpio_toggle();
  k_msleep(100);
  disable_gpios();
}
