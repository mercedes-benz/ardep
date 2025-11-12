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

UDSErr_t test_0x2F_io_control(struct iso14229_zephyr_instance *inst,
                              UDSEvent_t event,
                              void *arg,
                              void *user_context) {
  UDSIOCtrlArgs_t *args = arg;

  zassert_equal(args->dataId, 0x9B00);
  zassert_equal(args->ctrlStateAndMaskLen, 0x01);
  const uint8_t expected_data[] = {0x3C};
  zassert_mem_equal(args->ctrlStateAndMask, expected_data,
                    args->ctrlStateAndMaskLen);
  const uint8_t response_data[] = {0x0C};
  return args->copy(&inst->server, response_data, sizeof(response_data));
}

ZTEST_F(lib_iso14229, test_0x2F_io_control) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake = test_0x2F_io_control;

  uint8_t request_data[] = {
    0x05,  // PCI (single frame, 5 bytes of data)
    0x2F,  // SID (InputOutputControlByIdentifier)
    0x9B,  /* DataIdentifier [High Byte] */
    0x00,  /* DataIdentifier [Low Byte] */
    0x03,  /* ControlOptionRecord [inputOutputControlParamter] */
    0x3C,  /* ControlOptionRecord [State#1] */
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);
  advance_time_and_tick_thread(instance);
  advance_time_and_tick_thread(instance);
  // Needs 2 ticks (1 to process, 1 to send response)
  tick_thread(instance);

  uint8_t response_data[] = {
    0x05,  // PCI   (single frame, 5 byte(s))
    0x6F,  /* Response SID */
    0x9B,  /* DataIdentifier [High Byte] */
    0x00,  /* DataIdentifier [Low Byte] */
    0x03,  /* ControlOptionRecord [inputOutputControlParamter] */
    0x0C,  /* ControlOptionRecord [State#1] */

  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}
