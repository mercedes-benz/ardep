// This would be dedicated into a separate library
#include "ardep/uds_minimal.h"

#include <zephyr/logging/log.h>

#include <server.h>
#include <util.h>

LOG_MODULE_REGISTER(uds_minimal, CONFIG_UDS_MINIMAL_LOG_LEVEL);

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
  LOG_DBG("CAN RX: %03x [%u] %x ...", frame->id, frame->dlc, frame->data[0]);
  k_msgq_put((struct k_msgq *)user_data, frame, K_NO_WAIT);
}

int iso14229_zephyr_set_callback(struct iso14229_zephyr_instance *inst,
                                 uds_callback callback) {
  LOG_DBG("Setting UDS callback");
  k_mutex_lock(&inst->event_callback_mutex, K_FOREVER);
  inst->event_callback = callback;
  k_mutex_unlock(&inst->event_callback_mutex);
  return 0;
}

void iso14229_zephyr_thread_tick(struct iso14229_zephyr_instance *inst) {
  struct can_frame frame_phys;
  struct can_frame frame_func;
  int ret_phys = k_msgq_get(&inst->can_phys_msgq, &frame_phys, K_NO_WAIT);
  int ret_func = k_msgq_get(&inst->can_func_msgq, &frame_func, K_NO_WAIT);

  if (ret_phys == 0) {
    isotp_on_can_message(&inst->tp.phys_link, frame_phys.data, frame_phys.dlc);
  }

  if (ret_func == 0) {
    isotp_on_can_message(&inst->tp.func_link, frame_func.data, frame_func.dlc);
  }

  UDSServerPoll(&inst->server);
}

void iso14229_zephyr_thread(struct iso14229_zephyr_instance *inst) {
  while (1) {
    iso14229_zephyr_thread_tick(inst);
    k_sleep(K_MSEC(1));
  }
}

int iso14229_zephyr_init(struct iso14229_zephyr_instance *inst,
                         const UDSISOTpCConfig_t *iso_tp_config,
                         const struct device *can_dev,
                         void *user_context) {
  inst->user_context = user_context;
  inst->set_callback = iso14229_zephyr_set_callback;
  inst->thread_tick = iso14229_zephyr_thread_tick;
  inst->thread_run = iso14229_zephyr_thread;

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

  // Von CAN Nachrichten
  const struct can_filter phys_filter = {
    .id = inst->tp.phys_sa,
    .mask = CAN_STD_ID_MASK,
  };

  // KP woher das kommt?!
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
  err =
      can_add_rx_filter(can_dev, can_rx_cb, &inst->can_func_msgq, &func_filter);
  if (err < 0) {
    printk("Failed to add RX filter for functional address: %d\n", err);
    return err;
  }

  return 0;
}
