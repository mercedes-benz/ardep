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

UDSErr_t io_control_0x2F_check_fn(const struct uds_context *const context,
                                  bool *apply_action) {
  zassert_equal(context->event, UDS_EVT_IOControl);
  zassert_not_null(context->arg);

  UDSIOCtrlArgs_t *args = (UDSIOCtrlArgs_t *)context->arg;
  zassert_equal(args->dataId, data_id_rw);
  zassert_equal(args->ctrlStateAndMaskLen, 1);
  uint8_t ctrl_state_and_mask[1] = {0x01};
  zassert_mem_equal(args->ctrlStateAndMask, ctrl_state_and_mask, 1);
  zassert_equal(args->copy, copy);

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t io_control_0x2F_action_fn(struct uds_context *const context,
                                   bool *consume_event) {
  UDSIOCtrlArgs_t *args = (UDSIOCtrlArgs_t *)context->arg;

  uint8_t response_data[] = {0x11, 0x22};
  return args->copy(context->server, response_data, sizeof(response_data));
}

ZTEST_F(lib_uds, test_0x2F_io_control) {
  struct uds_instance_t *instance = fixture->instance;

  uint8_t ctrl_state_and_mask[1] = {0x01};
  UDSIOCtrlArgs_t args = {
    .dataId = data_id_rw,
    .ctrlStateAndMaskLen = 1,
    .ctrlStateAndMask = ctrl_state_and_mask,
    .copy = copy,
  };

  data_id_check_fn_fake.custom_fake = io_control_0x2F_check_fn;
  data_id_action_fn_fake.custom_fake = io_control_0x2F_action_fn;

  int ret = receive_event(instance, UDS_EVT_IOControl, &args);
  zassert_ok(ret);

  uint8_t response_data[] = {0x11, 0x22};
  assert_copy_data(response_data, sizeof(response_data));

  zassert_equal(data_id_action_fn_fake.call_count, 1);
}
