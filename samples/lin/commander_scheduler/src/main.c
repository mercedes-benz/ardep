/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
*/

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ardep/drivers/abstract_lin.h>
#include <ardep/drivers/lin_scheduler.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

static const struct device *lin = DEVICE_DT_GET(DT_NODELABEL(abstract_lin0));

static bool remote_led_target = false;
static bool remote_led_is = false;

static bool command_callback(struct lin_frame *frame, void *) {
  LOG_DBG("Command callback called");
  LOG_INF("Sending command %s", remote_led_target ? "on" : "off");
  frame->data[0] = remote_led_target;

  remote_led_target = !remote_led_target;

  // unconditionally send
  return true;
}

static void status_callback(const struct lin_frame *frame, void *) {
  LOG_DBG("Status callback called");
  switch (frame->data[0]) {
    case 0:
      LOG_INF("LED Status: off");
      remote_led_is = false;
      break;
    case 1:
      LOG_INF("LED Status: on");
      remote_led_is = true;
      break;
    default:
      LOG_WRN("Unknown status received. Ignoring");
      break;
  }
}

enum tables_t { TABLE_1, TABLE_2 };

static const struct abstract_lin_schedule_table_t table1 = {
  .count = 2,
  .entries =
      {
        {0x10, K_MSEC(500)},
        {0x11, K_MSEC(500)},
      },
};

static const struct abstract_lin_schedule_table_t table2 = {
  .count = 2,
  .entries =
      {
        {0x10, K_MSEC(125)},
        {0x11, K_MSEC(125)},
      },
};

static const struct abstract_lin_schedule_table_t *tables[] = {
  [TABLE_1] = &table1,
  [TABLE_2] = &table2,
};

ABSTRACT_LIN_REGISTER_SCHEDULER(lin, lin_scheduler, tables);

int main(void) {
  LOG_INF("Hello world!");

  if (!device_is_ready(lin)) {
    LOG_ERR("Device not ready");
    return -1;
  }

  int err;
  // check if we have enough free slots for our 2 callbacks
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

  if ((err = abstract_lin_register_incoming(lin, &status_callback, 0x10, 1,
                                            NULL))) {
    LOG_ERR("Error registering incoming status callback. err: %d", err);
  };
  if ((err = abstract_lin_register_outgoing(lin, &command_callback, 0x11, 1,
                                            NULL))) {
    LOG_ERR("Error registering outgoing command callback. err: %d", err);
  };

  LOG_INF("Registered cbs. Switching between schedule tables");

  while (1) {
    LOG_INF("Setting scheduler table 1");
    abstract_lin_scheduler_set_active_table(lin_scheduler, TABLE_1);
    k_msleep(5000);

    LOG_INF("Setting scheduler table 2");
    abstract_lin_scheduler_set_active_table(lin_scheduler, TABLE_2);
    k_msleep(5000);

    LOG_INF("Disable scheduler");
    abstract_lin_scheduler_disable(lin_scheduler);
    k_msleep(5000);
  }

  return 0;
}
