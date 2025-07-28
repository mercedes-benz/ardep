/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/drivers/can.h"
#include "zephyr/device.h"
#include "zephyr/devicetree.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys/printk.h"

#include <zephyr/kernel.h>

#define LOG_MODULE_NAME tester
LOG_MODULE_DECLARE(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

#define DEVICE_DT_BY_PROP_IDX(node_id, prop, idx)                              \
  DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx))

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct device *const can_devices[] = {DT_FOREACH_PROP_ELEM_SEP(
    ZEPHYR_USER_NODE, cans, DEVICE_DT_BY_PROP_IDX, (, ))};

static void rx_callback(const struct device *dev, struct can_frame *frame,
                 void *user_data) {
  LOG_INF("%s: frame id: %x, data: %x", dev->name, frame->id, frame->data[0]);
}

const struct can_filter receive_frame_filter = {
    .flags = 0, .id = 0x65, .mask = CAN_STD_ID_MASK};

#define CAN_CREATE_FUNCTIONS_BY_IDX(node_id, prop, idx)                        \
  int rx_filter_##idx;                                                         \
                                                                               \
  static void init_can_##idx() {                                                      \
    const struct device *dev = can_devices[idx];                               \
    int err;                                                                   \
                                                                               \
    if (!device_is_ready(dev)) {                                               \
      LOG_ERR("%s: device not ready", dev->name);                              \
      return;                                                                  \
    }                                                                          \
                                                                               \
    err = can_start(dev);                                                      \
    if (err != 0) {                                                            \
      LOG_ERR("%s: error starting CAN controller (err %d)", dev->name, err);   \
      return;                                                                  \
    }                                                                          \
                                                                               \
    rx_filter_##idx =                                                          \
        can_add_rx_filter(dev, rx_callback, NULL, &receive_frame_filter);      \
    if (rx_filter_##idx < 0) {                                                 \
      LOG_ERR("%s: error adding rx filter (err %d)", dev->name,                \
              rx_filter_##idx);                                                \
    }                                                                          \
  }                                                                            \
                                                                               \
  static void stop_can_##idx() {                                                      \
    const struct device *dev = can_devices[idx];                               \
    can_remove_rx_filter(dev, rx_filter_##idx);                                \
    can_stop(dev);                                                             \
  }

DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, cans, CAN_CREATE_FUNCTIONS_BY_IDX);

#define CAN_INIT_BY_IDX(node_id, prop, idx) init_can_##idx();

#define CAN_STOP_BY_IDX(node_id, prop, idx) stop_can_##idx();

static void init_can(void) {
  DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, cans, CAN_INIT_BY_IDX);
}

static void run_test(void) { k_msleep(5000); }

static void stop_can(void) {
  DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, cans, CAN_STOP_BY_IDX);
}

void can_test(void) {
  init_can();

  run_test();

  k_msleep(300);
  stop_can();
}