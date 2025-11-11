/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uds_sample, LOG_LEVEL_DBG);

#include "uds.h"

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>

#include <ardep/iso14229.h>
#include <ardep/uds.h>

static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

struct uds_instance_t instance;

UDS_REGISTER_ECU_DEFAULT_HARD_RESET_HANDLER(&instance);

int main(void) {
  int err;
  LOG_INF("ARDEP UDS Bus Sample");

  UDSISOTpCConfig_t cfg = {
    // Hardware Addresses
    .source_addr = CONFIG_UDS_ADDR_PHYS_SA,
    .target_addr = CONFIG_UDS_ADDR_PHYS_TA,

    // Functional Addresses
    .source_addr_func = 0x7DF,
    .target_addr_func = UDS_TP_NOOP_ADDR,
  };

  uds_init(&instance, &cfg, can_dev, NULL);

  if (!device_is_ready(can_dev)) {
    LOG_INF("CAN device not ready");
    return -ENODEV;
  }

  err = can_set_mode(can_dev, CAN_MODE_NORMAL);
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

uint32_t can_receive_addr = 0;
uint32_t can_send_addr = 0;

int receive_can_filter_id = -1;

UDSErr_t rw_data_by_id_check(const struct uds_context *const context,
                             bool *apply_action) {
  *apply_action = true;

  return UDS_OK;
}

UDSErr_t r_data_by_id_action(struct uds_context *const context,
                             bool *consume_event) {
  UDSRDBIArgs_t *args = context->arg;

  // todo: improve
  switch (args->dataId) {
    case 0x1100:
      args->copy(context->server, &can_receive_addr, sizeof(can_receive_addr));
      break;
    case 0x1101:
      args->copy(context->server, &can_send_addr, sizeof(can_send_addr));
      break;
    default:
      *consume_event = false;
      return UDS_OK;
  }

  *consume_event = true;

  return UDS_PositiveResponse;
}

struct can_frame received_frame;
static void rx_cb_work_handler(struct k_work *work) {
  struct can_frame to_send = {
    .dlc = received_frame.dlc + 1,
    .id = can_send_addr,
  };
  memcpy(&to_send.data, &received_frame.data, received_frame.dlc);
  to_send.data[received_frame.dlc] =
      to_send.data[0] ^ (CONFIG_UDS_ADDR_PHYS_SA & 0xff);
  can_send(can_dev, &to_send, K_FOREVER, NULL, NULL);
  LOG_INF("Sent CAN frame ID 0x%03X DLC %d", to_send.id, to_send.dlc);
  LOG_HEXDUMP_INF(to_send.data, to_send.dlc, "Sent data: ");
}

K_WORK_DEFINE(rx_cb_work, rx_cb_work_handler);

static void rx_cb(const struct device *dev,
                  struct can_frame *frame,
                  void *user_data) {
  received_frame = *frame;
  LOG_INF("Received CAN frame ID 0x%03X DLC %d", frame->id, frame->dlc);

  k_work_submit(&rx_cb_work);
}

UDSErr_t w_data_by_id_action(struct uds_context *const context,
                             bool *consume_event) {
  UDSWDBIArgs_t *args = context->arg;

  // todo: improve
  switch (args->dataId) {
    case 0x1100: {
      if (args->len != sizeof(can_receive_addr)) {
        return UDS_NRC_IncorrectMessageLengthOrInvalidFormat;
      }
      memcpy(&can_receive_addr, args->data, args->len);
      // todo: register can filter
      if (receive_can_filter_id != -1) {
        can_remove_rx_filter(can_dev, receive_can_filter_id);
      }
      struct can_filter filter = {
        .flags = 0,
        .id = can_receive_addr,
        .mask = CAN_STD_ID_MASK,
      };
      receive_can_filter_id = can_add_rx_filter(can_dev, rx_cb, NULL, &filter);
      LOG_INF("Registered CAN RX filter for ID 0x%03X", can_receive_addr);
      if (receive_can_filter_id < 0) {
        LOG_ERR("Failed to register CAN RX filter");
        return UDS_NRC_ConditionsNotCorrect;
      }
    }

    break;
    case 0x1101:
      if (args->len != sizeof(can_send_addr)) {
        return UDS_NRC_IncorrectMessageLengthOrInvalidFormat;
      }
      memcpy(&can_send_addr, args->data, args->len);
      LOG_INF("Set CAN send address to 0x%03X", can_send_addr);
      break;
    default:
      *consume_event = false;
      return UDS_OK;
  }

  *consume_event = true;

  return UDS_PositiveResponse;
}

UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(&instance,
                                        0x1100,
                                        NULL,
                                        rw_data_by_id_check,
                                        r_data_by_id_action,
                                        rw_data_by_id_check,
                                        w_data_by_id_action,
                                        NULL,
                                        NULL,
                                        NULL);
UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(&instance,
                                        0x1101,
                                        NULL,
                                        rw_data_by_id_check,
                                        r_data_by_id_action,
                                        rw_data_by_id_check,
                                        w_data_by_id_action,
                                        NULL,
                                        NULL,
                                        NULL);

K_SEM_DEFINE(master_receive_sem, 0, 1);

static void master_routine_rx(const struct device *dev,
                              struct can_frame *frame,
                              void *user_data) {
  struct can_frame *user_data_type = user_data;
  *user_data_type = *frame;
  k_sem_give(&master_receive_sem);
}

uint8_t routine_result[9];  // first byte for status + 8 bytes data

static void master_routine(struct k_work *work) {
  ARG_UNUSED(work);

  struct can_frame master_received_frame;
  struct can_filter rx_filter = {
    .id = 0x000,
    .mask = CAN_STD_ID_MASK,
    .flags = 0,
  };

  LOG_INF("Adding filter for master routine");
  int filter_id = can_add_rx_filter(can_dev, master_routine_rx,
                                    &master_received_frame, &rx_filter);

  struct can_frame initial_frame = {
    .id = can_send_addr,
    .data = {0xde},
    .dlc = 1,
    .flags = 0,
  };

  LOG_INF("Sending master routine start frame");
  int ret = can_send(can_dev, &initial_frame, K_FOREVER, NULL, NULL);
  if (ret != 0) {
    LOG_ERR("Can send returned err: %d", ret);
    return;
  }

  ret = k_sem_take(&master_receive_sem, K_MSEC(1000));

  can_remove_rx_filter(can_dev, filter_id);

  if (ret != 0) {
    routine_result[0] = 0x01;  // error
    LOG_ERR("Timeout waiting for cycle response");
  } else {
    routine_result[0] = 0x00;  // success
    memcpy(&routine_result[1], &master_received_frame.data,
           MIN(master_received_frame.dlc, 8));
    LOG_INF("Master routine received %d bytes", master_received_frame.dlc);
  }
}

K_WORK_DEFINE(master_routine_work, master_routine);

static UDSErr_t async_routine_control_check(
    const struct uds_context *const context, bool *apply_action) {
  *apply_action = true;
  return UDS_OK;
}

static UDSErr_t async_routine_control_action(struct uds_context *const context,
                                             bool *consume_event) {
  UDSRoutineCtrlArgs_t *args = context->arg;
  // struct uds_async_routine_data *data =
  //     context->registration->routine_control.user_context;

  *consume_event = true;

  switch (args->ctrlType) {
    case UDS_ROUTINE_CONTROL__START_ROUTINE: {
      routine_result[0] = 0xFF;  // default to not completed
      // Check if work is already running
      if (k_work_is_pending(&master_routine_work)) {
        return UDS_NRC_RequestSequenceError;
      }

      // // Check if work is already completed or stopped (should be idle to
      // start) k_mutex_lock(&async_work_mutex, K_FOREVER); if
      // (data->work_data->status == ASYNC_WORK_STATUS_RUNNING) {
      //   k_mutex_unlock(&async_work_mutex);
      //   return UDS_NRC_RequestSequenceError;
      // }
      // k_mutex_unlock(&async_work_mutex);

      // Schedule the work to start immediately
      int ret = k_work_submit(&master_routine_work);
      if (ret < 0) {
        LOG_ERR("Failed to schedule async work: %d", ret);
        return UDS_NRC_ConditionsNotCorrect;
      }

      LOG_INF("Submitted master routine");

      // Return success status for start
      return UDS_PositiveResponse;
    }

    case UDS_ROUTINE_CONTROL__STOP_ROUTINE: {
      // k_mutex_lock(&async_work_mutex, K_FOREVER);
      // if (data->work_data->status != ASYNC_WORK_STATUS_RUNNING) {
      //   k_mutex_unlock(&async_work_mutex);
      //   return UDS_NRC_RequestSequenceError;
      // }

      LOG_INF("Stopping master routine (not supported)");
      return UDS_NRC_SubFunctionNotSupported;

      // // Signal the work to stop
      // data->work_data->force_stop = true;
      // k_mutex_unlock(&async_work_mutex);

      // Cancel pending work if still scheduled
      // k_work_cancel(&master_routine_work);

      // // Return stop acknowledgment
      // uint8_t stop_status = ASYNC_WORK_STATUS_STOPPED;
      // return args->copyStatusRecord(context->server, &stop_status,
      //                               sizeof(stop_status));
    }

    case UDS_ROUTINE_CONTROL__REQUEST_ROUTINE_RESULTS: {
      // k_mutex_lock(&async_work_mutex, K_FOREVER);
      // LOG_INF("Sending async routine result");

      // uint8_t output[5] = {0};
      // output[0] = data->work_data->status;

      // // Include progress data
      // uint32_t progress_be =
      // sys_cpu_to_be32(data->work_data->current_progress); memcpy(&output[1],
      // &progress_be, sizeof(progress_be));

      // k_mutex_unlock(&async_work_mutex);

      return args->copyStatusRecord(context->server, routine_result,
                                    sizeof(routine_result));
    }

    default:
      return UDS_NRC_SubFunctionNotSupported;
  }
}

UDS_REGISTER_ROUTINE_CONTROL_HANDLER(&instance,
                                     0x0000,
                                     async_routine_control_check,
                                     async_routine_control_action,
                                     NULL);
