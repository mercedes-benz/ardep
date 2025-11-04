/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(firmware_loader, CONFIG_APP_LOG_LEVEL);

#include "uds.h"

#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>

#include <ardep/uds.h>
#include <sys/cdefs.h>

UDSErr_t on_session_ctrl_check(const struct uds_context *const context,
                               bool *apply_action) {
  // We want to handle all events
  *apply_action = true;
  return UDS_OK;
}

UDSErr_t on_session_ctrl_action(struct uds_context *const context,
                                bool *consume_event) {
  UDSDiagSessCtrlArgs_t *args = (UDSDiagSessCtrlArgs_t *)context->arg;

  // There are no other event handlers registered
  *consume_event = true;

  switch (args->type) {
    case UDS_DIAG_SESSION__PROGRAMMING:
      LOG_INF("Switching to programming session");
      return UDS_OK;
    case UDS_DIAG_SESSION__EXTENDED:
      LOG_INF("Switching to extended session");
      return UDS_OK;
  }

  LOG_WRN("Unsupported session type: 0x%02x", args->type);
  return UDS_NRC_SubFunctionNotSupported;
}

UDSErr_t on_session_timeout_check(const struct uds_context *const context,
                                  bool *apply_action) {
  // We want to handle all events
  *apply_action = true;
  return UDS_OK;
}

UDSErr_t on_session_timeout_action(struct uds_context *const context,
                                   bool *consume_event) {
  LOG_INF("Reboot due to session timeout failed!");

  // Flush logs
  log_flush();

  sys_reboot(SYS_REBOOT_COLD);
  // We should never reach this point
  CODE_UNREACHABLE;
}

UDS_REGISTER_DIAG_SESSION_CTRL_HANDLER(&instance,
                                       on_session_ctrl_check,
                                       on_session_ctrl_action,
                                       on_session_timeout_check,
                                       on_session_timeout_action,
                                       NULL);
