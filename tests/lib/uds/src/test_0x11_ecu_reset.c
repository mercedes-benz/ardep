/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"

#include <zephyr/ztest.h>

UDSErr_t custom_check_for_0x11_return_subfunc_not_sup(
    const struct uds_context *const context, bool *apply_action) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__ECU_RESET &&
      context->event == UDS_EVT_EcuReset) {
    UDSECUResetArgs_t *args = context->arg;
    if (args->type == ECU_RESET__KEY_OFF_ON) {
      *apply_action = true;
    }
  }
  return UDS_OK;
}

UDSErr_t custom_action_for_0x11_return_subfunc_not_sup(
    struct uds_context *const context, bool *consume_event) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__ECU_RESET &&
      context->event == UDS_EVT_EcuReset) {
    UDSECUResetArgs_t *args = context->arg;
    if (args->type == ECU_RESET__KEY_OFF_ON) {
      *consume_event = true;
    }
  }
  return UDS_OK;
}

ZTEST_F(lib_uds, test_0x11_ecu_reset_return_subfunc_not_sup) {
  struct uds_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_0x11_return_subfunc_not_sup;
  data_id_action_fn_fake.custom_fake =
      custom_action_for_0x11_return_subfunc_not_sup;

  UDSECUResetArgs_t arg = {
    .type = ECU_RESET__HARD,
    .powerDownTimeMillis = 500,
  };

  int ret = receive_event(instance, UDS_EVT_EcuReset, &arg);
  zassert_equal(ret, UDS_NRC_SubFunctionNotSupported);
}

/////////////////////////////////

UDSErr_t custom_check_for_0x11_ecu_reset_event_works(
    const struct uds_context *const context, bool *apply_action) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__ECU_RESET &&
      context->event == UDS_EVT_EcuReset) {
    UDSECUResetArgs_t *args = context->arg;
    if (args->type == ECU_RESET__KEY_OFF_ON) {
      *apply_action = true;
    }
  }
  return UDS_OK;
}

UDSErr_t custom_action_for_0x11_ecu_reset_event_works(
    struct uds_context *const context, bool *consume_event) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__ECU_RESET &&
      context->event == UDS_EVT_EcuReset) {
    UDSECUResetArgs_t *args = context->arg;
    if (args->type == ECU_RESET__KEY_OFF_ON) {
      *consume_event = true;
    }
  }
  return UDS_OK;
}

ZTEST_F(lib_uds, test_0x11_ecu_reset_ecu_reset_event_works) {
  struct uds_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_0x11_ecu_reset_event_works;
  data_id_action_fn_fake.custom_fake =
      custom_action_for_0x11_ecu_reset_event_works;

  UDSECUResetArgs_t arg = {
    .type = ECU_RESET__KEY_OFF_ON,
    .powerDownTimeMillis = 500,
  };

  int ret = receive_event(instance, UDS_EVT_EcuReset, &arg);
  zassert_ok(ret);

  zassert_true(data_id_check_fn_fake.call_count >= 1);
  zassert_equal(data_id_action_fn_fake.call_count, 1);
}

/////////////////////////////////

UDSErr_t custom_check_for_0x11_do_scheduled_reset_event_works(
    const struct uds_context *const context, bool *apply_action) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__ECU_RESET &&
      context->event == UDS_EVT_DoScheduledReset) {
    if (*(uint32_t *)context->arg == ECU_RESET__KEY_OFF_ON) {
      *apply_action = true;
    }
  }
  return UDS_OK;
}

UDSErr_t custom_action_for_0x11_do_scheduled_reset_event_works(
    struct uds_context *const context, bool *consume_event) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__ECU_RESET &&
      context->event == UDS_EVT_DoScheduledReset) {
    if (*(uint32_t *)context->arg == ECU_RESET__KEY_OFF_ON) {
      *consume_event = true;
    }
  }
  return UDS_OK;
}

ZTEST_F(lib_uds, test_0x11_ecu_reset_do_scheduled_reset_event_works) {
  struct uds_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_0x11_do_scheduled_reset_event_works;
  data_id_action_fn_fake.custom_fake =
      custom_action_for_0x11_do_scheduled_reset_event_works;

  uint32_t reset_type = ECU_RESET__KEY_OFF_ON;
  ;

  int ret = receive_event(instance, UDS_EVT_DoScheduledReset, &reset_type);
  zassert_ok(ret);

  zassert_true(data_id_check_fn_fake.call_count >= 1);
  zassert_equal(data_id_action_fn_fake.call_count, 1);
}
