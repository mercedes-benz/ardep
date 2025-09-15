/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_UDS_H
#define ARDEP_UDS_H

#include "ardep/iso14229.h"

#include <iso14229.h>

struct uds_instance_t;
struct uds_registration_t;

enum ecu_reset_type {
  ECU_RESET__HARD = 1,
  ECU_RESET__KEY_OFF_ON = 2,
  ECU_RESET__ENABLE_RAPID_POWER_SHUT_DOWN = 4,
  ECU_RESET__DISABLE_RAPID_POWER_SHUT_DOWN = 5,
  ECU_RESET__VEHICLE_MANUFACTURER_SPECIFIC_START = 0x40,
  ECU_RESET__VEHICLE_MANUFACTURER_SPECIFIC_END = 0x5F,
  ECU_RESET__SYSTEM_SUPPLIER_SPECIFIC_START = 0x60,
  ECU_RESET__SYSTEM_SUPPLIER_SPECIFIC_END = 0x7E,
};

enum uds_read_dtc_info_subfunc {
  UDS_READ_DTC_INFO_SUBFUNC__NUM_OF_DTC_BY_STATUS_MASK = 0x01,
  UDS_READ_DTC_INFO_SUBFUNC__DTC_BY_STATUS_MASK = 0x02,
  UDS_READ_DTC_INFO_SUBFUNC__DTC_SNAPSHOT_IDENTIFICATION = 0x03,
  UDS_READ_DTC_INFO_SUBFUNC__DTC_SNAPSHOT_RECORD_BY_DTC_NUM = 0x04,
  UDS_READ_DTC_INFO_SUBFUNC__DTC_STORED_DATA_BY_RECORD_NUM = 0x05,
  UDS_READ_DTC_INFO_SUBFUNC__DTC_EXT_DATA_RECORD_BY_DTC_NUM = 0x06,
  UDS_READ_DTC_INFO_SUBFUNC__NUM_OF_DTC_BY_SEVERITY_MASK_RECORD = 0x07,
  UDS_READ_DTC_INFO_SUBFUNC__DTC_BY_SEVERITY_MASK_RECORD = 0x08,
  UDS_READ_DTC_INFO_SUBFUNC__SEVERITY_INFO_OF_DTC = 0x09,
  UDS_READ_DTC_INFO_SUBFUNC__SUPPORTED_DTC = 0x0A,
  UDS_READ_DTC_INFO_SUBFUNC__FIRST_TEST_FAILED_DTC = 0x0B,
  UDS_READ_DTC_INFO_SUBFUNC__FIRST_CONFIRMED_DTC = 0x0C,
  UDS_READ_DTC_INFO_SUBFUNC__MOST_RECENT_TEST_FAILED_DTC = 0x0D,
  UDS_READ_DTC_INFO_SUBFUNC__MOST_RECENT_CONFIRMED_DTC = 0x0E,
  UDS_READ_DTC_INFO_SUBFUNC__DTC_FAULT_DETECTION_COUNTER = 0x14,
  UDS_READ_DTC_INFO_SUBFUNC__DTC_WITH_PERMANENT_STATUS = 0x15,
  UDS_READ_DTC_INFO_SUBFUNC__DTC_EXT_DATA_RECORD_BY_NUM = 0x16,
  UDS_READ_DTC_INFO_SUBFUNC__USER_DEF_MEM_DTC_BY_STATUS_MASK = 0x17,
  UDS_READ_DTC_INFO_SUBFUNC__USER_DEF_MEM_DTC_SNAPSHOT_RECORD_BY_DTC_NUM = 0x18,
  UDS_READ_DTC_INFO_SUBFUNC__USER_DEF_MEM_DTC_EXT_DATA_RECORD_BY_DTC_NUM = 0x19,
  UDS_READ_DTC_INFO_SUBFUNC__DTC_EXTENDED_DATA_RECORD_IDENTIFICATION = 0x1A,
  UDS_READ_DTC_INFO_SUBFUNC__WWHOBD_DTC_BY_MASK_RECORD = 0x42,
  UDS_READ_DTC_INFO_SUBFUNC__WWHOBD_DTC_WITH_PERMANENT_STATUS = 0x55,
  UDS_READ_DTC_INFO_SUBFUNC__DTC_INFO_BY_DTC_READINESS_GROUP_IDENTIFIER = 0x56
};

/**
 * @brief Callback type for ECU reset events
 *
 * @param inst Pointer to the UDS server instance
 * @param reset_type Type of reset to perform
 * @param user_context User-defined context pointer as passed to \ref
 * uds_init()
 */
typedef UDSErr_t (*ecu_reset_callback_t)(struct uds_instance_t *inst,
                                         enum ecu_reset_type reset_type,
                                         void *user_context);

/**
 * Set the ECU reset callback function for custom callbacks
 *
 * @param inst Pointer to the UDS server instance
 * @param callback Pointer to the callback function to set
 * @return 0 on success, negative error code on failure
 */
typedef int (*set_ecu_reset_callback_fn)(struct uds_instance_t *inst,
                                         ecu_reset_callback_t callback);

/**
 * @brief Context provided to Event handlers on an event
 */
struct uds_context {
  /**
   * @brief The instance the event was generated on
   */
  struct uds_instance_t *const instance;
  /**
   * @brief The registration instance to handle the event
   */
  struct uds_registration_t *const registration;
  /**
   * @brief The event type
   */
  UDSEvent_t event;
  /**
   * @brief Arguments associated with the event
   */
  void *arg;
};

/**
 * @brief Callback to check whether the associated `uds_action_fn`
 * should be executed on this event.
 *
 * @param context The context of this UDS Event
 * @param apply_action set to `true` when an associated action should be
 *                          applied to this event.
 * @returns UDS_PositiveResponse on success
 * @returns UDS_NRC_* on failure. This NRC is returned to the UDS client
 */
typedef UDSErr_t (*uds_check_fn)(const struct uds_context *const context,
                                 bool *apply_action);

/**
 * @brief Callback to act on an matching UDS Event
 *
 * When this callback is called, assume that the relevant conditions are met and
 * checked with an associated `uds_check_fn` beforehand.
 *
 * @param context The context of this UDS Event
 * @param consume_event Set to `false` if the event should not be consumed
 *                           by this action or to `true` to consume it.
 * @returns UDS_PositiveResponse on success
 * @returns UDS_NRC_* on failure. This NRC is returned to the UDS client
 *
 * @note You should always set `consume_event` and not rely on the default value
 */
typedef UDSErr_t (*uds_action_fn)(struct uds_context *const context,
                                  bool *consume_event);

/**
 * @brief Function to get the associated check function for a registration
 *
 * @param reg Pointer to the registration instance
 * @returns The associated check function
 * @returns NULL if no function is associated
 */
typedef uds_check_fn (*uds_get_check_fn)(
    const struct uds_registration_t *const reg);

/**
 * @brief Function to get the associated action function for a registration
 *
 * @param reg Pointer to the registration instance
 * @returns The associated check function
 * @returns NULL if no function is associated
 */
typedef uds_action_fn (*uds_get_action_fn)(
    const struct uds_registration_t *const reg);

/**
 * @brief Associates a check with an action
 */
struct uds_actor {
  /**
   * @brief Check function to evaluate whether the action should be executed
   */
  uds_check_fn check;
  /**
   * @brief Action function to execute when the check passes.
   */
  uds_action_fn action;
};

#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION

/**
 * @brief Function to register a new data identifier at runtime
 *
 * @param inst Pointer to the UDS server instance.
 * @param registration The registration information for the new data identifier.
 *
 * @returns 0 on success
 * @returns <0 on failure
 *
 */
typedef int (*register_event_handler_fn)(
    struct uds_instance_t *inst, struct uds_registration_t registration);

#endif  // CONFIG_UDS_USE_DYNAMIC_REGISTRATION

/**
 * @brief UDS server instance
 */
struct uds_instance_t {
  /**
   * @brief isotp instance
   */
  struct iso14229_zephyr_instance iso14229;

  void *user_context;

#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION
  /**
   * @brief Pointer to the head of the singly linked list of dynamic
   * registrations
   */
  struct uds_registration_t *dynamic_registrations;
  register_event_handler_fn register_event_handler;
#endif  // CONFIG_UDS_USE_DYNAMIC_REGISTRATION
};

int uds_init(struct uds_instance_t *inst,
             const UDSISOTpCConfig_t *iso_tp_config,
             const struct device *can_dev,
             void *user_context);

/**
 * @brief type identifier for `struct uds_registration_t`
 */
enum uds_registration_type_t {
  UDS_REGISTRATION_TYPE__ECU_RESET,
  UDS_REGISTRATION_TYPE__MEMORY,
  UDS_REGISTRATION_TYPE__READ_DTC_INFO,
  UDS_REGISTRATION_TYPE__DATA_IDENTIFIER,
  UDS_REGISTRATION_TYPE__DIAG_SESSION_CTRL,
};

/**
 * @brief Registration information for an UDS Event handler
 */
struct uds_registration_t {
  /**
   * @brief Instance the UDS Event handler is registered to
   */
  struct uds_instance_t *instance;

  /**
   * @brief Type of event handler
   */
  enum uds_registration_type_t type;

  /**
   * @brief Filter function to determine if the event can be handled by this
   * registration type
   *
   * We need to filter before any "check" functions because those reside
   * inside the unnamed union member. Thus accessing the wrong view on the data
   * can lead to incorrect data and behavior.
   */
  bool (*applies_to_event)(UDSEvent_t event);

  union {
    /**
     * @brief Data for the diagnostic session control event handler
     *
     * Handles *UDS_EVT_DiagSessCtrl* events
     */
    struct {
      /**
       * @brief User-defined context pointer
       */
      void *user_context;
      /**
       * @brief Actor for *UDS_EVT_DiagSessCtrl* events
       */
      struct uds_actor diag_sess_ctrl;
      /**
       * @brief Actor for *UDS_EVT_SessionTimeout* events
       */
      struct uds_actor session_timeout;
    } diag_session_ctrl;
    /**
     * @brief Data for the ECU Reset event handler
     *
     * Handles *UDS_EVT_EcuReset* and *UDS_EVT_DoScheduledReset* events
     */
    struct {
      /**
       * @brief User-defined context pointer
       */
      void *user_context;
      /**
       * @brief Actor for *UDS_EVT_EcuReset* events
       */
      struct uds_actor ecu_reset;
      /**
       * @brief Actor for *UDS_EVT_DoScheduledReset* events
       */
      struct uds_actor execute_scheduled_reset;
      /**
       * @brief Type of reset to perform
       */
      enum ecu_reset_type type;
    } ecu_reset;
    /**
     * @brief Data for the Read/Write Data by ID event handler
     *
     * Handles *UDS_EVT_ReadDataByIdent* and *UDS_EVT_WriteDataByIdent* events
     */
    struct {
      /**
       * @brief User-defined context pointer
       */
      void *user_context;
      /**
       * @brief Pointer to the data to read/write
       */
      void *data;
      /**
       * @brief Data identifier as defined in ISO 14229-1
       */
      uint16_t data_id;
      /**
       * @brief Actor for *UDS_EVT_ReadDataByIdent* events
       */
      struct uds_actor read;
      /**
       * @brief Actor for *UDS_EVT_WriteDataByIdent* events
       */
      struct uds_actor write;
    } data_identifier;
    /**
     * @brief Data for the Read/Write Memory by Address event handler
     *
     * Handles *UDS_EVT_ReadMemByAddr* and *UDS_EVT_WriteMemByAddr* events
     */
    struct {
      /**
       * @brief User-defined context pointer
       */
      void *user_context;
      /**
       * @brief Actor for *UDS_EVT_ReadMemByAddr* events
       */
      struct uds_actor read;
      /**
       * @brief Actor for *UDS_EVT_WriteMemByAddr* events
       */
      struct uds_actor write;
    } memory;
    /**
     * @brief Data for the Read DTC Information event handler
     *
     * Handles *UDS_EVT_ReadDTCInformation* events with all its sub-Functions
     */
    struct {
      /**
       * @brief User-defined context pointer
       */
      void *user_context;
      /**
       * @brief Sub-Function as defined in ISO 14229-1
       */
      enum uds_read_dtc_info_subfunc sub_function;
      /**
       * @brief Actor for *UDS_EVT_ReadDTCInformation* events
       */
      struct uds_actor actor;
    } read_dtc;
  };

#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION

  /**
   * @brief Pointer to the next dynamic registration
   *
   * @note: Only used for dynamic registration
   */
  struct uds_registration_t *next;
#endif  // CONFIG_UDS_USE_DYNAMIC_REGISTRATION
};

/**
 * @brief Default check function for the default ECU Hard Reset handler
 */
UDSErr_t uds_check_ecu_hard_reset(const struct uds_context *const context,
                                  bool *apply_action);

/**
 * @brief Default action function for the default ECU Hard Reset handler
 */
UDSErr_t uds_action_ecu_hard_reset(struct uds_context *const context,
                                   bool *consume_event);

/**
 * @brief Default check function for the default ECU Hard Reset handler
 */
UDSErr_t uds_check_execute_scheduled_reset(
    const struct uds_context *const context, bool *apply_action);

/**
 * @brief Default action function for the default ECU Hard Reset handler
 */
UDSErr_t uds_action_execute_scheduled_reset(struct uds_context *const context,
                                            bool *consume_event);

/**
 * @brief Default check function for the default memory read handler
 *
 * Checks RAM and Flash memory for read access
 */
UDSErr_t uds_check_default_memory_by_addr_read(
    const struct uds_context *const context, bool *apply_action);

/**
 * @brief Default action function for the default memory read handler
 *
 * Reads from RAM and Flash
 */
UDSErr_t uds_action_default_memory_by_addr_read(
    struct uds_context *const context, bool *consume_event);

/**
 * @brief Default check function for the default memory write handler
 *
 * Checks RAM and Flash memory for write access
 */
UDSErr_t uds_check_default_memory_by_addr_write(
    const struct uds_context *const context, bool *apply_action);

/**
 * @brief Default action function for the default memory write handler
 *
 * Writes to RAM and Flash
 */
UDSErr_t uds_action_default_memory_by_addr_write(
    struct uds_context *const context, bool *consume_event);

/**
 * @brief Filter for ECU Reset event handler registrations
 *
 * @param event the event to check against
 * @returns true if the `event` can be handled
 * @returns false otherwise
 */
bool uds_filter_for_ecu_reset_event(UDSEvent_t event);

/**
 * @brief Filter for Read/Write data by ID event handler registrations
 *
 * see @ref uds_filter_for_ecu_reset_event for details
 */
bool uds_filter_for_data_by_id_event(UDSEvent_t event);

/**
 * @brief Filter for Read/Write memory by address event handler registrations
 *
 * see @ref uds_filter_for_ecu_reset_event for details
 */
bool uds_filter_for_memory_by_addr(UDSEvent_t event);

/**
 * @brief Filter for Diagnostic Session Control event handler registrations
 *
 * see @ref uds_filter_for_ecu_reset_event for details
 */
bool uds_filter_for_diag_session_ctrl_event(UDSEvent_t event);

/**
 * @brief Filter for Read DTC Information event handler registrations
 *
 * see @ref uds_filter_for_ecu_reset_event for details
 */
bool uds_filter_for_read_dtc_info_event(UDSEvent_t event);

// Include macro declarations after all types are defined
#include "ardep/uds_macro.h"  // IWYU pragma: keep

#endif  // ARDEP_UDS_H