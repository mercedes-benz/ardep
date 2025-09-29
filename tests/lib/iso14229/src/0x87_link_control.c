/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
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

UDSErr_t test_0x87_link_control_callback(struct iso14229_zephyr_instance *inst,
                                         UDSEvent_t event,
                                         void *arg,
                                         void *user_context) {
  zassert_equal(event, UDS_EVT_LinkControl);

  UDSLinkCtrlArgs_t *args = arg;

  switch (args->type) {
    case 0x01:
      zassert_equal(args->len, 1);
      zassert_equal(*(uint8_t *)args->data, 0x05);
      return UDS_PositiveResponse;
    case 0x02:
      zassert_equal(args->len, 3);
      uint8_t expected_data[] = {0x02, 0x49, 0xF0};
      zassert_mem_equal(args->data, expected_data, sizeof(expected_data));
      return UDS_PositiveResponse;
    case 0x03:
      zassert_equal(args->len, 0);
      return UDS_PositiveResponse;
    case 0x40:
      return UDS_NRC_ConditionsNotCorrect;
  }

  return UDS_NRC_GeneralProgrammingFailure;
}

ZTEST_F(lib_iso14229, test_0x87_link_control_sub_0x01) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake = test_0x87_link_control_callback;

  uint8_t request_data[] = {
    0x03,  // PCI (single frame)
    0x87,  // SID (LinkControl)
    0x01,  // LinkControlType
    0x05,  // LinkControlModeIdentifier
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread_num(instance, 2);

  uint8_t response_data[] = {
    0x02,  // PCI (single frame)
    0xC7,  // Response SID
    0x01,  // LinkControlType
  };

  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);

  uint8_t transition_request[] = {
    0x02,  // PCI (single frame)
    0x87,  // SID (LinkControl)
    0x03,  // LinkControlType (transition)
  };

  receive_phys_can_frame_array(fixture, transition_request);
  advance_time_and_tick_thread_num(instance, 2);

  uint8_t transition_response[] = {
    0x02,  // PCI (single frame)
    0xC7,  // Response SID
    0x03,  // LinkControlType
  };

  assert_send_phy_can_frame_array(fixture, 1, transition_response);
  zassert_equal(fake_can_send_fake.call_count, 2);
}

ZTEST_F(lib_iso14229, test_0x87_link_control_sub_0x02) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake = test_0x87_link_control_callback;

  uint8_t request_data[] = {
    0x05,  // PCI (single frame)
    0x87,  // SID (LinkControl)
    0x02,  // LinkControlType
    0x02,  // Data byte 1
    0x49,  // Data byte 2
    0xF0,  // Data byte 3
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread_num(instance, 2);

  uint8_t response_data[] = {
    0x02,  // PCI (single frame)
    0xC7,  // Response SID
    0x02,  // LinkControlType
  };

  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}

ZTEST_F(lib_iso14229, test_0x87_link_control_negative_response) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake = test_0x87_link_control_callback;

  uint8_t request_data[] = {
    0x02,  // PCI (single frame)
    0x87,  // SID (LinkControl)
    0x40,  // LinkControlType (triggers NRC)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread_num(instance, 2);

  uint8_t negative_response[] = {
    0x03,  // PCI (single frame)
    0x7F,  // Negative Response SID
    0x87,  // Original SID
    0x22,  // NRC (ConditionsNotCorrect)
  };

  assert_send_phy_can_frame_array(fixture, 0, negative_response);
  zassert_equal(fake_can_send_fake.call_count, 1);
}