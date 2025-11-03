/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ardep/can_router.h>

LOG_MODULE_REGISTER(can_router, CONFIG_CAN_ROUTER_LOG_LEVEL);

static void can_router_tx_cb(const struct device *dev,
                             int error,
                             void *user_data) {}

static void can_router_frame_cb(const struct device *dev,
                                struct can_frame *frame,
                                void *user_data) {
  const struct device *to = user_data;

  // Note: we use a tx callback to make use can_send non-blocking
  const int err = can_send(to, frame, K_NO_WAIT, can_router_tx_cb, NULL);
  if (err) {
    LOG_WRN("Can send failed (%d)", err);
    return;
  }

  LOG_DBG("Routed frame from %s to %s", dev->name, to->name);
}

int can_router_register(const struct can_router_entry_t *entries,
                        int entry_count) {
  LOG_DBG("Registering %d can router entries", entry_count);
  for (int i = 0; i < entry_count; i++) {
    const int err =
        can_add_rx_filter(*entries[i].from, can_router_frame_cb,
                          (void *)*entries[i].to, &entries[i].filter);
    if (err) {
      return err;
    }
  }

  LOG_DBG("Registered %d can router entries", entry_count);

  return 0;
}

static int can_router_sysinit() {
  LOG_DBG("Initializing can router");

  STRUCT_SECTION_FOREACH (can_router_table_t, table) {
    int err = can_router_register(table->entries, table->entry_count);
    if (err) {
      LOG_ERR("could not register can router");
      return err;
    }
  }

  return 0;
}

SYS_INIT(can_router_sysinit, APPLICATION, CONFIG_CAN_ROUTER_INIT_PRIORITY);
