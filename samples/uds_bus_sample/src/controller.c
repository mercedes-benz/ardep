#include "uds.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(controller, LOG_LEVEL_INF);

static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

#define CONTROLLER_CAN_SEND_ADDR 0x001
#define CONTROLLER_CAN_RECEIVE_ADDR 0x000
#define CONTROLLER_CAN_TIMEOUT_MS 1000

enum controller_routine_status {
  CONTROLLER_ROUTINE_STATUS__SUCCESS = 0x00,
  CONTROLLER_ROUTINE_STATUS__ERROR = 0x01,
  CONTROLLER_ROUTINE_STATUS__NOT_COMPLETED = 0xFF,
};

struct {
  bool running;  // set at the start of the work, cleared at the end
  uint8_t routine_result[9];  // 1 byte status + 8 bytes data
} controller_routine_data;
K_MUTEX_DEFINE(controller_routine_data_mutex);

K_SEM_DEFINE(controller_receive_sem, 0, 1);

static void controller_routine_rx(const struct device *dev,
                                  struct can_frame *frame,
                                  void *user_data) {
  struct can_frame *user_data_type = user_data;
  *user_data_type = *frame;
  k_sem_give(&controller_receive_sem);
}

static void controller_routine(struct k_work *work) {
  ARG_UNUSED(work);

  k_mutex_lock(&controller_routine_data_mutex, K_FOREVER);
  controller_routine_data.running = true;
  k_mutex_unlock(&controller_routine_data_mutex);

  struct can_frame controller_received_frame;
  struct can_filter rx_filter = {
    .id = CONTROLLER_CAN_RECEIVE_ADDR,
    .mask = CAN_STD_ID_MASK,
    .flags = 0,
  };

  LOG_INF("Adding filter for controller routine");
  int filter_id = can_add_rx_filter(can_dev, controller_routine_rx,
                                    &controller_received_frame, &rx_filter);

  struct can_frame initial_frame = {
    .id = CONTROLLER_CAN_SEND_ADDR,
    .data = {0xde},
    .dlc = 1,
    .flags = 0,
  };

  LOG_INF("Sending controller routine start frame");
  int ret = can_send(can_dev, &initial_frame, K_FOREVER, NULL, NULL);
  if (ret != 0) {
    LOG_ERR("Can send returned err: %d", ret);
    k_mutex_lock(&controller_routine_data_mutex, K_FOREVER);
    controller_routine_data.routine_result[0] =
        CONTROLLER_ROUTINE_STATUS__ERROR;  // error
    controller_routine_data.running = false;
    k_mutex_unlock(&controller_routine_data_mutex);
    return;
  }

  ret = k_sem_take(&controller_receive_sem, K_MSEC(CONTROLLER_CAN_TIMEOUT_MS));

  can_remove_rx_filter(can_dev, filter_id);

  k_mutex_lock(&controller_routine_data_mutex, K_FOREVER);
  if (ret != 0) {
    controller_routine_data.routine_result[0] =
        CONTROLLER_ROUTINE_STATUS__ERROR;  // error
    LOG_ERR("Timeout waiting for cycle response");
  } else {
    controller_routine_data.routine_result[0] =
        CONTROLLER_ROUTINE_STATUS__SUCCESS;  // success
    memcpy(&controller_routine_data.routine_result[1],
           &controller_received_frame.data,
           MIN(controller_received_frame.dlc, 8));
    LOG_INF("Controller routine received %d bytes",
            controller_received_frame.dlc);
  }
  controller_routine_data.running = false;
  k_mutex_unlock(&controller_routine_data_mutex);
}

K_WORK_DEFINE(controller_routine_work, controller_routine);

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
      // Check if work is already running
      if (k_work_is_pending(&controller_routine_work)) {
        return UDS_NRC_RequestSequenceError;
      }

      k_mutex_lock(&controller_routine_data_mutex, K_FOREVER);
      if (controller_routine_data.running) {
        k_mutex_unlock(&controller_routine_data_mutex);
        return UDS_NRC_RequestSequenceError;
      }

      controller_routine_data.routine_result[0] =
          CONTROLLER_ROUTINE_STATUS__NOT_COMPLETED;  // default to not completed
      k_mutex_unlock(&controller_routine_data_mutex);

      // Schedule the work to start immediately
      int ret = k_work_submit(&controller_routine_work);
      if (ret < 0) {
        LOG_ERR("Failed to schedule async work: %d", ret);
        return UDS_NRC_ConditionsNotCorrect;
      }

      LOG_INF("Submitted controller routine to work queue");

      // Return success status for start
      return UDS_PositiveResponse;
    }

    case UDS_ROUTINE_CONTROL__STOP_ROUTINE: {
      LOG_INF("Stopping controller routine (not supported)");
      return UDS_NRC_SubFunctionNotSupported;
    }

    case UDS_ROUTINE_CONTROL__REQUEST_ROUTINE_RESULTS: {
      k_mutex_lock(&controller_routine_data_mutex, K_FOREVER);
      uint8_t routine_result[9];
      memcpy(routine_result, controller_routine_data.routine_result,
             sizeof(routine_result));
      k_mutex_unlock(&controller_routine_data_mutex);

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
