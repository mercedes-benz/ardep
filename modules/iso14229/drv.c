/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(isotp, CONFIG_MODULE_ISO14229_EXTERNAL_LOG_LEVEL);

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>

#include <iso14229.h>

void isotp_user_debug(const char* fmt, ...) {
#if CONFIG_ISO14229_LIB_LOG_LEVEL >= LOG_LEVEL_DEBUG
  va_list args;
  va_start(args, fmt);
  printf("ISOTP DEBUG: ");  // todo: improve, but how
  vprintf(fmt, args);
  va_end(args);
#endif
}

int isotp_user_send_can(const uint32_t arbitration_id,
                        const uint8_t* data,
                        const uint8_t size,
                        void* arg) {
  struct can_frame frame;
  frame.id = arbitration_id;
  frame.dlc = size;
  memcpy(frame.data, data, size);
  frame.flags = 0;
  const struct device* can_dev = arg;
  // LOG_WRN("CAN TX: %03x [%d] %02x ...", frame.id, frame.dlc, frame.data[0]);
  LOG_DBG("CAN TX: %03x [%d] %02x ...", frame.id, frame.dlc, frame.data[0]);
  int ret = can_send(can_dev, &frame, K_FOREVER, NULL, &frame);

  if (ret != 0) {
    return ISOTP_RET_ERROR;
  }

  return ISOTP_RET_OK;
}

uint32_t isotp_user_get_us(void) { return k_uptime_get_32() * 1000; }

// todo: comment schreiben
uint32_t UDSMillis() { return k_uptime_get_32(); }
