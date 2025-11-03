/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/device.h"
#include "zephyr/devicetree.h"
#include "zephyr/drivers/can.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys/printk.h"

#include <zephyr/kernel.h>

#define LOG_MODULE_NAME tester
LOG_MODULE_DECLARE(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

#define DEVICE_DT_BY_PROP_IDX(node_id, prop, idx) \
  DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx))

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct device* const can_devices[] = {DT_FOREACH_PROP_ELEM_SEP(
    ZEPHYR_USER_NODE, cans, DEVICE_DT_BY_PROP_IDX, (, ))};

static int rx_filters[ARRAY_SIZE(can_devices)];

static void rx_callback(const struct device* dev,
                        struct can_frame* frame,
                        void* user_data) {
  LOG_INF("%s: frame id: %x, data: %x", dev->name, frame->id, frame->data[0]);
}

const struct can_filter receive_frame_filter = {
  .flags = 0, .id = 0x65, .mask = CAN_STD_ID_MASK};

static void init_can(void) {
  for (int i = 0; i < ARRAY_SIZE(can_devices); i++) {
    const struct device* dev = can_devices[i];
    int err;

    if (!device_is_ready(dev)) {
      LOG_ERR("%s: device not ready", dev->name);
      continue;
    }

    err = can_start(dev);
    if (err != 0) {
      LOG_ERR("%s: error starting CAN controller (err %d)", dev->name, err);
      continue;
    }

    rx_filters[i] =
        can_add_rx_filter(dev, rx_callback, NULL, &receive_frame_filter);
    if (rx_filters[i] < 0) {
      LOG_ERR("%s: error adding rx filter (err %d)", dev->name, rx_filters[i]);
    }
  }
}

static void run_test(void) { k_msleep(CONFIG_CAN_TEST_DURATION_MS); }

static void stop_can(void) {
  for (int i = 0; i < ARRAY_SIZE(can_devices); i++) {
    can_remove_rx_filter(can_devices[i], rx_filters[i]);
    can_stop(can_devices[i]);
  }
}

void can_test(void) {
  init_can();

  run_test();

  k_msleep(CONFIG_POST_TEST_DELAY_MS);
  stop_can();
}
