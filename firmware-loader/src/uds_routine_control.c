/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "iso14229.h"
#include "zephyr/kernel.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(firmware_loader, CONFIG_APP_LOG_LEVEL);

#include "uds.h"

#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>

#include <ardep/uds.h>

enum uds_memory_erasure_routine_state {
  UDS_MEMORY_ERASURE_STATE__NOT_STARTED = 0,
  UDS_MEMORY_ERASURE_STATE__IN_PROGRESS = 1,
  UDS_MEMORY_ERASURE_STATE__COMPLETED = 2,
};

struct uds_memory_erasure_routine_status {
  enum uds_memory_erasure_routine_state state;
  struct k_mutex *mutex;
  uint8_t result;
};

K_MUTEX_DEFINE(memory_erasure_routine_mutex);

static struct uds_memory_erasure_routine_status erasure_status = {
  .state = UDS_MEMORY_ERASURE_STATE__NOT_STARTED,
  .result = 0,
  .mutex = &memory_erasure_routine_mutex,
};

UDSErr_t erase_memory_routine_check(const struct uds_context *const context,
                                    bool *apply_action) {
  UDSRoutineCtrlArgs_t *args = (UDSRoutineCtrlArgs_t *)context->arg;

  struct uds_memory_erasure_routine_status *status =
      (struct uds_memory_erasure_routine_status *)
          context->registration->routine_control.user_context;

  if (args->ctrlType == UDS_ROUTINE_CONTROL__START_ROUTINE &&
      status->state == UDS_MEMORY_ERASURE_STATE__IN_PROGRESS) {
    *apply_action = false;
    return UDS_NRC_RequestSequenceError;
  }

  if (args->ctrlType == UDS_ROUTINE_CONTROL__REQUEST_ROUTINE_RESULTS &&
      status->state != UDS_MEMORY_ERASURE_STATE__COMPLETED) {
    *apply_action = false;
    return UDS_NRC_RequestSequenceError;
  }

  if (args->ctrlType == UDS_ROUTINE_CONTROL__STOP_ROUTINE) {
    *apply_action = false;
    return UDS_NRC_SubFunctionNotSupported;
  }

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t erase_memory_routine_action(struct uds_context *const context,
                                     bool *consume_event) {
  UDSRoutineCtrlArgs_t *args = (UDSRoutineCtrlArgs_t *)context->arg;

  struct uds_memory_erasure_routine_status *status =
      (struct uds_memory_erasure_routine_status *)
          context->registration->routine_control.user_context;

  if (args->ctrlType == UDS_ROUTINE_CONTROL__START_ROUTINE) {
    k_mutex_lock(status->mutex, K_FOREVER);
    status->state = UDS_MEMORY_ERASURE_STATE__IN_PROGRESS;
    k_mutex_unlock(status->mutex);
  }
}

UDS_REGISTER_ROUTINE_CONTROL_HANDLER(&instance,
                                     0xFF00,
                                     erase_memory_routine_check,
                                     erase_memory_routine_action,
                                     &erasure_status);