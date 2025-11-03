/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_ISO14229_H
#define ARDEP_ISO14229_H

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include <iso14229.h>

struct iso14229_zephyr_instance;

/**
 * @brief Callback type for UDS events
 *
 * @param inst Pointer to the iso-14229 instance the event occurred on
 * @param event The event that occurred
 * @param arg Argument provided by the event source (might be NULL depending on
 *             the event)
 * @param user_context User-defined context pointer as passed to \ref uds_init()
 */
typedef UDSErr_t (*uds_callback)(struct iso14229_zephyr_instance* inst,
                                 UDSEvent_t event,
                                 void* arg,
                                 void* user_context);

/**
 * @brief A Zephyr-specific ISO-14229 instance
 */
struct iso14229_zephyr_instance {
  /**
   * @brief Underlying uds server instance
   */
  UDSServer_t server;
  /**
   * @brief Underlying isotp instance
   */
  UDSISOTpC_t tp;

  struct k_msgq can_phys_msgq;
  struct k_msgq can_func_msgq;

  char can_phys_buffer[sizeof(struct can_frame) * 25];
  char can_func_buffer[sizeof(struct can_frame) * 25];

  struct k_mutex event_callback_mutex;
  uds_callback event_callback;

  void* user_context;

#ifdef CONFIG_ISO14229_THREAD
  k_tid_t thread_id;
  struct k_thread thread_data;
  K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ISO14229_THREAD_STACK_SIZE);
  bool thread_running;
  atomic_t thread_stop_requested;
  struct k_mutex thread_mutex;
#endif  // CONFIG_ISO14229_THREAD

  /**
   * @brief Set the UDS event callback that gets called when a
   *        new event is emitted by the server
   */
  int (*set_callback)(struct iso14229_zephyr_instance* inst,
                      uds_callback callback);

  /**
   * @brief Runs one iteration of the iso14229 event loop.
   *
   * @note This function must be called periodically. Either inside the
   *       provided thread using @ref thread_start or by the user
   */
  void (*event_loop_tick)(struct iso14229_zephyr_instance* inst);

#ifdef CONFIG_ISO14229_THREAD
  /**
   * @brief Start the UDS server thread
   */
  int (*thread_start)(struct iso14229_zephyr_instance* inst);
  /**
   * @brief Stop the UDS server thread
   */
  int (*thread_stop)(struct iso14229_zephyr_instance* inst);
#endif  // CONFIG_ISO14229_THREAD
};

/**
 * @brief Initialize a Zephyr-specific ISO-14229 instance
 *
 * @param inst Pointer to the instance to initialize
 * @param iso_tp_config ISO-TP configuration
 * @param can_dev CAN device the UDS Server should use
 * @param user_context User-defined context pointer that is passed to event
 *                     callbacks
 *
 * @returns 0 on success
 * @returns <0 on failure
 */
int iso14229_zephyr_init(struct iso14229_zephyr_instance* inst,
                         const UDSISOTpCConfig_t* iso_tp_config,
                         const struct device* can_dev,
                         void* user_context);

#endif  // ARDEP_ISO14229_H
