/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixture.h"

#include <zephyr/ztest.h>

ZTEST_F(lib_uds, test_0x3D_write_memory_by_address_null_address) {
  struct uds_instance_t *instance = fixture->instance;

  const uint8_t data[4] = {0xAA, 0xBB, 0xCC, 0xDD};

  UDSWriteMemByAddrArgs_t args = {
    .memAddr = NULL,
    .memSize = sizeof(data),
    .data = data,
  };

  int ret = receive_event(instance, UDS_EVT_WriteMemByAddr, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds, test_0x3D_write_memory_by_address_zero_size) {
  struct uds_instance_t *instance = fixture->instance;

  uint8_t dest[8] = {0};
  const uint8_t data[1] = {0x11};

  UDSWriteMemByAddrArgs_t args = {
    .memAddr = dest,
    .memSize = 0,
    .data = data,
  };

  int ret = receive_event(instance, UDS_EVT_WriteMemByAddr, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

#if CONFIG_BOARD_NATIVE_SIM

ZTEST_F(lib_uds, test_0x3D_write_memory_by_address_valid_memory) {
  struct uds_instance_t *instance = fixture->instance;

  uint8_t dest[16] = {0};
  const uint8_t src[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                           0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};

  UDSWriteMemByAddrArgs_t args = {
    .memAddr = dest,
    .memSize = sizeof(src),
    .data = src,
  };

  int ret = receive_event(instance, UDS_EVT_WriteMemByAddr, &args);
  zassert_equal(ret, UDS_PositiveResponse);

  zassert_mem_equal(dest, src, sizeof(src));
}

ZTEST_F(lib_uds, test_0x3D_write_memory_by_address_large_buffer) {
  struct uds_instance_t *instance = fixture->instance;

  uint8_t dest[2096] = {0};
  uint8_t src[2096];

  // Fill source buffer with pattern
  for (size_t i = 0; i < sizeof(src); ++i) {
    src[i] = (uint8_t)(i % 256);
  }

  UDSWriteMemByAddrArgs_t args = {
    .memAddr = dest,
    .memSize = sizeof(src),
    .data = src,
  };

  int ret = receive_event(instance, UDS_EVT_WriteMemByAddr, &args);
  zassert_equal(ret, UDS_PositiveResponse);

  zassert_mem_equal(dest, src, sizeof(src));
}

#endif

// We use a nucleo board with known memory layout
#if CONFIG_BOARD_NUCLEO_G474RE

// STM32G474RE memory layout (correct addresses for this MCU)
const static uintptr_t known_ram_start = 0x20000000;    // SRAM start
const static uintptr_t known_ram_end = 0x20020000;      // 128KB SRAM
const static uintptr_t known_flash_start = 0x08000000;  // Flash start
const static uintptr_t known_flash_end = 0x08080000;    // 512KB Flash

ZTEST_F(lib_uds, test_0x3D_write_memory_by_address_nucleo_ram_valid) {
  struct uds_instance_t *instance = fixture->instance;

  // Use a local buffer within RAM for a safe write
  uint8_t dest[8] = {0};
  const uint8_t src[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED, 0xBA, 0xBE};

  UDSWriteMemByAddrArgs_t args = {
    .memAddr = dest,
    .memSize = sizeof(src),
    .data = src,
  };

  int ret = receive_event(instance, UDS_EVT_WriteMemByAddr, &args);
  zassert_equal(ret, UDS_PositiveResponse);
  zassert_mem_equal(dest, src, sizeof(src));
}

ZTEST_F(lib_uds, test_0x3D_write_memory_by_address_nucleo_ram_out_of_bounds) {
  struct uds_instance_t *instance = fixture->instance;

  // Writing after RAM end (should fail before attempting to write)
  const uint8_t src[1] = {0x11};
  UDSWriteMemByAddrArgs_t args = {
    .memAddr = (void *)known_ram_end,
    .memSize = 1,
    .data = src,
  };

  int ret = receive_event(instance, UDS_EVT_WriteMemByAddr, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds, test_0x3D_write_memory_by_address_nucleo_flash_not_writeable) {
  struct uds_instance_t *instance = fixture->instance;

  // Any Flash write should be rejected in implementation
  const uint8_t src4[4] = {1, 2, 3, 4};
  UDSWriteMemByAddrArgs_t args1 = {
    .memAddr = (void *)known_flash_start,
    .memSize = 4,
    .data = src4,
  };

  int ret1 = receive_event(instance, UDS_EVT_WriteMemByAddr, &args1);
  zassert_equal(ret1, UDS_NRC_RequestOutOfRange);

  UDSWriteMemByAddrArgs_t args2 = {
    .memAddr = (void *)(known_flash_end - 1),
    .memSize = 1,
    .data = src4,
  };

  int ret2 = receive_event(instance, UDS_EVT_WriteMemByAddr, &args2);
  zassert_equal(ret2, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds, test_0x3D_write_memory_by_address_nucleo_flash_out_of_bounds) {
  struct uds_instance_t *instance = fixture->instance;

  const uint8_t src[1] = {0x22};
  UDSWriteMemByAddrArgs_t args = {
    .memAddr = (void *)known_flash_end,
    .memSize = 1,
    .data = src,
  };

  int ret = receive_event(instance, UDS_EVT_WriteMemByAddr, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds, test_0x3D_write_memory_by_address_nucleo_invalid_peripheral) {
  struct uds_instance_t *instance = fixture->instance;

  const uint8_t src[4] = {0, 0, 0, 0};
  UDSWriteMemByAddrArgs_t args = {
    .memAddr = (void *)0x40000000,  // GPIO peripheral base
    .memSize = 4,
    .data = src,
  };

  int ret = receive_event(instance, UDS_EVT_WriteMemByAddr, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds, test_0x3D_write_memory_by_address_nucleo_invalid_low_memory) {
  struct uds_instance_t *instance = fixture->instance;

  const uint8_t src[4] = {0};
  UDSWriteMemByAddrArgs_t args = {
    .memAddr = (void *)0x00001000,  // Below Flash region
    .memSize = 4,
    .data = src,
  };

  int ret = receive_event(instance, UDS_EVT_WriteMemByAddr, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds, test_0x3D_write_memory_by_address_nucleo_large_write_ram) {
  struct uds_instance_t *instance = fixture->instance;

  // Large write to a local RAM buffer
  uint8_t dest[1024] = {0};
  uint8_t src[1024];
  for (size_t i = 0; i < sizeof(src); ++i) src[i] = (uint8_t)i;

  UDSWriteMemByAddrArgs_t args = {
    .memAddr = dest,
    .memSize = sizeof(src),
    .data = src,
  };

  int ret = receive_event(instance, UDS_EVT_WriteMemByAddr, &args);
  zassert_equal(ret, UDS_PositiveResponse);
  zassert_mem_equal(dest, src, sizeof(src));
}

ZTEST_F(lib_uds, test_0x3D_write_memory_by_address_nucleo_before_ram) {
  struct uds_instance_t *instance = fixture->instance;

  const uint8_t src[4] = {0};
  UDSWriteMemByAddrArgs_t args = {
    .memAddr = (void *)(known_ram_start - 1),
    .memSize = 4,  // overlapping into RAM
    .data = src,
  };

  int ret = receive_event(instance, UDS_EVT_WriteMemByAddr, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds, test_0x3D_write_memory_by_address_nucleo_before_flash) {
  struct uds_instance_t *instance = fixture->instance;

  const uint8_t src[4] = {0};
  UDSWriteMemByAddrArgs_t args = {
    .memAddr = (void *)(known_flash_start - 1),
    .memSize = 4,  // overlapping into Flash
    .data = src,
  };

  int ret = receive_event(instance, UDS_EVT_WriteMemByAddr, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

#endif
