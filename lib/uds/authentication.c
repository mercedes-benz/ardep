/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include "ardep/uds.h"
#include "iso14229.h"
#include "uds.h"

uds_check_fn uds_get_check_for_auth(
    const struct uds_registration_t* const reg) {
  return reg->auth.auth.check;
}
uds_action_fn uds_get_action_for_auth(
    const struct uds_registration_t* const reg) {
  return reg->auth.auth.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_auth_) = {
  .event = UDS_EVT_Auth,
  .get_check = uds_get_check_for_auth,
  .get_action = uds_get_action_for_auth,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__AUTHENTICATION,
};

uds_check_fn uds_get_check_for_auth_timeout(
    const struct uds_registration_t* const reg) {
  return reg->auth.timeout.check;
}
uds_action_fn uds_get_action_for_auth_timeout(
    const struct uds_registration_t* const reg) {
  return reg->auth.timeout.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_auth_timeout_) = {
  .event = UDS_EVT_AuthTimeout,
  .get_check = uds_get_check_for_auth_timeout,
  .get_action = uds_get_action_for_auth_timeout,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__AUTHENTICATION,
};
