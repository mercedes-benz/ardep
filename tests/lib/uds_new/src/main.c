/**
 * Copyright (c) Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"
#include "iso14229.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ardep/uds_new.h>

FAKE_VOID_FUNC(ecu_reset_work_handler, struct k_work *);

ZTEST_F(lib_uds_new, test_0x11_ecu_reset) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSECUResetArgs_t args = {.type = ECU_RESET_HARD};

  int ret = receive_event(instance, UDS_EVT_EcuReset, &args);
  zassert_ok(ret);

  // Wait for the scheduled worker to finish
  k_msleep(2000);
  zassert_equal(ecu_reset_work_handler_fake.call_count, 1);
}

ZTEST_F(lib_uds_new, test_0x11_ecu_reset_fails_when_subtype_not_implemented) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSECUResetArgs_t args = {.type = ECU_RESET_KEY_OFF_ON};

  int ret = receive_event(instance, UDS_EVT_EcuReset, &args);
  zassert_equal(ret, UDS_NRC_SubFunctionNotSupported);
}

ZTEST_F(lib_uds_new, test_0x22_read_by_id_static_single_element) {
  struct uds_new_instance_t *instance = fixture->instance;

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

ZTEST_F(lib_uds_new, test_0x22_read_by_id_fails_when_id_unknown) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSRDBIArgs_t args = {
    .dataId = 0xFFFF,  // unknown ID
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds_new, test_0x22_read_by_id_static_array) {
  struct uds_new_instance_t *instance = fixture->instance;

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
  struct uds_new_instance_t *instance = fixture->instance;

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
