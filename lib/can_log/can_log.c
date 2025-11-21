/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_backend_std.h>

#include <ardep/can_router.h>

static uint8_t buf[128];
static uint32_t log_format_type = LOG_OUTPUT_TEXT;
static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
static bool panic_mode = false;

static int can_log_line_out(uint8_t *data, size_t length, void *output_ctx) {
  struct can_frame frame;
  if (length > CAN_MAX_DLEN) {
    length = CAN_MAX_DLEN;
  }
  frame.id = CONFIG_CAN_LOG_ID;
  frame.dlc = length;
  frame.flags = 0;
  memcpy(frame.data, data, length);
  int ret = can_send(can_dev, &frame, panic_mode ? K_NO_WAIT : K_MSEC(100),
                     NULL, NULL);
  // in panic mode we dont reall care about errors. Sending
  // anything is better than nothing
  if (panic_mode) {
    return length;
  }

  if (ret < 0) {
    return 0;  // error -> nothing was sent
  }

  return length;
}

LOG_OUTPUT_DEFINE(can_log_output, can_log_line_out, buf, sizeof(buf));

static void can_log_process(const struct log_backend *const backend,
                            union log_msg_generic *msg) {
  log_format_func_t log_format_func = log_format_func_t_get(log_format_type);
  log_format_func(&can_log_output, &msg->log, log_backend_std_get_flags());
}

static int can_format_set(const struct log_backend *const backend,
                          uint32_t log_type) {
  log_format_type = log_type;
  return 0;
}

static void can_log_init(const struct log_backend *const backend) {
  ARG_UNUSED(backend);
  if (!device_is_ready(can_dev)) {
    printk("CAN device not ready");
    return;
  }
}

static void can_log_dropped(const struct log_backend *const backend,
                            uint32_t cnt) {
  log_backend_std_dropped(&can_log_output, cnt);
}

static void can_log_panic(const struct log_backend *const backend) {
  ARG_UNUSED(backend);
  panic_mode = true;
}

int can_log_is_ready(const struct log_backend *const backend) {
  ARG_UNUSED(backend);
  if (!device_is_ready(can_dev)) {
    return -ENODEV;
  }

  struct can_bus_err_cnt err_cnt;
  enum can_state state;

  int ret = can_get_state(can_dev, &state, &err_cnt);
  if (ret) {
    return -ENODEV;
  }

  if (state == CAN_STATE_ERROR_ACTIVE) {
    return 0;
  } else {
    return -EIO;
  }
}

struct log_backend_api can_log_backend_api = {
  .process = can_log_process,
  .format_set = can_format_set,
  .init = can_log_init,
  .is_ready = can_log_is_ready,
  .dropped = can_log_dropped,
  .panic = can_log_panic,
};

LOG_BACKEND_DEFINE(can_log_backend, can_log_backend_api, true);
