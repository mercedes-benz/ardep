/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include "uds_session.h"

void uds_session_timeout(struct k_timer *timer_id) {
  uds_session_state = UDS_SESSION_STATE_DEFAULT;
  LOG_INF("UDS session expired");
}

K_TIMER_DEFINE(uds_session_expiration_timer, uds_session_timeout, NULL);

enum UdsSessionState uds_session_state = UDS_SESSION_STATE_DEFAULT;

void restart_uds_session_timer(void) {
  k_timer_start(&uds_session_expiration_timer, K_SECONDS(2), K_NO_WAIT);
}

void change_session(enum UdsSessionState new_state) {
  uds_session_state = new_state;

  LOG_INF("UDS session changed to %d", uds_session_state);

  restart_uds_session_timer();
}
