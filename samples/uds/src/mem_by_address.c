/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds_macro.h"
#include "iso14229.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_sample, LOG_LEVEL_DBG);

#include "uds.h"

const uint32_t fake_memory_start_addr = 0x00001000;
uint8_t fake_memory[100] = {
  0x01, 0x02, 0x03, 0x04, 0x05, 0x05, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D,
  0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A,
  0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34,
  0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41,
  0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E,
  0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B,
  0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64};

UDSErr_t read_mem_by_address_check(const struct uds_context *const context,
                                   bool *apply_action) {
  UDSReadMemByAddrArgs_t *args = context->arg;
  uint32_t addr = (uint32_t)(uintptr_t)args->memAddr;
  uint32_t size = args->memSize;

  // Check if the address is within our fake memory range
  if (addr < fake_memory_start_addr ||
      (addr + size) >= (fake_memory_start_addr + sizeof(fake_memory))) {
    return UDS_OK;
  }

  *apply_action = true;
  LOG_INF("Check to read memory at address 0x%08X with size %d successful",
          addr, size);

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t read_mem_by_address_action(struct uds_context *const context,
                                    bool *consume_event) {
  UDSReadMemByAddrArgs_t *args = context->arg;
  uint32_t addr = (uint32_t)(uintptr_t)args->memAddr;
  uint32_t size = args->memSize;

  LOG_INF("Reading memory at address 0x%08X with size %d", addr, size);

  return args->copy(&context->instance->iso14229.server,
                    &fake_memory[addr - fake_memory_start_addr], size);
}

UDSErr_t write_mem_by_address_check(const struct uds_context *const context,
                                    bool *apply_action) {
  UDSWriteMemByAddrArgs_t *args = context->arg;
  uint32_t addr = (uint32_t)(uintptr_t)args->memAddr;
  uint32_t size = args->memSize;

  // Check if the address is within our fake memory range
  if (addr < fake_memory_start_addr ||
      (addr + size) >= (fake_memory_start_addr + sizeof(fake_memory))) {
    return UDS_OK;
  }

  *apply_action = true;
  LOG_INF("Check to write memory at address 0x%08X with size %d successful",
          addr, size);

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t write_mem_by_address_action(struct uds_context *const context,
                                     bool *consume_event) {
  UDSWriteMemByAddrArgs_t *args = context->arg;
  uint32_t addr = (uint32_t)(uintptr_t)args->memAddr;
  uint32_t size = args->memSize;

  LOG_HEXDUMP_INF(args->data, size, "Writing to memory");

  memcpy(&fake_memory[addr - fake_memory_start_addr], args->data, size);

  return UDS_PositiveResponse;
}

UDS_REGISTER_MEMORY_HANDLER(&instance,
                            read_mem_by_address_check,
                            read_mem_by_address_action,
                            write_mem_by_address_check,
                            write_mem_by_address_action,
                            NULL)
