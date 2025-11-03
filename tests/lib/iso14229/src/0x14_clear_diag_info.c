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

UDSErr_t test_0x14_clear_diagnostic_information(
    struct iso14229_zephyr_instance *inst,
    UDSEvent_t event,
    void *arg,
    void *user_context) {
  if (event == UDS_EVT_ClearDiagnosticInfo) {
    UDSCDIArgs_t *args = arg;
    zassert_equal(args->groupOfDTC, 0x00FFDD33);
    zassert_equal(args->memorySelection, 0x22);
    zassert_equal(args->hasMemorySelection, true);
    return UDS_PositiveResponse;
  }

  return UDS_NRC_GeneralProgrammingFailure;
}

ZTEST_F(lib_iso14229, test_0x14_clear_diagnostic_information) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake = test_0x14_clear_diagnostic_information;

  uint8_t request_data[] = {
    0x05,  // PCI (single frame, 5 bytes of data)
    0x14,  // SID (ClearDiagnosticInformation)
    0xFF,  // DTCHP  (GroupOfDTC [High Byte])
    0xDD,  // DTCMB  (GroupOfDTC [Middle Byte])
    0x33,  // DTCLB  (GroupOfDTC [Low Byte])
    0x22,  // MEMYS (MemorySelection)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);
  // Needs 2 ticks (1 to process, 1 to send response)
  tick_thread(instance);

  uint8_t response_data[] = {
    0x01,  // PCI   (single frame, 1 byte(s))
    0x54,  // SID   (ClearDiagnosticInformation)

  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}

UDSErr_t test_0x14_clear_diagnostic_information_send_nrc(
    struct iso14229_zephyr_instance *inst,
    UDSEvent_t event,
    void *arg,
    void *user_context) {
  if (event == UDS_EVT_ClearDiagnosticInfo) {
    return UDS_NRC_ConditionsNotCorrect;
  }

  return UDS_NRC_GeneralProgrammingFailure;
}

ZTEST_F(lib_iso14229, test_0x14_clear_diagnostic_information_send_nrc) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake =
      test_0x14_clear_diagnostic_information_send_nrc;

  uint8_t request_data[] = {
    0x05,  // PCI (single frame, 5 bytes of data)
    0x14,  // SID (ClearDiagnosticInformation)
    0xFF,  // DTCHP  (GroupOfDTC [High Byte])
    0xDD,  // DTCMB  (GroupOfDTC [Middle Byte])
    0x33,  // DTCLB  (GroupOfDTC [Low Byte])
    0x22,  // MEMYS (MemorySelection)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);
  // Needs 2 ticks (1 to process, 1 to send response)
  tick_thread(instance);

  uint8_t response_data[] = {
    0x03,  // PCI   (single frame, 1 byte(s))
    0x7F,  // SID   (Failure Response SID)
    0x14,  // SIDRQ (SID of failing service)
    0x22,  // NRC   response code  (ConditionsNotCorrect)

  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}
