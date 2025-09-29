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

UDSErr_t routine_control_0x87_check_fn(const struct uds_context *const context,
                                       bool *apply_action) {
  zassert_equal(context->event, UDS_EVT_LinkControl);
  zassert_not_null(context->arg);

  UDSLinkCtrlArgs_t *args = context->arg;

  zassert_equal(args->type,
                UDS_LINK_CONTROL__VERIFY_MODE_TRANSITION_WITH_FIXED_PARAMETER);
  zassert_equal(args->len, 1);
  uint8_t data[] = {
    UDS_LINK_CONTROL_MODIFIER__CAN_125000_BAUD,
  };
  zassert_mem_equal(args->data, data, sizeof(data));

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t routine_control_0x87_action_fn(struct uds_context *const context,
                                        bool *consume_event) {
  return UDS_PositiveResponse;
}

ZTEST_F(lib_uds, test_0x87_link_control) {
  struct uds_instance_t *instance = fixture->instance;

  uint8_t data[] = {
    UDS_LINK_CONTROL_MODIFIER__CAN_125000_BAUD,
  };
  UDSLinkCtrlArgs_t args = {
    .type = UDS_LINK_CONTROL__VERIFY_MODE_TRANSITION_WITH_FIXED_PARAMETER,
    .len = sizeof(data),
    .data = data,
  };

  data_id_check_fn_fake.custom_fake = routine_control_0x87_check_fn;
  data_id_action_fn_fake.custom_fake = routine_control_0x87_action_fn;

  int ret = receive_event(instance, UDS_EVT_LinkControl, &args);
  zassert_ok(ret);

  zassert_equal(data_id_action_fn_fake.call_count, 1);
}