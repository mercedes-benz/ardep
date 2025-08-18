/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <server.h>
#include <tp/isotp_c.h>

LOG_MODULE_REGISTER(uds_routine, LOG_LEVEL_DBG);

static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

uint8_t dummy_memory[512] = {0x01, 0x02, 0x03, 0x04, 0x05};

char can_phys_fifo_buffer[sizeof(struct can_frame) * 25];
char can_func_fifo_buffer[sizeof(struct can_frame) * 25];

struct k_msgq can_phys_fifo;
struct k_msgq can_func_fifo;

void can_cb(const struct device *dev,
            struct can_frame *frame,
            void *user_data) {
  printk("CAN RX: %x %x %x\n", frame->id, frame->dlc, frame->data[0]);

  k_msgq_put((struct k_msgq *)user_data, frame, K_NO_WAIT);
}

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
  k_msgq_init(&can_phys_fifo, can_phys_fifo_buffer, sizeof(struct can_frame),
              ARRAY_SIZE(can_phys_fifo_buffer));

  k_msgq_init(&can_func_fifo, can_func_fifo_buffer, sizeof(struct can_frame),
              ARRAY_SIZE(can_func_fifo_buffer));

  UDSServer_t server;
  UDSServerInit(&server);
  UDSISOTpC_t tp;
  UDSISOTpCConfig_t cfg = {
    .source_addr = 0x7E8,
    .target_addr = 0x7E0,
    .source_addr_func = 0x7DF,
    .target_addr_func = UDS_TP_NOOP_ADDR,
  };
  UDSISOTpCInit(&tp, &cfg);
  server.tp = &tp.hdl;
  server.fn = uds_cb;
  tp.phys_link.user_send_can_arg = (void *)can_dev;
  tp.func_link.user_send_can_arg = (void *)can_dev;

  int err;
  if (!device_is_ready(can_dev)) {
    printk("CAN device not ready\n");
    return -ENODEV;
  }

  const struct can_filter phys_filter = {
    .id = tp.phys_sa,
    .mask = CAN_STD_ID_MASK,
  };
  const struct can_filter func_filter = {
    .id = tp.func_sa,
    .mask = CAN_STD_ID_MASK,
  };
  err = can_add_rx_filter(can_dev, can_cb, &can_phys_fifo, &phys_filter);
  if (err < 0) {
    printk("Failed to add RX filter for physical address: %d\n", err);
    return err;
  }
  err = can_add_rx_filter(can_dev, can_cb, &can_func_fifo, &func_filter);
  if (err < 0) {
    printk("Failed to add RX filter for functional address: %d\n", err);
    return err;
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

  printk("Hello UDS! %x %x\n", tp.phys_sa, tp.func_sa);

  while (1) {
    struct can_frame frame_phys;
    struct can_frame frame_func;
    int ret_phys = k_msgq_get(&can_phys_fifo, &frame_phys, K_NO_WAIT);
    int ret_func = k_msgq_get(&can_func_fifo, &frame_func, K_NO_WAIT);

    if (ret_phys == 0) {
      isotp_on_can_message(&tp.phys_link, frame_phys.data, frame_phys.dlc);
    }

    if (ret_func == 0) {
      isotp_on_can_message(&tp.func_link, frame_func.data, frame_func.dlc);
    }

    UDSServerPoll(&server);
    k_sleep(K_MSEC(1));
  }
}
