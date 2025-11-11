/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds.h"
#include "uds.h"

#include <zephyr/logging/log.h>
#include <zephyr/retention/bootmode.h>
#include <zephyr/retention/retention.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#if DT_HAS_CHOSEN(zephyr_firmware_loader_args) && CONFIG_RETENTION_BOOT_MODE
static const struct device* retention_data =
    DEVICE_DT_GET(DT_CHOSEN(zephyr_firmware_loader_args));

UDSErr_t uds_switch_to_firmware_loader_with_programming_session() {
  LOG_INF("Switching to programming session in firmware loader");

  const uint8_t session_type = UDS_DIAG_SESSION__PROGRAMMING;

  int ret =
      retention_write(retention_data, 0, &session_type, sizeof(session_type));
  if (ret != 0) {
    LOG_ERR("Failed to write retention: %d", ret);
    return UDS_NRC_ConditionsNotCorrect;
  }

  ret = bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);
  if (ret != 0) {
    LOG_ERR("Failed to set bootmode to bootloader: %d", ret);
    return UDS_NRC_ConditionsNotCorrect;
  }

  sys_reboot(SYS_REBOOT_WARM);

  LOG_ERR("Reboot to bootloader failed!");

  return UDS_NRC_GeneralReject;
}
#endif

uds_check_fn uds_get_check_for_diag_session_ctrl(
    const struct uds_registration_t* const reg) {
  return reg->diag_session_ctrl.diag_sess_ctrl.check;
}
uds_action_fn uds_get_action_for_diag_session_ctrl(
    const struct uds_registration_t* const reg) {
  return reg->diag_session_ctrl.diag_sess_ctrl.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_diag_sess_ctrl_) = {
  .event = UDS_EVT_DiagSessCtrl,
  .get_check = uds_get_check_for_diag_session_ctrl,
  .get_action = uds_get_action_for_diag_session_ctrl,
  .default_nrc = UDS_PositiveResponse,
  .registration_type = UDS_REGISTRATION_TYPE__DIAG_SESSION_CTRL,
};

uds_check_fn uds_get_check_for_session_timeout(
    const struct uds_registration_t* const reg) {
  return reg->diag_session_ctrl.session_timeout.check;
}
uds_action_fn uds_get_action_for_session_timeout(
    const struct uds_registration_t* const reg) {
  return reg->diag_session_ctrl.session_timeout.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_session_timeout_) = {
  .event = UDS_EVT_SessionTimeout,
  .get_check = uds_get_check_for_session_timeout,
  .get_action = uds_get_action_for_session_timeout,
  .default_nrc = UDS_PositiveResponse,
  .registration_type = UDS_REGISTRATION_TYPE__DIAG_SESSION_CTRL,
};
