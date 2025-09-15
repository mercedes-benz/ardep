/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_sample, LOG_LEVEL_DBG);

#include "uds.h"

UDSErr_t diag_session_ctrl_check(const struct uds_context *const context,
                                 bool *apply_action) {
  UDSDiagSessCtrlArgs_t *args = context->arg;
  *apply_action = true;
  LOG_INF("Check to change diagnostic session to 0x%02X successful",
          args->type);
  return UDS_OK;
}

UDSErr_t diag_session_ctrl_action(struct uds_context *const context,
                                  bool *consume_event) {
  UDSDiagSessCtrlArgs_t *args = context->arg;
  LOG_INF("Changing diagnostic session to 0x%02X", args->type);

  *consume_event = true;

  return UDS_PositiveResponse;
}

UDSErr_t diag_session_timeout_check(const struct uds_context *const context,
                                    bool *apply_action) {
  *apply_action = true;
  LOG_INF("Check to act on Diagnostic Session Timeout successful");
  return UDS_OK;
}

UDSErr_t diag_session_timeout_action(struct uds_context *const context,
                                     bool *consume_event) {
  LOG_INF("Diagnostic Session Timeout handled");

  *consume_event = true;

  return UDS_PositiveResponse;
}

UDS_REGISTER_DIAG_SESSION_CTRL_HANDLER(&instance,
                                       diag_session_ctrl_check,
                                       diag_session_ctrl_action,
                                       diag_session_timeout_check,
                                       diag_session_timeout_action,
                                       NULL)