/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "diag_session_ctrl.h"
#include "memory_by_address.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uds, CONFIG_UDS_LOG_LEVEL);

#include "data_by_identifier.h"
#include "ecu_reset.h"
#include "read_dtc_info.h"

#include <ardep/iso14229.h>
#include <ardep/uds.h>
#include <iso14229.h>

// Wraps the logic to check and execute action on the event
UDSErr_t _uds_check_and_act_on_event(struct uds_instance_t* instance,
                                     struct uds_registration_t* reg,
                                     uds_check_fn check,
                                     uds_action_fn action,
                                     UDSEvent_t event,
                                     void* arg,
                                     bool* found_at_least_one_match,
                                     bool* consume_event) {
  struct uds_context context = {
    .instance = instance,
    .registration = reg,
    .event = event,
    .arg = arg,
  };
  UDSErr_t ret = UDS_OK;

  if (!reg->applies_to_event(event)) {
    *consume_event = false;
    return UDS_OK;
  }

  bool apply_action = false;
  if (!check) {
    *consume_event = false;
    return ret;
  }
  ret = check(&context, &apply_action);
  if (ret != UDS_OK) {
    LOG_WRN("Check failed for Registration at addr: %p. Err: %d", reg, ret);
    *consume_event = false;
    return ret;
  }

  if (!apply_action || !action) {
    *consume_event = false;
    return ret;
  }

  ret = action(&context, consume_event);
  if (ret != UDS_OK) {
    LOG_WRN("Action failed for Registration at addr: %p. Err: %d", reg, ret);
    return ret;
  }

  *found_at_least_one_match = true;
  return UDS_OK;
}

static UDSErr_t default_nrc_when_no_handler_found(UDSEvent_t event) {
  switch (event) {
    case UDS_EVT_DiagSessCtrl:
    case UDS_EVT_SessionTimeout:
      // We don't require a handler for this event
      return UDS_PositiveResponse;
    case UDS_EVT_WriteDataByIdent:
    case UDS_EVT_ReadDataByIdent:
      return UDS_NRC_RequestOutOfRange;
    case UDS_EVT_EcuReset:
    case UDS_EVT_DoScheduledReset:
    case UDS_EVT_ReadDTCInformation:
      return UDS_NRC_SubFunctionNotSupported;
    case UDS_EVT_Err:
    case UDS_EVT_ReadMemByAddr:
    case UDS_EVT_CommCtrl:
    case UDS_EVT_SecAccessRequestSeed:
    case UDS_EVT_SecAccessValidateKey:
    case UDS_EVT_WriteMemByAddr:
    case UDS_EVT_RoutineCtrl:
    case UDS_EVT_RequestDownload:
    case UDS_EVT_RequestUpload:
    case UDS_EVT_TransferData:
    case UDS_EVT_RequestTransferExit:
    case UDS_EVT_RequestFileTransfer:
    case UDS_EVT_Custom:
    case UDS_EVT_Poll:
    case UDS_EVT_SendComplete:
    case UDS_EVT_ResponseReceived:
    case UDS_EVT_Idle:
    case UDS_EVT_MAX:
    default:
      // TODO: Every event should be handled. This should be unreachable.
      return UDS_NRC_ConditionsNotCorrect;
      break;
  }
}

// Iterates over event handlers to apply the actions for the event
UDSErr_t uds_handle_event(struct uds_instance_t* instance,
                          UDSEvent_t event,
                          void* arg,
                          uds_get_check_fn get_check,
                          uds_get_action_fn get_action) {
  bool found_at_least_one_match = false;

  // We start with static registrations
  STRUCT_SECTION_FOREACH (uds_registration_t, reg) {
    bool consume_event = true;
    int ret = _uds_check_and_act_on_event(
        instance, reg, get_check(reg), get_action(reg), event, arg,
        &found_at_least_one_match, &consume_event);
    if (consume_event || ret != UDS_OK) {
      return ret;
    }
  }

  // Optional dynamic registrations
#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION
  struct uds_registration_t* reg = instance->dynamic_registrations;
  while (reg != NULL) {
    bool consume_event = false;
    int ret = _uds_check_and_act_on_event(
        instance, reg, get_check(reg), get_action(reg), event, arg,
        &found_at_least_one_match, &consume_event);
    if (consume_event || ret != UDS_OK) {
      return ret;
    }

    reg = reg->next;
  }
#endif  // CONFIG_UDS_USE_DYNAMIC_REGISTRATION

  if (!found_at_least_one_match) {
    return default_nrc_when_no_handler_found(event);
  }

  return UDS_PositiveResponse;
}

// Callback registers on the iso14229 lib to receive UDS events
UDSErr_t uds_event_callback(struct iso14229_zephyr_instance* inst,
                            UDSEvent_t event,
                            void* arg,
                            void* user_context) {
  struct uds_instance_t* instance = user_context;

  switch (event) {
    case UDS_EVT_DiagSessCtrl:
      return uds_handle_event(instance, event, arg,
                              uds_get_check_for_diag_session_ctrl,
                              uds_get_action_for_diag_session_ctrl);
    case UDS_EVT_SessionTimeout:
      return uds_handle_event(instance, event, arg,
                              uds_get_check_for_session_timeout,
                              uds_get_action_for_session_timeout);
    case UDS_EVT_EcuReset:
      return uds_handle_event(instance, event, arg, uds_get_check_for_ecu_reset,
                              uds_get_action_for_ecu_reset);

    case UDS_EVT_DoScheduledReset:
      return uds_handle_event(instance, event, arg,
                              uds_get_check_for_execute_scheduled_reset,
                              uds_get_action_for_execute_scheduled_reset);

    case UDS_EVT_ReadDataByIdent:
      return uds_handle_event(instance, event, arg,
                              uds_get_check_for_read_data_by_identifier,
                              uds_get_action_for_read_data_by_identifier);

    case UDS_EVT_ReadMemByAddr:
      return uds_handle_event(instance, event, arg,
                              uds_get_check_for_read_memory_by_addr,
                              uds_get_action_for_read_memory_by_addr);
    case UDS_EVT_WriteDataByIdent:
      return uds_handle_event(instance, event, arg,
                              uds_get_check_for_write_data_by_identifier,
                              uds_get_action_for_write_data_by_identifier);
    case UDS_EVT_WriteMemByAddr:
      return uds_handle_event(instance, event, arg,
                              uds_get_check_for_write_memory_by_addr,
                              uds_get_action_for_write_memory_by_addr);
    case UDS_EVT_ReadDTCInformation:
      return uds_handle_event(instance, event, arg,
                              uds_get_check_for_read_dtc_info,
                              uds_get_action_for_read_dtc_info);
    case UDS_EVT_Err:
    case UDS_EVT_ClearDiagnosticInfo:
    case UDS_EVT_CommCtrl:
    case UDS_EVT_SecAccessRequestSeed:
    case UDS_EVT_SecAccessValidateKey:
    case UDS_EVT_RoutineCtrl:
    case UDS_EVT_RequestDownload:
    case UDS_EVT_RequestUpload:
    case UDS_EVT_TransferData:
    case UDS_EVT_RequestTransferExit:
    case UDS_EVT_RequestFileTransfer:
    case UDS_EVT_Custom:
    case UDS_EVT_Poll:
    case UDS_EVT_SendComplete:
    case UDS_EVT_ResponseReceived:
    case UDS_EVT_Idle:
    case UDS_EVT_MAX:
    default:
      return UDS_NRC_ServiceNotSupported;
  }
}

#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION
// Registration function to dynamically register new handlers at runtime
// (Heap allocated)
static int uds_register_event_handler(struct uds_instance_t* inst,
                                      struct uds_registration_t registration) {
  registration.instance = inst;

  struct uds_registration_t* heap_registration =
      k_malloc(sizeof(struct uds_registration_t));
  if (heap_registration == NULL) {
    return -ENOMEM;
  }
  *heap_registration = registration;

  if (inst->dynamic_registrations == NULL) {
    inst->dynamic_registrations = heap_registration;
    heap_registration->next = NULL;
  } else {
    struct uds_registration_t* current = inst->dynamic_registrations;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = heap_registration;
    heap_registration->next = NULL;
  }

  return 0;
}
#endif  //  CONFIG_UDS_USE_DYNAMIC_REGISTRATION

int uds_init(struct uds_instance_t* inst,
             const UDSISOTpCConfig_t* iso_tp_config,
             const struct device* can_dev,
             void* user_context) {
  inst->user_context = user_context;

#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION
  inst->dynamic_registrations = NULL;
  inst->register_event_handler = uds_register_event_handler;
#endif  //  CONFIG_UDS_USE_DYNAMIC_REGISTRATION

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