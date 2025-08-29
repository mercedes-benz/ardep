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

/**
 * @brief Work handler that performs the actual ECU reset
 */
void __weak ecu_reset_work_handler(struct k_work *work) {
  LOG_INF("Performing ECU reset");
  // Allow logging to be processed
  k_msleep(1);
  sys_reboot(SYS_REBOOT_COLD);
}
K_WORK_DELAYABLE_DEFINE(reset_work, ecu_reset_work_handler);

UDSErr_t handle_ecu_reset_event(struct uds_new_instance_t *inst,
                                enum ecu_reset_type reset_type) {
  // By default only support hard reset
  if (reset_type != ECU_RESET_HARD) {
    return UDS_NRC_SubFunctionNotSupported;
  }

  uint32_t delay_ms =
      MAX(CONFIG_UDS_NEW_RESET_DELAY_MS, inst->iso14229.server.p2_ms);
  LOG_INF("Scheduling ECU reset in %u ms, type: %d", delay_ms, reset_type);

  int ret = 0;
  if (!k_work_delayable_is_pending(&reset_work)) {
    ret = k_work_schedule(&reset_work, K_MSEC(delay_ms));
  } else {
    LOG_WRN("ECU reset work item is already scheduled");
    ret = -1;
  }
  if (ret < 0) {
    LOG_ERR("Failed to schedule ECU reset work");
    return UDS_NRC_ConditionsNotCorrect;
  }
  return UDS_OK;
}
