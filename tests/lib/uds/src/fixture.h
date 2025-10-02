/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef APP_TESTS_LIB_ISO14229_SRC_FIXTURE_H_
#define APP_TESTS_LIB_ISO14229_SRC_FIXTURE_H_

#include "ardep/uds.h"

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/fff.h>

#include <ardep/iso14229.h>
#include <iso14229.h>

DECLARE_FAKE_VALUE_FUNC(uint8_t, copy, UDSServer_t *, const void *, uint16_t);

DECLARE_FAKE_VALUE_FUNC(uint8_t, set_auth_state, UDSServer_t *, uint8_t);

DECLARE_FAKE_VALUE_FUNC(UDSErr_t,
                        data_id_check_fn,
                        const struct uds_context *const,
                        bool *);

DECLARE_FAKE_VALUE_FUNC(UDSErr_t,
                        data_id_action_fn,
                        struct uds_context *const,
                        bool *);

extern const uint16_t data_id_r;
extern uint8_t data_id_r_data[4];

extern const uint16_t data_id_rw;
extern uint8_t data_id_rw_data[4];

extern const uint16_t data_id_rw_duplicated1;
extern const uint16_t data_id_rw_duplicated2;
extern uint8_t data_id_rw_duplicated_data[4];

extern const uint16_t routine_id;

#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION
extern bool test_dynamic_registration_check_invoked;
extern bool test_dynamic_registration_action_invoked;
#endif  // # CONFIG_UDS_USE_DYNAMIC_REGISTRATION

struct lib_uds_fixture {
  UDSISOTpCConfig_t cfg;
  struct uds_instance_t *instance;
  const struct device *can_dev;
};

/**
 * @brief Receive an event from iso14229
 */
UDSErr_t receive_event(struct uds_instance_t *inst,
                       UDSEvent_t event,
                       void *args);

/**
 * @brief Assert that the copied data matches the expected data.
 *
 * Beware that the data is in big endian!
 */
void assert_copy_data(const uint8_t *data, uint32_t len);

/**
 * @brief same as `assert_copy_data` but allows to specify an offset
 */
void assert_copy_data_offset(const uint8_t *data,
                             uint32_t len,
                             uint32_t offset);

/**
 * @brief Assert that when using authentication, the correct returnValue is set
 */
void assert_auth_state(uint8_t expected_state);

#endif  // APP_TESTS_LIB_ISO14229_SRC_FIXTURE_H_
