/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_UDS_MACRO_H
#define ARDEP_UDS_MACRO_H

#ifndef _UDS_CAT
#define _UDS_CAT(a, b) a##b
#endif

#ifndef _UDS_CAT_EXPAND
#define _UDS_CAT_EXPAND(a, b) _UDS_CAT(a, b)
#endif

//////////////// MEMORY BY ADDRESS ///////////////////

// clang-format off

/**
 * @brief Register a new memory by address event handler
 * 
 * @param _instance Pointer to associated the UDS server instance
 * @param _context Optional context provided by the user
 * @param _read_check Check if the `_read` action should be executed
 * @param _read Execute a read for the event
 * @param _write_check Check if the `_write` action should be executed
 * @param _write Execute a write for the event
 */
#define UDS_REGISTER_MEMORY_HANDLER(                                          \
  _instance,                                                                  \
  _context,                                                                   \
  _read_check,                                                                \
  _read,                                                                      \
  _write_check,                                                               \
  _write                                                                      \
)                                                                             \
  STRUCT_SECTION_ITERABLE(uds_registration_t,                             \
      /* Use a counter to generate unique names for the iterable section */   \
        _UDS_CAT_EXPAND(__uds_registration_id_memory_, __COUNTER__)) = {  \
    .instance = _instance,                                                    \
    .type = UDS_REGISTRATION_TYPE__MEMORY,                                \
    .applies_to_event = uds_filter_for_memory_by_addr,                    \
    .user_data = _context,                                                    \
    .memory = {                                                               \
      .read = {                                                               \
        .check = _read_check,                                                 \
        .action = _read,                                                      \
      },                                                                      \
      .write = {                                                              \
        .check = _write_check,                                                \
        .action = _write,                                                     \
      },                                                                      \
    }                                                                         \
  };

/**
 * @brief Register memory by address event handler with ability to read/write
 *        to/from flash and ram.
 */  
#define UDS_REGISTER_MEMORY_DEFAULT_HANDLER(_instance)                        \
  UDS_REGISTER_MEMORY_HANDLER(                                                \
    _instance,                                                                \
    NULL,                                                                     \
    uds_check_default_memory_by_addr_read,                                \
    uds_action_default_memory_by_addr_read,                               \
    uds_check_default_memory_by_addr_write,                               \
    uds_action_default_memory_by_addr_write                               \
  )

// clang-format on

//////////////// ECU RESET ///////////////////

// clang-format off

/**
 * @brief Register a new ecu reset event handler
 * 
 * @param _instance Pointer to associated the UDS server instance
 * @param _context Optional context provided by the user
 * @param _reset_type type of reset as defined in ISO 14229-1 10.3.2.1
 * @param _ecu_reset_check Check if the `_ecu_reset` action should be executed
 * @param _ecu_reset Execute the `_ecu_reset` action for the event
 * @param _do_scheduled_reset_check Check if the `_do_scheduled_reset` action
 *        should be executed
 * @param _do_scheduled_reset Execute the `_do_scheduled_reset` action for the
 *        event
 */
#define UDS_REGISTER_ECU_RESET_HANDLER(                                       \
  _instance,                                                                  \
  _context,                                                                   \
  _reset_type,                                                                \
  _ecu_reset_check,                                                           \
  _ecu_reset,                                                                 \
  _do_scheduled_reset_check,                                                  \
  _do_scheduled_reset                                                         \
)                                                                             \
  STRUCT_SECTION_ITERABLE(uds_registration_t,                             \
        _UDS_CAT_EXPAND(__uds_registration_id, _reset_type)) = {          \
    .instance = _instance,                                                    \
    .type = UDS_REGISTRATION_TYPE__ECU_RESET,                             \
    .applies_to_event = uds_filter_for_ecu_reset_event,                   \
    .user_data = _context,                                                    \
    .ecu_reset = {                                                            \
      .type = _reset_type,                                                    \
      .ecu_reset = {                                                          \
        .check = _ecu_reset_check,                                            \
        .action = _ecu_reset,                                                 \
      },                                                                      \
      .execute_scheduled_reset = {                                            \
        .check = _do_scheduled_reset_check,                                   \
        .action = _do_scheduled_reset,                                        \
      }                                                                       \
    }                                                                         \
  };

/**
 * @brief Register the default ECU Reset event handler for a hard reset.
 * 
 * @param _instance Pointer to associated the UDS server instance
 */
#define UDS_REGISTER_ECU_HARD_RESET_HANDLER(                                 \
  _instance                                                                  \
)                                                                            \
  UDS_REGISTER_ECU_RESET_HANDLER(                                            \
    _instance,                                                               \
    NULL,                                                                    \
    ECU_RESET__HARD,                                                         \
    uds_check_ecu_hard_reset,                                            \
    uds_action_ecu_hard_reset,                                           \
    uds_check_execute_scheduled_reset,                                   \
    uds_action_execute_scheduled_reset                                   \
  )

// clang-format on

//////////////// READ/WRITE BY IDENTIFIER ///////////////////

// clang-format off

/**
 * @brief Register a new static data identifier
 * 
 * @param _instance Pointer to associated the UDS server instance
 * @param _data_id The data identifier to register the handler for
 * @param data_ptr Custom context oder data the handle the event
 * @param _read_check Check if the `_read` action should be executed
 * @param _read Execute a read for the event
 * @param _write_check Check if the `_write` action should be executed
 * @param _write Execute a write for the event
 * @param _context Optional context provided by the user
 * 
 * @note: @p _write_check and @p _write are optional. Set to NULL for read-only
 *        data identifier
 */
#define UDS_REGISTER_DATA_IDENTIFIER_STATIC(                          \
  _instance,                                                          \
  _data_id,                                                           \
  data_ptr,                                                           \
  _read_check,                                                        \
  _read,                                                              \
  _write_check,                                                       \
  _write,                                                             \
  _context                                                            \
)                                                                     \
  STRUCT_SECTION_ITERABLE(uds_registration_t,                     \
        _UDS_CAT_EXPAND(__uds_registration_id, _data_id)) = {     \
    .instance = _instance,                                            \
    .type = UDS_REGISTRATION_TYPE__DATA_IDENTIFIER,               \
    .applies_to_event = uds_filter_for_data_by_id_event,          \
    .user_data = data_ptr,                                            \
    .data_identifier = {                                              \
      .user_context = _context,                                       \
      .data_id = _data_id,                                            \
      .read = {                                                       \
        .check = _read_check,                                         \
        .action = _read,                                              \
      },                                                              \
      .write = {                                                      \
        .check = _write_check,                                        \
        .action = _write,                                             \
      },                                                              \
    },                                                                \
  };

// clang-format on

#endif  // ARDEP_UDS_MACRO_H
