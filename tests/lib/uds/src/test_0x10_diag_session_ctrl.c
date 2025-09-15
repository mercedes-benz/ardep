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

UDSErr_t custom_check_for_0x10_diag_session_ctrl_callback_on_session_change(
    const struct uds_context *const context, bool *apply_action) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__DIAG_SESSION_CTRL &&
      context->event == UDS_EVT_DiagSessCtrl) {
    UDSDiagSessCtrlArgs_t *args = context->arg;
    zassert_equal(args->type, 0x02);
    zassert_equal(args->p2_ms, 0x100);
    zassert_equal(args->p2_star_ms, 0x200);
    *apply_action = true;
  }
  return UDS_OK;
}

ZTEST_F(lib_uds, test_0x10_diag_session_ctrl_callback_on_session_change) {
  struct uds_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_0x10_diag_session_ctrl_callback_on_session_change;

  UDSDiagSessCtrlArgs_t arg = {
    .type = 0x02,  // programming session
    .p2_ms = 0x100,
    .p2_star_ms = 0x200,
  };

  int ret = receive_event(instance, UDS_EVT_DiagSessCtrl, &arg);
  zassert_ok(ret);

  zassert_true(data_id_check_fn_fake.call_count >= 1);
  zassert_equal(data_id_action_fn_fake.call_count, 1);
}

UDSErr_t custom_check_for_0x10_diag_session_ctrl_callback_on_session_timeout(
    const struct uds_context *const context, bool *apply_action) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__DIAG_SESSION_CTRL &&
      context->event == UDS_EVT_SessionTimeout) {
    *apply_action = true;
    zassert_equal_ptr(context->arg, NULL);
  }
  return UDS_OK;
}

ZTEST_F(lib_uds, test_0x10_diag_session_ctrl_callback_on_session_timeout) {
  struct uds_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_0x10_diag_session_ctrl_callback_on_session_timeout;

  int ret = receive_event(instance, UDS_EVT_SessionTimeout, NULL);
  zassert_ok(ret);

  zassert_true(data_id_check_fn_fake.call_count >= 1);
  zassert_equal(data_id_action_fn_fake.call_count, 1);
}
