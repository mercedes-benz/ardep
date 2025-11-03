/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_sample, LOG_LEVEL_DBG);

#include "ardep/iso14229.h"
#include "ardep/uds.h"
#include "uds.h"

#include <zephyr/kernel.h>

K_MUTEX_DEFINE(dtc_record_mutex);

struct DtcRecord {
  uint8_t dtc[3];
  uint8_t status;
  bool is_active;
  bool is_frozen;
};

const uint8_t dtc_status_availability_mask = 0x7F;
struct DtcRecord dtc_records[] = {
  {.dtc = {0x0A, 0x9B, 0x17},
   .status = 0x24,
   .is_active = true,
   .is_frozen = false},
  {.dtc = {0x80, 0x51, 0x11},
   .status = 0x2F,
   .is_active = true,
   .is_frozen = false},
};

UDSErr_t read_dtc_info_check(const struct uds_context *const context,
                             bool *apply_action) {
  UDSRDTCIArgs_t *args = context->arg;

  // We don't need to filter/check the subfunction ID as this is checked
  // internally before calling this function.

  LOG_INF("Reading DT information by status mask: 0x%02X",
          args->subFuncArgs.numOfDTCByStatusMaskArgs.mask);

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t read_dtc_info_action(struct uds_context *const context,
                              bool *consume_event) {
  UDSRDTCIArgs_t *args = context->arg;

  int ret = args->copy(context->server, &dtc_status_availability_mask,
                       sizeof(dtc_status_availability_mask));
  if (ret != UDS_OK) {
    LOG_ERR("Failed to copy DTC status availability mask: %d", ret);
    return ret;
  }

  k_mutex_lock(&dtc_record_mutex, K_FOREVER);
  for (uint32_t i = 0; i < ARRAY_SIZE(dtc_records); i++) {
    if (dtc_records[i].status &
            args->subFuncArgs.numOfDTCByStatusMaskArgs.mask &&
        dtc_records[i].is_active) {
      ret = args->copy(context->server, dtc_records[i].dtc,
                       sizeof(dtc_records[i].dtc));
      if (ret != UDS_OK) {
        uint32_t dtc_value = (dtc_records[i].dtc[0] << 16) |
                             (dtc_records[i].dtc[1] << 8) |
                             dtc_records[i].dtc[2];
        LOG_ERR("Failed to copy DTC: 0x%06X ; Err = %d", dtc_value, ret);
        k_mutex_unlock(&dtc_record_mutex);
        return ret;
      }
      ret = args->copy(context->server, &dtc_records[i].status,
                       sizeof(dtc_records[i].status));
      if (ret != UDS_OK) {
        uint32_t dtc_value = (dtc_records[i].dtc[0] << 16) |
                             (dtc_records[i].dtc[1] << 8) |
                             dtc_records[i].dtc[2];
        LOG_ERR("Failed to copy DTC status: 0x%06X (status: 0x%02X) ; Err = %d",
                dtc_value, dtc_records[i].status, ret);
        k_mutex_unlock(&dtc_record_mutex);
        return ret;
      }
    }
  }

  k_mutex_unlock(&dtc_record_mutex);

  *consume_event = true;
  return UDS_PositiveResponse;
}

UDS_REGISTER_READ_DTC_INFO_HANDLER(
    &instance,
    read_dtc_info_check,
    read_dtc_info_action,
    UDS_READ_DTC_INFO_SUBFUNC__DTC_BY_STATUS_MASK,
    NULL)

static void dtc_reactivate_work_handler(struct k_work *work) {
  k_mutex_lock(&dtc_record_mutex, K_FOREVER);
  for (uint32_t i = 0; i < ARRAY_SIZE(dtc_records); i++) {
    dtc_records[i].is_active = true;
  }
  k_mutex_unlock(&dtc_record_mutex);

  LOG_DBG("DTCs reactivated after clear operation");
}

K_WORK_DELAYABLE_DEFINE(dtc_reactivate_work, dtc_reactivate_work_handler);

UDSErr_t clear_diag_info_check(const struct uds_context *const context,
                               bool *apply_action) {
  UDSCDIArgs_t *args = context->arg;

  LOG_INF("Clearing Diagnostic information with group of DTC: 0x%06X",
          args->groupOfDTC);

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t clear_diag_info_act(struct uds_context *const context,
                             bool *consume_event) {
  k_mutex_lock(&dtc_record_mutex, K_FOREVER);
  for (uint32_t i = 0; i < ARRAY_SIZE(dtc_records); i++) {
    dtc_records[i].is_active = false;
  }
  k_mutex_unlock(&dtc_record_mutex);

  // Schedule work to reactivate DTCs after 1 second
  if (k_work_delayable_is_pending(&dtc_reactivate_work)) {
    k_work_cancel_delayable(&dtc_reactivate_work);
  }
  k_work_schedule(&dtc_reactivate_work, K_SECONDS(1));

  *consume_event = true;
  return UDS_PositiveResponse;
}

UDS_REGISTER_CLEAR_DIAG_INFO_HANDLER(&instance,
                                     clear_diag_info_check,
                                     clear_diag_info_act,
                                     NULL)

// Control DTC Setting handlers
UDSErr_t control_dtc_setting_check(const struct uds_context *const context,
                                   bool *apply_action) {
  UDSControlDTCSettingArgs_t *args = context->arg;

  LOG_INF("Control DTC Setting request - Type: 0x%02X, Length: %u", args->type,
          args->len);

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t control_dtc_setting_action(struct uds_context *const context,
                                    bool *consume_event) {
  UDSControlDTCSettingArgs_t *args = context->arg;

  k_mutex_lock(&dtc_record_mutex, K_FOREVER);

  switch (args->type) {
    case 0x01:  // DTCSettingOn - Resume DTC status updates
      LOG_INF("DTC Setting ON - Resuming DTC status updates");
      for (uint32_t i = 0; i < ARRAY_SIZE(dtc_records); i++) {
        dtc_records[i].is_frozen = false;
      }
      break;

    case 0x02:  // DTCSettingOff - Freeze DTC status updates
      LOG_INF("DTC Setting OFF - Freezing DTC status updates");
      for (uint32_t i = 0; i < ARRAY_SIZE(dtc_records); i++) {
        dtc_records[i].is_frozen = true;
      }
      break;

    default:
      k_mutex_unlock(&dtc_record_mutex);
      LOG_WRN("Unsupported DTC Setting type: 0x%02X", args->type);
      *consume_event = false;
      return UDS_NRC_SubFunctionNotSupported;
  }

  k_mutex_unlock(&dtc_record_mutex);

  *consume_event = true;
  return UDS_PositiveResponse;
}

UDS_REGISTER_CONTROL_DTC_SETTING_HANDLER(&instance,
                                         control_dtc_setting_check,
                                         control_dtc_setting_action,
                                         NULL)

// Forward declaration for DTC status increment work
static void dtc_status_increment_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(dtc_status_increment_work,
                        dtc_status_increment_work_handler);

// DTC Status increment worker - runs every 50ms
static void dtc_status_increment_work_handler(struct k_work *work) {
  k_mutex_lock(&dtc_record_mutex, K_FOREVER);

  for (uint32_t i = 0; i < ARRAY_SIZE(dtc_records); i++) {
    if (!dtc_records[i].is_frozen) {
      dtc_records[i].status = (dtc_records[i].status + 1) & 0xFF;
    }
  }

  k_mutex_unlock(&dtc_record_mutex);

  // Reschedule for next increment
  k_work_schedule(&dtc_status_increment_work, K_MSEC(50));
}

// Initialize the DTC status increment worker
static int dtc_status_init(void) {
  LOG_INF("Starting DTC status increment worker (50ms interval)");
  k_work_schedule(&dtc_status_increment_work, K_MSEC(50));
  return 0;
}

SYS_INIT(dtc_status_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
