/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

#include <ardep/iso14229.h>
#include <iso14229.h>

UDSErr_t test_0x28_communication_control_test1(
    struct iso14229_zephyr_instance *inst,
    UDSEvent_t event,
    void *arg,
    void *user_context) {
  UDSCommCtrlArgs_t *args = arg;

  zassert_equal(args->ctrlType, 0x1);
  zassert_equal(args->commType, 0x2);

  return UDS_PositiveResponse;
}

ZTEST_F(lib_iso14229, test_0x28_communication_control_basic) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake = test_0x28_communication_control_test1;

  uint8_t request_data[] = {
    0x03,  // PCI (single frame)
    0x28,  // SID (CommunicationControl)
    0x01,  // controlType (enableRxAndDisableTx)
    0x02,  // communicationType (networkManagement)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);

  zassert_equal(test_uds_callback_fake.call_count, 1);
  zassert_equal(test_uds_callback_fake.arg1_val, UDS_EVT_CommCtrl);
  advance_time_and_tick_thread(instance);

  uint8_t response_data[] = {
    0x02,  // PCI (single frame)
    0x68,  // Positive response to 0x28
    0x01,  // controlType echo
  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}

UDSErr_t test_0x28_communication_control_test2(
    struct iso14229_zephyr_instance *inst,
    UDSEvent_t event,
    void *arg,
    void *user_context) {
  UDSCommCtrlArgs_t *args = arg;

  zassert_equal(args->ctrlType, 0x4);
  zassert_equal(args->commType, 0x1);
  zassert_equal(args->nodeId, 0x000A);

  return UDS_PositiveResponse;
}

ZTEST_F(lib_iso14229, test_0x28_communication_control_enhanced_address) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake = test_0x28_communication_control_test2;

  uint8_t request_data[] = {
    0x05,  // PCI (single frame)
    0x28,  // SID (CommunicationControl)
    0x04,  // controlType (enableRxAndDisableTxWithEnhancedAddressInformation)
    0x01,  // communicationType (normalMessage)
    0x00,  // nodeID (high byte)
    0x0A,  // nodeID (low byte)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);

  zassert_equal(test_uds_callback_fake.call_count, 1);
  zassert_equal(test_uds_callback_fake.arg1_val, UDS_EVT_CommCtrl);
  advance_time_and_tick_thread(instance);

  uint8_t response_data[] = {
    0x02,  // PCI (single frame)
    0x68,  // Positive response to 0x28
    0x04,  // controlType echo
  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}
