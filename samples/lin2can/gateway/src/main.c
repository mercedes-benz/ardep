/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>

#include <ardep/can_router.h>
#include <ardep/drivers/lin_scheduler.h>

static const struct device *can_a = DEVICE_DT_GET(DT_NODELABEL(can_a));
static const struct device *lincan = DEVICE_DT_GET(DT_NODELABEL(lin2can0));
static const struct device *lin = DEVICE_DT_GET(DT_NODELABEL(abstract_lin0));

// Router
static const struct can_router_entry_t entries[] = {
  {
    .from = &can_a,
    .to = &lincan,
    .filter =
        {
          // catch all
          .flags = 0,
          .id = 0,
          .mask = 0,
        },
  },
  {
    .from = &lincan,
    .to = &can_a,
    .filter =
        {
          // catch all
          .flags = 0,
          .id = 0,
          .mask = 0,
        },
  },
};

CAN_ROUTER_REGISTER(entries);

// Scheduler
static const struct abstract_lin_schedule_table_t table_def = {
  .count = 2,
  .entries =
      {
        {0x3D, K_MSEC(5)},
        {0x3C, K_MSEC(5)},
      },
};

static const struct abstract_lin_schedule_table_t *tables[] = {
  &table_def,
};

ABSTRACT_LIN_REGISTER_SCHEDULER_INITIAL_TABLE(lin, scheduler_def, tables, 0);

int main(void) {
  int ret = can_start(can_a);

  if (ret) {
    printk("can_start failed (%d)\n", ret);
    return ret;
  }

  printk("Hello world, CAN Router!\n");
}
