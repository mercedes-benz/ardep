/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include "uds.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>

#include <ardep/uds.h>
#include <iso14229.h>

uds_check_fn uds_get_check_for_control_dtc_setting(
    const struct uds_registration_t* const reg) {
  return reg->control_dtc_setting.actor.check;
}
uds_action_fn uds_get_action_for_control_dtc_setting(
    const struct uds_registration_t* const reg) {
  return reg->control_dtc_setting.actor.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_control_dtc_setting_) = {
  .event = UDS_EVT_ControlDTCSetting,
  .get_check = uds_get_check_for_control_dtc_setting,
  .get_action = uds_get_action_for_control_dtc_setting,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__CONTROL_DTC_SETTING,
};
