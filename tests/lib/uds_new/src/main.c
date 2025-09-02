/**
 * Copyright (c) Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"
#include "iso14229.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ardep/uds_new.h>

FAKE_VOID_FUNC(ecu_reset_work_handler, struct k_work *);

ZTEST_F(lib_uds_new, test_0x11_ecu_reset) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSECUResetArgs_t args = {.type = ECU_RESET_HARD};

  int ret = receive_event(instance, UDS_EVT_EcuReset, &args);
  zassert_ok(ret);

  // Wait for the scheduled worker to finish
  k_msleep(2000);
  zassert_equal(ecu_reset_work_handler_fake.call_count, 1);
}

ZTEST_F(lib_uds_new, test_0x11_ecu_reset_fails_when_subtype_not_implemented) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSECUResetArgs_t args = {.type = ECU_RESET_KEY_OFF_ON};

  int ret = receive_event(instance, UDS_EVT_EcuReset, &args);
  zassert_equal(ret, UDS_NRC_SubFunctionNotSupported);
}
