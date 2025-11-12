/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ardep/drivers/abstract_lin.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static const struct device *lin = DEVICE_DT_GET(DT_NODELABEL(abstract_lin0));

static bool led = false;

static bool status_callback(struct lin_frame *frame, void *) {
  LOG_DBG("Status callback called");
  frame->data[0] = led;

  // unconditionally respond
  return true;
}

static void command_callback(const struct lin_frame *frame, void *) {
  LOG_DBG("Command callback called");
  switch (frame->data[0]) {
    case 0:
      LOG_INF("Turn on led");
      led = false;
      break;
    case 1:
      LOG_INF("Turn off led");
      led = true;
      break;
    default:
      LOG_WRN("Unknown command received. Ignoring");
      break;
  }
}

int main(void) {
  LOG_INF("Hello world!");

  if (!device_is_ready(lin)) {
    LOG_ERR("Device not ready");
    return -1;
  }

  int err;
  {
    uint8_t free_slots = 0;
    if ((err = abstract_lin_get_free_callback_slot(lin, &free_slots))) {
      LOG_ERR("Error reading free slot count. err: %d", err);
    }
    if (free_slots < 2) {
      LOG_ERR("Not enough free slots for setting up callbacks. Free_slots: %d",
              free_slots);
      return -1;
    }
  }

  if ((err = abstract_lin_register_outgoing(lin, &status_callback, 0x10, 1,
                                            NULL))) {
    LOG_ERR("Error registering outgoing status callback. err: %d", err);
  };
  if ((err = abstract_lin_register_incoming(lin, &command_callback, 0x11, 1,
                                            NULL))) {
    LOG_ERR("Error registering incoming command callback. err: %d", err);
  };

  LOG_INF("Registered callbacks");

  return 0;
}
