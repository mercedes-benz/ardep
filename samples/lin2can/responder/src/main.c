/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
*/

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static const struct device *can_sim = DEVICE_DT_GET(DT_NODELABEL(lin2can0));

void can_rx_handler(const struct device *dev,
                    struct can_frame *frame,
                    void *user_data) {
  LOG_HEXDUMP_INF(frame->data, frame->dlc, "Received CAN Frame");
}

static void can_tx_cb(const struct device *dev, int err, void *user_data) {
  if (err) {
    LOG_ERR("TX failed");
  }
  LOG_DBG("TX Complete");
}

int main(void) {
  LOG_INF("Hello world!");

  if (!device_is_ready(can_sim)) {
    LOG_ERR("Device not ready");
    return -1;
  }

  struct can_filter filter = {
    .flags = 0,
    .id = 0x180,
    .mask = CAN_STD_ID_MASK,
  };
  can_add_rx_filter(can_sim, can_rx_handler, NULL, &filter);

  uint8_t i = 0;
  while (1) {
    struct can_frame test_frame = {
      .data = {0, i++},
      .dlc = 2,
      .id = 0x80,
    };

    // Note: setting callback to NULL would await sending the frame
    can_send(can_sim, &test_frame, K_FOREVER, can_tx_cb, NULL);
    k_msleep(2000);

    LOG_INF("Scheduled CAN Frame for transmission");
  }

  return 0;
}
