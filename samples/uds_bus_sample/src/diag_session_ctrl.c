/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_bus_sample);

#include "uds.h"

#include <ardep/uds.h>

const struct device* retention_data =
    DEVICE_DT_GET(DT_CHOSEN(zephyr_firmware_loader_args));

UDSErr_t diag_session_ctrl_check(const struct uds_context* const context,
                                 bool* apply_action) {
  *apply_action = true;

  return UDS_OK;
}

UDSErr_t diag_session_ctrl_action(struct uds_context* const context,
                                  bool* consume_event) {
  UDSDiagSessCtrlArgs_t* args = context->arg;

  if (args->type == UDS_DIAG_SESSION__PROGRAMMING) {
    LOG_INF("Switching into firmware loader");
    return uds_switch_to_firmware_loader_with_programming_session();
  }

  LOG_INF("Changing diagnostic session to 0x%02X", args->type);

  *consume_event = false;

  return UDS_PositiveResponse;
}

UDSErr_t diag_session_timeout_check(const struct uds_context* const context,
                                    bool* apply_action) {
  *apply_action = true;
  return UDS_OK;
}

UDSErr_t diag_session_timeout_action(struct uds_context* const context,
                                     bool* consume_event) {
  LOG_INF("Diagnostic Session Timeout");
  *consume_event = false;

  return UDS_PositiveResponse;
}

UDS_REGISTER_DIAG_SESSION_CTRL_HANDLER(&instance,
                                       diag_session_ctrl_check,
                                       diag_session_ctrl_action,
                                       diag_session_timeout_check,
                                       diag_session_timeout_action,
                                       NULL)
