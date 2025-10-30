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

static const struct device* const lin_devices[] = {DT_FOREACH_PROP_ELEM_SEP(
    ZEPHYR_USER_NODE, lins, DEVICE_DT_BY_PROP_IDX, (, ))};

static void setup_uarts() {
  for (int i = 0; i < ARRAY_SIZE(lin_devices); i++) {
    const struct device* dev = lin_devices[i];
    if (!device_is_ready(dev)) {
      LOG_ERR("%s: UART device not ready", dev->name);
      continue;
    }

    char byte;
    while (uart_fifo_read(dev, &byte, 1) == 1) {
    }
  }
}

static void send_messages() {
  for (int i = 0; i < ARRAY_SIZE(lin_devices); i++) {
    const struct device* dev = lin_devices[i];
    for (int j = 0; j < 4; j++) {
      LOG_INF("%s sending: f", dev->name);
      uart_poll_out(dev, 'f');
      k_msleep(CONFIG_LIN_MESSAGE_DELAY_MS);
    }
  }
}

void lin_test(void) {
  setup_uarts();
  k_msleep(CONFIG_LIN_PRE_TEST_DELAY_MS);
  send_messages();
}
