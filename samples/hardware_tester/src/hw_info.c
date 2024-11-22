/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/drivers/hwinfo.h"
#include "zephyr/kernel.h"
#include "zephyr/logging/log.h"

#define LOG_MODULE_NAME tester
LOG_MODULE_DECLARE(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

uint8_t hw_info[60];

void log_hardware_info(void) {
  ssize_t length = hwinfo_get_device_id(hw_info, 60);

  char hex_string[length * 2 + 1];
  for (int i = 0; i < length; i++) {
    sprintf(&hex_string[i * 2], "%02X", hw_info[i]);
  }
  hex_string[length * 2] = '\0';

  uint8_t i;
  for (i = 0; i < 60; i++) {
    hw_info[i] = 0;
  }

  LOG_INF("id: %s", hex_string);
}