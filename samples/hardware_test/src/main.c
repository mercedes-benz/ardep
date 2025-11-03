/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/devicetree.h"
#include "zephyr/drivers/gpio.h"
#include "zephyr/logging/log.h"
#include "zephyr/sys/printk.h"

#include <string.h>

#include <zephyr/console/console.h>
#include <zephyr/kernel.h>

#define LOG_MODULE_NAME sut
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

#define COMMAND_MAX_LEN 64
#define COMMAND_QUEUE_LEN 4
#define COMMAND_RECEIVER_STACK_SIZE 1024
#define COMMAND_DISPATCH_STACK_SIZE 2048

K_MSGQ_DEFINE(command_queue, COMMAND_MAX_LEN, COMMAND_QUEUE_LEN, 4);
K_MUTEX_DEFINE(command_mutex);

K_THREAD_STACK_DEFINE(command_receiver_stack, COMMAND_RECEIVER_STACK_SIZE);
K_THREAD_STACK_DEFINE(command_dispatch_stack, COMMAND_DISPATCH_STACK_SIZE);

static struct k_thread command_receiver_thread_data;
static struct k_thread command_dispatch_thread_data;

void gpio_test(void);
void log_hardware_info(void);
void uart_test(void);
void can_test(void);
void lin_test(void);

static void command_receiver_thread(void* p1, void* p2, void* p3);
static void command_dispatch_thread(void* p1, void* p2, void* p3);

int main() {
  console_getline_init();
  LOG_INF("Tester Firmware started");

  k_tid_t receiver_tid = k_thread_create(
      &command_receiver_thread_data, command_receiver_stack,
      K_THREAD_STACK_SIZEOF(command_receiver_stack), command_receiver_thread,
      NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

  k_tid_t dispatch_tid = k_thread_create(
      &command_dispatch_thread_data, command_dispatch_stack,
      K_THREAD_STACK_SIZEOF(command_dispatch_stack), command_dispatch_thread,
      NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

  k_thread_name_set(receiver_tid, "cmd_rx");
  k_thread_name_set(dispatch_tid, "cmd_dispatch");

  while (true) {
    k_msleep(1000);
  }
}

static void execute_command(const char* command) {
  if (strcmp(command, "hwInfo start") == 0) {
    LOG_INF("hwInfo start");
    log_hardware_info();
    LOG_INF("hwInfo stop");
  } else if (strcmp(command, "gpio start") == 0) {
    LOG_INF("gpio start");
    gpio_test();
    LOG_INF("gpio stop");
  } else if (strcmp(command, "can start") == 0) {
    LOG_INF("can start");
    can_test();
    LOG_INF("can stop");
  } else if (strcmp(command, "lin start") == 0) {
    LOG_INF("lin start");
    lin_test();
    LOG_INF("lin stop");
  } else if (strcmp(command, "uart start") == 0) {
    LOG_INF("uart start");
    uart_test();
    LOG_INF("uart stop");
  } else {
    LOG_ERR("Unknown command: %s", command);
  }
}

static void dispatch_command(const char* command) {
  int err = k_mutex_lock(&command_mutex, K_FOREVER);

  if (err != 0) {
    LOG_ERR("Failed to lock command mutex (err %d)", err);
    return;
  }

  execute_command(command);

  err = k_mutex_unlock(&command_mutex);
  if (err != 0) {
    LOG_ERR("Failed to unlock command mutex (err %d)", err);
  }
}

static void handle_console_input(void) {
  char* input = console_getline();

  if (input == NULL) {
    return;
  }

  size_t length = strlen(input);
  if (length >= COMMAND_MAX_LEN) {
    LOG_WRN("Command too long, will be truncated");
  }

  char command[COMMAND_MAX_LEN];
  memset(command, 0, sizeof(command));
  strncpy(command, input, sizeof(command) - 1);

  if (strcmp("idle", command) == 0) {
    int err = k_mutex_lock(&command_mutex, K_NO_WAIT);
    if (err != 0) {
      LOG_INF("idle false");
    } else {
      LOG_INF("idle true");
    }
    k_mutex_unlock(&command_mutex);

    return;
  }

  int err = k_msgq_put(&command_queue, command, K_FOREVER);
  if (err != 0) {
    LOG_ERR("Failed to enqueue command (err %d)", err);
  }
}

static void command_receiver_thread(void* p1, void* p2, void* p3) {
  ARG_UNUSED(p1);
  ARG_UNUSED(p2);
  ARG_UNUSED(p3);

  while (true) {
    handle_console_input();
  }
}

static void command_dispatch_thread(void* p1, void* p2, void* p3) {
  ARG_UNUSED(p1);
  ARG_UNUSED(p2);
  ARG_UNUSED(p3);

  char command[COMMAND_MAX_LEN];

  while (true) {
    int err = k_msgq_get(&command_queue, command, K_FOREVER);

    if (err != 0) {
      LOG_ERR("Failed to retrieve command (err %d)", err);
      continue;
    }

    command[COMMAND_MAX_LEN - 1] = '\0';

    dispatch_command(command);
  }
}
