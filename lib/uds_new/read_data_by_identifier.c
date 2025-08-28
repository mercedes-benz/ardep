#include "ardep/iso14229.h"
#include "read_data_by_identifier.h"
#include "zephyr/sys/byteorder.h"

#include <zephyr/kernel.h>

UDSErr_t _uds_new_data_identifier_static_read(
    void* data, size_t* len, struct uds_new_registration_t* reg) {
  if (*len < reg->data_identifier.len) {
    return UDS_NRC_IncorrectMessageLengthOrInvalidFormat;  // todo: better error
  }

  *len = reg->data_identifier.len * reg->data_identifier.len_elem;
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
  if (len < reg->data_identifier.len) {
    return UDS_NRC_IncorrectMessageLengthOrInvalidFormat;  // todo: better error
  }

  memcpy(reg->user_data, data, len);
  return UDS_OK;
}

UDSErr_t handle_data_read_by_identifier(struct uds_new_instance_t* instance,
                                        UDSRDBIArgs_t* args) {
  STRUCT_SECTION_FOREACH (uds_new_registration_t, reg) {
    if (reg->instance != instance) {
      continue;
    }

    if (reg->type != UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER) {
      continue;
    }

    if (reg->data_identifier.data_id != args->dataId) {
      continue;
    }

    uint8_t read_buf[32];
    size_t len = sizeof(read_buf);
    reg->data_identifier.read(read_buf, &len, reg);
    return args->copy(&instance->iso14229.server, read_buf, len);
  }

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
  struct uds_new_registration_t* current = instance->dynamic_registrations;

  while (current != NULL) {
    if (current->instance != instance) {
      current = current->next;
      continue;
    }

    if (current->type != UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER) {
      current = current->next;
      continue;
    }
    if (current->data_identifier.data_id != args->dataId) {
      current = current->next;
      continue;
    }

    uint8_t read_buf[32];
    size_t len = sizeof(read_buf);
    current->data_identifier.read(read_buf, &len, current);
    return args->copy(&instance->iso14229.server, read_buf, len);
  }
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID

  return UDS_NRC_RequestOutOfRange;
}

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
int uds_new_register_runtime_data_identifier(struct uds_new_instance_t* inst,
                                             uint16_t data_id,
                                             void* addr,
                                             size_t len,
                                             size_t len_elem) {
  struct uds_new_registration_t* reg =
      k_malloc(sizeof(struct uds_new_registration_t));

  reg->instance = inst;
  reg->type = UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER;
  reg->user_data = addr;
  reg->next = NULL;
  reg->data_identifier.data_id = data_id;
  reg->data_identifier.len = len;
  reg->data_identifier.len_elem = len_elem;
  reg->data_identifier.read = _uds_new_data_identifier_static_read;
  reg->data_identifier.write = _uds_new_data_identifier_static_write;

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