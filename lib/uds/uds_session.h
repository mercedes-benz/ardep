/*
 * Copyright (c) 2024 Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
*/

enum UdsSessionState {
  UDS_SESSION_STATE_DEFAULT = 1,
  UDS_SESSION_STATE_PROGRAMMING = 2,
  UDS_SESSION_STATE_EXTENDED_DIAGNOSTIC = 3,
};

extern enum UdsSessionState uds_session_state;

void restart_session_timer(void);
void change_session(enum UdsSessionState new_state);
