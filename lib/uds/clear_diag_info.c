/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds.h"
#include "iso14229.h"
#include "uds.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

uds_check_fn uds_get_check_for_clear_diag_info(
    const struct uds_registration_t* const reg) {
  return reg->clear_diagnostic_information.actor.check;
}
uds_action_fn uds_get_action_for_clear_diag_info(
    const struct uds_registration_t* const reg) {
  return reg->clear_diagnostic_information.actor.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_clear_diag_info_) = {
  .event = UDS_EVT_ClearDiagnosticInfo,
  .get_check = uds_get_check_for_clear_diag_info,
  .get_action = uds_get_action_for_clear_diag_info,
  .default_nrc = UDS_NRC_RequestOutOfRange,
  .registration_type = UDS_REGISTRATION_TYPE__CLEAR_DIAG_INFO,
};
