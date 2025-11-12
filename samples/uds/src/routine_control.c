/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_sample, LOG_LEVEL_DBG);

#include "ardep/uds.h"
#include "uds.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#define SYNCHRONOUS_ROUTINE_ID 0x1234
#define ASYNCHRONOUS_ROUTINE_ID 0x5678

static UDSErr_t sync_routine_control_check(
    const struct uds_context *const context, bool *apply_action) {
  UDSRoutineCtrlArgs_t *args = context->arg;

  // We don't need to check the routine id, the framework already did that for
  // us

  if (args->len != 4) {
    // we operate on an uint32_t so we check the length of the status record
    return UDS_NRC_IncorrectMessageLengthOrInvalidFormat;
  }

  *apply_action = true;
  return UDS_OK;
}

static uint32_t sync_action(uint32_t input) {
  // Simulate some work
  k_msleep(30);
  return ~input;
}

static UDSErr_t sync_routine_control_action(struct uds_context *const context,
                                            bool *consume_event) {
  UDSRoutineCtrlArgs_t *args = context->arg;
  uint32_t input = sys_be32_to_cpu(*(uint32_t *)args->optionRecord);

  uint32_t output = sys_cpu_to_be32(sync_action(input));

  // Signal this action consumes the event
  *consume_event = true;

  return args->copyStatusRecord(context->server, &output, sizeof(output));
}

UDS_REGISTER_ROUTINE_CONTROL_HANDLER(&instance,
                                     SYNCHRONOUS_ROUTINE_ID,
                                     sync_routine_control_check,
                                     sync_routine_control_action,
                                     NULL)

enum async_work_status {
  ASYNC_WORK_STATUS_IDLE = 0,
  ASYNC_WORK_STATUS_SUCCESS = 1,
  ASYNC_WORK_STATUS_STOPPED = 2,
  ASYNC_WORK_STATUS_RUNNING = 3,
};

struct uds_async_work_data {
  uint32_t current_progress;
  enum async_work_status status;
  bool force_stop;
} async_work_data;

K_MUTEX_DEFINE(async_work_mutex);

void async_action(struct k_work *work) {
  k_mutex_lock(&async_work_mutex, K_FOREVER);
  if (async_work_data.status == ASYNC_WORK_STATUS_RUNNING) {
    // work is already running, so we do nothing
    k_mutex_unlock(&async_work_mutex);
    return;
  }

  async_work_data.status = ASYNC_WORK_STATUS_RUNNING;
  async_work_data.current_progress = 0;
  async_work_data.force_stop = false;
  k_mutex_unlock(&async_work_mutex);

  for (size_t i = 0; i < 3000; i++) {
    // Simulate some work
    k_msleep(1);
    k_mutex_lock(&async_work_mutex, K_FOREVER);
    if (async_work_data.force_stop) {
      async_work_data.status = ASYNC_WORK_STATUS_STOPPED;
      k_mutex_unlock(&async_work_mutex);
      return;
    }
    async_work_data.current_progress += 1;
    k_mutex_unlock(&async_work_mutex);
  }

  k_mutex_lock(&async_work_mutex, K_FOREVER);
  async_work_data.status = ASYNC_WORK_STATUS_SUCCESS;
  k_mutex_unlock(&async_work_mutex);
}

K_WORK_DELAYABLE_DEFINE(async_routine_work, async_action);

struct uds_async_routine_data {
  struct uds_async_work_data *work_data;
  struct k_work_delayable *work;
} async_routine_data = {
  .work_data = &async_work_data,
  .work = &async_routine_work,
};

static UDSErr_t async_routine_control_check(
    const struct uds_context *const context, bool *apply_action) {
  *apply_action = true;
  return UDS_OK;
}

static UDSErr_t async_routine_control_action(struct uds_context *const context,
                                             bool *consume_event) {
  UDSRoutineCtrlArgs_t *args = context->arg;
  struct uds_async_routine_data *data =
      context->registration->routine_control.user_context;

  *consume_event = true;

  switch (args->ctrlType) {
    case UDS_ROUTINE_CONTROL__START_ROUTINE: {
      // Check if work is already running
      if (k_work_delayable_is_pending(data->work)) {
        return UDS_NRC_RequestSequenceError;
      }

      // Check if work is already completed or stopped (should be idle to start)
      k_mutex_lock(&async_work_mutex, K_FOREVER);
      if (data->work_data->status == ASYNC_WORK_STATUS_RUNNING) {
        k_mutex_unlock(&async_work_mutex);
        return UDS_NRC_RequestSequenceError;
      }
      k_mutex_unlock(&async_work_mutex);

      // Schedule the work to start immediately
      int ret = k_work_schedule(data->work, K_NO_WAIT);
      if (ret < 0) {
        LOG_ERR("Failed to schedule async work: %d", ret);
        return UDS_NRC_ConditionsNotCorrect;
      }

      LOG_INF("Started async routine");

      // Return success status for start
      return UDS_PositiveResponse;
    }

    case UDS_ROUTINE_CONTROL__STOP_ROUTINE: {
      k_mutex_lock(&async_work_mutex, K_FOREVER);
      if (data->work_data->status != ASYNC_WORK_STATUS_RUNNING) {
        k_mutex_unlock(&async_work_mutex);
        return UDS_NRC_RequestSequenceError;
      }

      LOG_INF("Stopping async routine");

      // Signal the work to stop
      data->work_data->force_stop = true;
      k_mutex_unlock(&async_work_mutex);

      // Cancel pending work if still scheduled
      k_work_cancel_delayable(data->work);

      // Return stop acknowledgment
      uint8_t stop_status = ASYNC_WORK_STATUS_STOPPED;
      return args->copyStatusRecord(context->server, &stop_status,
                                    sizeof(stop_status));
    }

    case UDS_ROUTINE_CONTROL__REQUEST_ROUTINE_RESULTS: {
      k_mutex_lock(&async_work_mutex, K_FOREVER);
      LOG_INF("Sending async routine result");

      uint8_t output[5] = {0};
      output[0] = data->work_data->status;

      // Include progress data
      uint32_t progress_be = sys_cpu_to_be32(data->work_data->current_progress);
      memcpy(&output[1], &progress_be, sizeof(progress_be));

      k_mutex_unlock(&async_work_mutex);

      return args->copyStatusRecord(context->server, output, sizeof(output));
    }

    default:
      return UDS_NRC_SubFunctionNotSupported;
  }
}

UDS_REGISTER_ROUTINE_CONTROL_HANDLER(&instance,
                                     ASYNCHRONOUS_ROUTINE_ID,
                                     async_routine_control_check,
                                     async_routine_control_action,
                                     &async_routine_data)
