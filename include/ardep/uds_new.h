/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_UDS_NEW_H
#define ARDEP_UDS_NEW_H

// Forward declaration to avoid include dependency issues
#include "ardep/iso14229.h"

#include <iso14229.h>

struct uds_new_instance_t;
struct uds_new_registration_t;

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

enum read_dtc_info_subfunc {
  READ_DTC_INFO_SUBFUNC__NUM_OF_DTC_BY_STATUS_MASK = 0x01,
  READ_DTC_INFO_SUBFUNC__DTC_BY_STATUS_MASK = 0x02,
  READ_DTC_INFO_SUBFUNC__DTC_SNAPSHOT_IDENTIFICATION = 0x03,
  READ_DTC_INFO_SUBFUNC__DTC_SNAPSHOT_RECORD_BY_DTC_NUM = 0x04,
  READ_DTC_INFO_SUBFUNC__DTC_STORED_DATA_BY_RECORD_NUM = 0x05,
  READ_DTC_INFO_SUBFUNC__DTC_EXT_DATA_RECORD_BY_DTC_NUM = 0x06,
  READ_DTC_INFO_SUBFUNC__NUM_OF_DTC_BY_SEVERITY_MASK_RECORD = 0x07,
  READ_DTC_INFO_SUBFUNC__DTC_BY_SEVERITY_MASK_RECORD = 0x08,
  READ_DTC_INFO_SUBFUNC__SEVERITY_INFO_OF_DTC = 0x09,
  READ_DTC_INFO_SUBFUNC__SUPPORTED_DTC = 0x0A,
  READ_DTC_INFO_SUBFUNC__FIRST_TEST_FAILED_DTC = 0x0B,
  READ_DTC_INFO_SUBFUNC__FIRST_CONFIRMED_DTC = 0x0C,
  READ_DTC_INFO_SUBFUNC__MOST_RECENT_TEST_FAILED_DTC = 0x0D,
  READ_DTC_INFO_SUBFUNC__MOST_RECENT_CONFIRMED_DTC = 0x0E,
  READ_DTC_INFO_SUBFUNC__DTC_FAULT_DETECTION_COUNTER = 0x14,
  READ_DTC_INFO_SUBFUNC__DTC_WITH_PERMANENT_STATUS = 0x15,
  READ_DTC_INFO_SUBFUNC__DTC_EXT_DATA_RECORD_BY_NUM = 0x16,
  READ_DTC_INFO_SUBFUNC__USER_DEF_MEM_DTC_BY_STATUS_MASK = 0x17,
  READ_DTC_INFO_SUBFUNC__USER_DEF_MEM_DTC_SNAPSHOT_RECORD_BY_DTC_NUM = 0x18,
  READ_DTC_INFO_SUBFUNC__USER_DEF_MEM_DTC_EXT_DATA_RECORD_BY_DTC_NUM = 0x19,
  READ_DTC_INFO_SUBFUNC__DTC_EXTENDED_DATA_RECORD_IDENTIFICATION = 0x1A,
  READ_DTC_INFO_SUBFUNC__WWHOBD_DTC_BY_MASK_RECORD = 0x42,
  READ_DTC_INFO_SUBFUNC__WWHOBD_DTC_WITH_PERMANENT_STATUS = 0x55,
  READ_DTC_INFO_SUBFUNC__DTC_INFO_BY_DTC_READINESS_GROUP_IDENTIFIER = 0x56
};

/**
 * @brief Callback type for ECU reset events
 *
 * @param inst Pointer to the UDS server instance
 * @param reset_type Type of reset to perform
 * @param user_context User-defined context pointer as passed to \ref
 * uds_new_init()
 */
typedef UDSErr_t (*ecu_reset_callback_t)(struct uds_new_instance_t *inst,
                                         enum ecu_reset_type reset_type,
                                         void *user_context);

/**
 * Set the ECU reset callback function for custom callbacks
 *
 * @param inst Pointer to the UDS server instance
 * @param callback Pointer to the callback function to set
 * @return 0 on success, negative error code on failure
 */
typedef int (*set_ecu_reset_callback_fn)(struct uds_new_instance_t *inst,
                                         ecu_reset_callback_t callback);

/**
 * @brief Context provided to Event handlers on an event
 */
struct uds_new_context {
  /**
   * @brief The instance the event was generated on
   */
  struct uds_new_instance_t *const instance;
  /**
   * @brief The registration instance to handle the event
   */
  struct uds_new_registration_t *const registration;
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
 * @brief Callback to check whether the associated `uds_new_action_fn`
 * should be executed on this event.
 *
 * @param[in] context The context of this UDS Event
 * @param[out] apply_action set to `true` when an associated action should be
 *                          applied to this event.
 * @returns UDS_PositiveResponse on success
 * @returns UDS_NRC_* on failure. This NRC is returned to the UDS client
 */
typedef UDSErr_t (*uds_new_check_fn)(
    const struct uds_new_context *const context, bool *apply_action);

/**
 * @brief Callback to act on an matching UDS Event
 *
 * When this callback is called, assume that the relevant conditions are met and
 * checked with an associated `uds_new_check_fn` beforehand.
 *
 * @param[in,out] context The context of this UDS Event
 * @param[out] consume_event Set to `false` if the event should not be consumed
 *                           by this action or to `true` to consume it.
 *                           This should always be set.
 * @returns UDS_PositiveResponse on success
 * @returns UDS_NRC_* on failure. This NRC is returned to the UDS client
 */
typedef UDSErr_t (*uds_new_action_fn)(struct uds_new_context *const context,
                                      bool *consume_event);

/**
 * @brief Function to get the associated check function for a registration
 *
 * @param[in] reg Pointer to the registration instance
 * @returns The associated check function
 * @returns NULL if no function is associated
 */
typedef uds_new_check_fn (*uds_new_get_check_fn)(
    const struct uds_new_registration_t *const reg);

/**
 * @brief Function to get the associated action function for a registration
 *
 * @param[in] reg Pointer to the registration instance
 * @returns The associated check function
 * @returns NULL if no function is associated
 */
typedef uds_new_action_fn (*uds_new_get_action_fn)(
    const struct uds_new_registration_t *const reg);

/**
 * @brief Associates a check with an action
 */
struct uds_new_actor {
  uds_new_check_fn check;
  uds_new_action_fn action;
};

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION

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
    struct uds_new_instance_t *inst,
    struct uds_new_registration_t registration);

#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION

struct uds_new_instance_t {
  struct iso14229_zephyr_instance iso14229;
  struct uds_new_registration_t *static_registrations;
  void *user_context;

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION
  struct uds_new_registration_t *dynamic_registrations;
  register_event_handler_fn register_event_handler;
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION
};

int uds_new_init(struct uds_new_instance_t *inst,
                 const UDSISOTpCConfig_t *iso_tp_config,
                 const struct device *can_dev,
                 void *user_context);

enum uds_new_registration_type_t {
  UDS_NEW_REGISTRATION_TYPE__ECU_RESET,
  UDS_NEW_REGISTRATION_TYPE__MEMORY,
  UDS_NEW_REGISTRATION_TYPE__READ_DTC_INFO,
  UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER,
};

/**
 * @brief Registration information for an UDS Event handler
 */
struct uds_new_registration_t {
  /**
   * @brief Instance the UDS Event handler is registered to
   */
  struct uds_new_instance_t *instance;

  /**
   * @brief Type of event handler
   */
  enum uds_new_registration_type_t type;

  /**
   * @brief Filter function to determine if the event can be handled by this
   * registration type
   *
   * We need to filter before any "check" functions because those reside
   * inside the unnamed union member. Thus accessing the wrong view on the data
   * can lead to incorrect data and behavior.
   */
  bool (*applies_to_event)(UDSEvent_t event);

  /**
   * @brief Event Handler specific context or user data
   */
  void *user_data;

  union {
    struct {
      struct uds_new_actor ecu_reset;
      struct uds_new_actor execute_scheduled_reset;
      uint8_t type;
    } ecu_reset;
    struct {
      uint16_t data_id;
      struct uds_new_actor read;
      struct uds_new_actor write;
      void *user_context;
    } data_identifier;
    struct {
      struct uds_new_actor read;
      struct uds_new_actor write;
    } memory;
    struct {
      uint8_t sub_function;
      struct uds_new_actor actor;
    } read_dtc;
  };

  /**
   * @brief Pointer to the next dynamic registration
   *
   * @note: Only used for dynamic registration
   */
  struct uds_new_registration_t *next;
};

/**
 * @brief Default check function for the default ECU Hard Reset handler
 */
UDSErr_t uds_new_check_ecu_hard_reset(
    const struct uds_new_context *const context, bool *apply_action);

/**
 * @brief Default action function for the default ECU Hard Reset handler
 */
UDSErr_t uds_new_action_ecu_hard_reset(struct uds_new_context *const context,
                                       bool *consume_event);

/**
 * @brief Default check function for the default ECU Hard Reset handler
 */
UDSErr_t uds_new_check_execute_scheduled_reset(
    const struct uds_new_context *const context, bool *apply_action);

/**
 * @brief Default action function for the default ECU Hard Reset handler
 */
UDSErr_t uds_new_action_execute_scheduled_reset(
    struct uds_new_context *const context, bool *consume_event);

/**
 * @brief Default check function for the default memory read handler
 *
 * Checks RAM and Flash memory for read access
 */
UDSErr_t uds_new_check_default_memory_by_addr_read(
    const struct uds_new_context *const context, bool *apply_action);

/**
 * @brief Default action function for the default memory read handler
 *
 * Reads from RAM and Flash
 */
UDSErr_t uds_new_action_default_memory_by_addr_read(
    struct uds_new_context *const context, bool *consume_event);

/**
 * @brief Default check function for the default memory write handler
 *
 * Checks RAM and Flash memory for write access
 */
UDSErr_t uds_new_check_default_memory_by_addr_write(
    const struct uds_new_context *const context, bool *apply_action);

/**
 * @brief Default action function for the default memory write handler
 *
 * Writes to RAM and Flash
 */
UDSErr_t uds_new_action_default_memory_by_addr_write(
    struct uds_new_context *const context, bool *consume_event);

/**
 * @brief Filter for ECU Reset event handler registrations
 *
 * @param[in] event the event to check against
 * @returns true if the `event` can be handled
 * @returns false otherwise
 */
bool uds_new_filter_for_ecu_reset_event(UDSEvent_t event);

/**
 * @brief Filter for Read/Write data by ID event handler registrations
 *
 * see @fn uds_new_filter_for_ecu_reset_event for details
 */
bool uds_new_filter_for_data_by_id_event(UDSEvent_t event);

/**
 * @brief Filter for Read/Write memory by address event handler registrations
 *
 * see @fn uds_new_filter_for_ecu_reset_event for details
 */
bool uds_new_filter_for_memory_by_addr(UDSEvent_t event);

/**
 * @brief Filter for Read DTC Information event handler registrations
 *
 * see @fn uds_new_filter_for_ecu_reset_event for details
 */
bool uds_new_filter_for_read_dtc_info_event(UDSEvent_t event);

#include "ardep/uds_new_macro.h"

#endif  // ARDEP_UDS_NEW_H