/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include "ardep/uds.h"
#include "iso14229.h"
#include "routine_control.h"

static UDSErr_t uds_check_with_routine_id_fn(
    const struct uds_context* const context, bool* apply_action) {
  const struct uds_registration_t* const reg = context->registration;

  UDSRoutineCtrlArgs_t* args = context->arg;

  if (reg->routine_control.routine_id != args->id) {
    *apply_action = false;
    return UDS_OK;
  }

  return reg->routine_control.actor.check(context, apply_action);
}

uds_check_fn uds_get_check_for_routine_control(
    const struct uds_registration_t* const reg) {
  return uds_check_with_routine_id_fn;
}
uds_action_fn uds_get_action_for_routine_control(
    const struct uds_registration_t* const reg) {
  return reg->routine_control.actor.action;
}