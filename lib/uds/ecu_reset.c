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
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>

#include <ardep/uds.h>
#include <iso14229.h>

uds_check_fn uds_get_check_for_ecu_reset(
    const struct uds_registration_t* const reg) {
  return reg->ecu_reset.ecu_reset.check;
}
uds_action_fn uds_get_action_for_ecu_reset(
    const struct uds_registration_t* const reg) {
  return reg->ecu_reset.ecu_reset.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_ecu_reset_) = {
  .event = UDS_EVT_EcuReset,
  .get_check = uds_get_check_for_ecu_reset,
  .get_action = uds_get_action_for_ecu_reset,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__ECU_RESET,
};

uds_check_fn uds_get_check_for_execute_scheduled_reset(
    const struct uds_registration_t* const reg) {
  return reg->ecu_reset.execute_scheduled_reset.check;
}
uds_action_fn uds_get_action_for_execute_scheduled_reset(
    const struct uds_registration_t* const reg) {
  return reg->ecu_reset.execute_scheduled_reset.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_do_scheduled_reset_) = {
  .event = UDS_EVT_DoScheduledReset,
  .get_check = uds_get_check_for_execute_scheduled_reset,
  .get_action = uds_get_action_for_execute_scheduled_reset,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__ECU_RESET,
};

UDSErr_t uds_check_ecu_hard_reset(const struct uds_context* const context,
                                  bool* apply_action) {
  UDSECUResetArgs_t* args = context->arg;

  if (args->type == ECU_RESET__HARD) {
    *apply_action = true;
  }

  return UDS_OK;
}

UDSErr_t uds_action_ecu_hard_reset(struct uds_context* const context,
                                   bool* consume_event) {
  UDSECUResetArgs_t* args = context->arg;

  // Issue reset just after Confirmation to ECU Reset request
  args->powerDownTimeMillis = context->instance->iso14229.server.p2_ms;
  // Do not consume event because we might have another event handler from the
  // 0x87 LinkControl function
  *consume_event = false;
  return UDS_OK;
}

UDSErr_t uds_check_execute_scheduled_reset(
    const struct uds_context* const context, bool* apply_action) {
  uint8_t reset_type = *(uint8_t*)context->arg;

  if (reset_type == ECU_RESET__HARD) {
    *apply_action = true;
  }

  return UDS_OK;
}

UDSErr_t uds_action_execute_scheduled_reset(struct uds_context* const context,
                                            bool* consume_event) {
  LOG_INF("Executing scheduled hard reset now");
  log_flush();
  k_msleep(100);
  sys_reboot(SYS_REBOOT_COLD);
  LOG_ERR("Error rebooting from ECU hard reset!");
  return UDS_NRC_ConditionsNotCorrect;
}
