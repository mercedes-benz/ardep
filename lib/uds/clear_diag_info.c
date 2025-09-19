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

#include "clear_diag_info.h"

uds_check_fn uds_get_check_for_clear_diag_info(
    const struct uds_registration_t* const reg) {
  return reg->clear_diagnostic_information.actor.check;
}
uds_action_fn uds_get_action_for_clear_diag_info(
    const struct uds_registration_t* const reg) {
  return reg->clear_diagnostic_information.actor.action;
}