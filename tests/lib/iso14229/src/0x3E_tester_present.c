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

UDSErr_t test_0x3E_tester_present(struct iso14229_zephyr_instance *inst,
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

ZTEST_F(lib_iso14229, test_0x3E_tester_present) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake = test_0x3E_tester_present;

  // Enter a programming session
  uint8_t request_data[] = {
    0x02,  // PCI (single frame, 2 bytes of data)
    0x10,  // SID (DiagnosticSessionControl)
    0x02,  // DS  (Programming Session)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);
  tick_thread(instance);

  // Assert we are in a programming session
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

  uint8_t request_data_tester_present[] = {
    0x02,  // PCI (single frame, 2 bytes of data)
    0x3E,  // SID (Tester Present)
    0x00,  // SubFunction (no subfunction)
  };

  uint8_t response_data_tester_present[] = {
    0x02,  // PCI   (single frame, 3 bytes)
    0x7E,  // SID   (Tester Present)
    0x00,  // SubFunction (no subfunction)
  };

  for (size_t i = 0; i < 10; i++) {
    receive_phys_can_frame_array(fixture, request_data_tester_present);
    advance_time_and_tick_thread(instance);
    tick_thread(instance);
    assert_send_phy_can_frame_array(fixture, i + 1,
                                    response_data_tester_present);
  }
  zassert_false(session_timeout_event_fired);
}
