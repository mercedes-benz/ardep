/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds.h"
#include "iso14229.h"
#include "uds.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

static UDSErr_t uds_check_read_with_data_id(
    const struct uds_context* const context, bool* apply_action) {
  const struct uds_registration_t* const reg = context->registration;
  UDSRDBIArgs_t* args = (UDSRDBIArgs_t*)context->arg;

  if (args->dataId != context->registration->data_identifier.data_id) {
    *apply_action = false;

    return UDS_OK;
  }

  if (reg->data_identifier.read.check) {
    return reg->data_identifier.read.check(context, apply_action);
  }

  return UDS_OK;
}

uds_check_fn uds_get_check_for_read_data_by_identifier(
    const struct uds_registration_t* const reg) {
  return uds_check_read_with_data_id;
}
uds_action_fn uds_get_action_for_read_data_by_identifier(
    const struct uds_registration_t* const reg) {
  return reg->data_identifier.read.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_read_data_by_ident_) = {
  .event = UDS_EVT_ReadDataByIdent,
  .get_check = uds_get_check_for_read_data_by_identifier,
  .get_action = uds_get_action_for_read_data_by_identifier,
  .default_nrc = UDS_NRC_RequestOutOfRange,
  .registration_type = UDS_REGISTRATION_TYPE__DATA_IDENTIFIER,
};

static UDSErr_t uds_check_write_with_data_id(
    const struct uds_context* const context, bool* apply_action) {
  const struct uds_registration_t* const reg = context->registration;
  UDSRDBIArgs_t* args = (UDSRDBIArgs_t*)context->arg;

  if (args->dataId != context->registration->data_identifier.data_id) {
    *apply_action = false;

    return UDS_OK;
  }

  return reg->data_identifier.write.check(context, apply_action);
}

uds_check_fn uds_get_check_for_write_data_by_identifier(
    const struct uds_registration_t* const reg) {
  return uds_check_write_with_data_id;
}
uds_action_fn uds_get_action_for_write_data_by_identifier(
    const struct uds_registration_t* const reg) {
  return reg->data_identifier.write.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_write_data_by_ident_) = {
  .event = UDS_EVT_WriteDataByIdent,
  .get_check = uds_get_check_for_write_data_by_identifier,
  .get_action = uds_get_action_for_write_data_by_identifier,
  .default_nrc = UDS_NRC_RequestOutOfRange,
  .registration_type = UDS_REGISTRATION_TYPE__DATA_IDENTIFIER,
};

static UDSErr_t uds_check_io_control_with_data_id(
    const struct uds_context* const context, bool* apply_action) {
  const struct uds_registration_t* const reg = context->registration;
  UDSRDBIArgs_t* args = (UDSRDBIArgs_t*)context->arg;

  if (args->dataId != context->registration->data_identifier.data_id) {
    *apply_action = false;

    return UDS_OK;
  }

  return reg->data_identifier.io_control.check(context, apply_action);
}

uds_check_fn uds_get_check_for_io_control_by_identifier(
    const struct uds_registration_t* const reg) {
  return uds_check_io_control_with_data_id;
}
uds_action_fn uds_get_action_for_io_control_by_identifier(
    const struct uds_registration_t* const reg) {
  return reg->data_identifier.io_control.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_io_control_) = {
  .event = UDS_EVT_IOControl,
  .get_check = uds_get_check_for_io_control_by_identifier,
  .get_action = uds_get_action_for_io_control_by_identifier,
  .default_nrc = UDS_NRC_RequestOutOfRange,
  .registration_type = UDS_REGISTRATION_TYPE__DATA_IDENTIFIER,
};
