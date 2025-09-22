/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds.h"
#include "uds.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

uds_check_fn uds_get_check_for_diag_session_ctrl(
    const struct uds_registration_t* const reg) {
  return reg->diag_session_ctrl.diag_sess_ctrl.check;
}
uds_action_fn uds_get_action_for_diag_session_ctrl(
    const struct uds_registration_t* const reg) {
  return reg->diag_session_ctrl.diag_sess_ctrl.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_err_) = {
  .event = UDS_EVT_Err,
  .get_check = uds_get_check_for_diag_session_ctrl,
  .get_action = uds_get_action_for_diag_session_ctrl,
  .default_nrc = UDS_PositiveResponse,
  .registration_type = UDS_REGISTRATION_TYPE__DIAG_SESSION_CTRL,
};

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_diag_sess_ctrl_) = {
  .event = UDS_EVT_DiagSessCtrl,
  .get_check = uds_get_check_for_diag_session_ctrl,
  .get_action = uds_get_action_for_diag_session_ctrl,
  .default_nrc = UDS_PositiveResponse,
  .registration_type = UDS_REGISTRATION_TYPE__DIAG_SESSION_CTRL,
};

uds_check_fn uds_get_check_for_session_timeout(
    const struct uds_registration_t* const reg) {
  return reg->diag_session_ctrl.session_timeout.check;
}
uds_action_fn uds_get_action_for_session_timeout(
    const struct uds_registration_t* const reg) {
  return reg->diag_session_ctrl.session_timeout.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_session_timeout_) = {
  .event = UDS_EVT_SessionTimeout,
  .get_check = uds_get_check_for_session_timeout,
  .get_action = uds_get_action_for_session_timeout,
  .default_nrc = UDS_PositiveResponse,
  .registration_type = UDS_REGISTRATION_TYPE__DIAG_SESSION_CTRL,
};