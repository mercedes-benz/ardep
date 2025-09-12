/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Use scripts/uds_iso14229_demo_script.py to test

#include "ardep/uds.h"

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <ardep/iso14229.h>
#include <ardep/uds.h>
#include <iso14229.h>

LOG_MODULE_REGISTER(iso14229_testing, LOG_LEVEL_DBG);

static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

uint8_t dummy_memory[512] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x66, 0x7, 0x8};

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
  // Return Ok, when we don't handle this event
  if (args->dataId != context->registration->data_identifier.data_id) {
    return UDS_OK;
  }
  LOG_INF("Checking to read data id: 0x%02X successful",
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
  LOG_INF("Checking to write data id: 0x%02X successful",
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

struct uds_instance_t instance;

UDS_REGISTER_ECU_HARD_RESET_HANDLER(&instance);

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

UDS_REGISTER_MEMORY_DEFAULT_HANDLER(&instance);

UDSErr_t read_mem_by_addr_impl(struct UDSServer *srv,
                               const UDSReadMemByAddrArgs_t *read_args,
                               void *user_context) {
  uint32_t addr = (uintptr_t)read_args->memAddr;

  LOG_INF(
      "Read Memory By Address: "
      "addr=0x%08X size=%u",
      addr, read_args->memSize);

  return read_args->copy(srv,
                         &dummy_memory[(uint32_t)(uintptr_t)read_args->memAddr],
                         read_args->memSize);
}

int main(void) {
  LOG_INF("ARDEP UDS Sample");

  UDSISOTpCConfig_t cfg = {
    // Hardwarea Addresses
    .source_addr = 0x7E8,
    .target_addr = 0x7E0,

    // Functional Addresses
    .source_addr_func = 0x7DF,
    .target_addr_func = UDS_TP_NOOP_ADDR,
  };

  uds_init(&instance, &cfg, can_dev, &instance);

  int err;
  if (!device_is_ready(can_dev)) {
    LOG_INF("CAN device not ready");
    return -ENODEV;
  }

  err = can_set_mode(can_dev, CAN_MODE_NORMAL);
  if (err) {
    LOG_ERR("Failed to set CAN mode: %d", err);
    return err;
  }

  err = can_start(can_dev);
  if (err) {
    LOG_ERR(
        "Failed to start CAN device: "
        "%d",
        err);
    return err;
  }

  LOG_INF("CAN device started\n");

  instance.iso14229.thread_run(&instance.iso14229);
}
