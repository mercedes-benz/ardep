/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Use scripts/uds_iso14229_demo_script.py to test

#include "ardep/uds_new.h"
#include "write_memory_by_addr_impl.h"

#include <errno.h>

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <ardep/uds_minimal.h>
#include <ardep/uds_new.h>
#include <server.h>
#include <tp/isotp_c.h>
#include <util.h>

LOG_MODULE_REGISTER(iso14229_testing, LOG_LEVEL_DBG);

static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

uint8_t dummy_memory[512] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x66, 0x7, 0x8};

struct uds_new_instance_t instance;

uint16_t variable = 5;
char variable2[] = "Hello world";
UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC(
    &instance, 0x1234, variable, true, true);
UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_ARRAY(
    &instance, 0x1235, variable2, true, true);

// todo: tickets für nötige msgs

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

  uds_new_init(&instance, &cfg, can_dev, &instance);

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

  instance.iso14229.thread_run(&instance.iso14229);
}
