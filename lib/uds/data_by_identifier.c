/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds.h"
#include "iso14229.h"
#include "uds.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

uds_check_fn uds_get_check_for_read_data_by_identifier(
    const struct uds_registration_t* const reg) {
  return reg->data_identifier.read.check;
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

uds_check_fn uds_get_check_for_write_data_by_identifier(
    const struct uds_registration_t* const reg) {
  return reg->data_identifier.write.check;
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

uds_check_fn uds_get_check_for_io_control_by_identifier(
    const struct uds_registration_t* const reg) {
  return reg->data_identifier.io_control.check;
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