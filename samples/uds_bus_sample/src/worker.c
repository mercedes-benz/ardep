#include "uds.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(worker, LOG_LEVEL_INF);

static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

static uint16_t can_receive_addr = 0;
static uint16_t can_send_addr = 0;

static int receive_can_filter_id = -1;

static struct can_frame received_frame;
static void rx_cb_work_handler(struct k_work *work) {
  struct can_frame to_send = received_frame;

  if (to_send.dlc + 1 > CAN_MAX_DLC) {
    LOG_WRN(
        "Cannot append own signature to received frame, DLC would exceed %d "
        "bytes. Sending without signature.",
        CAN_MAX_DLC);
    can_send(can_dev, &to_send, K_FOREVER, NULL, NULL);
    return;
  }

  to_send.id = can_send_addr;
  to_send.dlc = received_frame.dlc + 1;

  // append a simple signature: XOR of the first byte and LSB of physical source
  // address
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

static UDSErr_t rw_data_by_id_check(const struct uds_context *const context,
                                    bool *apply_action) {
  *apply_action = true;

  return UDS_OK;
}

static UDSErr_t r_data_by_id_action(struct uds_context *const context,
                                    bool *consume_event) {
  UDSRDBIArgs_t *args = context->arg;

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

static UDSErr_t w_data_by_id_action(struct uds_context *const context,
                                    bool *consume_event) {
  UDSWDBIArgs_t *args = context->arg;

  switch (args->dataId) {
    case 0x1100: {
      if (args->len != sizeof(can_receive_addr)) {
        return UDS_NRC_IncorrectMessageLengthOrInvalidFormat;
      }
      memcpy(&can_receive_addr, args->data, args->len);
      can_receive_addr = sys_le16_to_cpu(can_receive_addr);

      // deregister previous filter if it exists
      if (receive_can_filter_id != -1) {
        can_remove_rx_filter(can_dev, receive_can_filter_id);
      }

      // register a new filter with set address
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
      can_send_addr = sys_le16_to_cpu(can_send_addr);

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
