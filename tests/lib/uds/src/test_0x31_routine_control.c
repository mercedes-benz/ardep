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

UDSErr_t routine_control_0x31_check_fn(const struct uds_context *const context,
                                       bool *apply_action) {
  zassert_equal(context->event, UDS_EVT_RoutineCtrl);
  zassert_not_null(context->arg);

  UDSRoutineCtrlArgs_t *args = context->arg;

  zassert_equal(args->ctrlType, UDS_ROUTINE_CONTROL__START_ROUTINE);
  zassert_equal(args->id, routine_id);
  zassert_equal(args->len, 2);
  uint8_t option_record[2] = {0x12, 0x34};
  zassert_mem_equal(args->optionRecord, option_record, sizeof(option_record));

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t routine_control_0x31_action_fn(struct uds_context *const context,
                                        bool *consume_event) {
  UDSRoutineCtrlArgs_t *args = context->arg;

  uint8_t routine_status_record[] = {0x11, 0x22};
  return args->copyStatusRecord(context->server, routine_status_record,
                                sizeof(routine_status_record));
}

ZTEST_F(lib_uds, test_0x31_routine_control) {
  struct uds_instance_t *instance = fixture->instance;

  uint8_t optionRecord[] = {0x12, 0x34};
  UDSRoutineCtrlArgs_t args = {
    .id = routine_id,
    .ctrlType = UDS_ROUTINE_CONTROL__START_ROUTINE,
    .len = sizeof(optionRecord),
    .optionRecord = optionRecord,
    .copyStatusRecord = copy,
  };

  data_id_check_fn_fake.custom_fake = routine_control_0x31_check_fn;
  data_id_action_fn_fake.custom_fake = routine_control_0x31_action_fn;

  int ret = receive_event(instance, UDS_EVT_RoutineCtrl, &args);
  zassert_ok(ret);

  uint8_t response_data[] = {0x11, 0x22};
  assert_copy_data(response_data, sizeof(response_data));

  zassert_equal(data_id_action_fn_fake.call_count, 1);
}

ZTEST_F(lib_uds,
        test_0x31_routine_control_check_gets_not_called_with_wrong_id) {
  struct uds_instance_t *instance = fixture->instance;

  uint8_t optionRecord[] = {0x12, 0x34};
  UDSRoutineCtrlArgs_t args = {
    .id = routine_id + 1,
    .ctrlType = UDS_ROUTINE_CONTROL__START_ROUTINE,
    .len = sizeof(optionRecord),
    .optionRecord = optionRecord,
    .copyStatusRecord = copy,
  };

  data_id_check_fn_fake.custom_fake = routine_control_0x31_check_fn;
  data_id_action_fn_fake.custom_fake = routine_control_0x31_action_fn;

  int ret = receive_event(instance, UDS_EVT_RoutineCtrl, &args);
  zassert_equal(ret, UDS_NRC_SubFunctionNotSupported);
  zassert_equal(data_id_check_fn_fake.call_count, 0);
}
