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

ZTEST_F(lib_iso14229, test_0x10_diag_session_ctrl) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  uint8_t request_data[] = {
    0x02,  // PCI (single frame, 2 bytes of data)
    0x10,  // SID (DiagnosticSessionControl)
    0x02,  // DS  (Programming Session)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);

  zassert_equal(test_uds_callback_fake.call_count, 1);
  zassert_equal(test_uds_callback_fake.arg1_val, UDS_EVT_DiagSessCtrl);
  advance_time_and_tick_thread(instance);

  uint8_t response_data[] = {
    0x06,                             // PCI   (single frame, 6 bytes of data)
    0x50,                             // DSCPR (positive response to 0x10)
    0x02,                             // DS    (programming session)
    UDS_CLIENT_DEFAULT_P2_MS >> 8,    // SPREC (p2 upper byte)
    UDS_CLIENT_DEFAULT_P2_MS & 0xFF,  // SPREC (p2 lower byte)
    (uint8_t)((UDS_CLIENT_DEFAULT_P2_STAR_MS / 10) >>
              8),                                   // SPREC (p2* upper byte)
    (uint8_t)(UDS_CLIENT_DEFAULT_P2_STAR_MS / 10),  // SPREC (p2* lower byte)
  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}

UDSErr_t test_0x10_diag_session_ctrl_not_supported_uds_callback(
    struct iso14229_zephyr_instance *inst,
    UDSEvent_t event,
    void *arg,
    void *user_context) {
  if (event == UDS_EVT_DiagSessCtrl) {
    return UDS_NRC_SubFunctionNotSupported;
  }

  return UDS_OK;
}

ZTEST_F(lib_iso14229, test_0x10_diag_session_ctrl_not_supported) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake =
      test_0x10_diag_session_ctrl_not_supported_uds_callback;

  uint8_t request_data[] = {
    0x02,  // PCI (single frame, 2 bytes of data)
    0x10,  // SID (DiagnosticSessionControl)
    0x02,  // DS  (Programming Session)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);
  advance_time_and_tick_thread(instance);

  uint8_t response_data[] = {
    0x03,  // PCI   (single frame, 3 bytes)
    0x7F,  // SID   (Failure Response SID)
    0x10,  // SIDRQ (SID of failing service)
    0x12,  // NRC   response code  (SubFunctionNotSupported)
  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}

UDSErr_t test_0x10_diag_session_ctrl_with_session_timeout_callback(
    struct iso14229_zephyr_instance *inst,
    UDSEvent_t event,
    void *arg,
    void *user_context) {
  if (event == UDS_EVT_DiagSessCtrl) {
    return UDS_PositiveResponse;
  } else if (event == UDS_EVT_SessionTimeout) {
    session_timeout_event_fired = true;
    return UDS_PositiveResponse;
  }

  return UDS_NRC_GeneralProgrammingFailure;
}

ZTEST_F(lib_iso14229, test_0x10_diag_session_ctrl_with_session_timeout) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake =
      test_0x10_diag_session_ctrl_with_session_timeout_callback;

  uint8_t request_data[] = {
    0x02,  // PCI (single frame, 2 bytes of data)
    0x10,  // SID (DiagnosticSessionControl)
    0x02,  // DS  (Programming Session)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);
  tick_thread(instance);

  uint8_t response_data[] = {
    0x06,  // PCI   (single frame, 3 bytes)
    0x50,  // SID   (DiagnosticSessionControl)
    0x02,  // DS (Session Type)
    0x00,  // SPREC (p2 upper byte)
    0x96,  // SPREC (p2 lower byte)
    0x00,  // SPREC (p2* upper byte)
    0x96,  // SPREC (p2* lower byte)

  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);

  // Advance time to provoke session timeout event
  k_msleep(8000);
  advance_time_and_tick_thread(instance);

  zassert_true(session_timeout_event_fired);
}
