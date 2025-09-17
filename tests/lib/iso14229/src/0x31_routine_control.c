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

UDSErr_t test_0x31_routine_control_start_routine(
    struct iso14229_zephyr_instance *inst,
    UDSEvent_t event,
    void *arg,
    void *user_context) {
  UDSRoutineCtrlArgs_t *args = arg;

  zassert_equal(args->ctrlType, 0x01);
  zassert_equal(args->id, 0x0202);
  zassert_equal(args->len, 2);
  uint8_t expected_option_record[] = {0x06, 0x01};
  zassert_mem_equal(args->optionRecord, expected_option_record, args->len,
                    NULL);

  uint8_t response_data[] = {0x32, 0x33, 0x8F};
  return args->copyStatusRecord(&inst->server, response_data,
                                sizeof(response_data));
}

ZTEST_F(lib_iso14229, test_0x31_routine_control_start_routine) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake = test_0x31_routine_control_start_routine;

  uint8_t request_data[] = {
    0x06,  // PCI (single frame)
    0x31,  // SID (RoutineControl)
    0x01,  /* SubFunction [StartRoutine] */
    0x02,  /* RoutineIdentifier [High Byte] */
    0x02,  /* RoutineIdentifier [Low Byte] */
    0x06,  /* RoutineControlOption#1 */
    0x01,  /* RoutineControlOption#2 */
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread_num(instance, 2);

  uint8_t response_data[] = {
    0x07,  // PCI   (single frame)
    0x71,  /* Response SID */
    0x01,  /* SubFunction */
    0x02,  /* RoutineIdentifier [High Byte] */
    0x02,  /* RoutineIdentifier [Low Byte] */
    0x32,  /* RoutineStatusRecord#1 */
    0x33,  /* RoutineStatusRecord#2 */
    0x8F,  /* RoutineStatusRecord#3 */

  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}

UDSErr_t test_0x31_routine_control_stop_routine(
    struct iso14229_zephyr_instance *inst,
    UDSEvent_t event,
    void *arg,
    void *user_context) {
  UDSRoutineCtrlArgs_t *args = arg;

  zassert_equal(args->ctrlType, 0x02);
  zassert_equal(args->id, 0x0201);
  zassert_equal(args->len, 0);

  uint8_t response_data[] = {0x30};
  return args->copyStatusRecord(&inst->server, response_data,
                                sizeof(response_data));
}

ZTEST_F(lib_iso14229, test_0x31_routine_control_stop_routine) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake = test_0x31_routine_control_stop_routine;

  uint8_t request_data[] = {
    0x04,  // PCI (single frame)
    0x31,  // SID (RoutineControl)
    0x02,  /* SubFunction [StopRoutine] */
    0x02,  /* RoutineIdentifier [High Byte] */
    0x01,  /* RoutineIdentifier [Low Byte] */
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread_num(instance, 2);

  uint8_t response_data[] = {
    0x05,  // PCI (single frame)
    0x71,  /* Response SID */
    0x02,  /* SubFunction */
    0x02,  /* RoutineIdentifier [High Byte] */
    0x01,  /* RoutineIdentifier [Low Byte] */
    0x30,  /* RoutineStatusRecord#1 */

  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}

UDSErr_t test_0x31_routine_control_request_results(
    struct iso14229_zephyr_instance *inst,
    UDSEvent_t event,
    void *arg,
    void *user_context) {
  UDSRoutineCtrlArgs_t *args = arg;

  zassert_equal(args->ctrlType, 0x03);
  zassert_equal(args->id, 0x0201);
  zassert_equal(args->len, 0);

  uint8_t response_data[] = {0x30, 0x33};
  return args->copyStatusRecord(&inst->server, response_data,
                                sizeof(response_data));
}

ZTEST_F(lib_iso14229, test_0x31_routine_control_request_results) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake =
      test_0x31_routine_control_request_results;

  uint8_t request_data[] = {
    0x04,  // PCI (single frame)
    0x31,  // SID (RoutineControl)
    0x03,  /* SubFunction [requestResults] */
    0x02,  /* RoutineIdentifier [High Byte] */
    0x01,  /* RoutineIdentifier [Low Byte] */
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread_num(instance, 2);

  uint8_t response_data[] = {
    0x06,  // PCI   (single frame)
    0x71,  /* Response SID */
    0x03,  /* SubFunction */
    0x02,  /* RoutineIdentifier [High Byte] */
    0x01,  /* RoutineIdentifier [Low Byte] */
    0x30,  /* RoutineStatusRecord#1 */
    0x33,  /* RoutineStatusRecord#2 */

  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}