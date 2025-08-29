/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ardep/uds_new.h>

ZTEST_F(lib_uds_new, test_0x22_read_by_id_static_single_element) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSRDBIArgs_t args = {
    .dataId = by_id_data1_id,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &args);
  zassert_ok(ret);

  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg0_val, &instance->iso14229.server);
  zassert_equal(copy_fake.arg2_val, sizeof(by_id_data1));

  uint8_t expected[2] = {
    0x00,
    0x05,
  };
  assert_copy_data(expected, sizeof(expected));
}

ZTEST_F(lib_uds_new, test_0x22_read_by_id_static_array) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSRDBIArgs_t args = {
    .dataId = by_id_data2_id,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &args);
  zassert_ok(ret);

  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg0_val, &instance->iso14229.server);
  zassert_equal(copy_fake.arg2_val, sizeof(by_id_data2));

  uint8_t expected[6] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
  assert_copy_data(expected, sizeof(expected));
}

ZTEST_F(lib_uds_new, test_0x22_read_by_id_dynamic_array) {
  struct uds_new_instance_t *instance = fixture->instance;

  uint16_t id = 0x9988;
  uint32_t data[4] = {0x11223344, 0x55667788, 0x99AABBCC, 0xDDEEFF00};

  instance->register_data_by_identifier(instance, id, data, ARRAY_SIZE(data),
                                        sizeof(data[0]));

  UDSRDBIArgs_t args = {
    .dataId = id,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &args);
  zassert_ok(ret);

  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg0_val, &instance->iso14229.server);
  zassert_equal(copy_fake.arg2_val, sizeof(data));

  uint8_t expected[16] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                          0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00};

  assert_copy_data(expected, sizeof(expected));
}

ZTEST_F(lib_uds_new, test_0x23_read_memory_by_address_null_address) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSReadMemByAddrArgs_t args = {
    .memAddr = NULL,
    .memSize = 16,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadMemByAddr, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds_new, test_0x23_read_memory_by_address_zero_size) {
  struct uds_new_instance_t *instance = fixture->instance;

  UDSReadMemByAddrArgs_t args = {
    .memAddr = (void*)0x10000,
    .memSize = 0,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadMemByAddr, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

#if CONFIG_BOARD_NATIVE_SIM
ZTEST_F(lib_uds_new, test_0x23_read_memory_by_address_valid_memory) {
  struct uds_new_instance_t *instance = fixture->instance;

  uint8_t local_buffer[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                              0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
  
  UDSReadMemByAddrArgs_t args = {
    .memAddr = local_buffer,
    .memSize = 16,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadMemByAddr, &args);
  zassert_equal(ret, UDS_PositiveResponse);

  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg0_val, &instance->iso14229.server);
  zassert_equal(copy_fake.arg2_val, sizeof(local_buffer));

  assert_copy_data(local_buffer, sizeof(local_buffer));
}
#endif


// We use a nucleo board with known memory layout
#if CONFIG_BOARD_NUCLEO_G474RE

// STM32G474RE memory layout (correct addresses for this MCU)
const uintptr_t known_ram_start = 0x20000000;  // SRAM start
const uintptr_t known_ram_end = 0x20020000;    // 128KB SRAM
const uintptr_t known_flash_start = 0x08000000; // Flash start
const uintptr_t known_flash_end = 0x08080000;   // 512KB Flash

// Test cases for real hardware with known memory layout
ZTEST_F(lib_uds_new, test_0x23_read_memory_by_address_nucleo_ram_valid) {
  struct uds_new_instance_t *instance = fixture->instance;

  // Test reading from valid RAM location
  UDSReadMemByAddrArgs_t args = {
    .memAddr = (void*)known_ram_start,
    .memSize = 4,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadMemByAddr, &args);
  zassert_equal(ret, UDS_PositiveResponse);
  
  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg0_val, &instance->iso14229.server);
  zassert_equal(copy_fake.arg2_val, 4);
}

ZTEST_F(lib_uds_new, test_0x23_read_memory_by_address_nucleo_ram_boundary_end) {
  struct uds_new_instance_t *instance = fixture->instance;

  // Test reading from RAM end boundary (should be valid up to end-1)
  UDSReadMemByAddrArgs_t args = {
    .memAddr = (void*)(known_ram_end - 1),
    .memSize = 1,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadMemByAddr, &args);
  zassert_equal(ret, UDS_PositiveResponse);
  
  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg2_val, 1);
}

ZTEST_F(lib_uds_new, test_0x23_read_memory_by_address_nucleo_ram_out_of_bounds) {
  struct uds_new_instance_t *instance = fixture->instance;

  // Test reading after RAM end (should fail)
  UDSReadMemByAddrArgs_t args = {
    .memAddr = (void*)known_ram_end,
    .memSize = 1,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadMemByAddr, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds_new, test_0x23_read_memory_by_address_nucleo_flash_valid) {
  struct uds_new_instance_t *instance = fixture->instance;

  // Test reading from valid Flash location
  UDSReadMemByAddrArgs_t args = {
    .memAddr = (void*)known_flash_start,
    .memSize = 4,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadMemByAddr, &args);
  zassert_equal(ret, UDS_PositiveResponse);
  
  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg0_val, &instance->iso14229.server);
  zassert_equal(copy_fake.arg2_val, 4);
}

ZTEST_F(lib_uds_new, test_0x23_read_memory_by_address_nucleo_flash_boundary_end) {
  struct uds_new_instance_t *instance = fixture->instance;

  // Test reading from Flash end boundary (should be valid up to end-1)
  UDSReadMemByAddrArgs_t args = {
    .memAddr = (void*)(known_flash_end - 1),
    .memSize = 1,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadMemByAddr, &args);
  zassert_equal(ret, UDS_PositiveResponse);
  
  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg2_val, 1);
}

ZTEST_F(lib_uds_new, test_0x23_read_memory_by_address_nucleo_flash_out_of_bounds) {
  struct uds_new_instance_t *instance = fixture->instance;

  // Test reading after Flash end (should fail)
  UDSReadMemByAddrArgs_t args = {
    .memAddr = (void*)known_flash_end,
    .memSize = 1,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadMemByAddr, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds_new, test_0x23_read_memory_by_address_nucleo_invalid_peripheral) {
  struct uds_new_instance_t *instance = fixture->instance;

  // Test reading from peripheral region (should fail as not RAM/Flash)
  UDSReadMemByAddrArgs_t args = {
    .memAddr = (void*)0x40000000, // GPIO peripheral base
    .memSize = 4,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadMemByAddr, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds_new, test_0x23_read_memory_by_address_nucleo_invalid_low_memory) {
  struct uds_new_instance_t *instance = fixture->instance;

  // Test reading from invalid low memory region
  UDSReadMemByAddrArgs_t args = {
    .memAddr = (void*)0x00001000, // Below Flash region
    .memSize = 4,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadMemByAddr, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds_new, test_0x23_read_memory_by_address_nucleo_ram_overflow) {
  struct uds_new_instance_t *instance = fixture->instance;

  // Test reading that would overflow past RAM end
  UDSReadMemByAddrArgs_t args = {
    .memAddr = (void*)(known_ram_end - 2),
    .memSize = 4, // Would read 2 bytes past end
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadMemByAddr, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds_new, test_0x23_read_memory_by_address_nucleo_flash_overflow) {
  struct uds_new_instance_t *instance = fixture->instance;

  // Test reading that would overflow past Flash end
  UDSReadMemByAddrArgs_t args = {
    .memAddr = (void*)(known_flash_end - 2),
    .memSize = 4, // Would read 2 bytes past end
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadMemByAddr, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds_new, test_0x23_read_memory_by_address_nucleo_large_read_ram) {
  struct uds_new_instance_t *instance = fixture->instance;

  // Test reading large amount from RAM (1KB)
  UDSReadMemByAddrArgs_t args = {
    .memAddr = (void*)known_ram_start,
    .memSize = 1024,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadMemByAddr, &args);
  zassert_equal(ret, UDS_PositiveResponse);
  
  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg2_val, 1024);
}

ZTEST_F(lib_uds_new, test_0x23_read_memory_by_address_nucleo_large_read_flash) {
  struct uds_new_instance_t *instance = fixture->instance;

  // Test reading large amount from Flash (1KB)
  UDSReadMemByAddrArgs_t args = {
    .memAddr = (void*)known_flash_start,
    .memSize = 1024,
    .copy = copy,
  };

  int ret = receive_event(instance, UDS_EVT_ReadMemByAddr, &args);
  zassert_equal(ret, UDS_PositiveResponse);
  
  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg2_val, 1024);
}

#endif
