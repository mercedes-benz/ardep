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

bool uds_filter_for_ecu_reset_event(UDSEvent_t event) {
  return event == UDS_EVT_DoScheduledReset || event == UDS_EVT_EcuReset;
}

uds_check_fn uds_get_check_for_ecu_reset(
    const struct uds_registration_t* const reg) {
  return reg->ecu_reset.ecu_reset.check;
}
uds_action_fn uds_get_action_for_ecu_reset(
    const struct uds_registration_t* const reg) {
  return reg->ecu_reset.ecu_reset.action;
}

uds_check_fn uds_get_check_for_execute_scheduled_reset(
    const struct uds_registration_t* const reg) {
  return reg->ecu_reset.execute_scheduled_reset.check;
}
uds_action_fn uds_get_action_for_execute_scheduled_reset(
    const struct uds_registration_t* const reg) {
  return reg->ecu_reset.execute_scheduled_reset.action;
}

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
  *consume_event = true;
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
  // give logging a chance to be processed
  k_msleep(1);
  sys_reboot(SYS_REBOOT_COLD);
  LOG_ERR("Error rebooting from ECU hard reset!");
  return UDS_NRC_ConditionsNotCorrect;
}
