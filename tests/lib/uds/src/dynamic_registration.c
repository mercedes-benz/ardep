/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"
#include "iso14229.h"

#include <zephyr/ztest.h>

#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION

#define UDS_DYNAMIC_DATA_ID_1 0xABCD
#define UDS_DYNAMIC_DATA_ID_2 0x1234

// Global counters to track invocations
static bool dynamic_reg1_check_invoked = false;
static bool dynamic_reg1_action_invoked = false;
static bool dynamic_reg2_check_invoked = false;
static bool dynamic_reg2_action_invoked = false;
static bool dynamic_reg1_unregister_invoked = false;

static void reset_global_counters(void) {
  dynamic_reg1_check_invoked = false;
  dynamic_reg1_action_invoked = false;
  dynamic_reg2_check_invoked = false;
  dynamic_reg2_action_invoked = false;
  dynamic_reg1_unregister_invoked = false;
}

static UDSErr_t receive_rdbid_event(struct uds_instance_t *instance,
                                    UDSRDBIArgs_t *args) {
  return receive_event(instance, UDS_EVT_ReadDataByIdent, args);
}

UDSErr_t custom_check_for_dynamic_reg1(const struct uds_context *const context,
                                       bool *apply_action) {
  UDSRDBIArgs_t *args = context->arg;
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.data_id == args->dataId &&
      context->event == UDS_EVT_ReadDataByIdent) {
    dynamic_reg1_check_invoked = true;
    *apply_action = true;
  }
  return UDS_OK;
}

UDSErr_t custom_action_for_dynamic_reg1(struct uds_context *const context,
                                        bool *consume_event) {
  UDSRDBIArgs_t *args = context->arg;
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.data_id == args->dataId &&
      context->event == UDS_EVT_ReadDataByIdent) {
    dynamic_reg1_action_invoked = true;
    // Don't consume event to allow further handlers to run
    *consume_event = false;
  }
  return UDS_OK;
}

UDSErr_t custom_check_for_dynamic_reg2(const struct uds_context *const context,
                                       bool *apply_action) {
  UDSRDBIArgs_t *args = context->arg;
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.data_id == args->dataId &&
      context->event == UDS_EVT_ReadDataByIdent) {
    dynamic_reg2_check_invoked = true;
    *apply_action = true;
  }
  return UDS_OK;
}

UDSErr_t custom_action_for_dynamic_reg2(struct uds_context *const context,
                                        bool *consume_event) {
  UDSRDBIArgs_t *args = context->arg;
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.data_id == args->dataId &&
      context->event == UDS_EVT_ReadDataByIdent) {
    dynamic_reg2_action_invoked = true;
    // Don't consume event to allow further handlers to run
    *consume_event = false;
  }
  return UDS_OK;
}

int custom_unregister_for_dynamic_reg1(
    struct uds_registration_t *registration) {
  dynamic_reg1_unregister_invoked = true;
  return 0;
}

ZTEST_F(lib_uds, test_two_dynamic_registration) {
  struct uds_instance_t *instance = fixture->instance;

  reset_global_counters();

  // Create first dynamic registration
  struct uds_registration_t reg1;
  reg1.type = UDS_REGISTRATION_TYPE__DATA_IDENTIFIER;
  reg1.data_identifier.data_id = UDS_DYNAMIC_DATA_ID_1;
  reg1.data_identifier.read.check = custom_check_for_dynamic_reg1;
  reg1.data_identifier.read.action = custom_action_for_dynamic_reg1;
  reg1.data_identifier.write.check = NULL;
  reg1.data_identifier.write.action = NULL;
  reg1.data_identifier.io_control.check = NULL;
  reg1.data_identifier.io_control.action = NULL;
  reg1.unregister_registration_fn = custom_unregister_for_dynamic_reg1;

  uint32_t dynamic_id_1;
  int ret = instance->register_event_handler(instance, reg1, &dynamic_id_1);
  zassert_ok(ret);

  // Create second dynamic registration
  struct uds_registration_t reg2;
  reg2.type = UDS_REGISTRATION_TYPE__DATA_IDENTIFIER;
  reg2.data_identifier.data_id = UDS_DYNAMIC_DATA_ID_2;
  reg2.data_identifier.read.check = custom_check_for_dynamic_reg2;
  reg2.data_identifier.read.action = custom_action_for_dynamic_reg2;
  reg2.data_identifier.write.check = NULL;
  reg2.data_identifier.write.action = NULL;
  reg2.data_identifier.io_control.check = NULL;
  reg2.data_identifier.io_control.action = NULL;
  reg2.unregister_registration_fn = NULL;

  uint32_t dynamic_id_2;
  ret = instance->register_event_handler(instance, reg2, &dynamic_id_2);
  zassert_ok(ret);

  // Test first dynamic registration
  UDSRDBIArgs_t arg1 = {
    .dataId = UDS_DYNAMIC_DATA_ID_1,
    .copy = copy,
  };

  ret = receive_rdbid_event(instance, &arg1);
  zassert_ok(ret);

  // Verify first registration was called
  zassert_true(dynamic_reg1_check_invoked,
               "First dynamic registration check should be invoked");
  zassert_true(dynamic_reg1_action_invoked,
               "First dynamic registration action should be invoked");
  zassert_false(dynamic_reg2_check_invoked,
                "Second dynamic registration check should not be invoked yet");
  zassert_false(dynamic_reg2_action_invoked,
                "Second dynamic registration action should not be invoked yet");

  reset_global_counters();

  // Test second dynamic registration
  UDSRDBIArgs_t arg2 = {
    .dataId = UDS_DYNAMIC_DATA_ID_2,
    .copy = copy,
  };

  ret = receive_rdbid_event(instance, &arg2);
  zassert_ok(ret);

  // Verify second registration was called
  zassert_false(dynamic_reg1_check_invoked,
                "First dynamic registration check should not be invoked");
  zassert_false(dynamic_reg1_action_invoked,
                "First dynamic registration action should not be invoked");
  zassert_true(dynamic_reg2_check_invoked,
               "Second dynamic registration check should be invoked");
  zassert_true(dynamic_reg2_action_invoked,
               "Second dynamic registration action should be invoked");

  reset_global_counters();

  ret = instance->unregister_event_handler(instance, dynamic_id_1);
  zassert_ok(ret);

  // Verify custom unregister function was called
  zassert_true(dynamic_reg1_unregister_invoked,
               "Custom unregister function should be invoked");

  ret = receive_rdbid_event(instance, &arg1);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);

  zassert_false(dynamic_reg1_check_invoked,
                "First dynamic registration check should not be invoked");
  zassert_false(dynamic_reg1_action_invoked,
                "First dynamic registration action should not be invoked");
  zassert_false(dynamic_reg2_check_invoked,
                "Second dynamic registration check should not be invoked");
  zassert_false(dynamic_reg2_action_invoked,
                "Second dynamic registration action should not be invoked");

  reset_global_counters();

  ret = instance->unregister_event_handler(instance, dynamic_id_2);
  zassert_ok(ret);

  ret = receive_rdbid_event(instance, &arg2);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);

  zassert_false(dynamic_reg1_check_invoked,
                "First dynamic registration check should not be invoked");
  zassert_false(dynamic_reg1_action_invoked,
                "First dynamic registration action should not be invoked");
  zassert_false(dynamic_reg2_check_invoked,
                "Second dynamic registration check should not be invoked");
  zassert_false(dynamic_reg2_action_invoked,
                "Second dynamic registration action should not be invoked");

  reset_global_counters();
}

ZTEST_F(lib_uds, test_unregistering_non_existing_registration) {
  struct uds_instance_t *instance = fixture->instance;

  uint32_t reg_id = 0xFFFFFFFF;
  int ret = instance->unregister_event_handler(instance, reg_id);
  zassert_equal(ret, -ENOENT);
}

#endif  // CONFIG_UDS_USE_DYNAMIC_REGISTRATION