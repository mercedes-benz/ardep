/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/devicetree.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys/printk.h"

#include <zephyr/console/console.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#define LOG_MODULE_NAME tester
LOG_MODULE_DECLARE(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

#define DEVICE_DT_BY_PROP_IDX(node_id, prop, idx) \
  DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx))

static const struct device* const lin_devices[] = {DT_FOREACH_PROP_ELEM_SEP(
    ZEPHYR_USER_NODE, lins, DEVICE_DT_BY_PROP_IDX, (, ))};

static void uart_interrupt_callback(const struct device* dev, void* user_data) {
  uint8_t byte;

  if (!uart_irq_update(dev)) {
    return;
  }

  if (!uart_irq_rx_ready(dev)) {
    return;
  }

  while (uart_fifo_read(dev, &byte, 1) == 1) {
    LOG_INF("%s received %c", dev->name, byte);
  }
}

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

    int ret =
        uart_irq_callback_user_data_set(dev, uart_interrupt_callback, NULL);

    if (ret < 0) {
      if (ret == -ENOTSUP) {
        LOG_ERR("%s: Interrupt-driven UART API support not enabled", dev->name);
      } else if (ret == -ENOSYS) {
        LOG_ERR("%s: UART device does not support interrupt-driven API",
                dev->name);
      } else {
        LOG_ERR("%s: Error setting UART callback: %d", dev->name, ret);
      }
      continue;
    }
    uart_irq_rx_enable(dev);
  }
}

static void execute_test() {
  // wait for incoming messages
  k_msleep(CONFIG_CAN_TEST_DURATION_MS);
}

static void disable_uart_interrupts() {
  for (int i = 0; i < ARRAY_SIZE(lin_devices); i++) {
    uart_irq_rx_disable(lin_devices[i]);
  }
}

void lin_test(void) {
  setup_uarts();
  execute_test();
  disable_uart_interrupts();
  k_msleep(CONFIG_POST_TEST_DELAY_MS);
}
