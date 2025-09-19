/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds.h"
#include "iso14229.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include "data_by_identifier.h"

uds_check_fn uds_get_check_for_read_data_by_identifier(
    const struct uds_registration_t* const reg) {
  return reg->data_identifier.read.check;
}
uds_action_fn uds_get_action_for_read_data_by_identifier(
    const struct uds_registration_t* const reg) {
  return reg->data_identifier.read.action;
}

uds_check_fn uds_get_check_for_write_data_by_identifier(
    const struct uds_registration_t* const reg) {
  return reg->data_identifier.write.check;
}
uds_action_fn uds_get_action_for_write_data_by_identifier(
    const struct uds_registration_t* const reg) {
  return reg->data_identifier.write.action;
}

uds_check_fn uds_get_check_for_io_control_by_identifier(
    const struct uds_registration_t* const reg) {
  return reg->data_identifier.io_control.check;
}
uds_action_fn uds_get_action_for_io_control_by_identifier(
    const struct uds_registration_t* const reg) {
  return reg->data_identifier.io_control.action;
}