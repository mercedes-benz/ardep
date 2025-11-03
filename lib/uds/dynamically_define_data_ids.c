/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION

/* Requires: a handler for read_memory_by_address and read_data_by_id */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include "ardep/uds.h"
#include "iso14229.h"
#include "uds.h"
#include "zephyr/kernel.h"
#include "zephyr/sys/slist.h"
#include "zephyr/sys/util.h"

#include <stdint.h>

uds_check_fn uds_get_check_for_dynamically_define_data_ids(
    const struct uds_registration_t* const reg) {
  return reg->dynamically_define_data_ids.actor.check;
}
uds_action_fn uds_get_action_for_dynamically_define_data_ids(
    const struct uds_registration_t* const reg) {
  return reg->dynamically_define_data_ids.actor.action;
}

STRUCT_SECTION_ITERABLE(
    uds_event_handler_data,
    __uds_event_handler_data_dynamically_define_data_ids_) = {
  .event = UDS_EVT_DynamicDefineDataId,
  .get_check = uds_get_check_for_dynamically_define_data_ids,
  .get_action = uds_get_action_for_dynamically_define_data_ids,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__DYNAMIC_DEFINE_DATA_IDS,
};

UDSErr_t uds_check_default_dynamically_define_data_ids(
    const struct uds_context* const context, bool* apply_action) {
  UDSDDDIArgs_t* args = context->arg;
  if (args->type == UDS_DYNAMICALLY_DEFINED_DATA_IDS__DEFINE_BY_DATA_ID ||
      args->type ==
          UDS_DYNAMICALLY_DEFINED_DATA_IDS__DEFINE_BY_MEMORY_ADDRESS ||
      args->type == UDS_DYNAMICALLY_DEFINED_DATA_IDS__CLEAR) {
    *apply_action = true;
  }
  return UDS_OK;
}

static UDSErr_t uds_dynamic_data_by_id_read_data_by_id_check(
    const struct uds_context* const context, bool* apply_action) {
  UDSRDBIArgs_t* args = context->arg;
  if (context->registration->data_identifier.data_id == args->dataId) {
    *apply_action = true;
  }
  return UDS_OK;
}

static uint8_t temp_buffer[256];
static size_t temp_buffer_len = 0;

uint8_t uds_copy(UDSServer_t* srv, const void* src, uint16_t count) {
  if (srv == NULL) {
    return UDS_NRC_GeneralReject;
  }
  if (src == NULL) {
    return UDS_NRC_GeneralReject;
  }
  if (count <= sizeof(temp_buffer) - temp_buffer_len) {
    memmove(temp_buffer + temp_buffer_len, src, count);
    temp_buffer_len += count;
    return UDS_PositiveResponse;
  }
  return UDS_NRC_ResponseTooLong;
}

static UDSErr_t uds_dynamic_data_by_id_read_data_by_id_action(
    struct uds_context* context, bool* consume_event) {
  UDSRDBIArgs_t* parent_read_args = context->arg;

  sys_slist_t* list = context->registration->data_identifier.data;

  // Here we simulate an event for every data ID that makes up the dynamic
  // identifier. This ensures it will be handled exactly the same as if it was
  // read by the UDS service.
  sys_snode_t* node;
  SYS_SLIST_FOR_EACH_NODE (list, node) {
    struct uds_dynamically_defined_data* data =
        CONTAINER_OF(node, struct uds_dynamically_defined_data, node);

    if (data->type == UDS_DYNAMICALLY_DEFINED_DATA_TYPE__ID) {
      UDSRDBIArgs_t child_args = {.dataId = data->id.id, .copy = uds_copy};

      temp_buffer_len = 0;

      int ret = uds_event_callback(&context->instance->iso14229,
                                   UDS_EVT_ReadDataByIdent, &child_args,
                                   context->instance);
      if (ret != UDS_PositiveResponse) {
        return ret;
      }

      if (data->id.position + data->id.size > temp_buffer_len) {
        LOG_WRN(
            "Not enough data returned for data ID 0x%04X to satisfy configured "
            "dynamic data identifier 0x%04X",
            data->id.id, parent_read_args->dataId);
        return UDS_NRC_GeneralReject;
      }

      ret = parent_read_args->copy(
          context->server, temp_buffer + data->id.position, data->id.size);
      if (ret != UDS_OK) {
        return ret;
      }
    }

    if (data->type == UDS_DYNAMICALLY_DEFINED_DATA_TYPE__MEMORY) {
      UDSReadMemByAddrArgs_t child_args = {.memAddr = data->memory.memAddr,
                                           .memSize = data->memory.memSize,
                                           .copy = parent_read_args->copy};

      int ret = uds_event_callback(&context->instance->iso14229,
                                   UDS_EVT_ReadMemByAddr, &child_args,
                                   context->instance);
      if (ret != UDS_PositiveResponse) {
        return ret;
      }
    }
  }

  *consume_event = true;
  return UDS_OK;
}

static int uds_unregister_dynamic_identifier(struct uds_registration_t* this) {
  sys_slist_t* list = this->data_identifier.data;

  sys_snode_t* node;
  sys_snode_t* next_node;
  SYS_SLIST_FOR_EACH_NODE_SAFE (list, node, next_node) {
    struct uds_dynamically_defined_data* data =
        CONTAINER_OF(node, struct uds_dynamically_defined_data, node);
    // Free data list item to hold sub-item [3]
    k_free(data);  // FREE: [DATA_ITEM] - normal cleanup via unregister
  }
  // Free data list for sub-items to be read [1]
  k_free(list);  // FREE: [DATA_LIST] - normal cleanup via unregister

  return 0;
}

static UDSErr_t uds_append_dynamic_data_identifiers_to_registration(
    UDSDDDIArgs_t* args, sys_slist_t* data_list) {
  uint16_t id = args->subFuncArgs.defineById.sourceDataId;
  uint8_t position = args->subFuncArgs.defineById.position;
  uint8_t size = args->subFuncArgs.defineById.size;

  // Allocate data list item to hold sub-item [DATA_ITEM]
  // FREED: [DATA_ITEM] via uds_unregister_dynamic_identifier, error cleanup,
  // or caller error cleanup
  struct uds_dynamically_defined_data* data =
      k_malloc(sizeof(struct uds_dynamically_defined_data));
  if (!data) {
    // Cleanup all previously allocated items in this list
    sys_snode_t* node;
    sys_snode_t* next_node;
    SYS_SLIST_FOR_EACH_NODE_SAFE (data_list, node, next_node) {
      struct uds_dynamically_defined_data* cleanup_data =
          CONTAINER_OF(node, struct uds_dynamically_defined_data, node);
      sys_slist_remove(data_list, NULL, &cleanup_data->node);
      k_free(cleanup_data);  // FREE: [DATA_ITEM] - error cleanup in append
                             // function
    }
    LOG_ERR("Failed to allocate memory for dynamic data identifier");
    return UDS_NRC_GeneralReject;
  }

  data->type = UDS_DYNAMICALLY_DEFINED_DATA_TYPE__ID;
  data->id.id = id;
  data->id.position = position;
  data->id.size = size;
  data->node = (sys_snode_t){0};

  sys_slist_append(data_list, &data->node);
  return UDS_OK;
}

static UDSErr_t uds_append_dynamic_memory_addresses_to_registration(
    UDSDDDIArgs_t* args, sys_slist_t* data_list) {
  void* memAddr = args->subFuncArgs.defineByMemAddress.memAddr;
  size_t memSize = args->subFuncArgs.defineByMemAddress.memSize;

  // Allocate data list item to hold sub-item [DATA_ITEM]
  // FREED: [DATA_ITEM] via uds_unregister_dynamic_identifier, error cleanup,
  // or caller error cleanup
  struct uds_dynamically_defined_data* data =
      k_malloc(sizeof(struct uds_dynamically_defined_data));
  if (!data) {
    // Cleanup all previously allocated items in this list
    sys_snode_t* node;
    sys_snode_t* next_node;
    SYS_SLIST_FOR_EACH_NODE_SAFE (data_list, node, next_node) {
      struct uds_dynamically_defined_data* cleanup_data =
          CONTAINER_OF(node, struct uds_dynamically_defined_data, node);
      sys_slist_remove(data_list, NULL, &cleanup_data->node);
      k_free(cleanup_data);  // FREE: [DATA_ITEM] - error cleanup in append
                             // function
    }
    LOG_ERR("Failed to allocate memory for dynamic memory address");
    return UDS_NRC_GeneralReject;
  }

  data->type = UDS_DYNAMICALLY_DEFINED_DATA_TYPE__MEMORY;
  data->memory.memAddr = memAddr;
  data->memory.memSize = memSize;
  data->node = (sys_snode_t){0};

  sys_slist_append(data_list, &data->node);
  return UDS_OK;
}

static int uds_find_existing_registration_by_data_id(
    struct uds_context* const context,
    uint16_t data_id,
    struct uds_registration_t** read_data_by_id_reg) {
  struct uds_registration_t* temp_reg;
  SYS_SLIST_FOR_EACH_CONTAINER (&context->instance->dynamic_registrations,
                                temp_reg, node) {
    if (temp_reg->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER) {
      if (temp_reg->data_identifier.data_id == data_id) {
        struct dynamic_registration_id_sll_item* dynamic_registration_id_item;
        SYS_SLIST_FOR_EACH_CONTAINER (
            &context->registration->dynamically_define_data_ids
                 .dynamic_registration_id_list,
            dynamic_registration_id_item, node) {
          if (dynamic_registration_id_item->dynamic_registration_id ==
              temp_reg->dynamic_registration_id) {
            *read_data_by_id_reg = temp_reg;
            return 0;
          }
        }
      }
    }
  }

  return -1;
}

static int uds_create_new_data_identifier_by_id(
    struct uds_context* const context,
    uint16_t data_id,
    struct uds_registration_t** read_data_by_id_reg_ptr) {
  // Allocating data list for sub-items to be read [DATA_LIST]
  // FREED: [DATA_LIST] via uds_unregister_dynamic_identifier or error cleanup
  sys_slist_t* data_list = k_malloc(sizeof(sys_slist_t));
  if (!data_list) {
    return -1;
  }
  sys_slist_init(data_list);

  // Allocate new temporary registration item [TEMP_REG]
  // FREED: [TEMP_REG] after successful registration or via error cleanup
  *read_data_by_id_reg_ptr = k_malloc(sizeof(struct uds_registration_t));
  if (*read_data_by_id_reg_ptr == NULL) {
    // Free [DATA_LIST]
    k_free(data_list);  // FREE: [DATA_LIST] - error cleanup in create function
    return -2;
  }

  struct uds_registration_t* read_data_by_id_reg = *read_data_by_id_reg_ptr;
  memset(read_data_by_id_reg, 0, sizeof(*read_data_by_id_reg));

  read_data_by_id_reg->type = UDS_REGISTRATION_TYPE__DATA_IDENTIFIER;
  read_data_by_id_reg->instance = context->instance;
  read_data_by_id_reg->unregister_registration_fn =
      uds_unregister_dynamic_identifier;
  read_data_by_id_reg->data_identifier.data = data_list;
  read_data_by_id_reg->data_identifier.data_id = data_id;
  read_data_by_id_reg->data_identifier.read.check =
      uds_dynamic_data_by_id_read_data_by_id_check;
  read_data_by_id_reg->data_identifier.read.action =
      uds_dynamic_data_by_id_read_data_by_id_action;

  return 0;
}

static UDSErr_t uds_register_new_data_by_id_item(
    struct uds_context* const context,
    struct uds_registration_t* read_data_by_id_reg) {
  uint32_t dynamic_id;
  struct uds_registration_t* registration_out;
  int ret = context->instance->register_event_handler(
      context->instance, *read_data_by_id_reg, &dynamic_id, &registration_out);
  if (ret < 0) {
    LOG_ERR("Failed to register dynamic data identifier. ERR: %d", ret);
    return ret;
  }

  // Allocate node to hold dynamic registration ID's for the dynamically defined
  // data ids event handler [REG_ID_ITEM]
  // FREED: [REG_ID_ITEM] when removing single ID or removing all IDs
  struct dynamic_registration_id_sll_item* registration_item =
      k_malloc(sizeof(struct dynamic_registration_id_sll_item));
  if (registration_item == NULL) {
    LOG_ERR("Failed to allocate memory for dynamic registration ID item.");
    // Cleanup: unregister the event handler we just registered
    context->instance->unregister_event_handler(context->instance, dynamic_id);
    return UDS_NRC_GeneralReject;
  }
  memset(registration_item, 0, sizeof(*registration_item));

  sys_slist_t* identifier_list =
      &context->registration->dynamically_define_data_ids
           .dynamic_registration_id_list;
  registration_item->dynamic_registration_id = dynamic_id;

  sys_slist_append(identifier_list, &registration_item->node);
  // Free new temporary registration item [TEMP_REG]
  k_free(
      read_data_by_id_reg);  // FREE: [TEMP_REG] - after successful registration

  return UDS_OK;
}

static UDSErr_t uds_dynamicallY_define_data_by_id_add_new_id(
    struct uds_context* const context, bool* consume_event) {
  UDSDDDIArgs_t* args = context->arg;

  struct uds_registration_t* read_data_by_id_reg = NULL;
  int ret = uds_find_existing_registration_by_data_id(
      context, args->dynamicDataId, &read_data_by_id_reg);

  bool is_existing_registration = ret == 0;

  if (!is_existing_registration) {
    ret = uds_create_new_data_identifier_by_id(context, args->dynamicDataId,
                                               &read_data_by_id_reg);
    if (ret < 0) {
      LOG_ERR("Failed to create new data identifier registration. ERR: %d",
              ret);
      return UDS_NRC_GeneralReject;
    }
  }

  sys_slist_t* data_list = read_data_by_id_reg->data_identifier.data;

  ret = uds_append_dynamic_data_identifiers_to_registration(args, data_list);
  if (ret != UDS_OK) {
    if (!is_existing_registration) {
      // If we created a new registration but failed to add data, clean it up
      k_free(read_data_by_id_reg->data_identifier
                 .data);  // FREE: [DATA_LIST] - error cleanup in caller
      k_free(
          read_data_by_id_reg);  // FREE: [TEMP_REG] - error cleanup in caller
    }
    return ret;
  }

  if (!is_existing_registration) {
    ret = uds_register_new_data_by_id_item(context, read_data_by_id_reg);
    if (ret != UDS_OK) {
      // Clean up the data list and registration we created
      sys_snode_t* node;
      sys_snode_t* next_node;
      SYS_SLIST_FOR_EACH_NODE_SAFE (data_list, node, next_node) {
        struct uds_dynamically_defined_data* cleanup_data =
            CONTAINER_OF(node, struct uds_dynamically_defined_data, node);
        sys_slist_remove(data_list, NULL, &cleanup_data->node);
        k_free(cleanup_data);  // FREE: [DATA_ITEM] - error cleanup in caller
      }
      k_free(read_data_by_id_reg->data_identifier
                 .data);  // FREE: [DATA_LIST] - error cleanup in caller
      k_free(
          read_data_by_id_reg);  // FREE: [TEMP_REG] - error cleanup in caller
      return ret;
    }
  }

  *consume_event = true;
  return UDS_OK;
}

static UDSErr_t uds_dynamicallY_define_data_by_memory_address_add_new_id(
    struct uds_context* const context, bool* consume_event) {
  UDSDDDIArgs_t* args = context->arg;

  struct uds_registration_t* read_data_by_id_reg = NULL;
  int ret = uds_find_existing_registration_by_data_id(
      context, args->dynamicDataId, &read_data_by_id_reg);

  bool is_existing_registration = ret == 0;

  if (!is_existing_registration) {
    ret = uds_create_new_data_identifier_by_id(context, args->dynamicDataId,
                                               &read_data_by_id_reg);
    if (ret < 0) {
      LOG_ERR("Failed to create new data identifier registration. ERR: %d",
              ret);
      return UDS_NRC_GeneralReject;
    }
  }

  sys_slist_t* data_list = read_data_by_id_reg->data_identifier.data;

  ret = uds_append_dynamic_memory_addresses_to_registration(args, data_list);
  if (ret != UDS_OK) {
    if (!is_existing_registration) {
      // If we created a new registration but failed to add data, clean it up
      k_free(read_data_by_id_reg->data_identifier
                 .data);  // FREE: [DATA_LIST] - error cleanup in caller
      k_free(
          read_data_by_id_reg);  // FREE: [TEMP_REG] - error cleanup in caller
    }
    return ret;
  }

  if (!is_existing_registration) {
    ret = uds_register_new_data_by_id_item(context, read_data_by_id_reg);
    if (ret != UDS_OK) {
      // Clean up the data list and registration we created
      sys_snode_t* node;
      sys_snode_t* next_node;
      SYS_SLIST_FOR_EACH_NODE_SAFE (data_list, node, next_node) {
        struct uds_dynamically_defined_data* cleanup_data =
            CONTAINER_OF(node, struct uds_dynamically_defined_data, node);
        sys_slist_remove(data_list, NULL, &cleanup_data->node);
        k_free(cleanup_data);  // FREE: [DATA_ITEM] - error cleanup in caller
      }
      k_free(read_data_by_id_reg->data_identifier
                 .data);  // FREE: [DATA_LIST] - error cleanup in caller
      k_free(
          read_data_by_id_reg);  // FREE: [TEMP_REG] - error cleanup in caller
      return ret;
    }
  }

  *consume_event = true;
  return UDS_OK;
}

static void uds_dynamicallY_define_data_by_id_remove_single_id(
    struct uds_instance_t* instance, uint16_t data_id) {
  uint32_t dynamic_id_to_remove = 0;

  // Remove the data ID from the dynamic registrations
  {
    struct uds_registration_t* dynamic_reg;
    SYS_SLIST_FOR_EACH_CONTAINER (&instance->dynamic_registrations, dynamic_reg,
                                  node) {
      LOG_INF("dynamic registration with dynamic id: 0x%04X",
              dynamic_reg->dynamic_registration_id);

      if (dynamic_reg->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER &&
          dynamic_reg->data_identifier.data_id == data_id) {
        dynamic_id_to_remove = dynamic_reg->dynamic_registration_id;
        LOG_INF("Found dynamic registration ID to remove: %u",
                dynamic_id_to_remove);
        int ret =
            instance->unregister_event_handler(instance, dynamic_id_to_remove);
        if (ret != 0) {
          LOG_WRN(
              "Failed to unregister dynamic registration identifier ID "
              "%u. "
              "ERR: %d",
              dynamic_id_to_remove, ret);
        }
        break;  // Found and processed the registration, exit the loop
      }
    }
  }

  // Remove the dynamic_id of the removed registration item from the
  // list of the dynamically defined data ids event handler
  {
    STRUCT_SECTION_FOREACH (uds_registration_t, reg) {
      if (reg->type == UDS_REGISTRATION_TYPE__DYNAMIC_DEFINE_DATA_IDS) {
        struct dynamic_registration_id_sll_item* item;
        struct dynamic_registration_id_sll_item* next;

        SYS_SLIST_FOR_EACH_CONTAINER_SAFE (
            &reg->dynamically_define_data_ids.dynamic_registration_id_list,
            item, next, node) {
          if (item->dynamic_registration_id == dynamic_id_to_remove) {
            sys_slist_find_and_remove(
                &reg->dynamically_define_data_ids.dynamic_registration_id_list,
                &item->node);
            k_free(item);  // FREE: [REG_ID_ITEM] - remove single ID from static
                           // registrations
          }
        }
      }
    }

    struct uds_registration_t* dynamic_reg;
    SYS_SLIST_FOR_EACH_CONTAINER (&instance->dynamic_registrations, dynamic_reg,
                                  node) {
      if (dynamic_reg->type == UDS_REGISTRATION_TYPE__DYNAMIC_DEFINE_DATA_IDS) {
        struct dynamic_registration_id_sll_item* item;
        struct dynamic_registration_id_sll_item* next;

        SYS_SLIST_FOR_EACH_CONTAINER_SAFE (
            &dynamic_reg->dynamically_define_data_ids
                 .dynamic_registration_id_list,
            item, next, node) {
          if (item->dynamic_registration_id == dynamic_id_to_remove) {
            sys_slist_find_and_remove(&dynamic_reg->dynamically_define_data_ids
                                           .dynamic_registration_id_list,
                                      &item->node);
            k_free(item);  // FREE: [REG_ID_ITEM] - remove single ID from
                           // dynamic registrations
          }
        }
      }
    }
  }
}

static void uds_dynamicallY_define_data_by_id_remove_all_ids(
    struct uds_instance_t* instance) {
  // Remove all dynamic registrations referenced by static registrations
  STRUCT_SECTION_FOREACH (uds_registration_t, reg) {
    if (reg->type == UDS_REGISTRATION_TYPE__DYNAMIC_DEFINE_DATA_IDS) {
      struct dynamic_registration_id_sll_item* item;
      SYS_SLIST_FOR_EACH_CONTAINER (
          &reg->dynamically_define_data_ids.dynamic_registration_id_list, item,
          node) {
        uint32_t dynamic_id_to_remove = item->dynamic_registration_id;

        LOG_INF("Removing dynamic registration with dynamic id: 0x%04X",
                dynamic_id_to_remove);

        int ret =
            instance->unregister_event_handler(instance, dynamic_id_to_remove);
        if (ret != 0) {
          LOG_WRN(
              "Failed to unregister dynamic registration identifier ID %u. "
              "ERR: %d",
              dynamic_id_to_remove, ret);
        }
      }
    }
  }

  // Remove all dynamic registrations referenced by dynamic registrations
  struct uds_registration_t* dynamic_reg;
  SYS_SLIST_FOR_EACH_CONTAINER (&instance->dynamic_registrations, dynamic_reg,
                                node) {
    if (dynamic_reg->type == UDS_REGISTRATION_TYPE__DYNAMIC_DEFINE_DATA_IDS) {
      struct dynamic_registration_id_sll_item* item;
      SYS_SLIST_FOR_EACH_CONTAINER (&dynamic_reg->dynamically_define_data_ids
                                         .dynamic_registration_id_list,
                                    item, node) {
        uint32_t dynamic_id_to_remove = item->dynamic_registration_id;

        LOG_INF("Removing dynamic registration with dynamic id: 0x%04X",
                dynamic_id_to_remove);

        int ret =
            instance->unregister_event_handler(instance, dynamic_id_to_remove);
        if (ret != 0) {
          LOG_WRN(
              "Failed to unregister dynamic registration identifier ID %u. "
              "ERR: %d",
              dynamic_id_to_remove, ret);
        }
      }
    }
  }

  // Clean up dynamic_registration_id_list entries from all static event
  // handlers
  STRUCT_SECTION_FOREACH (uds_registration_t, reg) {
    if (reg->type == UDS_REGISTRATION_TYPE__DYNAMIC_DEFINE_DATA_IDS) {
      struct dynamic_registration_id_sll_item* item;
      struct dynamic_registration_id_sll_item* next;

      SYS_SLIST_FOR_EACH_CONTAINER_SAFE (
          &reg->dynamically_define_data_ids.dynamic_registration_id_list, item,
          next, node) {
        sys_slist_find_and_remove(
            &reg->dynamically_define_data_ids.dynamic_registration_id_list,
            &item->node);
        k_free(item);  // FREE: [REG_ID_ITEM] - remove all IDs from static
                       // registrations
      }
    }
  }

  // Clean up dynamic_registration_id_list entries from all dynamic event
  // handlers
  SYS_SLIST_FOR_EACH_CONTAINER (&instance->dynamic_registrations, dynamic_reg,
                                node) {
    if (dynamic_reg->type == UDS_REGISTRATION_TYPE__DYNAMIC_DEFINE_DATA_IDS) {
      struct dynamic_registration_id_sll_item* item;
      struct dynamic_registration_id_sll_item* next;

      SYS_SLIST_FOR_EACH_CONTAINER_SAFE (
          &dynamic_reg->dynamically_define_data_ids
               .dynamic_registration_id_list,
          item, next, node) {
        sys_slist_find_and_remove(&dynamic_reg->dynamically_define_data_ids
                                       .dynamic_registration_id_list,
                                  &item->node);
        k_free(item);  // FREE: [REG_ID_ITEM] - remove all IDs from dynamic
                       // registrations
      }
    }
  }
}

static UDSErr_t uds_dynamicallY_define_data_by_id_remove_ids(
    struct uds_context* const context, bool* consume_event) {
  UDSDDDIArgs_t* args = context->arg;

  if (args->allDataIds) {
    uds_dynamicallY_define_data_by_id_remove_all_ids(context->instance);
  } else {
    uds_dynamicallY_define_data_by_id_remove_single_id(context->instance,
                                                       args->dynamicDataId);
  }

  *consume_event = true;
  return UDS_OK;
}

UDSErr_t uds_action_default_dynamically_define_data_ids(
    struct uds_context* const context, bool* consume_event) {
  UDSDDDIArgs_t* args = context->arg;
  switch (args->type) {
    case UDS_DYNAMICALLY_DEFINED_DATA_IDS__DEFINE_BY_DATA_ID:
      return uds_dynamicallY_define_data_by_id_add_new_id(context,
                                                          consume_event);
    case UDS_DYNAMICALLY_DEFINED_DATA_IDS__DEFINE_BY_MEMORY_ADDRESS:
      return uds_dynamicallY_define_data_by_memory_address_add_new_id(
          context, consume_event);
    case UDS_DYNAMICALLY_DEFINED_DATA_IDS__CLEAR:
      return uds_dynamicallY_define_data_by_id_remove_ids(context,
                                                          consume_event);
    default:
      return UDS_NRC_SubFunctionNotSupported;
  }
  return UDS_OK;
}

#endif  // CONFIG_UDS_USE_DYNAMIC_REGISTRATION
