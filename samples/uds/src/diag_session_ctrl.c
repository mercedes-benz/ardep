/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_sample, LOG_LEVEL_DBG);

#include "uds.h"

// Check function for the Diagnostic Session Control event
UDSErr_t diag_session_ctrl_check(const struct uds_context *const context,
                                 bool *apply_action) {
  // Type the argument object
  // We don't need to check, that the event matches. This is done internally by
  // the UDS lib
  UDSDiagSessCtrlArgs_t *args = context->arg;

  // Set to true, if the action should be applied.
  // Here you can check other conditions if needed
  *apply_action = true;
  LOG_INF("Check to change diagnostic session to 0x%02X successful",
          args->type);

  // Return `UDS_OK` if everything is fine, or an NRC to abort event handling
  // and return this error code to the client
  return UDS_OK;
}

// Action function for the Diagnostic Session Control event
UDSErr_t diag_session_ctrl_action(struct uds_context *const context,
                                  bool *consume_event) {
  UDSDiagSessCtrlArgs_t *args = context->arg;
  LOG_INF("Changing diagnostic session to 0x%02X", args->type);

  // We don't want to consume the event here, because the LinkControl handler
  // must know about it so it can reset the bitrate to its default value
  // If there are no other event handlers that would need the event, you can
  // stop iteration over other event handlers by setting `*consume_event =
  // true;`
  *consume_event = false;

  // Return the response code that the service should return to the client
  return UDS_PositiveResponse;
}

// Check function for the Diagnostic Session Timeout event
// This event is generated independent of the client and emitted when the
// Session timeout is reached
UDSErr_t diag_session_timeout_check(const struct uds_context *const context,
                                    bool *apply_action) {
  *apply_action = true;
  LOG_INF("Check to act on Diagnostic Session Timeout successful");
  return UDS_OK;
}

// Action function for the Diagnostic Session Timeout event
UDSErr_t diag_session_timeout_action(struct uds_context *const context,
                                     bool *consume_event) {
  LOG_INF("Diagnostic Session Timeout handled");

  // We don't want to consume the event here, because the LinkControl handler
  // must know about it so it can reset the bitrate to its default value
  *consume_event = false;

  // As this is not tied to a client request, nothing will be returned to the
  // client. This just signals the unterlying server whether the event was
  // handled successfully.
  return UDS_PositiveResponse;
}

// Registration Macro to register the event handlers we defined above
UDS_REGISTER_DIAG_SESSION_CTRL_HANDLER(
    // The UDS instance we want to register
    // the handler for
    &instance,
    // Check and Action functions for the events
    diag_session_ctrl_check,
    diag_session_ctrl_action,
    diag_session_timeout_check,
    diag_session_timeout_action,
    // Optional user context pointer that is passed wit the context to every
    // handler
    NULL)