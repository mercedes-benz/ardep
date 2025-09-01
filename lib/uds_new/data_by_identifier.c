/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "iso14229.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_new, CONFIG_UDS_NEW_LOG_LEVEL);

#include "ardep/iso14229.h"
#include "data_by_identifier.h"
#include "zephyr/sys/byteorder.h"

#include <zephyr/kernel.h>

#define DATA_LEN_IN_BYTES(reg) \
  (reg->data_identifier.num_of_elem * reg->data_identifier.len_elem)

UDSErr_t _uds_new_data_identifier_static_read(
    void* data, size_t* len, struct uds_new_registration_t* reg) {
  if (*len < DATA_LEN_IN_BYTES(reg)) {
    LOG_WRN("Buffer too small to read Data Identifier 0x%04X",
            reg->data_identifier.data_id);
    return UDS_NRC_ConditionsNotCorrect;
  }

  *len = DATA_LEN_IN_BYTES(reg);
  memcpy(data, reg->user_data, *len);

  if (reg->data_identifier.len_elem == 1) {
    return UDS_OK;
  }

  // Data is send MSB first, so we convert every element of the array to BE
  for (uint32_t i = 0; i < *len; i += reg->data_identifier.len_elem) {
    sys_cpu_to_be(((uint8_t*)data) + i, reg->data_identifier.len_elem);
  }

  return UDS_OK;
}

UDSErr_t _uds_new_data_identifier_static_write(
    const void* data, size_t len, struct uds_new_registration_t* reg) {
  if (len != DATA_LEN_IN_BYTES(reg)) {
    LOG_WRN("Wrong length to write Data to Identifier 0x%04X",
            reg->data_identifier.data_id);
    return UDS_NRC_ConditionsNotCorrect;
  }

  // Use separate buffer to convert to system byte order to minimize access time
  // to user_data
  uint8_t write_buf[DATA_LEN_IN_BYTES(reg)];
  memcpy(write_buf, data, DATA_LEN_IN_BYTES(reg));

  // Data is send MSB first, so we convert every element of the array back to
  // system byteorder
  if (reg->data_identifier.len_elem > 1) {
    for (uint32_t i = 0; i < DATA_LEN_IN_BYTES(reg);
         i += reg->data_identifier.len_elem) {
      sys_be_to_cpu(write_buf + i, reg->data_identifier.len_elem);
    }
  }

  memmove(reg->user_data, write_buf, DATA_LEN_IN_BYTES(reg));
  return UDS_OK;
}

static UDSErr_t uds_new_try_read_from_identifier(
    struct uds_new_instance_t* instance,
    UDSRDBIArgs_t* args,
    struct uds_new_registration_t* reg) {
  if (reg->instance != instance) {
    return UDS_FAIL;
  }

  if (reg->type != UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER) {
    return UDS_FAIL;
  }

  if (reg->data_identifier.data_id != args->dataId) {
    return UDS_FAIL;
  }

  uint8_t read_buf[DATA_LEN_IN_BYTES(reg)];
  size_t len = sizeof(read_buf);
  reg->data_identifier.read(read_buf, &len, reg);
  return args->copy(&instance->iso14229.server, read_buf, len);
}

UDSErr_t uds_new_handle_read_data_by_identifier(
    struct uds_new_instance_t* instance, UDSRDBIArgs_t* args) {
  STRUCT_SECTION_FOREACH (uds_new_registration_t, reg) {
    int ret = uds_new_try_read_from_identifier(instance, args, reg);
    if (ret != UDS_FAIL) {
      return ret;
    }
  }

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
  struct uds_new_registration_t* current = instance->dynamic_registrations;
  while (current != NULL) {
    int ret = uds_new_try_read_from_identifier(instance, args, current);
    if (ret != UDS_FAIL) {
      current = current->next;
      return ret;
    }
  }
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
  return UDS_NRC_RequestOutOfRange;
}

static UDSErr_t uds_new_try_write_to_identifier(
    struct uds_new_instance_t* instance,
    UDSWDBIArgs_t* args,
    struct uds_new_registration_t* reg) {
  if (reg->instance != instance) {
    return UDS_FAIL;
  }

  if (reg->type != UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER) {
    return UDS_FAIL;
  }

  if (reg->data_identifier.data_id != args->dataId) {
    return UDS_FAIL;
  }

  if (!reg->can_write) {
    LOG_WRN("Data Identifier 0x%04X cannot be written", args->dataId);
    return UDS_NRC_RequestOutOfRange;
  }

  return reg->data_identifier.write(args->data, args->len, reg);
}

UDSErr_t uds_new_handle_write_data_by_identifier(
    struct uds_new_instance_t* instance, UDSWDBIArgs_t* args) {
  STRUCT_SECTION_FOREACH (uds_new_registration_t, reg) {
    int ret = uds_new_try_write_to_identifier(instance, args, reg);
    if (ret != UDS_FAIL) {
      return ret;
    }
  }

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
  struct uds_new_registration_t* current = instance->dynamic_registrations;
  while (current != NULL) {
    int ret = uds_new_try_write_to_identifier(instance, args, current);
    if (ret != UDS_FAIL) {
      current = current->next;
      return ret;
    }
  }
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID

  return UDS_NRC_RequestOutOfRange;
}

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
int uds_new_register_runtime_data_identifier(struct uds_new_instance_t* inst,
                                             uint16_t data_id,
                                             void* addr,
                                             size_t num_of_elem,
                                             size_t len_elem,
                                             bool can_write) {
  struct uds_new_registration_t* reg =
      k_malloc(sizeof(struct uds_new_registration_t));

  reg->instance = inst;
  reg->type = UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER;
  reg->user_data = addr;
  reg->next = NULL;
  reg->data_identifier.data_id = data_id;
  reg->data_identifier.num_of_elem = num_of_elem;
  reg->data_identifier.len_elem = len_elem;
  reg->data_identifier.read = _uds_new_data_identifier_static_read;
  reg->data_identifier.write = _uds_new_data_identifier_static_write;
  reg->can_write = can_write;

  // Append reg to the singly linked list at inst->dynamic_registrations
  if (inst->dynamic_registrations == NULL) {
    inst->dynamic_registrations = reg;
  } else {
    struct uds_new_registration_t* current = inst->dynamic_registrations;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = reg;
  }

  return 0;
}

#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID