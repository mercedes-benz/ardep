/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "fixture.h"

#include <zephyr/ztest.h>

ZTEST_F(lib_uds, test_0x2E_write_by_id_fails_when_writing_to_readonly_element) {
  struct uds_instance_t *instance = fixture->instance;

  uint32_t data = 0x11223344;

  UDSWDBIArgs_t arg = {
    .dataId = data_id_rw,
    .data = (uint8_t *)&data,
    .len = sizeof(data),
  };

  int ret = receive_event(instance, UDS_EVT_WriteDataByIdent, &arg);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
  zassert_true(data_id_check_fn_fake.call_count >= 1);
  zassert_equal(data_id_action_fn_fake.call_count, 0);
}

//////////////////////7

ZTEST_F(lib_uds, test_0x2E_write_by_id_fails_when_no_action_applies) {
  struct uds_instance_t *instance = fixture->instance;

  uint32_t data = 0x11223344;

  UDSWDBIArgs_t arg = {
    .dataId = data_id_rw,
    .data = (uint8_t *)&data,
    .len = sizeof(data),
  };

  int ret = receive_event(instance, UDS_EVT_WriteDataByIdent, &arg);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
  zassert_true(data_id_check_fn_fake.call_count >= 1);
  zassert_equal(data_id_action_fn_fake.call_count, 0);
}

//////////////////////7

UDSErr_t custom_check_for_0x2E_applies_action_when_check_succeeds(
    const struct uds_context *const context, bool *apply_action) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.data_id == data_id_rw &&
      context->event == UDS_EVT_WriteDataByIdent) {
    *apply_action = true;
  }
  return UDS_OK;
}

ZTEST_F(lib_uds, test_0x2E_write_by_id_applies_action_when_check_succeeds) {
  struct uds_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_0x2E_applies_action_when_check_succeeds;

  uint32_t data = 0x11223344;

  UDSWDBIArgs_t arg = {
    .dataId = data_id_rw,
    .data = (uint8_t *)&data,
    .len = sizeof(data),
  };

  int ret = receive_event(instance, UDS_EVT_WriteDataByIdent, &arg);
  zassert_ok(ret);

  zassert_true(data_id_check_fn_fake.call_count >= 1);
  zassert_equal(data_id_action_fn_fake.call_count, 1);
}

//////////////////////7

UDSErr_t custom_check_for_0x2E_consume_event_by_default_on_action(
    const struct uds_context *const context, bool *apply_action) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.data_id ==
          data_id_rw_duplicated1 &&
      context->event == UDS_EVT_WriteDataByIdent) {
    *apply_action = true;
  }
  return UDS_OK;
}

ZTEST_F(lib_uds, test_0x2E_write_by_id_consume_event_by_default_on_action) {
  struct uds_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_0x2E_consume_event_by_default_on_action;

  uint32_t data = 0x11223344;

  UDSWDBIArgs_t arg = {
    .dataId = data_id_rw,
    .data = (uint8_t *)&data,
    .len = sizeof(data),
  };

  int ret = receive_event(instance, UDS_EVT_WriteDataByIdent, &arg);
  zassert_ok(ret);

  zassert_true(data_id_check_fn_fake.call_count >= 1);
  zassert_equal(data_id_action_fn_fake.call_count, 1);
}

// //////////////////////7

UDSErr_t custom_check_for_0x2E_both_actions_are_executed(
    const struct uds_context *const context, bool *apply_action) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.data_id ==
          data_id_rw_duplicated1 &&
      context->event == UDS_EVT_WriteDataByIdent) {
    *apply_action = true;
  }
  return UDS_OK;
}

UDSErr_t custom_action_for_0x2E_both_actions_are_executed(
    struct uds_context *const context, bool *consume_event) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.data_id ==
          data_id_rw_duplicated1 &&
      context->event == UDS_EVT_WriteDataByIdent) {
    *consume_event = false;
  }
  return UDS_OK;
}

ZTEST_F(lib_uds, test_0x2E_write_by_id_both_actions_are_executed) {
  struct uds_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_0x2E_both_actions_are_executed;
  data_id_action_fn_fake.custom_fake =
      custom_action_for_0x2E_both_actions_are_executed;

  uint32_t data = 0x11223344;

  UDSWDBIArgs_t arg = {
    .dataId = data_id_rw_duplicated1,
    .data = (uint8_t *)&data,
    .len = sizeof(data),
  };

  int ret = receive_event(instance, UDS_EVT_WriteDataByIdent, &arg);
  zassert_ok(ret);

  zassert_true(data_id_check_fn_fake.call_count >= 1);
  zassert_equal(data_id_action_fn_fake.call_count, 2);
}

// //////////////////////7

UDSErr_t custom_check_for_0x2E_returns_action_returncode(
    const struct uds_context *const context, bool *apply_action) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.data_id ==
          data_id_rw_duplicated1 &&
      context->event == UDS_EVT_WriteDataByIdent) {
    *apply_action = true;
  }
  return UDS_OK;
}

UDSErr_t custom_action_for_0x2E_returns_action_returncode(
    struct uds_context *const context, bool *consume_event) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.data_id ==
          data_id_rw_duplicated1 &&
      context->event == UDS_EVT_WriteDataByIdent) {
    *consume_event = false;
    return UDS_NRC_GeneralProgrammingFailure;
  }
  return UDS_OK;
}

ZTEST_F(lib_uds, test_0x2E_write_by_id_returns_action_returncode) {
  struct uds_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_0x2E_returns_action_returncode;
  data_id_action_fn_fake.custom_fake =
      custom_action_for_0x2E_returns_action_returncode;

  uint32_t data = 0x11223344;

  UDSWDBIArgs_t arg = {
    .dataId = data_id_rw,
    .data = (uint8_t *)&data,
    .len = sizeof(data),
  };

  int ret = receive_event(instance, UDS_EVT_WriteDataByIdent, &arg);
  zassert_equal(ret, UDS_NRC_GeneralProgrammingFailure);

  zassert_true(data_id_check_fn_fake.call_count >= 1);
  zassert_equal(data_id_action_fn_fake.call_count, 1);
}

// //////////////////////7
#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION

#define UDS_UNIQUE_DATA_ID 0xFEEF

UDSErr_t custom_check_for_dynamic_registration(
    const struct uds_context *const context, bool *apply_action) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.data_id == UDS_UNIQUE_DATA_ID &&
      context->event == UDS_EVT_WriteDataByIdent) {
    test_dynamic_registration_check_invoked = true;
    *apply_action = true;
  }
  return UDS_OK;
}

UDSErr_t custom_action_for_dynamic_registration(
    struct uds_context *const context, bool *consume_event) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.data_id == UDS_UNIQUE_DATA_ID &&
      context->event == UDS_EVT_WriteDataByIdent) {
    consume_event = false;
    test_dynamic_registration_action_invoked = true;
  }
  return UDS_OK;
}

ZTEST_F(lib_uds, test_0x2E_write_by_id_dynamic_registration) {
  struct uds_instance_t *instance = fixture->instance;

  struct uds_registration_t reg;
  reg.type = UDS_REGISTRATION_TYPE__DATA_IDENTIFIER;
  reg.applies_to_event = uds_filter_for_data_by_id_event;
  reg.data_identifier.data_id = UDS_UNIQUE_DATA_ID;
  reg.data_identifier.read.check = NULL;
  reg.data_identifier.read.action = NULL;
  reg.data_identifier.write.check = custom_check_for_dynamic_registration;
  reg.data_identifier.write.action = custom_action_for_dynamic_registration;

  int ret = instance->register_event_handler(instance, reg);
  zassert_ok(ret);

  uint32_t data = 0x11223344;

  UDSWDBIArgs_t arg = {
    .dataId = data_id_rw,
    .data = (uint8_t *)&data,
    .len = sizeof(data),
  };

  ret = receive_event(instance, UDS_EVT_WriteDataByIdent, &arg);
  zassert_ok(ret);

  zassert_true(test_dynamic_registration_check_invoked);
  zassert_true(test_dynamic_registration_action_invoked);
}

//////////////////////7

ZTEST_F(
    lib_uds,
    test_0x2E_write_by_id_dynamic_registration_with_no_write_fn_smoke_test) {
  struct uds_instance_t *instance = fixture->instance;

  struct uds_registration_t reg;
  reg.type = UDS_REGISTRATION_TYPE__DATA_IDENTIFIER;
  reg.applies_to_event = uds_filter_for_data_by_id_event;
  reg.data_identifier.data_id = UDS_UNIQUE_DATA_ID;
  reg.data_identifier.read.check = NULL;
  reg.data_identifier.read.action = NULL;
  reg.data_identifier.write.check = NULL;
  reg.data_identifier.write.action = NULL;

  int ret = instance->register_event_handler(instance, reg);
  zassert_ok(ret);

  uint32_t data = 0x11223344;

  UDSWDBIArgs_t arg = {
    .dataId = data_id_rw,
    .data = (uint8_t *)&data,
    .len = sizeof(data),
  };

  ret = receive_event(instance, UDS_EVT_WriteDataByIdent, &arg);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}
//////////////////////7

#endif  // CONFIG_UDS_USE_DYNAMIC_REGISTRATION