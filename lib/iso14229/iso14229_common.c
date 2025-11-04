/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/iso14229.h"

#include <zephyr/logging/log.h>

#include <iso14229.h>

LOG_MODULE_REGISTER(iso14229, CONFIG_ISO14229_LOG_LEVEL);

UDSErr_t uds_cb(struct UDSServer *srv, UDSEvent_t event, void *arg) {
  LOG_DBG("UDS Event: %s", UDSEventToStr(event));
  struct iso14229_zephyr_instance *inst =
      (struct iso14229_zephyr_instance *)srv->fn_data;

  UDSErr_t ret = UDS_OK;
  k_mutex_lock(&inst->event_callback_mutex, K_FOREVER);
  if (inst->event_callback) {
    ret = inst->event_callback(inst, event, arg, inst->user_context);
  }
  k_mutex_unlock(&inst->event_callback_mutex);
  return ret;
}

static void can_rx_cb(const struct device *dev,
                      struct can_frame *frame,
                      void *user_data) {
  // LOG_WRN("CAN RX: %03x [%u] %x ...", frame->id, frame->dlc, frame->data[0]);
  LOG_DBG("CAN RX: %03x [%u] %x ...", frame->id, frame->dlc, frame->data[0]);
  int ret = k_msgq_put((struct k_msgq *)user_data, frame, K_NO_WAIT);
  if (ret != 0) {
    LOG_ERR("Dropped CAN frame, error: %d", ret);
  }
}

void iso14229_inject_can_frame_rx(struct iso14229_zephyr_instance *inst,
                                  struct can_frame *frame) {
  LOG_INF("Injecting Received CAN Frame: %03x [%u] %x ...", frame->id,
          frame->dlc, frame->data[0]);
  can_rx_cb((const struct device *)inst->tp.phys_link.user_send_can_arg, frame,
            &inst->can_phys_msgq);
}

int iso14229_zephyr_set_callback(struct iso14229_zephyr_instance *inst,
                                 uds_callback callback) {
  LOG_DBG("Setting UDS callback");
  k_mutex_lock(&inst->event_callback_mutex, K_FOREVER);
  inst->event_callback = callback;
  k_mutex_unlock(&inst->event_callback_mutex);
  return 0;
}

static void iso14229_zephyr_event_loop_tick(
    struct iso14229_zephyr_instance *inst) {
  struct can_frame frame_phys;
  struct can_frame frame_func;

  while (k_msgq_get(&inst->can_phys_msgq, &frame_phys, K_NO_WAIT) == 0) {
    isotp_on_can_message(&inst->tp.phys_link, frame_phys.data, frame_phys.dlc);
  }

  while (k_msgq_get(&inst->can_func_msgq, &frame_func, K_NO_WAIT) == 0) {
    isotp_on_can_message(&inst->tp.func_link, frame_func.data, frame_func.dlc);
  }

  UDSServerPoll(&inst->server);
}

#ifdef CONFIG_ISO14229_THREAD

static void iso14229_thread_entry(void *p1, void *p2, void *p3) {
  struct iso14229_zephyr_instance *inst = (struct iso14229_zephyr_instance *)p1;

  while (atomic_get(&inst->thread_stop_requested) == 0) {
    iso14229_zephyr_event_loop_tick(inst);
    k_usleep(CONFIG_ISO14229_THREAD_SLEEP_US);
  }

  k_mutex_lock(&inst->thread_mutex, K_FOREVER);
  inst->thread_running = false;
  k_mutex_unlock(&inst->thread_mutex);
}

int iso14229_zephyr_thread_start(struct iso14229_zephyr_instance *inst) {
  LOG_DBG("Starting UDS thread");

  k_mutex_lock(&inst->thread_mutex, K_FOREVER);

  if (inst->thread_running) {
    LOG_WRN("Thread is already running");
    k_mutex_unlock(&inst->thread_mutex);
    return -EALREADY;
  }

  atomic_set(&inst->thread_stop_requested, 0);
  inst->thread_id = k_thread_create(&inst->thread_data, inst->thread_stack,
                                    K_KERNEL_STACK_SIZEOF(inst->thread_stack),
                                    iso14229_thread_entry, inst, NULL, NULL,
                                    K_PRIO_COOP(7), 0, K_NO_WAIT);

  if (inst->thread_id == NULL) {
    LOG_ERR("Failed to create UDS thread");
    k_mutex_unlock(&inst->thread_mutex);
    return -ENOMEM;
  }

  inst->thread_running = true;
  k_mutex_unlock(&inst->thread_mutex);
  return 0;
}

int iso14229_zephyr_thread_stop(struct iso14229_zephyr_instance *inst) {
  LOG_DBG("Stopping UDS thread");

  k_mutex_lock(&inst->thread_mutex, K_FOREVER);

  if (!inst->thread_running) {
    LOG_WRN("Thread is not running");
    k_mutex_unlock(&inst->thread_mutex);
    return -EALREADY;
  }

  atomic_set(&inst->thread_stop_requested, 1);
  k_mutex_unlock(&inst->thread_mutex);

  // Wait for thread to finish
  k_thread_join(inst->thread_id, K_FOREVER);

  LOG_DBG("UDS thread stopped");
  return 0;
}

#endif  // CONFIG_ISO14229_THREAD

int iso14229_zephyr_init(struct iso14229_zephyr_instance *inst,
                         const UDSISOTpCConfig_t *iso_tp_config,
                         const struct device *can_dev,
                         void *user_context) {
  inst->user_context = user_context;
  inst->set_callback = iso14229_zephyr_set_callback;

  int ret = k_mutex_init(&inst->event_callback_mutex);
  if (ret != 0) {
    LOG_ERR("Failed to initialize event callback mutex");
    return ret;
  }

  k_msgq_init(&inst->can_phys_msgq, inst->can_phys_buffer,
              sizeof(struct can_frame),
              ARRAY_SIZE(inst->can_phys_buffer) / sizeof(struct can_frame));

  k_msgq_init(&inst->can_func_msgq, inst->can_func_buffer,
              sizeof(struct can_frame),
              ARRAY_SIZE(inst->can_func_buffer) / sizeof(struct can_frame));

  UDSServerInit(&inst->server);
  UDSISOTpCInit(&inst->tp, iso_tp_config);

  inst->server.fn = uds_cb;
  inst->server.fn_data = inst;
  inst->server.tp = &inst->tp.hdl;
  inst->tp.phys_link.user_send_can_arg = (void *)can_dev;
  inst->tp.func_link.user_send_can_arg = (void *)can_dev;

  const struct can_filter phys_filter = {
    .id = inst->tp.phys_sa,
    .mask = CAN_STD_ID_MASK,
  };

  const struct can_filter func_filter = {
    .id = inst->tp.func_sa,
    .mask = CAN_STD_ID_MASK,
  };

  int err =
      can_add_rx_filter(can_dev, can_rx_cb, &inst->can_phys_msgq, &phys_filter);
  if (err < 0) {
    printk("Failed to add RX filter for physical address: %d\n", err);
    return err;
  }

  if (inst->tp.func_sa != UDS_TP_NOOP_ADDR) {
    err = can_add_rx_filter(can_dev, can_rx_cb, &inst->can_func_msgq,
                            &func_filter);
    if (err < 0) {
      printk("Failed to add RX filter for functional address: %d\n", err);
      return err;
    }
  }

  inst->event_loop_tick = iso14229_zephyr_event_loop_tick;

#ifdef CONFIG_ISO14229_THREAD
  inst->thread_start = iso14229_zephyr_thread_start;
  inst->thread_stop = iso14229_zephyr_thread_stop;

  ret = k_mutex_init(&inst->thread_mutex);
  if (ret != 0) {
    LOG_ERR("Failed to initialize thread mutex");
    return ret;
  }

  inst->thread_running = false;
  atomic_set(&inst->thread_stop_requested, 0);
#endif  // CONFIG_ISO14229_THREAD

  return 0;
}
