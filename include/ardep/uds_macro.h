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

// #region READ_DTC_INFORMATION

// clang-format off

/**
 * @brief Register a new read dtc information event handler
 * 
 * @param _instance Pointer to associated the UDS server instance
 * @param _context Optional context provided by the user
 * @param _check Check if the `_act` action should be executed
 * @param _act Execute the event handler for the event
 * @param _subfunc_id The subfunction ID to register the handler for
 */
#define UDS_REGISTER_READ_DTC_INFO_HANDLER(                                         \
  _instance,                                                                        \
  _context,                                                                         \
  _check,                                                                           \
  _act,                                                                             \
  _subfunc_id                                                                       \
)                                                                                   \
  STRUCT_SECTION_ITERABLE(uds_registration_t,                                       \
      /* Use a counter to generate unique names for the iterable section */         \
        _UDS_CAT_EXPAND(__uds_registration_id_read_dtc_info, __COUNTER__)) = {      \
    .instance = _instance,                                                          \
    .type = UDS_REGISTRATION_TYPE__READ_DTC_INFO,                                   \
    .applies_to_event = uds_filter_for_read_dtc_info_event,                         \
    .user_data = _context,                                                          \
    .read_dtc = {                                                                   \
      .sub_function = _subfunc_id,                                                  \
      .actor = {                                                                    \
        .check = _check,                                                            \
        .action = _act,                                                             \
      },                                                                            \
    }                                                                               \
  };

// clang-format on

/* Count 1..24 varargs */
#define _UDS_VA_NARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, \
                           _13, _14, _15, _16, _17, _18, _19, _20, _21, _22,  \
                           _23, _24, N, ...)                                  \
  N
#define _UDS_VA_NARGS(...)                                                    \
  _UDS_VA_NARGS_IMPL(__VA_ARGS__, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, \
                     13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

/* Base “apply one” primitive */
#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_ONE(_inst, _ctx, _chk, _act, \
                                                      _sf)                     \
  UDS_REGISTER_READ_DTC_INFO_HANDLER(_inst, _ctx, _chk, _act, _sf)

/* Unrolled fan-out (1..27). Each line ends up at file scope just like the
 * original) */
#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s1) \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_ONE(i, c, k, a, s1)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_2(i, c, k, a, s1, s2) \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s1)           \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s2)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_3(i, c, k, a, s1, s2, s3) \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_2(i, c, k, a, s1, s2)           \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s3)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_4(i, c, k, a, s1, s2, s3, \
                                                    s4)                     \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_3(i, c, k, a, s1, s2, s3)       \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s4)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_5(i, c, k, a, s1, s2, s3, \
                                                    s4, s5)                 \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_4(i, c, k, a, s1, s2, s3, s4)   \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s5)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_6(i, c, k, a, s1, s2, s3,   \
                                                    s4, s5, s6)               \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_5(i, c, k, a, s1, s2, s3, s4, s5) \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s6)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_7(i, c, k, a, s1, s2, s3,   \
                                                    s4, s5, s6, s7)           \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_6(i, c, k, a, s1, s2, s3, s4, s5, \
                                              s6)                             \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s7)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_8(i, c, k, a, s1, s2, s3,   \
                                                    s4, s5, s6, s7, s8)       \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_7(i, c, k, a, s1, s2, s3, s4, s5, \
                                              s6, s7)                         \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s8)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_9(i, c, k, a, s1, s2, s3,   \
                                                    s4, s5, s6, s7, s8, s9)   \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_8(i, c, k, a, s1, s2, s3, s4, s5, \
                                              s6, s7, s8)                     \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s9)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_10(                         \
    i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10)                      \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_9(i, c, k, a, s1, s2, s3, s4, s5, \
                                              s6, s7, s8, s9)                 \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s10)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_11(                          \
    i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11)                  \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_10(i, c, k, a, s1, s2, s3, s4, s5, \
                                               s6, s7, s8, s9, s10)            \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s11)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_12(                          \
    i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12)             \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_11(i, c, k, a, s1, s2, s3, s4, s5, \
                                               s6, s7, s8, s9, s10, s11)       \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s12)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_13(                          \
    i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13)        \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_12(i, c, k, a, s1, s2, s3, s4, s5, \
                                               s6, s7, s8, s9, s10, s11, s12)  \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s13)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_14(                        \
    i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14) \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_13(                              \
      i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13)    \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s14)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_15(                          \
    i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14,   \
    s15)                                                                       \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_14(                                \
      i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14) \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s15)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_16(                          \
    i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14,   \
    s15, s16)                                                                  \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_15(i, c, k, a, s1, s2, s3, s4, s5, \
                                               s6, s7, s8, s9, s10, s11, s12,  \
                                               s13, s14, s15)                  \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s16)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_17(                          \
    i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14,   \
    s15, s16, s17)                                                             \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_16(i, c, k, a, s1, s2, s3, s4, s5, \
                                               s6, s7, s8, s9, s10, s11, s12,  \
                                               s13, s14, s15, s16)             \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s17)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_18(                          \
    i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14,   \
    s15, s16, s17, s18)                                                        \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_17(i, c, k, a, s1, s2, s3, s4, s5, \
                                               s6, s7, s8, s9, s10, s11, s12,  \
                                               s13, s14, s15, s16, s17)        \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s18)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_19(                          \
    i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14,   \
    s15, s16, s17, s18, s19)                                                   \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_18(i, c, k, a, s1, s2, s3, s4, s5, \
                                               s6, s7, s8, s9, s10, s11, s12,  \
                                               s13, s14, s15, s16, s17, s18)   \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s19)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_20(                          \
    i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14,   \
    s15, s16, s17, s18, s19, s20)                                              \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_19(                                \
      i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, \
      s15, s16, s17, s18, s19)                                                 \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s20)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_21(                          \
    i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14,   \
    s15, s16, s17, s18, s19, s20, s21)                                         \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_20(                                \
      i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, \
      s15, s16, s17, s18, s19, s20)                                            \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s21)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_22(                          \
    i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14,   \
    s15, s16, s17, s18, s19, s20, s21, s22)                                    \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_21(                                \
      i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, \
      s15, s16, s17, s18, s19, s20, s21)                                       \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s22)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_23(                          \
    i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14,   \
    s15, s16, s17, s18, s19, s20, s21, s22, s23)                               \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_22(                                \
      i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, \
      s15, s16, s17, s18, s19, s20, s21, s22)                                  \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s23)

#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_24(                          \
    i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14,   \
    s15, s16, s17, s18, s19, s20, s21, s22, s23, s24)                          \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_23(                                \
      i, c, k, a, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11, s12, s13, s14, \
      s15, s16, s17, s18, s19, s20, s21, s22, s23)                             \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_1(i, c, k, a, s24)

/* Dispatcher */
#define _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_DISPATCH(n, i, c, k, a, ...) \
  _UDS_CAT_EXPAND(_UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_, n)(i, c, k, a,   \
                                                                 __VA_ARGS__)

// clang-format off


/**
 * @brief Register a new read dtc information event handler for several subfunction IDs
 * 
 * @param _instance Pointer to associated the UDS server instance
 * @param _context Optional context provided by the user
 * @param _check Check if the `_act` action should be executed
 * @param _act Execute the event handler for the event
 * @param ... List of subfunction IDs to register the handler for
 */
#define UDS_REGISTER_READ_DTC_INFO_HANDLER_MANY(          \
  _instance,                                              \
  _context,                                               \
  _check,                                                 \
  _act,                                                   \
  ...                                                     \
)                                                         \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_DISPATCH(     \
      _UDS_VA_NARGS(__VA_ARGS__),                         \
      _instance,                                          \
      _context,                                           \
      _check,                                             \
      _act,                                               \
      __VA_ARGS__                                         \
)


/**
 * @brief Register a new read dtc information event handler for all subfunction IDs
 * 
 * @param _instance Pointer to associated the UDS server instance
 * @param _context Optional context provided by the user
 * @param _check Check if the `_act` action should be executed
 * @param _act Execute the event handler for the event
 */
#define UDS_REGISTER_READ_DTC_INFO_HANDLER_ALL(             \
  _instance,                                                \
  _context,                                                 \
  _check,                                                   \
  _act                                                      \
)                                                           \
  _UDS_REGISTER_READ_DTC_INFO_HANDLER_APPLY_DISPATCH(       \
      24,                                                   \
      _instance,                                            \
      _context,                                             \
      _check,                                               \
      _act,                                                 \
      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, \
      0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x14, 0x15, 0x16, 0x17, \
      0x18, 0x19, 0x1A, 0x42,  0x55, 0x56                   \
)

// clang-format on

// #endregion READ_DTC_INFORMATION

// #region MEMORY_BY_ADDRESS

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
  STRUCT_SECTION_ITERABLE(uds_registration_t,                                 \
      /* Use a counter to generate unique names for the iterable section */   \
        _UDS_CAT_EXPAND(__uds_registration_id_memory_, __COUNTER__)) = {      \
    .instance = _instance,                                                    \
    .type = UDS_REGISTRATION_TYPE__MEMORY,                                    \
    .applies_to_event = uds_filter_for_memory_by_addr,                        \
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
    uds_check_default_memory_by_addr_read,                                    \
    uds_action_default_memory_by_addr_read,                                   \
    uds_check_default_memory_by_addr_write,                                   \
    uds_action_default_memory_by_addr_write                                   \
  )

// clang-format on

// #endregion MEMORY_BY_ADDRESS

// #region ECU_RESET

// clang-format off



/**
 * @brief Register a new ecu reset event handler
 * 
 * @details The `UDS_EVT_DoScheduledReset` event is triggered after the 
 *          response to the `UDS_EVT_EcuReset` event is send and the wait
 *          duration has elapsed
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
  STRUCT_SECTION_ITERABLE(uds_registration_t,                                 \
        _UDS_CAT_EXPAND(__uds_registration_id, _reset_type)) = {              \
    .instance = _instance,                                                    \
    .type = UDS_REGISTRATION_TYPE__ECU_RESET,                                 \
    .applies_to_event = uds_filter_for_ecu_reset_event,                       \
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
#define UDS_REGISTER_ECU_DEFAULT_HARD_RESET_HANDLER(                                 \
  _instance                                                                  \
)                                                                            \
  UDS_REGISTER_ECU_RESET_HANDLER(                                            \
    _instance,                                                               \
    NULL,                                                                    \
    ECU_RESET__HARD,                                                         \
    uds_check_ecu_hard_reset,                                                \
    uds_action_ecu_hard_reset,                                               \
    uds_check_execute_scheduled_reset,                                       \
    uds_action_execute_scheduled_reset                                       \
  )

// clang-format on

// #endregion ECU_RESET

// #region READ_WRITE_BY_IDENTIFIER

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
  STRUCT_SECTION_ITERABLE(uds_registration_t,                         \
        _UDS_CAT_EXPAND(__uds_registration_id, _data_id)) = {         \
    .instance = _instance,                                            \
    .type = UDS_REGISTRATION_TYPE__DATA_IDENTIFIER,                   \
    .applies_to_event = uds_filter_for_data_by_id_event,              \
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

// #endregion READ_WRITE_BY_IDENTIFIER

// #region DIAG_SESSION_CTRL

// clang-format off

/**
 * @brief Register a new static data identifier
 * 
 * @param _instance Pointer to associated the UDS server instance
 * @param _diag_session_ctrl_check Check if the `_diag_session_ctrl` action should be executed
 * @param _diag_session_ctrl Execute a read for the event
 * @param _session_timeout_check Check if the `_session_timeout` action should be executed
 * @param _session_timeout Execute a write for the event
 * @param _context Optional context provided by the user
 * 
 */
#define UDS_REGISTER_DIAG_SESSION_CTRL_HANDLER(                                \
  _instance,                                                                   \
  _diag_session_ctrl_check,                                                    \
  _diag_session_ctrl,                                                          \
  _session_timeout_check,                                                      \
  _session_timeout,                                                            \
  _context                                                                     \
)                                                                              \
  STRUCT_SECTION_ITERABLE(uds_registration_t,                                  \
        _UDS_CAT_EXPAND(__uds_registration_diag_session_id_, __COUNTER__)) = { \
    .instance = _instance,                                                     \
    .type = UDS_REGISTRATION_TYPE__DIAG_SESSION_CTRL,                          \
    .applies_to_event = uds_filter_for_diag_session_ctrl_event,                \
    .user_data = _context,                                                     \
    .diag_session_ctrl = {                                                     \
      .diag_sess_ctrl = {                                                      \
        .check = _diag_session_ctrl_check,                                     \
        .action = _diag_session_ctrl,                                          \
      },                                                                       \
      .session_timeout = {                                                     \
        .check = _session_timeout_check,                                       \
        .action = _session_timeout,                                            \
      },                                                                       \
    },                                                                         \
  };

// clang-format on

// #endregion DIAG_SESSION_CTRL

#endif  // ARDEP_UDS_MACRO_H
