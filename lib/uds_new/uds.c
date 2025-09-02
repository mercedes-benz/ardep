/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uds_new, CONFIG_UDS_NEW_LOG_LEVEL);

#include "data_by_identifier.h"
#include "ecu_reset.h"
#include "memory_by_address.h"

#include <ardep/iso14229.h>
#include <ardep/uds_new.h>
#include <iso14229.h>

// Wraps the logic to check and execute action on the event
UDSErr_t _uds_new_check_and_act_on_event(struct uds_new_instance_t* instance,
                                         struct uds_new_registration_t* reg,
                                         uds_new_check_fn check,
                                         uds_new_action_fn action,
                                         UDSEvent_t event,
                                         void* arg,
                                         bool* found_at_least_one_match,
                                         bool* consume_event) {
  struct uds_new_context context = {
    .instance = instance,
    .registration = reg,
    .event = event,
    .arg = arg,
  };
  UDSErr_t ret = UDS_OK;

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

// Iterates over event handlers to apply the actions for the event
UDSErr_t uds_new_handle_event(struct uds_new_instance_t* instance,
                              UDSEvent_t event,
                              void* arg,
                              uds_new_get_check_fn get_check,
                              uds_new_get_action_fn get_action) {
  bool found_at_least_one_match = false;

  // We start with static registrations
  STRUCT_SECTION_FOREACH (uds_new_registration_t, reg) {
    bool consume_event = true;
    int ret = _uds_new_check_and_act_on_event(
        instance, reg, get_check(reg), get_action(reg), event, arg,
        &found_at_least_one_match, &consume_event);
    if (consume_event || ret != UDS_OK) {
      return ret;
    }
  }

  // Optional dynamic registrations
#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION
  struct uds_new_registration_t* reg = instance->dynamic_registrations;
  while (reg != NULL) {
    bool consume_event = false;
    int ret = _uds_new_check_and_act_on_event(
        instance, reg, get_check(reg), get_action(reg), event, arg,
        &found_at_least_one_match, &consume_event);
    if (consume_event || ret != UDS_OK) {
      return ret;
    }

    reg = reg->next;
  }
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION

  if (!found_at_least_one_match) {
    return UDS_NRC_RequestOutOfRange;
  }

  return UDS_PositiveResponse;
}

// Callback registers on the iso14229 lib to receive UDS events
UDSErr_t uds_event_callback(struct iso14229_zephyr_instance* inst,
                            UDSEvent_t event,
                            void* arg,
                            void* user_context) {
  struct uds_new_instance_t* instance = user_context;

  switch (event) {
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
    case UDS_EVT_ReadDataByIdent:
      return uds_new_handle_event(
          instance, event, arg, uds_new_get_check_for_read_data_by_identifier,
          uds_new_get_action_for_read_data_by_identifier);

    case UDS_EVT_ReadMemByAddr: {
      UDSReadMemByAddrArgs_t* args = arg;
      return handle_read_memory_by_address(instance, args);
    }
    case UDS_EVT_WriteDataByIdent:
      return uds_new_handle_event(
          instance, event, arg, uds_new_get_check_for_write_data_by_identifier,
          uds_new_get_action_for_write_data_by_identifier);
    case UDS_EVT_WriteMemByAddr: {
      UDSWriteMemByAddrArgs_t* args = arg;
      return handle_write_memory_by_address(instance, args);
    }
    case UDS_EVT_Err:
    case UDS_EVT_CommCtrl:
    case UDS_EVT_SecAccessRequestSeed:
    case UDS_EVT_SecAccessValidateKey:
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

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION
// Registration function to dynamically register new handlers at runtime
// (Heap allocated)
static int uds_new_register_event_handler(
    struct uds_new_instance_t* inst,
    struct uds_new_registration_t registration) {
  registration.instance = inst;

  struct uds_new_registration_t* heap_registration =
      k_malloc(sizeof(struct uds_new_registration_t));
  if (heap_registration == NULL) {
    return -ENOMEM;
  }
  *heap_registration = registration;

  if (inst->dynamic_registrations == NULL) {
    inst->dynamic_registrations = heap_registration;
    heap_registration->next = NULL;
  } else {
    struct uds_new_registration_t* current = inst->dynamic_registrations;
    while (current->next != NULL) {
      current = current->next;
    }
    current->next = heap_registration;
    heap_registration->next = NULL;
  }

  return 0;
}
#endif  //  CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION

int uds_new_init(struct uds_new_instance_t* inst,
                 const UDSISOTpCConfig_t* iso_tp_config,
                 const struct device* can_dev,
                 void* user_context) {
  inst->user_context = user_context;

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION
  inst->dynamic_registrations = NULL;
  inst->register_event_handler = uds_new_register_event_handler;
#endif  //  CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION

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