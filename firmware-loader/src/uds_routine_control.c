/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "iso14229.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(firmware_loader, CONFIG_APP_LOG_LEVEL);

#include "uds.h"

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/byteorder.h>
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
  int32_t result;
  struct k_work_delayable work;
};

K_MUTEX_DEFINE(memory_erasure_routine_mutex);

static void erase_slot0_work_handler(struct k_work *work);

static struct uds_memory_erasure_routine_status erasure_status = {
  .state = UDS_MEMORY_ERASURE_STATE__NOT_STARTED,
  .result = UDS_OK,
  .mutex = &memory_erasure_routine_mutex,
};

static int erase_memory_routine_init(void) {
  k_work_init_delayable(&erasure_status.work, erase_slot0_work_handler);
  return 0;
}

SYS_INIT(erase_memory_routine_init,
         APPLICATION,
         CONFIG_APPLICATION_INIT_PRIORITY);

static void erase_slot0_work_handler(struct k_work *work) {
  struct k_work_delayable *dwork = k_work_delayable_from_work(work);
  struct uds_memory_erasure_routine_status *status =
      CONTAINER_OF(dwork, struct uds_memory_erasure_routine_status, work);

  const struct flash_area *fa;
  int rc = flash_area_open(FIXED_PARTITION_ID(slot0_partition), &fa);

  int32_t result = UDS_OK;

  if (rc < 0) {
    LOG_ERR("Failed to open slot0 partition: %d", rc);
    result = UDS_NRC_GeneralProgrammingFailure;
  } else {
    LOG_INF("Erasing slot0 partition at 0x%08x, size: %zu bytes",
            (uint32_t)fa->fa_off, fa->fa_size);

    rc = flash_area_erase(fa, 0, fa->fa_size);

    if (rc < 0) {
      LOG_ERR("Failed to erase slot0 partition: %d", rc);
    } else {
      LOG_INF("Successfully erased slot0 partition");
    }

    flash_area_close(fa);
  }

  k_mutex_lock(status->mutex, K_FOREVER);
  status->result = result;
  status->state = UDS_MEMORY_ERASURE_STATE__COMPLETED;
  k_mutex_unlock(status->mutex);
}

UDSErr_t erase_memory_routine_check(const struct uds_context *const context,
                                    bool *apply_action) {
  LOG_INF("Routine Control CHECK");
  UDSRoutineCtrlArgs_t *args = (UDSRoutineCtrlArgs_t *)context->arg;

  struct uds_memory_erasure_routine_status *status =
      (struct uds_memory_erasure_routine_status *)
          context->registration->routine_control.user_context;

  if (args->ctrlType == UDS_ROUTINE_CONTROL__START_ROUTINE &&
      status->state == UDS_MEMORY_ERASURE_STATE__IN_PROGRESS) {
    LOG_WRN("Memory erasure routine already in progress");
    *apply_action = false;
    return UDS_NRC_RequestSequenceError;
  }

  if (args->ctrlType == UDS_ROUTINE_CONTROL__REQUEST_ROUTINE_RESULTS &&
      status->state != UDS_MEMORY_ERASURE_STATE__COMPLETED) {
    LOG_WRN("Memory erasure routine not completed yet");
    *apply_action = false;
    return UDS_NRC_RequestSequenceError;
  }

  if (args->ctrlType == UDS_ROUTINE_CONTROL__STOP_ROUTINE) {
    *apply_action = false;
    LOG_WRN("Stop routine not supported");
    return UDS_NRC_SubFunctionNotSupported;
  }

  *apply_action = true;
  LOG_INF("Routine Control CHECK finished");
  return UDS_OK;
}

UDSErr_t erase_memory_routine_action(struct uds_context *const context,
                                     bool *consume_event) {
  LOG_INF("Routine Control Action");
  UDSRoutineCtrlArgs_t *args = (UDSRoutineCtrlArgs_t *)context->arg;

  struct uds_memory_erasure_routine_status *status =
      (struct uds_memory_erasure_routine_status *)
          context->registration->routine_control.user_context;

  *consume_event = true;

  switch (args->ctrlType) {
    case UDS_ROUTINE_CONTROL__START_ROUTINE: {
      k_mutex_lock(status->mutex, K_FOREVER);
      status->state = UDS_MEMORY_ERASURE_STATE__IN_PROGRESS;
      status->result = UDS_OK;
      k_mutex_unlock(status->mutex);

      // Submit the work item to erase slot0 with a 10ms delay
      // So the response can be sent first
      k_work_schedule(&status->work, K_MSEC(150));

      LOG_INF("Memory erasure routine started");
      return UDS_PositiveResponse;
    }
    case UDS_ROUTINE_CONTROL__REQUEST_ROUTINE_RESULTS: {
      k_mutex_lock(status->mutex, K_FOREVER);
      enum uds_memory_erasure_routine_state current_state = status->state;
      int32_t result = status->result;
      k_mutex_unlock(status->mutex);

      if (current_state != UDS_MEMORY_ERASURE_STATE__COMPLETED) {
        LOG_WRN("Memory erasure routine not completed yet");
        return UDS_NRC_RequestSequenceError;
      }
      LOG_INF("Memory erasure routine requested result: 0x%02x", result);

      int32_t be_result = sys_cpu_to_be32(result);
      return args->copyStatusRecord(context->server, &be_result,
                                    sizeof(be_result));
    }
    default:
      LOG_WRN("Unsupported control type: 0x%02x", args->ctrlType);
      *consume_event = false;
      return UDS_NRC_SubFunctionNotSupported;
  }
}

UDS_REGISTER_ROUTINE_CONTROL_HANDLER(&instance,
                                     0xFF00,
                                     erase_memory_routine_check,
                                     erase_memory_routine_action,
                                     &erasure_status);
