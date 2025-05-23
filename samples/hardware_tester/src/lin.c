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

#define DEVICE_DT_BY_PROP_IDX(node_id, prop, idx)                              \
  DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx))

static const struct device *const lin_devices[] = {DT_FOREACH_PROP_ELEM_SEP(
    ZEPHYR_USER_NODE, lins, DEVICE_DT_BY_PROP_IDX, (, ))};

#define UART_CREATE_FUNCTIONS_BY_IDX(node_id, prop, idx)                       \
  static void uart_interrupt_callback_##idx(const struct device *dev,                 \
                                     void *user_data) {                        \
    uint8_t byte;                                                              \
                                                                               \
    if (!uart_irq_update(dev)) {                                               \
      return;                                                                  \
    }                                                                          \
                                                                               \
    if (!uart_irq_rx_ready(dev)) {                                             \
      return;                                                                  \
    }                                                                          \
                                                                               \
    while (uart_fifo_read(dev, &byte, 1) == 1) {                               \
      LOG_INF("%s received %c", dev->name, byte);                              \
    }                                                                          \
  }                                                                            \
                                                                               \
  static void setup_uart_##idx() {                                                    \
    const struct device *dev = lin_devices[idx];                              \
    if (!device_is_ready(dev)) {                                               \
      LOG_ERR("%s: UART device not ready", dev->name);                         \
      return;                                                                  \
    }                                                                          \
                                                                               \
    int ret = uart_irq_callback_user_data_set(                                 \
        dev, uart_interrupt_callback_##idx, NULL);                             \
                                                                               \
    if (ret < 0) {                                                             \
      if (ret == -ENOTSUP) {                                                   \
        LOG_ERR("%s: Interrupt-driven UART API support not enabled",           \
                dev->name);                                                    \
      } else if (ret == -ENOSYS) {                                             \
        LOG_ERR("%s: UART device does not support interrupt-driven API",       \
                dev->name);                                                    \
      } else {                                                                 \
        LOG_ERR("%s: Error setting UART callback: %d", dev->name, ret);        \
      }                                                                        \
      return;                                                                  \
    }                                                                          \
    uart_irq_rx_enable(dev);                                                   \
  }                                                                            \
                                                                               \
  static void disable_uart_irq_##idx() {                                              \
    const struct device *dev = lin_devices[idx];                              \
    uart_irq_rx_disable(dev);                                                  \
  }                                                                            \
                                                                               \

DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, lins, UART_CREATE_FUNCTIONS_BY_IDX);

#define UART_SETUP_BY_IDX(node_id, prop, idx) setup_uart_##idx();

#define UART_STOP_INTERRUPTS_BY_IDX(node_id, prop, idx)                        \
  disable_uart_irq_##idx();

static void setup_uarts() {
  DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, lins, UART_SETUP_BY_IDX);
}

static void execute_test() {
  // wait for incoming messages
  k_msleep(5000);
}

static void disable_uart_interrupts() {
  DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, lins, UART_STOP_INTERRUPTS_BY_IDX);
}

void lin_test(void) {
  setup_uarts();
  execute_test();
  disable_uart_interrupts();
  k_msleep(500);
}
