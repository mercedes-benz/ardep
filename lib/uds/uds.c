/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
LOG_MODULE_REGISTER(uds, CONFIG_UDS_LOG_LEVEL);

#include <ardep/iso14229.h>
#include <ardep/uds.h>
#include <iso14229.h>

/**
 * @brief Associated events with other data required to handle them
 */
struct uds_event_handler_data {
  UDSEvent_t event;
  uds_get_check_fn get_check;
  uds_get_action_fn get_action;
  UDSErr_t default_nrc;
  enum uds_registration_type_t registration_type;
};

// Wraps the logic to check and execute action on the event
static UDSErr_t uds_check_and_act_on_event(
    struct uds_context* context,
    const struct uds_event_handler_data* handler,
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
                          const struct uds_event_handler_data* handler) {
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
  struct uds_registration_t* reg;
  SYS_SLIST_FOR_EACH_CONTAINER (&instance->dynamic_registrations, reg, node) {
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

  // Look up the event in the handler mapping section
  STRUCT_SECTION_FOREACH (uds_event_handler_data, handler) {
    if (handler->event == event) {
      return uds_handle_event(instance, event, arg, handler);
    }
  }

  // Event not supported
  return UDS_NRC_ServiceNotSupported;
}

#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION

// Find next available dynamic ID, starting from 1
// Returns 0 if no ID is available
// TODO: This is inefficient for large numbers of registrations
static uint32_t find_next_dynamic_id(struct uds_instance_t* inst) {
  // Find the next available ID starting from 1
  uint32_t candidate_id = 1;
  bool id_found = false;

  while (candidate_id != 0) {  // Check for overflow (UINT32_MAX + 1 = 0)
    bool id_in_use = false;

    // Check if this ID is already in use
    struct uds_registration_t* reg;
    SYS_SLIST_FOR_EACH_CONTAINER (&inst->dynamic_registrations, reg, node) {
      if (reg->dynamic_id == candidate_id) {
        id_in_use = true;
        break;
      }
    }

    if (!id_in_use) {
      id_found = true;
      break;
    }

    if (candidate_id == UINT32_MAX) {
      id_found = false;
      candidate_id = 0;
    } else {
      candidate_id++;
    }
  }

  if (!id_found) {
    return 0;  // No free ID available
  }

  return candidate_id;
}

// Registration function to dynamically register new handlers at runtime
// (Heap allocated)
static int uds_register_event_handler(struct uds_instance_t* inst,
                                      struct uds_registration_t registration,
                                      uint32_t* dynamic_id) {
  registration.instance = inst;

  uint32_t next_id = find_next_dynamic_id(inst);
  if (next_id == 0) {
    return -ENOSPC;  // No free ID available
  }

  struct uds_registration_t* heap_registration =
      k_malloc(sizeof(struct uds_registration_t));
  if (heap_registration == NULL) {
    return -ENOMEM;
  }

  *heap_registration = registration;

  // Never append a node that might contain garbage pointers
  heap_registration->node = (sys_snode_t){0};
  heap_registration->dynamic_id = next_id;

  sys_slist_append(&inst->dynamic_registrations, &heap_registration->node);

  // Return the assigned ID to the caller
  *dynamic_id = next_id;

  return 0;
}

int uds_unregister_event_handler(struct uds_instance_t* inst,
                                 uint32_t dynamic_id) {
  struct uds_registration_t* reg;
  struct uds_registration_t* tmp;

  SYS_SLIST_FOR_EACH_CONTAINER_SAFE (&inst->dynamic_registrations, reg, tmp,
                                     node) {
    if (reg->dynamic_id == dynamic_id) {
      /* Call custom unregister function if provided */
      if (reg->unregister_registration_fn) {
        int ret = reg->unregister_registration_fn(reg);
        if (ret < 0) {
          LOG_ERR(
              "Custom unregister function failed for registration ID %u: %d",
              dynamic_id, ret);
          return ret;
        }
      }

      /* Remove from list and free */
      bool removed =
          sys_slist_find_and_remove(&inst->dynamic_registrations, &reg->node);
      __ASSERT(removed, "node not found during remove (should not happen)");
      k_free(reg);
      return 0;
    }
  }

  return -ENOENT;  // Registration not found
}

#endif  //  CONFIG_UDS_USE_DYNAMIC_REGISTRATION

int uds_init(struct uds_instance_t* inst,
             const UDSISOTpCConfig_t* iso_tp_config,
             const struct device* can_dev,
             void* user_context) {
  inst->user_context = user_context;

#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION
  sys_slist_init(&inst->dynamic_registrations);
  inst->register_event_handler = uds_register_event_handler;
  inst->unregister_event_handler = uds_unregister_event_handler;
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