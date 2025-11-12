/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ardep/drivers/lin_scheduler.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static const struct device *lin = DEVICE_DT_GET(DT_NODELABEL(abstract_lin0));
static const struct device *can_sim = DEVICE_DT_GET(DT_NODELABEL(lin2can0));

static const struct abstract_lin_schedule_table_t table_def = {
  .count = 2,
  .entries =
      {
        {0x3D, K_MSEC(250)},
        {0x3C, K_MSEC(250)},
      },
};

static const struct abstract_lin_schedule_table_t *tables[] = {
  &table_def,
};

ABSTRACT_LIN_REGISTER_SCHEDULER_INITIAL_TABLE(lin, scheduler_def, tables, 0);

void can_rx_handler(const struct device *dev,
                    struct can_frame *frame,
                    void *user_data) {
  LOG_HEXDUMP_INF(frame->data, frame->dlc, "Received CAN Frame");
}

int main(void) {
  LOG_INF("Hello world!");

  if (!device_is_ready(can_sim)) {
    LOG_ERR("Device not ready");
    return -1;
  }

  struct can_filter filter = {
    .flags = 0,
    .id = 0x80,
    .mask = CAN_STD_ID_MASK,
  };
  int err = can_add_rx_filter(can_sim, can_rx_handler, NULL, &filter);
  if (err) {
    LOG_ERR("Error adding filter %d", err);
  }

  uint8_t i = 0;
  while (1) {
    LOG_INF("Sending can frame...");
    struct can_frame test_frame = {
      .data = {0x00, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xbe, i++},
      .dlc = 8,
      .id = 0x180,
    };
    can_send(can_sim, &test_frame, K_FOREVER, NULL, NULL);
    LOG_INF("CAN Frame sent");
    k_msleep(1000);
  }

  return 0;
}
