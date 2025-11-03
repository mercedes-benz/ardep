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

UDSErr_t control_dtc_setting_0x85_check_fn(
    const struct uds_context *const context, bool *apply_action) {
  zassert_equal(context->event, UDS_EVT_ControlDTCSetting);
  zassert_not_null(context->arg);

  UDSControlDTCSettingArgs_t *args = context->arg;

  *apply_action = true;

  switch (args->type) {
    case 0x01:  // DTCSettingOn
      zassert_equal(args->len, 2);
      uint8_t expected_data_on[2] = {0xAB, 0xCD};
      zassert_mem_equal(args->data, expected_data_on, sizeof(expected_data_on));
      return UDS_OK;

    case 0x02:  // DTCSettingOff
      zassert_equal(args->len, 0);
      zassert_is_null(args->data);
      return UDS_OK;

    case 0x40:
      return UDS_NRC_ConditionsNotCorrect;

    default:
      *apply_action = false;
      return UDS_OK;
  }
}

UDSErr_t control_dtc_setting_0x85_action_fn(struct uds_context *const context,
                                            bool *consume_event) {
  *consume_event = true;
  return UDS_PositiveResponse;
}

ZTEST_F(lib_uds, test_0x85_control_dtc_setting_on) {
  struct uds_instance_t *instance = fixture->instance;

  uint8_t dtc_setting_data[] = {0xAB, 0xCD};
  UDSControlDTCSettingArgs_t args = {
    .type = 0x01,  // DTCSettingOn
    .len = sizeof(dtc_setting_data),
    .data = dtc_setting_data,
  };

  data_id_check_fn_fake.custom_fake = control_dtc_setting_0x85_check_fn;
  data_id_action_fn_fake.custom_fake = control_dtc_setting_0x85_action_fn;

  int ret = receive_event(instance, UDS_EVT_ControlDTCSetting, &args);
  zassert_ok(ret);

  zassert_equal(data_id_check_fn_fake.call_count, 1);
  zassert_equal(data_id_action_fn_fake.call_count, 1);
}

ZTEST_F(lib_uds, test_0x85_control_dtc_setting_off) {
  struct uds_instance_t *instance = fixture->instance;

  UDSControlDTCSettingArgs_t args = {
    .type = 0x02,  // DTCSettingOff
    .len = 0,
    .data = NULL,
  };

  data_id_check_fn_fake.custom_fake = control_dtc_setting_0x85_check_fn;
  data_id_action_fn_fake.custom_fake = control_dtc_setting_0x85_action_fn;

  int ret = receive_event(instance, UDS_EVT_ControlDTCSetting, &args);
  zassert_ok(ret);

  zassert_equal(data_id_check_fn_fake.call_count, 1);
  zassert_equal(data_id_action_fn_fake.call_count, 1);
}

ZTEST_F(lib_uds, test_0x85_control_dtc_setting_negative_response) {
  struct uds_instance_t *instance = fixture->instance;

  UDSControlDTCSettingArgs_t args = {
    .type = 0x40,
    .len = 0,
    .data = NULL,
  };

  data_id_check_fn_fake.custom_fake = control_dtc_setting_0x85_check_fn;
  data_id_action_fn_fake.custom_fake = control_dtc_setting_0x85_action_fn;

  int ret = receive_event(instance, UDS_EVT_ControlDTCSetting, &args);
  zassert_equal(ret, UDS_NRC_ConditionsNotCorrect);

  zassert_equal(data_id_check_fn_fake.call_count, 1);
  // Action should not be called when check returns NRC
  zassert_equal(data_id_action_fn_fake.call_count, 0);
}
