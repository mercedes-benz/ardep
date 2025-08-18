// This would be dedicated into a separate library

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>

#include <server.h>
#include <tp/isotp_c.h>

struct iso14229_zephyr_instance {
  UDSServer_t server;
  UDSISOTpC_t tp;

  struct k_msgq can_phys_msgq;
  struct k_msgq can_func_msgq;

  char can_phys_buffer[sizeof(struct can_frame) * 25];
  char can_func_buffer[sizeof(struct can_frame) * 25];
};

int iso14229_zephyr_init(struct iso14229_zephyr_instance* inst,
                         const UDSISOTpCConfig_t* iso_tp_config,
                         const struct device* can_dev,
                         UDSErr_t (*uds_cb)(struct UDSServer* srv,
                                            UDSEvent_t event,
                                            void* arg));
void iso14229_zephyr_thread(struct iso14229_zephyr_instance* inst);
