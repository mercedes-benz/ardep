/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"

#include <zephyr/ztest.h>

UDSErr_t read_dtc_info_0x01_check_fn(const struct uds_context *const context,
                                     bool *apply_action) {
  zassert_equal(context->event, UDS_EVT_ReadDTCInformation);
  UDSRDTCIArgs_t *args = context->arg;
  zassert_not_null(args);
  zassert_equal(args->type,
                UDS_READ_DTC_INFO_SUBFUNC__NUM_OF_DTC_BY_STATUS_MASK);
  *apply_action = true;
  return UDS_OK;
}

UDSErr_t read_dtc_info_0x01_action_fn(struct uds_context *const context,
                                      bool *consume_event) {
  UDSRDTCIArgs_t *args = context->arg;
  zassert_equal(args->type,
                UDS_READ_DTC_INFO_SUBFUNC__NUM_OF_DTC_BY_STATUS_MASK);

  copy(&context->instance->iso14229.server, (uint8_t[]){0x01, 0x02, 0x03}, 3);

  return UDS_OK;
}

ZTEST_F(lib_uds, test_0x19_read_dtc_info_0x01_num_of_dtc_by_status_mask) {
  struct uds_instance_t *instance = fixture->instance;

  UDSRDTCIArgs_t args = {
    .type = UDS_READ_DTC_INFO_SUBFUNC__NUM_OF_DTC_BY_STATUS_MASK,
    .copy = copy,
  };

  data_id_check_fn_fake.custom_fake = read_dtc_info_0x01_check_fn;
  data_id_action_fn_fake.custom_fake = read_dtc_info_0x01_action_fn;

  int ret = receive_event(instance, UDS_EVT_ReadDTCInformation, &args);
  zassert_ok(ret);

  zassert_equal(data_id_action_fn_fake.call_count, 1);
  assert_copy_data((uint8_t[]){0x01, 0x02, 0x03}, 3);
}

UDSErr_t read_dtc_info_0x02_nrc_check_fn(
    const struct uds_context *const context, bool *apply_action) {
  zassert_equal(context->event, UDS_EVT_ReadDTCInformation);
  UDSRDTCIArgs_t *args = context->arg;
  zassert_not_null(args);
  zassert_equal(args->type, UDS_READ_DTC_INFO_SUBFUNC__DTC_BY_STATUS_MASK);
  *apply_action = true;
  return UDS_OK;
}

UDSErr_t read_dtc_info_0x02_nrc_action_fn(struct uds_context *const context,
                                          bool *consume_event) {
  UDSRDTCIArgs_t *args = context->arg;
  zassert_equal(args->type, UDS_READ_DTC_INFO_SUBFUNC__DTC_BY_STATUS_MASK);

  return UDS_NRC_RequestOutOfRange;
}

ZTEST_F(lib_uds, test_0x19_read_dtc_info_0x02_dtc_by_status_mask_return_nrc) {
  struct uds_instance_t *instance = fixture->instance;

  UDSRDTCIArgs_t args = {
    .type = UDS_READ_DTC_INFO_SUBFUNC__DTC_BY_STATUS_MASK,
    .copy = copy,
  };

  data_id_check_fn_fake.custom_fake = read_dtc_info_0x02_nrc_check_fn;
  data_id_action_fn_fake.custom_fake = read_dtc_info_0x02_nrc_action_fn;

  int ret = receive_event(instance, UDS_EVT_ReadDTCInformation, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}