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

static const struct device *const uart_devices[] = {DT_FOREACH_PROP_ELEM_SEP(
    ZEPHYR_USER_NODE, uarts, DEVICE_DT_BY_PROP_IDX, (, ))};

#define UART_CREATE_FUNCTIONS_BY_IDX(node_id, prop, idx)                 \
  static void uart_interrupt_callback_##idx(const struct device *dev,    \
                                            void *user_data) {           \
    uint8_t byte;                                                        \
                                                                         \
    if (!uart_irq_update(dev)) {                                         \
      return;                                                            \
    }                                                                    \
                                                                         \
    if (!uart_irq_rx_ready(dev)) {                                       \
      return;                                                            \
    }                                                                    \
                                                                         \
    while (uart_fifo_read(dev, &byte, 1) == 1) {                         \
      LOG_INF("%s received %c", dev->name, byte);                        \
    }                                                                    \
  }                                                                      \
                                                                         \
  static void setup_uart_##idx() {                                       \
    const struct device *dev = uart_devices[idx];                        \
    if (!device_is_ready(dev)) {                                         \
      LOG_ERR("%s: UART device not ready", dev->name);                   \
      return;                                                            \
    }                                                                    \
                                                                         \
    char byte;                                                           \
    while (uart_fifo_read(dev, &byte, 1) == 1) {                         \
    }                                                                    \
                                                                         \
    int ret = uart_irq_callback_user_data_set(                           \
        dev, uart_interrupt_callback_##idx, NULL);                       \
                                                                         \
    if (ret < 0) {                                                       \
      if (ret == -ENOTSUP) {                                             \
        LOG_ERR("%s: Interrupt-driven UART API support not enabled",     \
                dev->name);                                              \
      } else if (ret == -ENOSYS) {                                       \
        LOG_ERR("%s: UART device does not support interrupt-driven API", \
                dev->name);                                              \
      } else {                                                           \
        LOG_ERR("%s: Error setting UART callback: %d", dev->name, ret);  \
      }                                                                  \
      return;                                                            \
    }                                                                    \
    uart_irq_rx_enable(dev);                                             \
  }                                                                      \
                                                                         \
  static void disable_uart_irq_##idx() {                                 \
    const struct device *dev = uart_devices[idx];                        \
    uart_irq_rx_disable(dev);                                            \
  }                                                                      \
                                                                         \
  static void send_character_##idx() {                                   \
    const struct device *dev = uart_devices[idx];                        \
    LOG_INF("%s sending: f", dev->name);                                 \
    uart_poll_out(dev, 'f');                                             \
    k_msleep(5);                                                         \
    LOG_INF("%s sending: r", dev->name);                                 \
    uart_poll_out(dev, 'r');                                             \
    k_msleep(5);                                                         \
    LOG_INF("%s sending: i", dev->name);                                 \
    uart_poll_out(dev, 'i');                                             \
    k_msleep(5);                                                         \
    LOG_INF("%s sending: c", dev->name);                                 \
    uart_poll_out(dev, 'c');                                             \
    k_msleep(5);                                                         \
    LOG_INF("%s sending: k", dev->name);                                 \
    uart_poll_out(dev, 'k');                                             \
    k_msleep(5);                                                         \
    LOG_INF("%s sending: l", dev->name);                                 \
    uart_poll_out(dev, 'l');                                             \
    k_msleep(5);                                                         \
    LOG_INF("%s sending: y", dev->name);                                 \
    uart_poll_out(dev, 'y');                                             \
    k_msleep(5);                                                         \
  }

DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, uarts, UART_CREATE_FUNCTIONS_BY_IDX);

#define UART_SETUP_BY_IDX(node_id, prop, idx) setup_uart_##idx();

#define UART_SEND_CHARACTER_BY_IDX(node_id, prop, idx) send_character_##idx();

#define UART_STOP_INTERRUPTS_BY_IDX(node_id, prop, idx) \
  disable_uart_irq_##idx();

void setup_uarts() {
  DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, uarts, UART_SETUP_BY_IDX);
}

void send_characters() {
  DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, uarts, UART_SEND_CHARACTER_BY_IDX);
}

void disable_uart_interrupts() {
  DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, uarts, UART_STOP_INTERRUPTS_BY_IDX);
}

void uart_test(void) {
  setup_uarts();
  k_msleep(500);
  send_characters();
  k_msleep(100);
  disable_uart_interrupts();
  k_msleep(500);
}
