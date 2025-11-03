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

UDSErr_t auth_0x29_check_fn(const struct uds_context *const context,
                            bool *apply_action) {
  if (context->event == UDS_EVT_Auth) {
    UDSAuthArgs_t *args = (UDSAuthArgs_t *)context->arg;

    switch (args->type) {
      case UDS_LEV_AT_VCU:
      case UDS_LEV_AT_AC:
        *apply_action = true;
        return UDS_OK;
    }
  } else if (context->event == UDS_EVT_AuthTimeout) {
    *apply_action = true;
    return UDS_OK;
  }
  return UDS_NRC_SubFunctionNotSupported;
}

UDSErr_t auth_0x29_action_fn(struct uds_context *const context,
                             bool *consume_event) {
  if (context->event == UDS_EVT_Auth) {
    UDSAuthArgs_t *args = (UDSAuthArgs_t *)context->arg;
    *consume_event = true;
    switch (args->type) {
      case UDS_LEV_AT_AC:
        return args->set_auth_state(context->server, UDS_AT_ACAPCE);
      default:
        return UDS_NRC_SubFunctionNotSupported;
    }

    return UDS_PositiveResponse;
  } else if (context->event == UDS_EVT_AuthTimeout) {
    *consume_event = true;
    return UDS_PositiveResponse;
  }
  return UDS_NRC_SubFunctionNotSupported;
}

ZTEST_F(lib_uds, test_0x29_auth_timeout) {
  struct uds_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake = auth_0x29_check_fn;
  data_id_action_fn_fake.custom_fake = auth_0x29_action_fn;

  uint32_t action_call_count = data_id_action_fn_fake.call_count;

  int ret = receive_event(instance, UDS_EVT_AuthTimeout, NULL);
  zassert_ok(ret);
  zassert_true(data_id_action_fn_fake.call_count > action_call_count);
}

ZTEST_F(lib_uds, test_0x29_auth_get_auch_config) {
  struct uds_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake = auth_0x29_check_fn;
  data_id_action_fn_fake.custom_fake = auth_0x29_action_fn;

  UDSAuthArgs_t args = {
    .type = UDS_LEV_AT_AC,
    .copy = copy,
    .set_auth_state = set_auth_state,
  };

  uint32_t action_call_count = data_id_action_fn_fake.call_count;

  int ret = receive_event(instance, UDS_EVT_Auth, &args);
  zassert_ok(ret);
  zassert_equal(set_auth_state_fake.call_count, 1);
  zassert_true(data_id_action_fn_fake.call_count > action_call_count);
  assert_auth_state(UDS_AT_ACAPCE);
}
