/**
 * Copyright (c) Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"

#include <string.h>

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/ztest.h>

#include <iso14229.h>

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(UDSErr_t,
                       test_uds_callback,
                       struct iso14229_zephyr_instance *,
                       UDSEvent_t,
                       void *,
                       void *);

static const UDSISOTpCConfig_t cfg = {
  // Hardware Addresses
  .source_addr = 0x7E8,  // Can ID Server (us)
  .target_addr = 0x7E0,  // Can ID Client (them)

  // Functional Addresses
  .source_addr_func = 0x7DF,             // ID Server (us)
  .target_addr_func = UDS_TP_NOOP_ADDR,  // ID Client (them)
};

// Variables to capture callback and test data
static can_rx_callback_t captured_rx_callback_phys = NULL;
static void *captured_user_data_phy = NULL;
static can_rx_callback_t captured_rx_callback_func = NULL;
static void *captured_user_data_func = NULL;

// Variable to capture the last sent CAN frame
static struct can_frame send_can_frames[20];
static uint32_t send_can_frame_count = 0;

void tick_thread(struct iso14229_zephyr_instance *instance) {
  instance->thread_tick(instance);
}

void advance_time_and_tick_thread(struct iso14229_zephyr_instance *instance) {
  k_msleep(1000);
  tick_thread(instance);
}

void receive_phys_can_frame(const struct lib_iso14229_fixture *fixture,
                            uint8_t *data,
                            uint8_t data_len) {
  const struct device *dev = fixture->can_dev;

  struct can_frame frame = {
    .id = fixture->cfg.source_addr,  // 0x7E8 - message TO the server (us)
    .dlc = data_len,                 // data_len == dlc for Can CC
    .flags = 0,
  };
  memcpy(frame.data, data, data_len);

  captured_rx_callback_phys(dev, &frame, captured_user_data_phy);
}

void assert_send_phy_can_frame(const struct lib_iso14229_fixture *fixture,
                               uint32_t frame_index,
                               uint8_t *data,
                               uint8_t data_len) {
  struct can_frame expected_frame = {
    .id = fixture->cfg.target_addr,  // 0x7E8 - message TO the server
    .dlc = data_len,                 // data_len == dlc for Can CC
    .flags = 0,
  };
  memcpy(expected_frame.data, data, data_len);

  struct can_frame actual_frame = send_can_frames[frame_index];

  zassert_equal(actual_frame.id, expected_frame.id);  // response address
  zassert_equal(actual_frame.dlc,
                expected_frame.dlc);  // data_len == dlc for Can CC
  zassert_mem_equal(actual_frame.data, expected_frame.data, data_len);
}

// Actual definition in zephyr/drivers/can/can_common.c
// Re-defined here for proper injection fake can send command
struct can_tx_default_cb_ctx {
  struct k_sem done;
  int status;
};

// Fake CAN send to set the return code and
// unlock the semaphore AND capture the frame data
static int can_send_t_fake_impl(const struct device *dev,
                                const struct can_frame *frame,
                                k_timeout_t timeout,
                                can_tx_callback_t callback,
                                void *user_data) {
  struct can_tx_default_cb_ctx *ctx = user_data;

  // Capture the frame data
  send_can_frames[send_can_frame_count] = *frame;
  send_can_frame_count++;

  k_sem_give(&ctx->done);
  ctx->status = 0;  // Success
  return 0;
}

// Custom fake to capture the RX filter callback
// These are used to inject received CAN messages
static int capture_rx_filter_fake(const struct device *dev,
                                  can_rx_callback_t callback,
                                  void *user_data,
                                  const struct can_filter *filter) {
  ARG_UNUSED(dev);
  ARG_UNUSED(filter);

  if (filter->id == cfg.source_addr) {
    captured_rx_callback_phys = callback;
    captured_user_data_phy = user_data;
  } else if (filter->id == cfg.source_addr_func) {
    captured_rx_callback_func = callback;
    captured_user_data_func = user_data;
  } else {
    // If the filter ID does not match, we do not capture it
    return -EINVAL;  // Return error for unsupported filter ID
  }

  return 0;
}

static void *uds_new_setup(void) {
  static struct lib_iso14229_fixture fixture = {
    .cfg = cfg,
    .can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
  };

  return &fixture;
}

static void uds_new_before(void *f) {
  struct lib_iso14229_fixture *fixture = f;
  const struct device *dev = fixture->can_dev;
  struct iso14229_zephyr_instance *uds_instance = &fixture->instance;

  RESET_FAKE(fake_can_send);
  RESET_FAKE(test_uds_callback);
  FFF_RESET_HISTORY();

  memset(send_can_frames, 0, sizeof(send_can_frames));
  send_can_frame_count = 0;

  // Set up the fake to capture RX filter callbacks
  fake_can_send_fake.custom_fake = can_send_t_fake_impl;
  fake_can_add_rx_filter_fake.custom_fake = capture_rx_filter_fake;

  // Variables to capture CAN RX filter setup
  captured_rx_callback_phys = NULL;
  captured_user_data_phy = NULL;

  // Configure UDS TP settings
  UDSISOTpCConfig_t tp_config = {
    .source_addr = fixture->cfg.source_addr,            // 0x7E8 (client)
    .target_addr = fixture->cfg.target_addr,            // 0x7E0 (server - us)
    .source_addr_func = fixture->cfg.source_addr_func,  // 0x7DF
    .target_addr_func = fixture->cfg.target_addr_func,  // UDS_TP_NOOP_ADDR
  };

  int ret = iso14229_zephyr_init(uds_instance, &tp_config, dev, NULL);

  assert(ret == 0);
  // we add 2 can filters in iso14229_zephyr_init()
  assert(fake_can_add_rx_filter_fake.call_count == 2);
  assert(uds_instance->server.fn);

  // Set the unified callback
  uds_instance->set_callback(uds_instance, test_uds_callback);
}

ZTEST_SUITE(lib_iso14229, NULL, uds_new_setup, uds_new_before, NULL, NULL);