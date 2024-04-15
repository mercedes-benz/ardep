/*
 * Copyright (c) 2024 Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_INCLUDE_CAN_ROUTER_H_
#define ARDEP_INCLUDE_CAN_ROUTER_H_

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>

struct can_router_entry_t {
  const struct device **from;
  const struct device **to;
  const struct can_filter filter;
};

struct can_router_table_t {
  const struct can_router_entry_t *entries;
  int entry_count;
};

int can_router_register(const struct can_router_entry_t *entries,
                        int entry_count);

#define CAN_ROUTER_REGISTER(entry_array)                       \
  const STRUCT_SECTION_ITERABLE(can_router_table_t,            \
                                router_table##__COUNTER__) = { \
    .entries = (entry_array),                                  \
    .entry_count = ARRAY_SIZE((entry_array)),                  \
  };

// note: registering uses iterable sections (see zephyr docs)

#endif
