/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_new, CONFIG_UDS_NEW_LOG_LEVEL);

#include "write_memory_by_addr_impl.h"

UDSErr_t customuds_decode_write_memory_by_addr(
    UDSCustomArgs_t* data, struct CUSTOMUDS_WriteMemoryByAddr* args) {
  if (data->len < 1) {
    return UDS_NRC_IncorrectMessageLengthOrInvalidFormat;
  }
  uint8_t memory_addr_len = data->optionRecord[0] & 0x0f;
  uint8_t memory_size_len = (data->optionRecord[0] >> 4) & 0x0f;

  if (memory_size_len > 4 || memory_addr_len > 4) {
    return UDS_NRC_RequestOutOfRange;
  }

  if (data->len < memory_addr_len + memory_size_len + 1) {
    return UDS_NRC_IncorrectMessageLengthOrInvalidFormat;
  }

  uint32_t addr = 0;
  for (int i = 0; i < memory_addr_len; i++) {
    addr <<= 8;
    addr |= data->optionRecord[i + 1];
  }

  uint32_t size = 0;
  for (int i = 0; i < memory_size_len; i++) {
    size <<= 8;
    size |= data->optionRecord[i + 1 + memory_addr_len];
  }

  if (data->len != memory_addr_len + memory_size_len + 1 + size) {
    return UDS_NRC_IncorrectMessageLengthOrInvalidFormat;
  }

  args->addr = addr;
  args->len = size;
  args->data = &data->optionRecord[memory_addr_len + memory_size_len + 1];

  args->_answer_context.memory_addr_len = memory_addr_len;
  args->_answer_context.memory_size_len = memory_size_len;

  return UDS_OK;
}

UDSErr_t customuds_answer(UDSServer_t* srv,
                          UDSCustomArgs_t* data,
                          const struct CUSTOMUDS_WriteMemoryByAddr* args) {
  // copy addressAndLengthFormatIdentifier, memoryAddress and memorySize
  uint8_t to_copy_bytes = 1 + args->_answer_context.memory_addr_len +
                          args->_answer_context.memory_size_len;

  // copy these from original message
  data->copyResponse(srv, data->optionRecord, to_copy_bytes);

  return UDS_PositiveResponse;
}
