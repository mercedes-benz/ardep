/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds.h"
#include "iso14229.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_sample, LOG_LEVEL_DBG);

#include "uds.h"

struct DtcRecord {
  uint8_t dtc[3];
  uint8_t status;
};

const uint8_t dtc_status_availability_mask = 0x7F;
const struct DtcRecord dtc_records[] = {
  {.dtc = {0x0A, 0x9B, 0x17}, .status = 0x24},
  {.dtc = {0x80, 0x51, 0x11}, .status = 0x2F},
};

UDSErr_t read_dtc_info_check(const struct uds_context *const context,
                             bool *apply_action) {
  UDSRDTCIArgs_t *args = context->arg;

  // We don't need to filter/chck the subfunction ID as this is checked
  // internally before calling this function.

  LOG_INF("Reading DT information by status mask: 0x%02X",
          args->numOfDTCByStatusMaskArgs.mask);

  *apply_action = true;
  return UDS_OK;
}

UDSErr_t read_dtc_info_action(struct uds_context *const context,
                              bool *consume_event) {
  UDSRDTCIArgs_t *args = context->arg;

  int ret = args->copy(&context->instance->iso14229.server,
                       &dtc_status_availability_mask,
                       sizeof(dtc_status_availability_mask));
  if (ret != UDS_OK) {
    LOG_ERR("Failed to copy DTC status availability mask: %d", ret);
    return ret;
  }

  for (uint32_t i = 0; i < ARRAY_SIZE(dtc_records); i++) {
    if (dtc_records[i].status & args->numOfDTCByStatusMaskArgs.mask) {
      ret = args->copy(&context->instance->iso14229.server, dtc_records[i].dtc,
                       sizeof(dtc_records[i].dtc));
      if (ret != UDS_OK) {
        uint32_t dtc_value = (dtc_records[i].dtc[0] << 16) |
                             (dtc_records[i].dtc[1] << 8) |
                             dtc_records[i].dtc[2];
        LOG_ERR("Failed to copy DTC: 0x%06X ; Err = %d", dtc_value, ret);
        return ret;
      }
      ret = args->copy(&context->instance->iso14229.server,
                       &dtc_records[i].status, sizeof(dtc_records[i].status));
      if (ret != UDS_OK) {
        uint32_t dtc_value = (dtc_records[i].dtc[0] << 16) |
                             (dtc_records[i].dtc[1] << 8) |
                             dtc_records[i].dtc[2];
        LOG_ERR("Failed to copy DTC status: 0x%06X (status: 0x%02X) ; Err = %d",
                dtc_value, dtc_records[i].status, ret);
        return ret;
      }
    }
  }

  *consume_event = true;
  return UDS_PositiveResponse;
}

UDS_REGISTER_READ_DTC_INFO_HANDLER(
    &instance,
    NULL,
    read_dtc_info_check,
    read_dtc_info_action,
    UDS_READ_DTC_INFO_SUBFUNC__DTC_BY_STATUS_MASK)