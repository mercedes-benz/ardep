

// #ifdef UDS_NEW_ENABLE_RESET
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>

#include <ardep/uds_new.h>
#include <iso14229.h>

LOG_MODULE_REGISTER(uds_new, LOG_LEVEL_INF);

K_MUTEX_DEFINE(custom_callback_mutex);
static ecu_reset_callback_t ecu_reset_custom_callback = NULL;

/**
 * @brief Work handler that performs the actual ECU reset
 */
static void ecu_reset_work_handler(struct k_work *work) {
  LOG_INF("Performing ECU reset");
  // Allow logging to be processed
  k_msleep(1);
  sys_reboot(SYS_REBOOT_COLD);
}
K_WORK_DELAYABLE_DEFINE(reset_work, ecu_reset_work_handler);

UDSErr_t handle_ecu_reset_event(struct uds_new_instance_t *inst,
                                enum ecu_reset_type reset_type) {
  int ret = k_mutex_lock(&custom_callback_mutex, K_FOREVER);
  if (ret < 0) {
    LOG_ERR(
        "Failed to acquire ECU Reset custom callback mutex from event handler");
    return ret;
  }
  if (ecu_reset_custom_callback) {
    UDSErr_t callback_result =
        ecu_reset_custom_callback(inst, reset_type, inst->user_context);
    k_mutex_unlock(&custom_callback_mutex);
    return callback_result;
  }
  ret = k_mutex_unlock(&custom_callback_mutex);
  if (ret < 0) {
    LOG_ERR("Failed to release ECU Reset custom callback mutex");
    return ret;
  }

  // Only support these two reset types by default
  if (reset_type != ECU_RESET_HARD && reset_type != ECU_RESET_KEY_OFF_ON) {
    return UDS_NRC_SubFunctionNotSupported;
  }

  uint32_t delay_ms =
      MAX(CONFIG_UDS_NEW_RESET_DELAY_MS, inst->iso14229.server.p2_ms);
  LOG_INF("Scheduling ECU reset in %u ms, type: %d", delay_ms, reset_type);
  if (!k_work_delayable_is_pending(&reset_work)) {
    ret = k_work_schedule(&reset_work, K_MSEC(delay_ms));
  } else {
    LOG_WRN("ECU reset work item is already scheduled");
    ret = UDS_NRC_ConditionsNotCorrect;
  }
  if (ret < 0) {
    LOG_ERR("Failed to schedule ECU reset work");
    return UDS_NRC_ConditionsNotCorrect;
  }
  return UDS_OK;
}

int uds_new_set_ecu_reset_callback(ecu_reset_callback_t callback) {
  int ret = k_mutex_lock(&custom_callback_mutex, K_FOREVER);
  if (ret < 0) {
    LOG_ERR("Failed to acquire ECU Reset custom callback mutex");
    return ret;
  }
  ecu_reset_custom_callback = callback;
  return k_mutex_unlock(&custom_callback_mutex);
}

// #endif  // UDS_NEW_ENABLE_RESET