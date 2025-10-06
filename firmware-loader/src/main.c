/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(firmware_loader, CONFIG_APP_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/retention/retention.h>

const struct device *retention0 = DEVICE_DT_GET(DT_NODELABEL(retention0));
const struct device *retention1 = DEVICE_DT_GET(DT_NODELABEL(retention1));

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
  return 0;
}