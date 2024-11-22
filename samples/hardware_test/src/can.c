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

#define LOG_MODULE_NAME sut
LOG_MODULE_DECLARE(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

#define DEVICE_DT_BY_PROP_IDX(node_id, prop, idx)                              \
  DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx))

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct device *const can_devices[] = {DT_FOREACH_PROP_ELEM_SEP(
    ZEPHYR_USER_NODE, cans, DEVICE_DT_BY_PROP_IDX, (, ))};

#define CAN_CREATE_FUNCTIONS_BY_IDX(node_id, prop, idx)                        \
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
  }                                                                            \
                                                                               \
  static void send_frame_##idx() {                                                    \
    const struct device *dev = can_devices[idx];                               \
    struct can_frame frame = {0};                                              \
                                                                               \
    frame.id = 0x65;                                                           \
    frame.data[0] = 0x34;                                                      \
    frame.dlc = 1;                                                             \
                                                                               \
    LOG_INF("%s: frame id: %x, data: %x", dev->name, frame.id, frame.data[0]); \
                                                                               \
    int rc = can_send(dev, &frame, K_NO_WAIT, NULL, NULL);                     \
    if (rc < 0) {                                                              \
      LOG_ERR("%s: error sending CAN frame: %d", dev->name, rc);               \
    }                                                                          \
  }                                                                            \
                                                                               \
  static void stop_can_##idx() {                                                      \
    const struct device *dev = can_devices[idx];                               \
    can_stop(dev);                                                             \
  }

DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, cans, CAN_CREATE_FUNCTIONS_BY_IDX);

#define CAN_INIT_BY_IDX(node_id, prop, idx) init_can_##idx();

#define CAN_SEND_FRAME_BY_IDX(node_id, prop, idx) send_frame_##idx();

#define CAN_STOP_BY_IDX(node_id, prop, idx) stop_can_##idx();

static void init_can(void) {
  DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, cans, CAN_INIT_BY_IDX);
}

static void send_can_frame(void) {
  DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, cans, CAN_SEND_FRAME_BY_IDX);
}

static void send_frames(void) {
  for (int i = 0; i < 10; i++) {
    send_can_frame();
    k_msleep(50);
  }
}

static void stop_can(void) {
  DT_FOREACH_PROP_ELEM(ZEPHYR_USER_NODE, cans, CAN_STOP_BY_IDX);
}

void can_test(void) {
  init_can();
  k_msleep(1000);

  send_frames();

  k_msleep(300);
  stop_can();
}