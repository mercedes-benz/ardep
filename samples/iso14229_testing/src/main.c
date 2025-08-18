/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Use scripts/uds_iso14229_demo_script.py to test

#include "iso14229_common.h"
#include "write_memory_by_addr_impl.h"

#include <errno.h>

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <server.h>
#include <tp/isotp_c.h>

LOG_MODULE_REGISTER(iso14229_testing, LOG_LEVEL_DBG);

static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

uint8_t dummy_memory[512] = {0x01, 0x02, 0x03, 0x04, 0x05};

struct iso14229_zephyr_instance inst;

UDSErr_t uds_cb(struct UDSServer *srv, UDSEvent_t event, void *arg) {
  LOG_DBG("UDS Event: %d", event);
  switch (event) {
    case UDS_EVT_Err: {
      UDSErr_t *err = (UDSErr_t *)arg;
      LOG_ERR("UDS Error: %d", *err);
      break;
    }
    case UDS_EVT_DiagSessCtrl: {
      UDSDiagSessCtrlArgs_t *session_args = (UDSDiagSessCtrlArgs_t *)arg;
      LOG_INF("Diagnostic Session Control: %d", session_args->type);
      break;
    }
    case UDS_EVT_EcuReset: {
      uint8_t *reset_type = (uint8_t *)arg;
      LOG_INF("ECU Reset: %d", *reset_type);
      break;
    }
    case UDS_EVT_SessionTimeout: {
      LOG_WRN("Session Timeout");
      srv->sessionType = UDS_LEV_DS_DS;  // reset to default session
      break;
    }
    case UDS_EVT_RoutineCtrl: {
      UDSRoutineCtrlArgs_t *routine = (UDSRoutineCtrlArgs_t *)arg;
      LOG_INF("Routine Control: %d %d", routine->id, routine->ctrlType);
      // as per the standard, basically any data can be returned here
      uint8_t data = 1;
      routine->copyStatusRecord(srv, &data, 1);
      break;
    }
    case UDS_EVT_RequestDownload: {
      UDSRequestDownloadArgs_t *req = (UDSRequestDownloadArgs_t *)arg;
      LOG_INF("Request Download: addr=%p size=%zu format=%d", req->addr,
              req->size, req->dataFormatIdentifier);
      break;
    }
    case UDS_EVT_TransferData: {  //! note: very import: the first block number
                                  //! must be 1 with this library
      UDSTransferDataArgs_t *transfer_args = (UDSTransferDataArgs_t *)arg;
      LOG_HEXDUMP_INF(transfer_args->data, transfer_args->len, "Transfer Data");
      LOG_INF("Transfer Data: len=%d", transfer_args->len);
      break;
    }
    case UDS_EVT_RequestTransferExit: {
      UDSRequestTransferExitArgs_t *exit_args =
          (UDSRequestTransferExitArgs_t *)arg;
      LOG_INF("Request Transfer Exit: data=%p len=%d", exit_args->data,
              exit_args->len);
      break;
    }
    case UDS_EVT_ReadMemByAddr: {
      UDSReadMemByAddrArgs_t *read_args = (UDSReadMemByAddrArgs_t *)arg;
      LOG_INF("Read Memory By Address: addr=%p size=%zu", read_args->memAddr,
              read_args->memSize);
      read_args->copy(srv, &dummy_memory[(uint32_t)read_args->memAddr],
                      read_args->memSize);
      break;
    }
    default:
      UDSCustomArgs_t *custom_args = (UDSCustomArgs_t *)arg;

      if (custom_args->sid == CUSTOMUDS_WriteMemoryByAddr_SID) {
        struct CUSTOMUDS_WriteMemoryByAddr args;
        UDSErr_t e = customuds_decode_write_memory_by_addr(custom_args, &args);
        if (e != UDS_PositiveResponse) {
          return e;
        }

        memcpy(&dummy_memory[args.addr], args.data, args.len);

        LOG_HEXDUMP_INF(args.data, args.len, "Write Memory By Address");
        LOG_INF("Write Memory By Address: addr=0x%08X len=%zu", args.addr,
                args.len);

        return customuds_answer(srv, custom_args, &args);
      }

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
