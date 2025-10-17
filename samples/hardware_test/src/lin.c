/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/logging/log.h"
#include "zephyr/sys/printk.h"

#include <zephyr/console/console.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#define LOG_MODULE_NAME sut
LOG_MODULE_DECLARE(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

#define DEVICE_DT_BY_PROP_IDX(node_id, prop, idx) \
  DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx))

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct device *const lin_devices[] = {DT_FOREACH_PROP_ELEM_SEP(
    ZEPHYR_USER_NODE, lins, DEVICE_DT_BY_PROP_IDX, (, ))};

#define UART_CREATE_FUNCTIONS_BY_IDX(node_id, prop, idx) \
  static void setup_uart_##idx() {                       \
    const struct device *dev = lin_devices[idx];         \
    if (!device_is_ready(dev)) {                         \
      LOG_ERR("%s: UART device not ready", dev->name);   \
      return;                                            \
    }                                                    \
  }                                                      \
                                                         \
  static void send_character_##idx() {                   \
    const struct device *dev = lin_devices[idx];         \
    for (int i = 0; i < 4; i++) {                        \
      LOG_INF("%s sending: f", dev->name);               \
      uart_poll_out(dev, 'f');                           \
      k_msleep(100);                                     \
    }                                                    \
  }

DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, lins, UART_CREATE_FUNCTIONS_BY_IDX);

#define UART_SETUP_BY_IDX(node_id, prop, idx) setup_uart_##idx();

#define UART_SEND_MESSAGE_BY_IDX(node_id, prop, idx) send_character_##idx();

static void setup_uarts() {
  DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, lins, UART_SETUP_BY_IDX);
}

static void send_messages() {
  DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, lins, UART_SEND_MESSAGE_BY_IDX);
}

void lin_test(void) {
  setup_uarts();
  send_messages();
}
