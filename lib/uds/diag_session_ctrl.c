/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include "diag_session_ctrl.h"

uds_check_fn uds_get_check_for_diag_session_ctrl(
    const struct uds_registration_t* const reg) {
  return reg->diag_session_ctrl.diag_sess_ctrl.check;
}
uds_action_fn uds_get_action_for_diag_session_ctrl(
    const struct uds_registration_t* const reg) {
  return reg->diag_session_ctrl.diag_sess_ctrl.action;
}

uds_check_fn uds_get_check_for_session_timeout(
    const struct uds_registration_t* const reg) {
  return reg->diag_session_ctrl.session_timeout.check;
}
uds_action_fn uds_get_action_for_session_timeout(
    const struct uds_registration_t* const reg) {
  return reg->diag_session_ctrl.session_timeout.action;
}