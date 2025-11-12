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

UDSErr_t comm_control_0x28_check_fn(const struct uds_context *const context,
                                    bool *apply_action) {
  zassert_equal(context->event, UDS_EVT_CommCtrl);
  zassert_not_null(context->arg);

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t comm_control_0x28_action_fn(struct uds_context *const context,
                                     bool *consume_event) {
  UDSCommCtrlArgs_t *args = context->arg;

  if (args->commType == 0x01) {
    zassert_equal(args->ctrlType, 3);

    return UDS_PositiveResponse;
  }

  zassert_equal(args->ctrlType, 5);
  zassert_equal(args->nodeId, 0x000A);
  return UDS_NRC_ConditionsNotCorrect;
}

ZTEST_F(lib_uds, test_comm_control_0x28) {
  struct uds_instance_t *instance = fixture->instance;

  UDSCommCtrlArgs_t args = {
    .commType = 0x01,
    .ctrlType = 0x03,
  };

  data_id_check_fn_fake.custom_fake = comm_control_0x28_check_fn;
  data_id_action_fn_fake.custom_fake = comm_control_0x28_action_fn;

  int ret = receive_event(instance, UDS_EVT_CommCtrl, &args);
  zassert_ok(ret);

  zassert_equal(data_id_action_fn_fake.call_count, 1);
}

ZTEST_F(lib_uds, test_comm_control_0x28_neg_resp) {
  struct uds_instance_t *instance = fixture->instance;

  UDSCommCtrlArgs_t args = {
    .commType = 0x04,
    .ctrlType = 0x05,
    .nodeId = 0x000A,
  };

  data_id_check_fn_fake.custom_fake = comm_control_0x28_check_fn;
  data_id_action_fn_fake.custom_fake = comm_control_0x28_action_fn;

  int ret = receive_event(instance, UDS_EVT_CommCtrl, &args);
  zassert_equal(ret, UDS_NRC_ConditionsNotCorrect);

  zassert_equal(data_id_action_fn_fake.call_count, 1);
}
