/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(firmware_loader, CONFIG_APP_LOG_LEVEL);

#include "uds.h"

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/retention/retention.h>

#include <ardep/iso14229.h>
#include <ardep/uds.h>

const struct device *retention_data = DEVICE_DT_GET(DT_NODELABEL(retention1));

const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

struct uds_instance_t instance;

UDS_REGISTER_ECU_DEFAULT_HARD_RESET_HANDLER(&instance);

int main() {
  LOG_INF("Hello firmware-loader");

  uint8_t session_type;

  int ret = retention_read(retention_data, 0, &session_type, 1);
  if (ret != 0) {
    LOG_ERR("Failed to read from retention 1: %d", ret);
    return ret;
  }
  LOG_INF("Read stored session type from retention data: %02x", session_type);

  LOG_INF("Configuring UDS...");

  UDSISOTpCConfig_t cfg = {
    // Hardware Addresses
    .source_addr = 0x7E8,
    .target_addr = 0x7E0,

    // Functional Addresses
    .source_addr_func = 0x7DF,
    .target_addr_func = UDS_TP_NOOP_ADDR,
  };

  uds_init(&instance, &cfg, can_dev, NULL);

  if (!device_is_ready(can_dev)) {
    LOG_INF("CAN device not ready");
    return -ENODEV;
  }

  int err = can_set_mode(can_dev, CAN_MODE_NORMAL);
  if (err) {
    LOG_ERR("Failed to set CAN mode: %d", err);
    return err;
  }

  if (session_type == UDS_DIAG_SESSION__PROGRAMMING) {
    LOG_INF("Injecting DiagnosticSessionControl (Programming) frame");
    struct can_frame frame = {
      .id = cfg.source_addr,
      .dlc = 3,
      .flags = 0,
    };
    frame.data[0] = 0x02;  // PCI (single frame, 2 bytes of data)
    frame.data[1] = 0x10;  // SID (DiagnosticSessionControl)
    frame.data[2] = 0x02;  // DS  (Programming Session)

    iso14229_inject_can_frame_rx(&instance.iso14229, &frame);
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
