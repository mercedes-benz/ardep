/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_sample, LOG_LEVEL_DBG);

#include "uds.h"

#include <zephyr/sys/byteorder.h>

const uint16_t primitive_type_id = 0x50;
uint16_t primitive_type = 5;
uint16_t primitive_type_size = sizeof(primitive_type);

const uint16_t string_id = 0x100;
char string[] = "Hello from UDS";
// Include NULL as string terminator
uint16_t string_size = sizeof(string);

UDSErr_t read_data_by_id_check(const struct uds_context *const context,
                               bool *apply_action) {
  UDSRDBIArgs_t *args = context->arg;
  // Return Ok, when we don't handle this event but don't set
  // `apply_action` to true
  if (args->dataId != context->registration->data_identifier.data_id) {
    return UDS_OK;
  }
  LOG_INF("Check to read data id: 0x%02X successful",
          context->registration->data_identifier.data_id);
  // Set to true, when we want to handle this event
  *apply_action = true;
  return UDS_OK;
}

UDSErr_t read_data_by_id_action(struct uds_context *const context,
                                bool *consume_event) {
  LOG_INF("Reading data id: 0x%02X",
          context->registration->data_identifier.data_id);

  UDSRDBIArgs_t *args = context->arg;

  uint8_t temp[50] = {0};

  if (context->registration->data_identifier.data_id == primitive_type_id) {
    // Convert to MSB-First as defined in ISO 14229
    *(uint16_t *)temp =
        sys_cpu_to_be16(*(uint16_t *)context->registration->user_data);
  } else if (context->registration->data_identifier.data_id == string_id) {
    // Transport string as raw data without conversion
    memcpy(temp, context->registration->user_data,
           *(uint16_t *)context->registration->data_identifier.user_context);
  }

  // Signal this action consumes the event
  *consume_event = true;

  return args->copy(
      &context->instance->iso14229.server, temp,
      *(uint16_t *)context->registration->data_identifier.user_context);
}

UDSErr_t write_data_by_id_check(const struct uds_context *const context,
                                bool *apply_action) {
  UDSRDBIArgs_t *args = context->arg;
  // Return Ok, when we don't handle this event
  if (args->dataId != context->registration->data_identifier.data_id) {
    return UDS_OK;
  }
  LOG_INF("Check to write data id: 0x%02X successful",
          context->registration->data_identifier.data_id);
  // Set to true, when we want to handle this event
  *apply_action = true;
  return UDS_OK;
}

UDSErr_t write_data_by_id_action(struct uds_context *const context,
                                 bool *consume_event) {
  UDSWDBIArgs_t *args = context->arg;

  uint16_t *data = context->registration->user_data;
  *data = sys_be16_to_cpu(*(uint16_t *)args->data);

  LOG_INF("Written data to id 0x%02X: 0x%04X",
          context->registration->data_identifier.data_id, *data);

  // Signal this action consumes the event
  *consume_event = true;

  return UDS_PositiveResponse;
}

UDS_REGISTER_DATA_IDENTIFIER_STATIC(&instance,
                                    primitive_type_id,
                                    &primitive_type,
                                    read_data_by_id_check,
                                    read_data_by_id_action,
                                    write_data_by_id_check,
                                    write_data_by_id_action,
                                    &primitive_type_size);

UDS_REGISTER_DATA_IDENTIFIER_STATIC(&instance,
                                    string_id,
                                    &string,
                                    read_data_by_id_check,
                                    read_data_by_id_action,
                                    NULL,
                                    NULL,
                                    &string_size);