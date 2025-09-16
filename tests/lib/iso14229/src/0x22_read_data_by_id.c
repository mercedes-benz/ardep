/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

// Include UDS minimal library headers
#include <ardep/iso14229.h>
#include <iso14229.h>

static const uint16_t read_data_by_id_1 = 0x0102;
static const uint8_t read_data_by_id_1_data[] = {0x70, 0x71, 0x72};
static const uint16_t read_data_by_id_2 = 0x0304;
static const uint8_t read_data_by_id_2_data[] = {0xF0, 0xF1, 0xF2, 0xF3,
                                                 0xF4, 0xF5, 0xF6, 0xF7};

UDSErr_t test_0x22_read_data_by_identifier_uds_callback(
    struct iso14229_zephyr_instance *inst,
    UDSEvent_t event,
    void *arg,
    void *user_context) {
  if (event == UDS_EVT_ReadDataByIdent) {
    UDSRDBIArgs_t *args = arg;
    if (args->dataId == read_data_by_id_1) {
      args->copy(&inst->server, read_data_by_id_1_data,
                 ARRAY_SIZE(read_data_by_id_1_data));
    } else if (args->dataId == read_data_by_id_2) {
      args->copy(&inst->server, read_data_by_id_2_data,
                 ARRAY_SIZE(read_data_by_id_2_data));
    }
  }

  return UDS_OK;
}

ZTEST_F(lib_iso14229, test_0x22_read_data_by_identifier) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake =
      test_0x22_read_data_by_identifier_uds_callback;

  uint8_t request_data[] = {
    0x05,                             // PCI    (single frame, 5 bytes of data)
    0x22,                             // RDBI   (Read Data by Identifier)
    (read_data_by_id_1 >> 8) & 0xFF,  // DID_HB (Identifier 1 High Byte)
    read_data_by_id_1 & 0xFF,         // DID_LB (Identifier 1 Low Byte)
    (read_data_by_id_2 >> 8) & 0xFF,  // DID_HB (Identifier 2 High Byte)
    read_data_by_id_2 & 0xFF,         // DID_LB (Identifier 2 Low Byte)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);

  zassert_equal(test_uds_callback_fake.call_count, 2);
  zassert_equal(test_uds_callback_fake.arg1_val, UDS_EVT_ReadDataByIdent);

  advance_time_and_tick_thread(instance);

  uint8_t ff_response_data[] = {
    0x10,                             // PCI_HB (first frame, upper nibble)
    0x10,                             // PCI_LB (16 bytes of data total)
    0x62,                             // RDBIPR (positive response to 0x22)
    (read_data_by_id_1 >> 8) & 0xFF,  // DID_HB (Identifier 1 High Byte)
    read_data_by_id_1 & 0xFF,         // DID_LB (Identifier 1 Low Byte)
    read_data_by_id_1_data[0],        // DREC_DATA_1
    read_data_by_id_1_data[1],        // DREC_DATA_2
    read_data_by_id_1_data[2],        // DREC_DATA_3
  };
  assert_send_phy_can_frame_array(fixture, 0, ff_response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);

  uint8_t control_flow_frame[] = {
    0x30,  // PCI         (control flow frame, continue to send)
    0x00,  // CTL_FCBS    (Block Size 0 == send all)
    0x00,  // CTL_FCSTMIN (Separator time minimum (0ms))
  };

  receive_phys_can_frame_array(fixture, control_flow_frame);
  advance_time_and_tick_thread(instance);

  uint8_t mf_response_data_1[] = {
    0x21,  // PCI_HB (conecutive frame, frame index 1)
    (read_data_by_id_2 >> 8) & 0xFF,  // DID_HB (Identifier 2 High Byte)
    read_data_by_id_2 & 0xFF,         // DID_LB (Identifier 2 Low Byte)
    read_data_by_id_2_data[0],        // DREC_DATA_1
    read_data_by_id_2_data[1],        // DREC_DATA_2
    read_data_by_id_2_data[2],        // DREC_DATA_3
    read_data_by_id_2_data[3],        // DREC_DATA_4
    read_data_by_id_2_data[4],        // DREC_DATA_5
  };
  assert_send_phy_can_frame_array(fixture, 1, mf_response_data_1);
  zassert_equal(fake_can_send_fake.call_count, 2);

  advance_time_and_tick_thread(instance);

  uint8_t mf_response_data_2[] = {
    0x22,                       // PCI_HB (conecutive frame, frame index 1)
    read_data_by_id_2_data[5],  // DREC_DATA_6
    read_data_by_id_2_data[6],  // DREC_DATA_7
    read_data_by_id_2_data[7],  // DREC_DATA_8
  };
  assert_send_phy_can_frame_array(fixture, 2, mf_response_data_2);
  zassert_equal(fake_can_send_fake.call_count, 3);
}