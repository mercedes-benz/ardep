/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ardep/drivers/lin_scheduler.h>

void _abstract_lin_scheduler_thread(void *p1, void *p2, void *p3) {
  struct abstract_lin_scheduler_t *data = p1;

  k_sem_init(&data->active, 0, 1);
  k_sem_init(&data->skip, 0, 1);

  if (data->current_table != -1) {
    k_sem_give(&data->active);
  }

  while (1) {
    k_sem_take(&data->active, K_FOREVER);
    const struct abstract_lin_schedule_table_t *table =
        data->tables[data->current_table];
    const struct abstract_lin_schedule_entry_t *entry =
        &table->entries[data->current_table_entry++];

    if (data->current_table_entry >= table->count) {
      data->current_table_entry = 0;
    }

    abstract_lin_schedule_now(*data->lin, entry->frame_id);

    k_sem_give(&data->active);

    // same as k_msleep but skippable using k_sem_give(&data->skip)
    k_sem_take(&data->skip, entry->delay);
  }
}

int abstract_lin_scheduler_set_active_table(
    abstract_lin_scheduler_handle_t sched, size_t table_index) {
  if (table_index >= sched->table_count) {
    return -ENOENT;
  }

  bool active_before = sched->current_table != -1;

  if (active_before) {
    k_sem_take(&sched->active, K_FOREVER);
  }

  sched->current_table = table_index;
  sched->current_table_entry = 0;  // start from the beginning again.

  k_sem_give(&sched->active);

  return 0;
}

void abstract_lin_scheduler_disable(abstract_lin_scheduler_handle_t sched) {
  if (sched->current_table == -1) {
    return;  // already stopped.
  }

  // let thread actively wait for set_active_table
  k_sem_take(&sched->active, K_FOREVER);

  sched->current_table = -1;
}
