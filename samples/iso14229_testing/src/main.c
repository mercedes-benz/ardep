/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "iso14229_common.h"

#include <errno.h>

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <server.h>
#include <tp/isotp_c.h>

LOG_MODULE_REGISTER(uds_routine, LOG_LEVEL_DBG);

static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

uint8_t dummy_memory[512] = {0x01, 0x02, 0x03, 0x04, 0x05};

struct iso14229_zephyr_instance inst;

UDSErr_t uds_cb(struct UDSServer *srv, UDSEvent_t event, void *arg) {
  printk("UDS Event: %d\n", event);
  switch (event) {
    case UDS_EVT_Err: {
      UDSErr_t *err = (UDSErr_t *)arg;
      printk("UDS Error: %d\n", *err);
      break;
    }
    case UDS_EVT_DiagSessCtrl: {
      UDSDiagSessCtrlArgs_t *session_args = (UDSDiagSessCtrlArgs_t *)arg;
      printk("Diagnostic Session Control: %d\n", session_args->type);
      break;
    }
    case UDS_EVT_EcuReset: {
      uint8_t *reset_type = (uint8_t *)arg;
      printk("ECU Reset: %d\n", *reset_type);
      break;
    }
    case UDS_EVT_SessionTimeout: {
      printk("Session Timeout\n");
      srv->sessionType = UDS_LEV_DS_DS;  // reset to default session
      break;
    }
    case UDS_EVT_RoutineCtrl: {
      UDSRoutineCtrlArgs_t *routine = (UDSRoutineCtrlArgs_t *)arg;
      printk("Routine Control: %d %d\n", routine->id, routine->ctrlType);
      uint8_t data = 1;
      routine->copyStatusRecord(srv, &data, 1);
      break;
    }
    case UDS_EVT_RequestDownload: {
      UDSRequestDownloadArgs_t *req = (UDSRequestDownloadArgs_t *)arg;
      printk("Request Download: addr=%p size=%zu format=%d\n", req->addr,
             req->size, req->dataFormatIdentifier);
      break;
    }
    case UDS_EVT_TransferData: {  //! note: very import: the first block number
                                  //! must be 1 with this library
      UDSTransferDataArgs_t *transfer_args = (UDSTransferDataArgs_t *)arg;
      LOG_HEXDUMP_INF(transfer_args->data, transfer_args->len, "Received data");
      break;
    }
    case UDS_EVT_RequestTransferExit: {
      UDSRequestTransferExitArgs_t *exit_args =
          (UDSRequestTransferExitArgs_t *)arg;
      printk("Request Transfer Exit: data=%p len=%d\n", exit_args->data,
             exit_args->len);
      break;
    }
    case UDS_EVT_ReadMemByAddr: {
      UDSReadMemByAddrArgs_t *read_args = (UDSReadMemByAddrArgs_t *)arg;
      printk("Read Memory By Address: addr=%p size=%zu\n", read_args->memAddr,
             read_args->memSize);
      read_args->copy(srv, &dummy_memory[(uint32_t)read_args->memAddr],
                      read_args->memSize);
      break;
    }
    default:
      UDSCustomArgs_t *custom_args = (UDSCustomArgs_t *)arg;
      // todo: handle not implemented messages
      return UDS_NRC_ServiceNotSupported;
  }
  return UDS_OK;
}

int main(void) {
  UDSISOTpCConfig_t cfg = {
    .source_addr = 0x7E8,
    .target_addr = 0x7E0,
    .source_addr_func = 0x7DF,
    .target_addr_func = UDS_TP_NOOP_ADDR,
  };

  iso14229_zephyr_init(&inst, &cfg, can_dev, uds_cb);

  int err;
  if (!device_is_ready(can_dev)) {
    printk("CAN device not ready\n");
    return -ENODEV;
  }

  err = can_set_mode(can_dev, CAN_MODE_NORMAL);
  if (err) {
    printk("Failed to set CAN mode: %d\n", err);
    return err;
  }

  err = can_start(can_dev);
  if (err) {
    printk("Failed to start CAN device: %d\n", err);
    return err;
  }
  printk("CAN device started\n");

  while (1) {
    iso14229_zephyr_thread(&inst);
  }
}
