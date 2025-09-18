/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds.h"
#include "fixture.h"
#include "iso14229.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/ztest.h>

UDSErr_t security_access_0x27_request_seed_check_fn(
    const struct uds_context *const context, bool *apply_action) {
  zassert_equal(context->event, UDS_EVT_SecAccessRequestSeed);
  zassert_not_null(context->arg);

  UDSSecAccessRequestSeedArgs_t *args = context->arg;

  zassert_equal(args->level, 1);
  zassert_equal(args->len, 0);
  zassert_is_null(args->dataRecord);

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t security_access_0x27_request_seed_action_fn(
    struct uds_context *const context, bool *consume_event) {
  UDSSecAccessRequestSeedArgs_t *args = context->arg;

  uint8_t seed[] = {0x11, 0x22};
  return args->copySeed(&context->instance->iso14229.server, seed,
                        sizeof(seed));
}

ZTEST_F(lib_uds, test_0x27_security_access_request_seed) {
  struct uds_instance_t *instance = fixture->instance;

  UDSSecAccessRequestSeedArgs_t args = {
    .level = 1,
    .len = 0,
    .dataRecord = NULL,
    .copySeed = copy,
  };

  data_id_check_fn_fake.custom_fake =
      security_access_0x27_request_seed_check_fn;
  data_id_action_fn_fake.custom_fake =
      security_access_0x27_request_seed_action_fn;

  int ret = receive_event(instance, UDS_EVT_SecAccessRequestSeed, &args);
  zassert_ok(ret);

  uint8_t response_data[] = {0x11, 0x22};
  assert_copy_data(response_data, sizeof(response_data));

  zassert_equal(data_id_action_fn_fake.call_count, 1);
}

UDSErr_t security_access_0x27_validate_key_check_fn(
    const struct uds_context *const context, bool *apply_action) {
  zassert_equal(context->event, UDS_EVT_SecAccessValidateKey);
  zassert_not_null(context->arg);

  UDSSecAccessValidateKeyArgs_t *args = context->arg;

  zassert_equal(args->level, 1);
  zassert_equal(args->len, 2);
  uint8_t key[] = {0xEE, 0xDD};
  zassert_mem_equal(args->key, key, args->len);

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t security_access_0x27_validate_key_action_fn(
    struct uds_context *const context, bool *consume_event) {
  return UDS_PositiveResponse;
}

ZTEST_F(lib_uds, test_0x27_security_access_validate_key) {
  struct uds_instance_t *instance = fixture->instance;

  uint8_t key[] = {0xEE, 0xDD};
  UDSSecAccessValidateKeyArgs_t args = {
    .level = 1,
    .len = 2,
    .key = key,
  };

  data_id_check_fn_fake.custom_fake =
      security_access_0x27_validate_key_check_fn;
  data_id_action_fn_fake.custom_fake =
      security_access_0x27_validate_key_action_fn;

  int ret = receive_event(instance, UDS_EVT_SecAccessValidateKey, &args);
  zassert_ok(ret);

  zassert_equal(data_id_action_fn_fake.call_count, 1);
}