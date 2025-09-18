/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_TESTS_LIB_ISO14229_SRC_FIXTURE_H_
#define APP_TESTS_LIB_ISO14229_SRC_FIXTURE_H_

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/fff.h>

#include <ardep/iso14229.h>
#include <iso14229.h>

extern bool session_timeout_event_fired;

// Test callback function for UDS events
DECLARE_FAKE_VALUE_FUNC(UDSErr_t,
                        test_uds_callback,
                        struct iso14229_zephyr_instance *,
                        UDSEvent_t,
                        void *,
                        void *);

struct lib_iso14229_fixture {
  UDSISOTpCConfig_t cfg;

  struct iso14229_zephyr_instance instance;

  const struct device *can_dev;
};

/**
 * Tick the iso14229 thread once
 *
 * @param fixture The fixture containing the configuration and device
 */
void tick_thread(struct iso14229_zephyr_instance *instance);

/**
 * Advance Time andTick the iso14229 thread once
 *
 * Necessary to elapse the timeout to send the next response to the client
 *
 * @param fixture The fixture containing the configuration and device
 */
void advance_time_and_tick_thread(struct iso14229_zephyr_instance *instance);

/**
 * Advance Time andTick the iso14229 thread once
 *
 * Necessary to elapse the timeout to send the next response to the client
 *
 * @param fixture The fixture containing the configuration and device
 * @param num_of_ticks Number of times to tick the thread
 */
void advance_time_and_tick_thread_num(struct iso14229_zephyr_instance *instance,
                                      size_t num_of_ticks);

/**
 *  Fake the reception of a physical CAN Frame
 *
 * @param fixture The fixture containing the configuration and device
 * @param data The whole CAN frame data
 * @param data_len  The length of the CAN frame data (== dlc)
 */
void receive_phys_can_frame(const struct lib_iso14229_fixture *fixture,
                            uint8_t *data,
                            uint8_t data_len);

/**
 * Fake the reception of a physical CAN Frame from an Array of bytes
 *
 * The array holds the full CAN Frame received. Array-length == dlc
 */
#define receive_phys_can_frame_array(fixture, data_array) \
  receive_phys_can_frame(fixture, data_array, ARRAY_SIZE(data_array))

/**
 * Assert that a CAN Frame was send to the physical target address
 *
 * @param fixture The fixture containing the configuration and device
 * @param data The CAN frame data
 * @param data_len The length of the CAN frame data (== dlc)
 */
void assert_send_phy_can_frame(const struct lib_iso14229_fixture *fixture,
                               uint32_t frame_index,
                               uint8_t *data,
                               uint8_t data_len);

/**
 * Assert that a CAN Frame was send to the physical target address
 *
 * The array holds the expected full CAN Frame.
 *
 */
#define assert_send_phy_can_frame_array(fixture, frame_index, data_array) \
  assert_send_phy_can_frame(fixture, frame_index, data_array,             \
                            ARRAY_SIZE(data_array))

#endif  // APP_TESTS_LIB_ISO14229_SRC_FIXTURE_H_