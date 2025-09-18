/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>

#include <ardep/iso14229.h>

UDSErr_t test_0x27_security_access(struct iso14229_zephyr_instance *inst,
                                   UDSEvent_t event,
                                   void *arg,
                                   void *user_context) {
  if (event == UDS_EVT_SecAccessRequestSeed) {
    UDSSecAccessRequestSeedArgs_t *args = arg;

    zassert_equal(args->level, 1);
    zassert_equal(args->len, 0);

    uint8_t seed[] = {0x36, 0x56};
    return args->copySeed(&inst->server, seed, sizeof(seed));
  } else if (event == UDS_EVT_SecAccessValidateKey) {
    UDSSecAccessValidateKeyArgs_t *args = arg;

    zassert_equal(args->level, 1);
    zassert_equal(args->len, 2);
    uint8_t expected_key[] = {0xC9, 0xA9};
    zassert_mem_equal(args->key, expected_key, args->len);

    return UDS_PositiveResponse;
  }

  return UDS_NRC_GeneralProgrammingFailure;
}

ZTEST_F(lib_iso14229, test_0x27_security_access) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake = test_0x27_security_access;

  uint8_t request_data_seed[] = {
    0x02, /* PCI (single frame) */
    0x27, /* SID (SecurityAccess) */
    0x01, /* SubFunction [Request Seed] */
  };

  receive_phys_can_frame_array(fixture, request_data_seed);
  advance_time_and_tick_thread_num(instance, 2);

  uint8_t response_data_seed[] = {
    0x04, /* PCI   (single frame) */
    0x67, /* Response SID */
    0x01, /* SubFunction */
    0x36, /* SecuritySeed [Byte#1] */
    0x56, /* SecuritySeed [Byte#2] */
  };

  assert_send_phy_can_frame_array(fixture, 0, response_data_seed);
  zassert_equal(fake_can_send_fake.call_count, 1);

  uint8_t request_data_key[] = {
    0x04, /* PCI (single frame) */
    0x27, /* SID (SecurityAccess) */
    0x02, /* SubFunction [Validate Key] */
    0xC9, /* Key [Byte#1] */
    0xA9  /* Key [Byte#2] */
  };

  receive_phys_can_frame_array(fixture, request_data_key);
  advance_time_and_tick_thread_num(instance, 2);

  uint8_t response_data_key[] = {
    0x02, /* PCI (single frame) */
    0x67, /* Response SID */
    0x02, /* SubFunction */
  };

  assert_send_phy_can_frame_array(fixture, 1, response_data_key);
  zassert_equal(fake_can_send_fake.call_count, 2);
  zassert_equal(instance->server.securityLevel, 1);
}

ZTEST_F(lib_iso14229, test_0x27_security_access_already_unlocked) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  instance->server.securityLevel = 1;

  test_uds_callback_fake.custom_fake = test_0x27_security_access;

  uint8_t request_data_seed[] = {
    0x02, /* PCI (single frame) */
    0x27, /* SID (SecurityAccess) */
    0x01, /* SubFunction [Request Seed] */
  };

  receive_phys_can_frame_array(fixture, request_data_seed);
  advance_time_and_tick_thread_num(instance, 2);

  uint8_t response_data_seed[] = {
    0x04, /* PCI   (single frame) */
    0x67, /* Response SID */
    0x01, /* SubFunction */
    0x0,  /* SecuritySeed [Byte#1] */
    0x0,  /* SecuritySeed [Byte#2] */
  };

  assert_send_phy_can_frame_array(fixture, 0, response_data_seed);
  zassert_equal(fake_can_send_fake.call_count, 1);
  zassert_equal(test_uds_callback_fake.call_count, 0);
}
