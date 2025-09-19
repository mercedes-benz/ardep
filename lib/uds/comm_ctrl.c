/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

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
