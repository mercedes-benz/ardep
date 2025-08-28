/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ardep/uds_new.h>

ZTEST_F(lib_uds_new, test_0x22_read_by_id_static_single_element) {
  struct uds_new_instance_t *instance = &fixture->instance;

  UDSRDBIArgs_t args = {
    .dataId = by_id_data1_id,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &args);
  zassert_ok(ret);

  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg0_val, &instance->iso14229.server);
  zassert_equal(copy_fake.arg2_val, sizeof(by_id_data1));

  uint8_t expected[2] = {
    0x00,
    0x05,
  };
  assert_copy_data(expected, sizeof(expected));
}

ZTEST_F(lib_uds_new, test_0x22_read_by_id_static_array) {
  struct uds_new_instance_t *instance = &fixture->instance;

  UDSRDBIArgs_t args = {
    .dataId = by_id_data2_id,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &args);
  zassert_ok(ret);

  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg0_val, &instance->iso14229.server);
  zassert_equal(copy_fake.arg2_val, sizeof(by_id_data2));

  uint8_t expected[6] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
  assert_copy_data(expected, sizeof(expected));
}

ZTEST_F(lib_uds_new, test_0x22_read_by_id_dynamic_array) {
  struct uds_new_instance_t *instance = &fixture->instance;

  uint16_t id = 0x9988;
  uint32_t data[4] = {0x11223344, 0x55667788, 0x99AABBCC, 0xDDEEFF00};

  instance->register_data_by_identifier(instance, id, data, ARRAY_SIZE(data),
                                        sizeof(data[0]));

  UDSRDBIArgs_t args = {
    .dataId = id,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &args);
  zassert_ok(ret);

  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg0_val, &instance->iso14229.server);
  zassert_equal(copy_fake.arg2_val, sizeof(data));

  uint8_t expected[16] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00};

  assert_copy_data(expected, sizeof(expected));
}

ZTEST_F(lib_uds_new, test_0x23_read_memory_by_address) {
  struct uds_new_instance_t *instance = &fixture->instance;

  // Test 1: Invalid address should be rejected
  UDSReadMemByAddrArgs_t args1 = {
    .memAddr = (void*)0x20000000,
    .memSize = 16,
    .copy = copy,
  };

  int ret1 = receive_event(instance, UDS_EVT_ReadMemByAddr, &args1);
  zassert_equal(ret1, UDS_PositiveResponse);

  // Test 2: NULL address should be rejected
  UDSReadMemByAddrArgs_t args2 = {
    .memAddr = NULL,
    .memSize = 16,
    .copy = copy,
  };

  int ret2 = receive_event(instance, UDS_EVT_ReadMemByAddr, &args2);
  zassert_equal(ret2, UDS_NRC_RequestOutOfRange);

  // Test 3: Zero size should be rejected
  UDSReadMemByAddrArgs_t args3 = {
    .memAddr = (void*)0x10000,
    .memSize = 0,
    .copy = copy,
  };

  int ret3 = receive_event(instance, UDS_EVT_ReadMemByAddr, &args3);
  zassert_equal(ret3, UDS_NRC_RequestOutOfRange);

  // Test 4: Too large size should be rejected
  UDSReadMemByAddrArgs_t args4 = {
    .memAddr = (void*)0x10000,
    .memSize = 0x20000, // 128KB - too large
    .copy = copy,
  };

  int ret4 = receive_event(instance, UDS_EVT_ReadMemByAddr, &args4);
  zassert_equal(ret4, UDS_NRC_RequestOutOfRange);
}
