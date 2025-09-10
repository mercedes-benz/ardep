/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_new, CONFIG_UDS_NEW_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>

#include <ardep/uds_new.h>
#include <iso14229.h>

bool uds_new_filter_for_read_dtc_info_event(UDSEvent_t event) {
  return event == UDS_EVT_ReadDTCInformation;
}

static UDSErr_t my_uds_new_check_fn(const struct uds_new_context* const context,
                                    bool* apply_action) {
  const struct uds_new_registration_t* const reg = context->registration;

  UDSRDTCIArgs_t* args = (UDSRDTCIArgs_t*)context->arg;

  if (reg->read_dtc.sub_function != args->type) {
    *apply_action = false;
    return UDS_OK;
  }

  return reg->read_dtc.actor.check(context, apply_action);
}

uds_new_check_fn uds_new_get_check_for_read_dtc_info(
    const struct uds_new_registration_t* const reg) {
  return my_uds_new_check_fn;
}
uds_new_action_fn uds_new_get_action_for_read_dtc_info(
    const struct uds_new_registration_t* const reg) {
  return reg->read_dtc.actor.action;
}
