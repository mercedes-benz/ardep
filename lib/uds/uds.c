/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
LOG_MODULE_REGISTER(uds, CONFIG_UDS_LOG_LEVEL);

#include "clear_diag_info.h"
#include "comm_ctrl.h"
#include "data_by_identifier.h"
#include "diag_session_ctrl.h"
#include "ecu_reset.h"
#include "memory_by_address.h"
#include "read_dtc_info.h"
#include "routine_control.h"
#include "security_access.h"

#include <ardep/iso14229.h>
#include <ardep/uds.h>
#include <iso14229.h>

/**
 * @brief Associated events with other data required to handle them
 */
struct uds_event_to_handler_mapping {
  UDSEvent_t event;
  uds_get_check_fn get_check;
  uds_get_action_fn get_action;
  UDSErr_t default_nrc;
  enum uds_registration_type_t registration_type;
};

static const struct uds_event_to_handler_mapping event_handler_mappings[] = {
  {
    .event = UDS_EVT_Err,
    .get_check = uds_get_check_for_diag_session_ctrl,
    .get_action = uds_get_action_for_diag_session_ctrl,
    .default_nrc = UDS_PositiveResponse,
    .registration_type = UDS_REGISTRATION_TYPE__DIAG_SESSION_CTRL,
  },
  {
    .event = UDS_EVT_DiagSessCtrl,
    .get_check = uds_get_check_for_diag_session_ctrl,
    .get_action = uds_get_action_for_diag_session_ctrl,
    .default_nrc = UDS_PositiveResponse,
    .registration_type = UDS_REGISTRATION_TYPE__DIAG_SESSION_CTRL,
  },
  {
    .event = UDS_EVT_SessionTimeout,
    .get_check = uds_get_check_for_session_timeout,
    .get_action = uds_get_action_for_session_timeout,
    .default_nrc = UDS_PositiveResponse,
    .registration_type = UDS_REGISTRATION_TYPE__DIAG_SESSION_CTRL,
  },
  {
    .event = UDS_EVT_EcuReset,
    .get_check = uds_get_check_for_ecu_reset,
    .get_action = uds_get_action_for_ecu_reset,
    .default_nrc = UDS_NRC_SubFunctionNotSupported,
    .registration_type = UDS_REGISTRATION_TYPE__ECU_RESET,
  },
  {
    .event = UDS_EVT_DoScheduledReset,
    .get_check = uds_get_check_for_execute_scheduled_reset,
    .get_action = uds_get_action_for_execute_scheduled_reset,
    .default_nrc = UDS_NRC_SubFunctionNotSupported,
    .registration_type = UDS_REGISTRATION_TYPE__ECU_RESET,
  },
  {
    .event = UDS_EVT_ReadDataByIdent,
    .get_check = uds_get_check_for_read_data_by_identifier,
    .get_action = uds_get_action_for_read_data_by_identifier,
    .default_nrc = UDS_NRC_RequestOutOfRange,
    .registration_type = UDS_REGISTRATION_TYPE__DATA_IDENTIFIER,
  },
  {
    .event = UDS_EVT_ReadMemByAddr,
    .get_check = uds_get_check_for_read_memory_by_addr,
    .get_action = uds_get_action_for_read_memory_by_addr,
    .default_nrc = UDS_NRC_ConditionsNotCorrect,
    .registration_type = UDS_REGISTRATION_TYPE__MEMORY,
  },
  {
    .event = UDS_EVT_WriteDataByIdent,
    .get_check = uds_get_check_for_write_data_by_identifier,
    .get_action = uds_get_action_for_write_data_by_identifier,
    .default_nrc = UDS_NRC_RequestOutOfRange,
    .registration_type = UDS_REGISTRATION_TYPE__DATA_IDENTIFIER,
  },
  {
    .event = UDS_EVT_WriteMemByAddr,
    .get_check = uds_get_check_for_write_memory_by_addr,
    .get_action = uds_get_action_for_write_memory_by_addr,
    .default_nrc = UDS_NRC_ConditionsNotCorrect,
    .registration_type = UDS_REGISTRATION_TYPE__MEMORY,
  },
  {
    .event = UDS_EVT_ReadDTCInformation,
    .get_check = uds_get_check_for_read_dtc_info,
    .get_action = uds_get_action_for_read_dtc_info,
    .default_nrc = UDS_NRC_SubFunctionNotSupported,
    .registration_type = UDS_REGISTRATION_TYPE__READ_DTC_INFO,
  },
  {
    .event = UDS_EVT_ClearDiagnosticInfo,
    .get_check = uds_get_check_for_clear_diag_info,
    .get_action = uds_get_action_for_clear_diag_info,
    .default_nrc = UDS_NRC_RequestOutOfRange,
    .registration_type = UDS_REGISTRATION_TYPE__CLEAR_DIAG_INFO,
  },
  {
    .event = UDS_EVT_IOControl,
    .get_check = uds_get_check_for_io_control_by_identifier,
    .get_action = uds_get_action_for_io_control_by_identifier,
    .default_nrc = UDS_NRC_RequestOutOfRange,
    .registration_type = UDS_REGISTRATION_TYPE__DATA_IDENTIFIER,
  },
  {
    .event = UDS_EVT_RoutineCtrl,
    .get_check = uds_get_check_for_routine_control,
    .get_action = uds_get_action_for_routine_control,
    .default_nrc = UDS_NRC_SubFunctionNotSupported,
    .registration_type = UDS_REGISTRATION_TYPE__ROUTINE_CONTROL,
  },
  {
    .event = UDS_EVT_SecAccessRequestSeed,
    .get_check = uds_get_check_for_security_access_request_seed,
    .get_action = uds_get_action_for_security_access_request_seed,
    .default_nrc = UDS_NRC_ConditionsNotCorrect,
    .registration_type = UDS_REGISTRATION_TYPE__SECURITY_ACCESS,
  },
  {
    .event = UDS_EVT_SecAccessValidateKey,
    .get_check = uds_get_check_for_security_access_validate_key,
    .get_action = uds_get_action_for_security_access_validate_key,
    .default_nrc = UDS_NRC_ConditionsNotCorrect,
    .registration_type = UDS_REGISTRATION_TYPE__SECURITY_ACCESS,
  },
  {
    .event = UDS_EVT_CommCtrl,
    .get_check = uds_get_check_for_communication_control,
    .get_action = uds_get_action_for_communication_control,
    .default_nrc = UDS_NRC_RequestOutOfRange,
    .registration_type = UDS_REGISTRATION_TYPE__COMMUNICATION_CONTROL,
  },
};

// Wraps the logic to check and execute action on the event
static UDSErr_t uds_check_and_act_on_event(
    struct uds_context* context,
    const struct uds_event_to_handler_mapping* handler,
    bool* found_at_least_one_match,
    bool* consume_event) {
  struct uds_registration_t* reg = context->registration;
  UDSErr_t ret = UDS_OK;

  if (reg->type != handler->registration_type) {
    *consume_event = false;
    return UDS_OK;
  }

  bool apply_action = false;
  uds_check_fn check = handler->get_check(reg);
  if (!check) {
    *consume_event = false;
    return ret;
  }
  ret = check(context, &apply_action);
  if (ret != UDS_OK) {
    LOG_WRN("Check failed for Registration at addr: %p. Err: %d", reg, ret);
    *consume_event = false;
    return ret;
  }

  uds_action_fn action = handler->get_action(reg);
  if (!apply_action || !action) {
    *consume_event = false;
    return ret;
  }

  ret = action(context, consume_event);
  if (ret != UDS_OK) {
    LOG_WRN("Action failed for Registration at addr: %p. Err: %d", reg, ret);
    return ret;
  }

  *found_at_least_one_match = true;
  return UDS_OK;
}

// Iterates over event handlers to apply the actions for the event
UDSErr_t uds_handle_event(struct uds_instance_t* instance,
                          UDSEvent_t event,
                          void* arg,
                          const struct uds_event_to_handler_mapping* handler) {
  bool found_at_least_one_match = false;

  // We start with static registrations
  STRUCT_SECTION_FOREACH (uds_registration_t, reg) {
    bool consume_event = true;

    struct uds_context context = {
      .instance = instance,
      .registration = reg,
      .server = &instance->iso14229.server,
      .event = event,
      .arg = arg,
    };

    int ret = uds_check_and_act_on_event(
        &context, handler, &found_at_least_one_match, &consume_event);
    if (consume_event || ret != UDS_OK) {
      return ret;
    }
  }

  // Optional dynamic registrations
#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION
  struct uds_registration_t* reg = instance->dynamic_registrations;
  while (reg != NULL) {
    bool consume_event = false;

    struct uds_context context = {
      .instance = instance,
      .registration = reg,
      .server = &instance->iso14229.server,
      .event = event,
      .arg = arg,
    };

    int ret = uds_check_and_act_on_event(
        &context, handler, &found_at_least_one_match, &consume_event);
    if (consume_event || ret != UDS_OK) {
      return ret;
    }

    reg = reg->next;
  }
#endif  // CONFIG_UDS_USE_DYNAMIC_REGISTRATION

  if (!found_at_least_one_match) {
    return handler->default_nrc;
  }

  return UDS_PositiveResponse;
}

// Callback registers on the iso14229 lib to receive UDS events
UDSErr_t uds_event_callback(struct iso14229_zephyr_instance* inst,
                            UDSEvent_t event,
                            void* arg,
                            void* user_context) {
  struct uds_instance_t* instance = user_context;

  // Look up the event in the handler mapping table
  for (size_t i = 0; i < ARRAY_SIZE(event_handler_mappings); i++) {
    if (event_handler_mappings[i].event == event) {
      return uds_handle_event(instance, event, arg, &event_handler_mappings[i]);
    }
  }

  // Event not supported
  return UDS_NRC_ServiceNotSupported;
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