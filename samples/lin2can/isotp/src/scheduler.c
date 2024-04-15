/*
 * Copyright (c) 2024 Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ardep/drivers/lin_scheduler.h>

// Note: the responder doesn't need the scheduler but it is simpler to use to
// just let that be here

static const struct device *lin = DEVICE_DT_GET(DT_NODELABEL(abstract_lin0));

static const struct abstract_lin_schedule_table_t table_def = {
  .count = 2,
  .entries =
      {
        {0x3D, K_MSEC(25)},
        {0x3C, K_MSEC(25)},
      },
};

static const struct abstract_lin_schedule_table_t *tables[] = {
  &table_def,
};

ABSTRACT_LIN_REGISTER_SCHEDULER_INITIAL_TABLE(lin, scheduler_def, tables, 0);
