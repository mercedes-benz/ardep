/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Use scripts/uds_iso14229_demo_script.py to test

#include "ardep/uds.h"

#include <errno.h>

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <ardep/iso14229.h>
#include <iso14229.h>

LOG_MODULE_REGISTER(iso14229_testing, LOG_LEVEL_DBG);

static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

uint8_t dummy_memory[512] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x66, 0x7, 0x8};

struct iso14229_zephyr_instance inst;

UDSErr_t read_mem_by_addr_impl(struct UDSServer *srv,
                               const UDSReadMemByAddrArgs_t *read_args,
                               void *user_context) {
  uint32_t addr = (uintptr_t)read_args->memAddr;

  LOG_INF("Read Memory By Address: addr=0x%08X size=%u", addr,
          read_args->memSize);

  return read_args->copy(srv,
                         &dummy_memory[(uint32_t)(uintptr_t)read_args->memAddr],
                         read_args->memSize);
}

int main(void) {
  UDSISOTpCConfig_t cfg = {
    // Hardwarea Addresses
    .source_addr = 0x7E8,  // Can ID Server (us)
    .target_addr = 0x7E0,  // Can ID Client (them)

    // Functional Addresses
    .source_addr_func = 0x7DF,             // ID Client
    .target_addr_func = UDS_TP_NOOP_ADDR,  // ID Server (us)
  };

  int err = iso14229_zephyr_init(&inst, &cfg, can_dev, NULL);
  if (err) {
    printk("Failed to initialize ISO 14229 Zephyr instance: %d\n", err);
    return err;
  }

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

  inst.thread_run(&inst);
}
