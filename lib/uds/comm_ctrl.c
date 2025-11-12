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

uds_check_fn uds_get_check_for_communication_control(
    const struct uds_registration_t* const reg) {
  return reg->communication_control.actor.check;
}

uds_action_fn uds_get_action_for_communication_control(
    const struct uds_registration_t* const reg) {
  return reg->communication_control.actor.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_comm_ctrl_) = {
  .event = UDS_EVT_CommCtrl,
  .get_check = uds_get_check_for_communication_control,
  .get_action = uds_get_action_for_communication_control,
  .default_nrc = UDS_NRC_RequestOutOfRange,
  .registration_type = UDS_REGISTRATION_TYPE__COMMUNICATION_CONTROL,
};
