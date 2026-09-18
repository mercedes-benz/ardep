/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"

#include <zephyr/ztest.h>

// The catch-all handler registered in the fixture uses `catchall_check_fn` and
// `catchall_action_fn`. By default those fakes do not apply the action, so this
// helper opts the catch-all in for a specific event.
static UDSEvent_t catchall_expected_event;

static UDSErr_t catchall_apply_for_event(
    const struct uds_context *const context, bool *apply_action) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER &&
      context->registration->data_identifier.ignore_data_id &&
      context->event == catchall_expected_event) {
    *apply_action = true;
  }
  return UDS_OK;
}

//////////////////////7
// The catch-all handler must fire for a DID that has no dedicated handler.

ZTEST_F(lib_uds, test_catchall_read_invoked_for_unregistered_did) {
  struct uds_instance_t *instance = fixture->instance;

  catchall_expected_event = UDS_EVT_ReadDataByIdent;
  catchall_check_fn_fake.custom_fake = catchall_apply_for_event;

  UDSRDBIArgs_t arg = {
    .dataId = data_id_unregistered,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &arg);
  zassert_ok(ret);

  zassert_true(catchall_check_fn_fake.call_count >= 1);
  zassert_equal(catchall_action_fn_fake.call_count, 1);
}

//////////////////////7

ZTEST_F(lib_uds, test_catchall_write_invoked_for_unregistered_did) {
  struct uds_instance_t *instance = fixture->instance;

  catchall_expected_event = UDS_EVT_WriteDataByIdent;
  catchall_check_fn_fake.custom_fake = catchall_apply_for_event;

  uint32_t data = 0xDEADBEEF;
  UDSWDBIArgs_t arg = {
    .dataId = data_id_unregistered,
    .data = (uint8_t *)&data,
    .len = sizeof(data),
  };

  int ret = receive_event(instance, UDS_EVT_WriteDataByIdent, &arg);
  zassert_ok(ret);

  zassert_true(catchall_check_fn_fake.call_count >= 1);
  zassert_equal(catchall_action_fn_fake.call_count, 1);
}

//////////////////////7
// Regression: the io-control check must honor `ignore_data_id` too, otherwise
// the catch-all handler never receives io-control events.

ZTEST_F(lib_uds, test_catchall_io_control_invoked_for_unregistered_did) {
  struct uds_instance_t *instance = fixture->instance;

  catchall_expected_event = UDS_EVT_IOControl;
  catchall_check_fn_fake.custom_fake = catchall_apply_for_event;

  uint8_t ctrl_state_and_mask[1] = {0x01};
  UDSIOCtrlArgs_t arg = {
    .dataId = data_id_unregistered,
    .ctrlStateAndMaskLen = 1,
    .ctrlStateAndMask = ctrl_state_and_mask,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_IOControl, &arg);
  zassert_ok(ret);

  zassert_true(catchall_check_fn_fake.call_count >= 1);
  zassert_equal(catchall_action_fn_fake.call_count, 1);
}

//////////////////////7
// The catch-all handler also sees DIDs that do have a dedicated handler.

ZTEST_F(lib_uds, test_catchall_invoked_for_registered_did) {
  struct uds_instance_t *instance = fixture->instance;

  catchall_expected_event = UDS_EVT_ReadDataByIdent;
  catchall_check_fn_fake.custom_fake = catchall_apply_for_event;

  UDSRDBIArgs_t arg = {
    .dataId = data_id_r,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &arg);
  zassert_ok(ret);

  zassert_true(catchall_check_fn_fake.call_count >= 1);
  zassert_equal(catchall_action_fn_fake.call_count, 1);
}

//////////////////////7
// When the catch-all opts out (default fakes), an unknown DID is still
// rejected as out of range.

ZTEST_F(lib_uds, test_catchall_does_not_apply_when_check_declines) {
  struct uds_instance_t *instance = fixture->instance;

  UDSRDBIArgs_t arg = {
    .dataId = data_id_unregistered,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &arg);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);

  zassert_true(catchall_check_fn_fake.call_count >= 1);
  zassert_equal(catchall_action_fn_fake.call_count, 0);
}
