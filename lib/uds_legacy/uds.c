/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/canbus/isotp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uds_legacy, CONFIG_UDS_LEGACY_LOG_LEVEL);

#include "uds.h"
#include "uds_session.h"

#include <zephyr/dfu/flash_img.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/reboot.h>

#define STACKSIZE 10240
#define PRIORITY 7

const struct isotp_fc_opts fc_opts = {.bs = 0, .stmin = 10};

const struct isotp_msg_id rx_addr = {
  .std_id = 0x80,
};
const struct isotp_msg_id tx_addr = {
  .std_id = 0x180,
};

const struct device *can_dev;
struct isotp_recv_ctx recv_ctx;

static int init_can() {
  int ret;
  can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
  if (!device_is_ready(can_dev)) {
    LOG_ERR("CAN: Device driver not ready.\n");
    return 0;
  }

  enum can_state state;
  ret = can_get_state(can_dev, &state, NULL);

  if (ret != 0) {
    LOG_ERR("CAN: Failed to get state [%d]\n", ret);
    return ret;
  }

  if (state != CAN_STATE_STOPPED) {
    LOG_INF("can already started, skipping start\n");
    return 0;
  }

  ret = can_set_mode(can_dev, 0);
  if (ret != 0) {
    LOG_ERR("CAN: Failed to set mode [%d]", ret);
    return ret;
  }

  ret = can_start(can_dev);
  if (ret != 0) {
    LOG_ERR("CAN: Failed to start device [%d]\n", ret);
    return ret;
  }

  LOG_DBG("CAN controller initialized: %s", can_dev->name);

  return 0;
}

static void send_complete_cb(int error_nr, void *arg) {
  ARG_UNUSED(arg);
  LOG_DBG("TX complete cb [%d]\n", error_nr);
}

static struct isotp_send_ctx send_ctx;
void send_negative_response(enum UDS_SID request_sid,
                            enum UDS_NRC response_code) {
  int ret;
  uint8_t tx_data[] = {UDS_SID_NEGATIVE_RESPONSE, request_sid, response_code};
  ret = isotp_send(&send_ctx, can_dev, tx_data, sizeof(tx_data), &tx_addr,
                   &rx_addr, send_complete_cb, NULL);
  if (ret != ISOTP_N_OK) {
    LOG_ERR("Error while sending data to ID %d [%d]\n", tx_addr.std_id, ret);
  }
}

static void send_session_control_response(enum UdsSessionState session_type,
                                          uint16_t p2_timeout_ms,
                                          uint16_t p2_star_timeout_ms) {
  int ret;
  uint8_t positive_sid = UDS_SID_DIAGNOSTIC_SESSION_CONTROL + 0x40;

  uint8_t tx_data[] = {
    positive_sid,
    session_type,
    (p2_timeout_ms >> 8) & 0xFF,
    p2_timeout_ms & 0xFF,
    ((p2_star_timeout_ms / 10) >> 8) & 0xFF,
    (p2_star_timeout_ms / 10) & 0xFF,
  };

  ret = isotp_send(&send_ctx, can_dev, tx_data, sizeof(tx_data), &tx_addr,
                   &rx_addr, send_complete_cb, NULL);
  if (ret != ISOTP_N_OK) {
    LOG_ERR("Error while sending data to ID %d [%d]\n", tx_addr.std_id, ret);
  }
}

void send_positive_reset_response(uint8_t reset_type) {
  int ret;
  uint8_t positive_sid = UDS_SID_ECU_RESET + 0x40;

  uint8_t tx_data[] = {positive_sid, reset_type};

  ret = isotp_send(&send_ctx, can_dev, tx_data, sizeof(tx_data), &tx_addr,
                   &rx_addr, send_complete_cb, NULL);
  if (ret != ISOTP_N_OK) {
    LOG_ERR("Error while sending data to ID %d [%d]\n", tx_addr.std_id, ret);
  }
}

void handle_ecu_reset(uint8_t *data, size_t len) {
  if (len != 2) {
    send_negative_response(UDS_SID_ECU_RESET,
                           UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT);
    return;
  }

  uint8_t reset_type = data[1];

  if (reset_type == UDS_RESET_TYPE_HARD) {
    send_positive_reset_response(UDS_RESET_TYPE_HARD);

    k_sleep(K_MSEC(100));

    LOG_DBG("Hard reset\n");
    sys_reboot(SYS_REBOOT_COLD);
  } else {
    send_negative_response(UDS_SID_ECU_RESET,
                           UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
  }
}

void send_request_download_response() {
  int ret;
  uint8_t positive_sid = UDS_SID_REQUEST_DOWNLOAD + 0x40;

  // TODO: get max packet length from configured buffers sizes
  // e.g. size of net buffers

  uint8_t tx_data[] = {
    positive_sid,
    0x20,  // positive response code
    0x00,
    0xD2,  // block size = 258 - 2
  };

  ret = isotp_send(&send_ctx, can_dev, tx_data, sizeof(tx_data), &tx_addr,
                   &rx_addr, send_complete_cb, NULL);
  if (ret != ISOTP_N_OK) {
    LOG_ERR("Error while sending data to ID %d [%d]\n", tx_addr.std_id, ret);
  }
}

static void send_transfer_data_response(uint8_t block_sequence_counter) {
  int ret;
  uint8_t positive_sid = UDS_SID_TRANSFER_DATA + 0x40;

  uint8_t tx_data[] = {positive_sid, block_sequence_counter};

  ret = isotp_send(&send_ctx, can_dev, tx_data, sizeof(tx_data), &tx_addr,
                   &rx_addr, send_complete_cb, NULL);
  if (ret != ISOTP_N_OK) {
    LOG_ERR("Error while sending data to ID %d [%d]\n", tx_addr.std_id, ret);
  }
}

static void send_routine_control_result(uint16_t routine_id, uint8_t result) {
  int ret;
  uint8_t positive_sid = UDS_SID_ROUTINE_CONTROL + 0x40;

  uint8_t tx_data[] = {positive_sid, 0x1, (routine_id >> 8) & 0xFF,
                       routine_id & 0xFF, result};

  ret = isotp_send(&send_ctx, can_dev, tx_data, sizeof(tx_data), &tx_addr,
                   &rx_addr, send_complete_cb, NULL);
  if (ret != ISOTP_N_OK) {
    LOG_ERR("Error while sending data to ID %d [%d]\n", tx_addr.std_id, ret);
  }
}

static bool flash_context_initialized = false;
static uint8_t block_id = 0xF;
static struct flash_img_context flash_context;

static void handle_erase_routine() {
  int ret;

  send_negative_response(UDS_SID_ROUTINE_CONTROL,
                         UDS_NRC_REQUEST_RECEIVED_RESPONSE_PENDING);

  ret = flash_img_init(&flash_context);

  if (ret != 0) {
    LOG_ERR("Error while initializing flash context [%d]\n", ret);
    send_routine_control_result(1337, ret);
    return;
  }

  flash_context_initialized = true;

  // LOG_DBG("Start erase");

  // ret = boot_erase_img_bank(flash_context.flash_area->fa_id);

  // if (ret != 0) {
  //   LOG_ERR("Error while erasing flash [%d]\n", ret);
  //   send_routine_control_result(1337, ret);
  //   return;
  // }

  // LOG_DBG("Erase done");

  block_id = 0xFF;

  send_routine_control_result(1337, 0);
}

static void handle_confirm_routine() {
  int ret;

  ret = boot_request_upgrade(BOOT_UPGRADE_PERMANENT);

  send_routine_control_result(1338, 0);
}

static void handle_transfer_data(uint8_t *data, size_t len) {
  int ret;

  if (len < 2) {
    send_negative_response(UDS_SID_TRANSFER_DATA,
                           UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT);
    return;
  }

  uint8_t block_sequence_counter = data[1];

  if (block_sequence_counter == block_id) {
    // retransmit of last block
    send_transfer_data_response(block_id);
    return;
  }

  block_id++;

  if (block_sequence_counter != block_id) {
    LOG_ERR("Block sequence counter mismatch: %d != %d\n",
            block_sequence_counter, block_id);
    send_negative_response(UDS_SID_TRANSFER_DATA,
                           UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT);
    return;
  }

  if (!flash_context_initialized) {
    LOG_ERR("Flash context not initialized\n");
    send_negative_response(
        UDS_SID_TRANSFER_DATA,
        UDS_NRC_FAILURE_PREVENTS_EXECUTION_OF_REQUESTED_ACTION);
    return;
  }

  ret = flash_img_buffered_write(&flash_context, data + 2, len - 2, false);
  if (ret != 0) {
    LOG_ERR("Error while writing to flash [%d]\n", ret);

    send_negative_response(
        UDS_SID_TRANSFER_DATA,
        UDS_NRC_FAILURE_PREVENTS_EXECUTION_OF_REQUESTED_ACTION);
  }

  send_transfer_data_response(block_id);
}

static void send_transer_exit_positive_response() {
  int ret;
  uint8_t positive_sid = UDS_SID_REQUEST_TRANSFER_EXIT + 0x40;

  uint8_t tx_data[] = {positive_sid};

  ret = isotp_send(&send_ctx, can_dev, tx_data, sizeof(tx_data), &tx_addr,
                   &rx_addr, send_complete_cb, NULL);
  if (ret != ISOTP_N_OK) {
    LOG_ERR("Error while sending data to ID %d [%d]\n", tx_addr.std_id, ret);
  }
}

static void handle_transfer_exit() {
  int ret;

  if (!flash_context_initialized) {
    LOG_ERR("Flash context not initialized\n");
    send_negative_response(
        UDS_SID_REQUEST_TRANSFER_EXIT,
        UDS_NRC_FAILURE_PREVENTS_EXECUTION_OF_REQUESTED_ACTION);
    return;
  }

  // flush remaining data
  ret = flash_img_buffered_write(&flash_context, NULL, 0, true);
  if (ret != 0) {
    LOG_ERR("Error while writing to flash [%d]\n", ret);

    send_negative_response(
        UDS_SID_REQUEST_TRANSFER_EXIT,
        UDS_NRC_FAILURE_PREVENTS_EXECUTION_OF_REQUESTED_ACTION);
  }

  flash_context_initialized = false;

  send_transer_exit_positive_response();
}

static void thread_entry(void *arg1, void *arg2, void *arg3) {
  ARG_UNUSED(arg1);
  ARG_UNUSED(arg2);
  ARG_UNUSED(arg3);
  int ret, received_len;
  static uint8_t rx_buffer[512];

  // wait for other threads to initialize can
  k_sleep(K_MSEC(1000));

  ret = init_can();
  if (ret != 0) {
    LOG_ERR("CAN: Failed to init device [%d]\n", ret);
    return;
  }

  ret =
      isotp_bind(&recv_ctx, can_dev, &rx_addr, &tx_addr, &fc_opts, K_MSEC(50));
  if (ret != ISOTP_N_OK) {
    LOG_ERR("Failed to bind to rx ID %d [%d]\n", rx_addr.std_id, ret);
    return;
  }

  while (1) {
    received_len =
        isotp_recv(&recv_ctx, rx_buffer, sizeof(rx_buffer) - 1U, K_MSEC(2000));
    if (received_len < 0) {
      LOG_DBG("Receiving error [%d]\n", received_len);
      continue;
    }

    LOG_DBG("Received %d bytes\n", received_len);
    LOG_HEXDUMP_DBG(rx_buffer, received_len, "RX");
    k_sleep(K_MSEC(50));

    uint8_t sid = rx_buffer[0];

    if (sid == UDS_SID_DIAGNOSTIC_SESSION_CONTROL) {
      if (received_len != 2) {
        send_negative_response(
            sid, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT);
      } else {
        enum UdsSessionState session_type = rx_buffer[1];
        if (session_type == UDS_SESSION_STATE_DEFAULT ||
            session_type == UDS_SESSION_STATE_PROGRAMMING ||
            session_type == UDS_SESSION_STATE_EXTENDED_DIAGNOSTIC) {
          change_session(session_type);
          send_session_control_response(session_type, 4000, 10000);
        } else {
          send_negative_response(sid, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
        }
      }
    } else if (sid == UDS_SID_ECU_RESET) {
      handle_ecu_reset(rx_buffer, received_len);
    } else if (sid == UDS_SID_ROUTINE_CONTROL) {
      if (received_len < 3) {
        send_negative_response(
            sid, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT);
      } else {
        if (rx_buffer[1] != 0x01) {
          send_negative_response(sid, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
          return;
        }

        uint16_t routine_id = (rx_buffer[2] << 8) | rx_buffer[3];

        LOG_DBG("Routine ID: %d\n", routine_id);
        if (routine_id == 1337) {
          handle_erase_routine();
        } else if (routine_id == 1338) {
          handle_confirm_routine();
        } else {
          send_negative_response(sid, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
        }
      }
    } else if (sid == UDS_SID_REQUEST_DOWNLOAD) {
      if (received_len != 11) {
        send_negative_response(
            sid, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT);
      } else {
        send_request_download_response();
      }
    } else if (sid == UDS_SID_TRANSFER_DATA) {
      handle_transfer_data(rx_buffer, received_len);
    } else if (sid == UDS_SID_REQUEST_TRANSFER_EXIT) {
      handle_transfer_exit();
    } else {
      LOG_ERR("Service %x not supported\n", sid);
      send_negative_response(sid, UDS_NRC_SERVICE_NOT_SUPPORTED);
    }
  }
}

K_THREAD_DEFINE(
    thread_id, STACKSIZE, thread_entry, NULL, NULL, NULL, PRIORITY, 0, 0);
