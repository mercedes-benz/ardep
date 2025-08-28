#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_new, CONFIG_UDS_NEW_LOG_LEVEL);

#include "ecu_reset.h"
#include "read_data_by_identifier.h"
#include "read_memory_by_address.h"
#include "uds.h"

#include <ardep/uds_minimal.h>
#include <ardep/uds_new.h>
#include <server.h>

UDSErr_t uds_event_callback(struct iso14229_zephyr_instance* inst,
                            UDSEvent_t event,
                            void* arg,
                            void* user_context) {
  struct uds_new_instance_t* instance = user_context;

  switch (event) {
    case UDS_EVT_Err:
    case UDS_EVT_DiagSessCtrl:
      break;
    case UDS_EVT_EcuReset: {
#ifdef CONFIG_UDS_NEW_ENABLE_RESET
      UDSECUResetArgs_t* args = arg;

      return handle_ecu_reset_event(instance, (enum ecu_reset_type)args->type);
#else
      return UDS_NRC_ServiceNotSupported;
#endif
    }
    case UDS_EVT_ReadDataByIdent: {
      UDSRDBIArgs_t* args = arg;
      return handle_data_read_by_identifier(instance, args);
    }
    case UDS_EVT_ReadMemByAddr: {
      UDSReadMemByAddrArgs_t* args = arg;
      return handle_read_memory_by_address(instance, args);
    }
    case UDS_EVT_CommCtrl:
    case UDS_EVT_SecAccessRequestSeed:
    case UDS_EVT_SecAccessValidateKey:
    case UDS_EVT_WriteDataByIdent:
    case UDS_EVT_RoutineCtrl:
    case UDS_EVT_RequestDownload:
    case UDS_EVT_RequestUpload:
    case UDS_EVT_TransferData:
    case UDS_EVT_RequestTransferExit:
    case UDS_EVT_SessionTimeout:
    case UDS_EVT_DoScheduledReset:
    case UDS_EVT_RequestFileTransfer:
    case UDS_EVT_Custom:
    case UDS_EVT_Poll:
    case UDS_EVT_SendComplete:
    case UDS_EVT_ResponseReceived:
    case UDS_EVT_Idle:
    case UDS_EVT_MAX:
      break;
  }

  return UDS_NRC_ServiceNotSupported;
}

int uds_new_init(struct uds_new_instance_t* inst,
                 const UDSISOTpCConfig_t* iso_tp_config,
                 const struct device* can_dev,
                 void* user_context) {
  inst->user_context = user_context;
  inst->set_ecu_reset_callback = uds_new_set_ecu_reset_callback;

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
  inst->register_data_by_identifier = uds_new_register_runtime_data_identifier;
  inst->dynamic_registrations = NULL;
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID

  int ret = iso14229_zephyr_init(&inst->iso14229, iso_tp_config, can_dev, inst);
  if (ret < 0) {
    LOG_ERR("Failed to initialize UDS instance");
    return ret;
  }

  ret = inst->iso14229.set_callback(&inst->iso14229, uds_event_callback);
  if (ret < 0) {
    LOG_ERR("Failed to set UDS event callback");
    return ret;
  }

  return 0;
}