/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "iso14229.h"
#include "uds.h"

#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <ardep/uds.h>

LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

/**
 * Check if the requested memory address range is within valid memory bounds
 * @param addr Starting address to check
 * @param size Size of memory region to check
 * @param readonly Set to `true` if the specified region should be read
 *                 (not written)
 * @return true if address range is valid, false otherwise
 */
static bool is_memory_address_valid(uintptr_t addr,
                                    size_t size,
                                    bool readonly) {
  // Prevent overflow
  if (addr > UINTPTR_MAX - size) {
    LOG_ERR("Memory address overflow detected");
    return false;
  }

#if CONFIG_BOARD_NATIVE_SIM
  return true;  // In simulation, allow all addresses
#else
// Check SRAM address range
#if DT_HAS_CHOSEN(zephyr_sram) && DT_NODE_HAS_PROP(DT_CHOSEN(zephyr_sram), reg)
#define RAM_NODE DT_CHOSEN(zephyr_sram)
  const static uintptr_t ram_base = DT_REG_ADDR(RAM_NODE);
  const static uintptr_t ram_end = ram_base + DT_REG_SIZE(RAM_NODE);

  if (addr >= ram_base && (addr + size) <= ram_end) {
    return true;
  }
#endif

// Flash memory range check
#if DT_HAS_CHOSEN(zephyr_flash) && \
    DT_NODE_HAS_PROP(DT_CHOSEN(zephyr_flash), reg)
#define FLASH_NODE DT_CHOSEN(zephyr_flash)
  if (readonly) {
    const static uintptr_t flash_base = DT_REG_ADDR(FLASH_NODE);
    const static uintptr_t flash_end = flash_base + DT_REG_SIZE(FLASH_NODE);

    if (addr >= flash_base && (addr + size) <= flash_end) {
      return true;
    }
  }
#endif

  LOG_ERR("Memory address 0x%08lX not in valid range", (unsigned long)addr);
  return false;
#endif
}

uds_check_fn uds_get_check_for_read_memory_by_addr(
    const struct uds_registration_t* const reg) {
  return reg->memory.read.check;
}
uds_action_fn uds_get_action_for_read_memory_by_addr(
    const struct uds_registration_t* const reg) {
  return reg->memory.read.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_read_mem_by_addr_) = {
  .event = UDS_EVT_ReadMemByAddr,
  .get_check = uds_get_check_for_read_memory_by_addr,
  .get_action = uds_get_action_for_read_memory_by_addr,
  .default_nrc = UDS_NRC_ConditionsNotCorrect,
  .registration_type = UDS_REGISTRATION_TYPE__MEMORY,
};

uds_check_fn uds_get_check_for_write_memory_by_addr(
    const struct uds_registration_t* const reg) {
  return reg->memory.write.check;
}
uds_action_fn uds_get_action_for_write_memory_by_addr(
    const struct uds_registration_t* const reg) {
  return reg->memory.write.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_write_mem_by_addr_) = {
  .event = UDS_EVT_WriteMemByAddr,
  .get_check = uds_get_check_for_write_memory_by_addr,
  .get_action = uds_get_action_for_write_memory_by_addr,
  .default_nrc = UDS_NRC_ConditionsNotCorrect,
  .registration_type = UDS_REGISTRATION_TYPE__MEMORY,
};

UDSErr_t uds_check_default_memory_by_addr_read(
    const struct uds_context* const context, bool* apply_action) {
  UDSReadMemByAddrArgs_t* args = context->arg;
  if (args->memAddr == NULL) {
    LOG_ERR("Read Memory By Address: NULL memory address");
    return UDS_NRC_RequestOutOfRange;
  }

  if (args->memSize == 0) {
    LOG_ERR("Read Memory By Address: Zero memory size requested");
    return UDS_NRC_RequestOutOfRange;
  }

  uintptr_t mem_addr = (uintptr_t)args->memAddr;

  LOG_DBG("Read Memory By Address: addr=0x%08lX, size=%zu",
          (unsigned long)mem_addr, args->memSize);

  if (!is_memory_address_valid(mem_addr, args->memSize, true)) {
    LOG_WRN("Read Memory By Address: Invalid address range 0x%08lX-0x%08lX",
            (unsigned long)mem_addr,
            (unsigned long)(mem_addr + args->memSize - 1));
    return UDS_NRC_RequestOutOfRange;
  }

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t uds_action_default_memory_by_addr_read(
    struct uds_context* const context, bool* consume_event) {
  UDSReadMemByAddrArgs_t* args = context->arg;
  uintptr_t mem_addr = (uintptr_t)args->memAddr;

  uint8_t copy_result =
      args->copy(context->server, args->memAddr, args->memSize);
  if (copy_result != UDS_PositiveResponse) {
    LOG_ERR("Read Memory By Address: Copy failed with result %d", copy_result);
    return UDS_NRC_RequestOutOfRange;
  }

  LOG_DBG("Read Memory By Address: Successfully copied %zu bytes from 0x%08lX",
          args->memSize, (unsigned long)mem_addr);

  return UDS_PositiveResponse;
}

UDSErr_t uds_check_default_memory_by_addr_write(
    const struct uds_context* const context, bool* apply_action) {
  UDSWriteMemByAddrArgs_t* args = context->arg;
  if (args->memAddr == NULL) {
    LOG_ERR("Write Memory By Address: NULL memory address");
    return UDS_NRC_RequestOutOfRange;
  }

  if (args->memSize == 0) {
    LOG_ERR("Write Memory By Address: Zero memory size requested");
    return UDS_NRC_RequestOutOfRange;
  }

  uintptr_t mem_addr = (uintptr_t)args->memAddr;

  LOG_DBG("Write Memory By Address: addr=0x%08lX, size=%zu",
          (unsigned long)mem_addr, args->memSize);

  if (!is_memory_address_valid(mem_addr, args->memSize, false)) {
    LOG_WRN("Write Memory By Address: Invalid address range 0x%08lX-0x%08lX",
            (unsigned long)mem_addr,
            (unsigned long)(mem_addr + args->memSize - 1));
    return UDS_NRC_RequestOutOfRange;
  }

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t uds_action_default_memory_by_addr_write(
    struct uds_context* const context, bool* consume_event) {
  UDSWriteMemByAddrArgs_t* args = context->arg;

  uintptr_t mem_addr = (uintptr_t)args->memAddr;

  memmove((void*)mem_addr, args->data, args->memSize);

  LOG_DBG("Write Memory By Address: Successfully copied %zu bytes to 0x%08lX",
          args->memSize, (unsigned long)mem_addr);

  return UDS_PositiveResponse;
}
