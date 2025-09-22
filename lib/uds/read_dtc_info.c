/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
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

static UDSErr_t uds_check_with_subfunc_fn(
    const struct uds_context* const context, bool* apply_action) {
  const struct uds_registration_t* const reg = context->registration;

  UDSRDTCIArgs_t* args = (UDSRDTCIArgs_t*)context->arg;

  if (reg->read_dtc.sub_function != args->type) {
    *apply_action = false;
    return UDS_OK;
  }

  return reg->read_dtc.actor.check(context, apply_action);
}

uds_check_fn uds_get_check_for_read_dtc_info(
    const struct uds_registration_t* const reg) {
  return uds_check_with_subfunc_fn;
}
uds_action_fn uds_get_action_for_read_dtc_info(
    const struct uds_registration_t* const reg) {
  return reg->read_dtc.actor.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_read_dtc_information_) = {
  .event = UDS_EVT_ReadDTCInformation,
  .get_check = uds_get_check_for_read_dtc_info,
  .get_action = uds_get_action_for_read_dtc_info,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__READ_DTC_INFO,
};
