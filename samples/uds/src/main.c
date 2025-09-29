/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uds_sample, LOG_LEVEL_DBG);

#include "uds.h"

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>

#include <ardep/iso14229.h>
#include <ardep/uds.h>

static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

struct uds_instance_t instance;

UDS_REGISTER_ECU_DEFAULT_HARD_RESET_HANDLER(&instance);

int main(void) {
  LOG_INF("ARDEP UDS Sample");

  UDSISOTpCConfig_t cfg = {
    // Hardware Addresses
    .source_addr = 0x7E8,
    .target_addr = 0x7E0,

    // Functional Addresses
    .source_addr_func = 0x7DF,
    .target_addr_func = UDS_TP_NOOP_ADDR,
  };

  uds_init(&instance, &cfg, can_dev, &instance);

  int err;
  if (!device_is_ready(can_dev)) {
    LOG_INF("CAN device not ready");
    return -ENODEV;
  }

  err = can_set_mode(can_dev, CAN_MODE_NORMAL);
  if (err) {
    LOG_ERR("Failed to set CAN mode: %d", err);
    return err;
  }

  err = can_start(can_dev);
  if (err) {
    LOG_ERR(
        "Failed to start CAN device: "
        "%d",
        err);
    return err;
  }
  LOG_INF("CAN device started");

  instance.iso14229.thread_start(&instance.iso14229);
  LOG_INF("UDS thread started");
}

UDS_REGISTER_LINK_CONTROL_DEFAULT_HANDLER(&instance);