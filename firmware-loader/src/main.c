/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(firmware_loader, CONFIG_APP_LOG_LEVEL);

#include "main.h"

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/retention/retention.h>

#include <ardep/iso14229.h>

const struct device *retention0 = DEVICE_DT_GET(DT_NODELABEL(retention0));
const struct device *retention1 = DEVICE_DT_GET(DT_NODELABEL(retention1));

const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

struct uds_instance_t instance;

UDS_REGISTER_ECU_DEFAULT_HARD_RESET_HANDLER(&instance);

int main() {
  LOG_INF("Hello firmware-loader");

  uint8_t data;

  int ret = retention_read(retention0, 0, &data, 1);
  if (ret != 0) {
    LOG_ERR("Failed to read from retention 0: %d", ret);
    return ret;
  }
  LOG_INF("Read data from retention 0: %02x", data);

  ret = retention_read(retention1, 0, &data, 1);
  if (ret != 0) {
    LOG_ERR("Failed to read from retention 1: %d", ret);
    return ret;
  }
  LOG_INF("Read data from retention 1: %02x", data);

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