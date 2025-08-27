/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

// Include UDS minimal library headers
#include <ardep/uds_minimal.h>
#include <server.h>
#include <tp/isotp_c.h>
#include <uds.h>

static const uint8_t uds_mem_data[255] = {
  0x01, 0x02, 0x03, 0x04, 0x05, 0x66, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
  0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A,
  0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34,
  0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41,
  0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E,
  0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B,
  0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
  0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75,
  0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82,
  0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C,
  0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9,
  0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
  0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3,
  0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0,
  0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD,
  0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
  0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
  0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
};

ZTEST_F(lib_uds_minimal, test_0x10_diag_session_ctrl) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  uint8_t request_data[] = {
    0x02,  // PCI (single frame, 2 bytes of data)
    0x10,  // SID (DiagnosticSessionControl)
    0x02,  // DS  (Programming Session)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);

  zassert_equal(test_uds_callback_fake.call_count, 1);
  zassert_equal(test_uds_callback_fake.arg1_val, UDS_EVT_DiagSessCtrl);
  advance_time_and_tick_thread(instance);

  uint8_t response_data[] = {
    0x06,                             // PCI   (single frame, 6 bytes of data)
    0x50,                             // DSCPR (positive response to 0x10)
    0x02,                             // DS    (programming session)
    UDS_CLIENT_DEFAULT_P2_MS >> 8,    // SPREC (p2 upper byte)
    UDS_CLIENT_DEFAULT_P2_MS & 0xFF,  // SPREC (p2 lower byte)
    (uint8_t)((UDS_CLIENT_DEFAULT_P2_STAR_MS / 10) >>
              8),                                   // SPREC (p2* upper byte)
    (uint8_t)(UDS_CLIENT_DEFAULT_P2_STAR_MS / 10),  // SPREC (p2* lower byte)
  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}

UDSErr_t test_0x10_diag_session_ctrl_not_supported_uds_callback(
    struct iso14229_zephyr_instance *inst,
    UDSEvent_t event,
    void *arg,
    void *user_context) {
  if (event == UDS_EVT_DiagSessCtrl) {
    return UDS_NRC_SubFunctionNotSupported;
  }

  return UDS_OK;
}

ZTEST_F(lib_uds_minimal, test_0x10_diag_session_ctrl_not_supported) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake =
      test_0x10_diag_session_ctrl_not_supported_uds_callback;

  uint8_t request_data[] = {
    0x02,  // PCI (single frame, 2 bytes of data)
    0x10,  // SID (DiagnosticSessionControl)
    0x02,  // DS  (Programming Session)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);
  advance_time_and_tick_thread(instance);

  uint8_t response_data[] = {
    0x03,  // PCI   (single frame, 3 bytes)
    0x7F,  // SID   (Failure Response SID)
    0x10,  // SIDRQ (SID of failing service)
    0x12,  // NRC   response code  (SubFunctionNotSupported)
  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}

ZTEST_F(lib_uds_minimal, test_0x11_ecu_reset) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  uint8_t request_data[] = {
    0x02,  // PCI    (single frame, 2 bytes of data)
    0x11,  // ER     (ECU Reset)
    0x01,  // LEV_RT (Hard Reset)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);

  zassert_equal(test_uds_callback_fake.call_count, 1);
  zassert_equal(test_uds_callback_fake.arg1_val, UDS_EVT_EcuReset);

  advance_time_and_tick_thread(instance);

  uint8_t response_data[] = {
    0x02,  // PCI    (single frame, 2 bytes of data)
    0x51,  // ERPR   (positive response to 0x11)
    0x01,  // LEV_RT (hard reset)
  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}

ZTEST_F(lib_uds_minimal, test_0x11_ecu_reset_response_pending) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  uint8_t request_data[] = {
    0x02,  // PCI    (single frame, 2 bytes of data)
    0x11,  // ER     (ECU Reset)
    0x01,  // LEV_RT (Hard Reset)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);
  zassert_equal(test_uds_callback_fake.call_count, 1);

  advance_time_and_tick_thread(instance);
  zassert_equal(fake_can_send_fake.call_count, 1);

  // There should be no response to this request
  uint8_t denied_request_data[] = {
    0x02,  // PCI (single frame, 2 bytes of data)
    0x10,  // SID (DiagnosticSessionControl)
    0x02,  // DS  (Programming Session)
  };
  receive_phys_can_frame_array(fixture, denied_request_data);
  advance_time_and_tick_thread(instance);
  advance_time_and_tick_thread(instance);

  // Session Control request should have been ignored
  zassert_equal(fake_can_send_fake.call_count, 1);
}

UDSErr_t test_0x11_ecu_reset_rapid_power_shutdown_uds_callback(
    struct iso14229_zephyr_instance *inst,
    UDSEvent_t event,
    void *arg,
    void *user_context) {
  if (event == UDS_EVT_EcuReset) {
    UDSECUResetArgs_t *args = arg;
    args->powerDownTimeMillis = 2000;
  }

  return UDS_OK;
}

ZTEST_F(lib_uds_minimal, test_0x11_ecu_reset_rapid_power_shutdown) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake =
      test_0x11_ecu_reset_rapid_power_shutdown_uds_callback;

  uint8_t request_data[] = {
    0x02,  // PCI    (single frame, 2 bytes of data)
    0x11,  // ER     (ECU Reset)
    0x04,  // LEV_RT (Rapid Power Shutdown)
  };

  receive_phys_can_frame_array(fixture, request_data);
  advance_time_and_tick_thread(instance);

  zassert_equal(test_uds_callback_fake.call_count, 1);
  zassert_equal(test_uds_callback_fake.arg1_val, UDS_EVT_EcuReset);
  advance_time_and_tick_thread(instance);

  uint8_t response_data[] = {
    0x03,  // PCI    (single frame, 4 bytes of data)
    0x51,  // ERPR   (positive response to 0x11)
    0x04,  // LEV_RT (rapid power shutdown)
    0x02,  // PDT    (2s power down timer)
  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}

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

ZTEST_F(lib_uds_minimal, test_0x22_read_data_by_identifier) {
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

UDSErr_t test_0x23_read_memory_uds_callback(
    struct iso14229_zephyr_instance *inst,
    UDSEvent_t event,
    void *arg,
    void *user_context) {
  UDSReadMemByAddrArgs_t *args = arg;

  return args->copy(&inst->server,
                    &uds_mem_data[(uint32_t)(uintptr_t)args->memAddr],
                    args->memSize);
}

ZTEST_F(lib_uds_minimal, test_0x23_read_memory) {
  struct iso14229_zephyr_instance *instance = &fixture->instance;

  test_uds_callback_fake.custom_fake = test_0x23_read_memory_uds_callback;

  uint8_t request_data[] = {
    0x07,  // PCI    (single frame, 7 bytes of data)
    0x23,  // SID    (ReadMemoryByAddress)
    0x14,  // ALFID  (size length = 1 byte, address length = 4 bytes)
    0x00,  // Address byte 1
    0x00,  // Address byte 2
    0x00,  // Address byte 3
    0x01,  // Address byte 4 (address = 0x00000001)
    0x04,  // Size   (4 bytes)
  };

  receive_phys_can_frame_array(fixture, request_data);
  tick_thread(instance);

  // Verify the unified callback was called correctly for ReadMemByAddr event
  zassert_equal(test_uds_callback_fake.call_count, 1);
  // arg1_val is the UDSEvent_t event parameter
  zassert_equal(test_uds_callback_fake.arg1_val, UDS_EVT_ReadMemByAddr);
  UDSReadMemByAddrArgs_t *args =
      (UDSReadMemByAddrArgs_t *)test_uds_callback_fake.arg2_val;

  zassert_equal_ptr(args->memAddr, (void *)0x00000001);
  zassert_equal(args->memSize, 4);
  advance_time_and_tick_thread(instance);

  uint8_t response_data[] = {
    0x05,  // PCI    (single frame, 5 bytes of data)
    0x63,  // RMBAPR (positive response to 0x23)
    0x02,  // DREC   (data read from memory)
    0x03,  // DREC   (data read from memory)
    0x04,  // DREC   (data read from memory)
    0x05,  // DREC   (data read from memory)
  };
  assert_send_phy_can_frame_array(fixture, 0, response_data);
  zassert_equal(fake_can_send_fake.call_count, 1);
}
