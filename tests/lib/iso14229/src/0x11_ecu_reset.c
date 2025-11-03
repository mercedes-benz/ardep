/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

// Include UDS minimal library headers
#include <ardep/iso14229.h>
#include <iso14229.h>

ZTEST_F(lib_iso14229, test_0x11_ecu_reset) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  uint8_t request_data[] = {
    0x02,  // PCI    (single frame, 2 bytes of data)
    0x11,  // ER     (ECU Reset)
    0x01,  // LEV_RT (Hard Reset)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);

  zassert_equal(test_uds_callback_fake.call_count, 1);
  zassert_equal(test_uds_callback_fake.arg1_val, UDS_EVT_EcuReset);

  advance_time_and_tick_thread(instance);

  uint8_t response_data[] = {
    0x02,  // PCI    (single frame, 2 bytes of data)
    0x51,  // ERPR   (positive response to 0x11)
    0x01,  // LEV_RT (hard reset)
  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}

ZTEST_F(lib_iso14229, test_0x11_ecu_reset_response_pending) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  uint8_t request_data[] = {
    0x02,  // PCI    (single frame, 2 bytes of data)
    0x11,  // ER     (ECU Reset)
    0x01,  // LEV_RT (Hard Reset)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);
  zassert_equal(test_uds_callback_fake.call_count, 1);

  advance_time_and_tick_thread(instance);
  zassert_equal(fake_can_send_fake.call_count, 1);

  // There should be no response to this request
  uint8_t denied_request_data[] = {
    0x02,  // PCI (single frame, 2 bytes of data)
    0x10,  // SID (DiagnosticSessionControl)
    0x02,  // DS  (Programming Session)
  };
  receive_phys_can_frame_array(fixture, denied_request_data);
  advance_time_and_tick_thread(instance);
  advance_time_and_tick_thread(instance);

  // Session Control request should have been ignored
  zassert_equal(fake_can_send_fake.call_count, 1);
}

UDSErr_t test_0x11_ecu_reset_rapid_power_shutdown_uds_callback(
    struct iso14229_zephyr_instance *inst,
    UDSEvent_t event,
    void *arg,
    void *user_context) {
  if (event == UDS_EVT_EcuReset) {
    UDSECUResetArgs_t *args = arg;
    args->powerDownTimeMillis = 2000;
  }

  return UDS_OK;
}

ZTEST_F(lib_iso14229, test_0x11_ecu_reset_rapid_power_shutdown) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake =
      test_0x11_ecu_reset_rapid_power_shutdown_uds_callback;

  uint8_t request_data[] = {
    0x02,  // PCI    (single frame, 2 bytes of data)
    0x11,  // ER     (ECU Reset)
    0x04,  // LEV_RT (Rapid Power Shutdown)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);

  zassert_equal(test_uds_callback_fake.call_count, 1);
  zassert_equal(test_uds_callback_fake.arg1_val, UDS_EVT_EcuReset);
  advance_time_and_tick_thread(instance);

  uint8_t response_data[] = {
    0x03,  // PCI    (single frame, 4 bytes of data)
    0x51,  // ERPR   (positive response to 0x11)
    0x04,  // LEV_RT (rapid power shutdown)
    0x02,  // PDT    (2s power down timer)
  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}
