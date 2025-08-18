// This would be dedicated into a separate library
#include "iso14229_common.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(iso14229_common, LOG_LEVEL_DBG);

static void can_rx_cb(const struct device* dev,
                      struct can_frame* frame,
                      void* user_data) {
  LOG_DBG("CAN RX: %x %x %x", frame->id, frame->dlc, frame->data[0]);
  k_msgq_put((struct k_msgq*)user_data, frame, K_NO_WAIT);
}

int iso14229_zephyr_init(struct iso14229_zephyr_instance* inst,
                         const UDSISOTpCConfig_t* iso_tp_config,
                         const struct device* can_dev,
                         UDSErr_t (*uds_cb)(struct UDSServer* srv,
                                            UDSEvent_t event,
                                            void* arg)) {
  k_msgq_init(&inst->can_phys_msgq, inst->can_phys_buffer,
              sizeof(struct can_frame), ARRAY_SIZE(inst->can_phys_buffer));

  k_msgq_init(&inst->can_func_msgq, inst->can_func_buffer,
              sizeof(struct can_frame), ARRAY_SIZE(inst->can_func_buffer));

  UDSServerInit(&inst->server);
  UDSISOTpCInit(&inst->tp, iso_tp_config);

  inst->server.fn = uds_cb;
  inst->server.tp = &inst->tp.hdl;
  inst->tp.phys_link.user_send_can_arg = (void*)can_dev;
  inst->tp.func_link.user_send_can_arg = (void*)can_dev;

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
  err =
      can_add_rx_filter(can_dev, can_rx_cb, &inst->can_func_msgq, &func_filter);
  if (err < 0) {
    printk("Failed to add RX filter for functional address: %d\n", err);
    return err;
  }

  return 0;
}

void iso14229_zephyr_thread(struct iso14229_zephyr_instance* inst) {
  while (1) {
    struct can_frame frame_phys;
    struct can_frame frame_func;
    int ret_phys = k_msgq_get(&inst->can_phys_msgq, &frame_phys, K_NO_WAIT);
    int ret_func = k_msgq_get(&inst->can_func_msgq, &frame_func, K_NO_WAIT);

    if (ret_phys == 0) {
      isotp_on_can_message(&inst->tp.phys_link, frame_phys.data,
                           frame_phys.dlc);
    }

    if (ret_func == 0) {
      isotp_on_can_message(&inst->tp.func_link, frame_func.data,
                           frame_func.dlc);
    }

    UDSServerPoll(&inst->server);

    k_sleep(K_MSEC(1));
  }
}
