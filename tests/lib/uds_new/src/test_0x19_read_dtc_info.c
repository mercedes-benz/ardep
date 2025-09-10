/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"
#include "iso14229.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/ztest.h>

#include <ardep/uds_new.h>

ZTEST_F(lib_uds_new, test_0x19_read_dtc_info_no_handler_returns_nrc_oor) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSRDTCIArgs_t args = {
    .type = READ_DTC_INFO_SUBFUNC__NUM_OF_DTC_BY_STATUS_MASK,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDTCInformation, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}