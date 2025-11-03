/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds.h"
#include "fixture.h"
#include "iso14229.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/ztest.h>

UDSErr_t clear_diag_info_0x14_check_fn(const struct uds_context *const context,
                                       bool *apply_action) {
  zassert_equal(context->event, UDS_EVT_ClearDiagnosticInfo);
  zassert_not_null(context->arg);
  *apply_action = true;
  return UDS_OK;
}

UDSErr_t clear_diag_info_0x14_action_fn(struct uds_context *const context,
                                        bool *consume_event) {
  return UDS_OK;
}

ZTEST_F(lib_uds, test_0x14_clear_diag_info) {
  struct uds_instance_t *instance = fixture->instance;

  UDSCDIArgs_t args = {
    .groupOfDTC = 0x00FFDD33,
    .hasMemorySelection = false,
    .memorySelection = 0,
  };

  data_id_check_fn_fake.custom_fake = clear_diag_info_0x14_check_fn;
  data_id_action_fn_fake.custom_fake = clear_diag_info_0x14_action_fn;

  int ret = receive_event(instance, UDS_EVT_ClearDiagnosticInfo, &args);
  zassert_ok(ret);

  zassert_equal(data_id_action_fn_fake.call_count, 1);
}

UDSErr_t clear_diag_info_0x14_action_fn_failure(
    struct uds_context *const context, bool *consume_event) {
  return UDS_NRC_ConditionsNotCorrect;
}

ZTEST_F(lib_uds, test_0x14_clear_diag_info_failure) {
  struct uds_instance_t *instance = fixture->instance;

  UDSCDIArgs_t args = {
    .groupOfDTC = 0x00FFDD33,
    .hasMemorySelection = false,
    .memorySelection = 0,
  };

  data_id_check_fn_fake.custom_fake = clear_diag_info_0x14_check_fn;
  data_id_action_fn_fake.custom_fake = clear_diag_info_0x14_action_fn_failure;

  int ret = receive_event(instance, UDS_EVT_ClearDiagnosticInfo, &args);
  zassert_equal(ret, UDS_NRC_ConditionsNotCorrect);

  zassert_equal(data_id_action_fn_fake.call_count, 1);
}
