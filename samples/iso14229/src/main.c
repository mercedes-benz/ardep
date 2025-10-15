/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "iso14229.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(iso14229_testing, LOG_LEVEL_DBG);

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <ardep/iso14229.h>

static const struct device* can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

struct iso14229_zephyr_instance inst;

UDSErr_t event_callback(struct iso14229_zephyr_instance* inst,
                        UDSEvent_t event,
                        void* arg,
                        void* user_context) {
  if (event == UDS_EVT_DiagSessCtrl) {
    UDSDiagSessCtrlArgs_t args = *(UDSDiagSessCtrlArgs_t*)arg;
    LOG_INF("Session changed to session ID: 0x%02X", args.type);
    return UDS_PositiveResponse;
  } else if (event == UDS_EVT_SessionTimeout) {
    LOG_INF("Session timeout occurred");
    return UDS_PositiveResponse;
  }

  return UDS_NRC_SubFunctionNotSupported;
}

int main(void) {
  LOG_INF("ISO14229 sample");

  UDSISOTpCConfig_t cfg = {
    // Hardware Addresses
    .source_addr = 0x7E8,  // Can ID Server (us)
    .target_addr = 0x7E0,  // Can ID Client (them)

    // Functional Addresses
    .source_addr_func = 0x7DF,             // ID Client
    .target_addr_func = UDS_TP_NOOP_ADDR,  // ID Server (us)
  };

  int err = iso14229_zephyr_init(&inst, &cfg, can_dev, NULL);
  if (err) {
    LOG_ERR("Failed to initialize ISO 14229 Zephyr instance: %d", err);
    return err;
  }

  if (!device_is_ready(can_dev)) {
    LOG_ERR("CAN device not ready");
    return -ENODEV;
  }

  err = can_set_mode(can_dev, CAN_MODE_NORMAL);
  if (err) {
    LOG_ERR("Failed to set CAN mode: %d", err);
    return err;
  }

  err = can_start(can_dev);
  if (err) {
    LOG_ERR("Failed to start CAN device: %d", err);
    return err;
  }
  LOG_INF("CAN device started\n");

  err = inst.set_callback(&inst, event_callback);
  if (err) {
    LOG_ERR("Failed to set event callback: %d", err);
    return err;
  }

  err = inst.thread_start(&inst);
  if (err) {
    LOG_ERR("Failed to start UDS thread: %d", err);
    return err;
  }

  LOG_INF("ISO14229 Thread started");
}
