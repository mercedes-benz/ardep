/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
*/

#ifndef ARDEP_INCLUDE_DRIVERS_ABSTRACT_LIN_SCHEDULER_H_
#define ARDEP_INCLUDE_DRIVERS_ABSTRACT_LIN_SCHEDULER_H_

#include <zephyr/kernel.h>

#include <ardep/drivers/abstract_lin.h>

struct abstract_lin_schedule_entry_t {
  uint8_t frame_id;
  k_timeout_t delay;
};

struct abstract_lin_schedule_table_t {
  size_t count;
  const struct abstract_lin_schedule_entry_t entries[];
};

void _abstract_lin_scheduler_thread(void *p1, void *p2, void *p3);

struct abstract_lin_scheduler_t {
  const struct device **lin;
  const struct abstract_lin_schedule_table_t **tables;
  size_t table_count;
  size_t current_table;
  size_t current_table_entry;

  struct k_sem skip;
  struct k_sem active;
};

typedef struct abstract_lin_scheduler_t *abstract_lin_scheduler_handle_t;

/**
 * @brief Register a new scheduler for given abstract lin device with an initial
 * active table
 *
 * @param drv `const struct device*` device pointer
 * @param name name of the scheduler handle variable
 * @param tables_array a `const struct abstract_lin_schedule_table_t` array that
 * contains all tables
 * @param initial_table index of the table that should be pre-activated. -1 for
 * disabled
 */
#define ABSTRACT_LIN_REGISTER_SCHEDULER_INITIAL_TABLE(drv, name, tables_array, \
                                                      initial_table)           \
  struct abstract_lin_scheduler_t name##_struct = {                            \
    .lin = &drv,                                                               \
    .tables = tables_array,                                                    \
    .table_count = ARRAY_SIZE(tables_array),                                   \
    .current_table = initial_table,                                            \
    .current_table_entry = 0,                                                  \
  };                                                                           \
  abstract_lin_scheduler_handle_t name = &name##_struct;                       \
  K_THREAD_DEFINE(name##_thread, CONFIG_ABSTRACT_LIN_SCHEDULER_STACK_SIZE,     \
                  _abstract_lin_scheduler_thread, &name##_struct, NULL, NULL,  \
                  CONFIG_ABSTRACT_LIN_SCHEDULER_PRIORITY, 0, 0)

/**
 * @brief Register a new scheduler for given abstract lin device
 *
 * @param drv `const struct device*` device pointer
 * @param name name of the scheduler handle variable
 * @param tables_array a `const struct abstract_lin_schedule_table_t` array that
 * contains all tables
 */
#define ABSTRACT_LIN_REGISTER_SCHEDULER(drv, name, tables_array) \
  ABSTRACT_LIN_REGISTER_SCHEDULER_INITIAL_TABLE(drv, name, tables_array, -1)

/**
 * @brief Set the active table index of a previously created scheduler
 *
 * @param sched scheduler handle
 * @param table_index index of the active table
 * @retval 0 on success
 * @retval -ENOENT if table_index is out of bounds
 */
int abstract_lin_scheduler_set_active_table(
    abstract_lin_scheduler_handle_t sched, size_t table_index);

/**
 * @brief Disable the scheduler. Use `abstract_lin_scheduler_set_active_table`
 * to re-enable
 *
 * @param sched scheduler handle
 */
void abstract_lin_scheduler_disable(abstract_lin_scheduler_handle_t sched);

#endif
