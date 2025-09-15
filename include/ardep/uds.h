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
  uds_check_fn check;
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

  /**
   * @brief Event Handler specific context or user data
   */
  void *user_data;

  union {
    struct {
      struct uds_actor diag_sess_ctrl;
      struct uds_actor session_timeout;
    } diag_session_ctrl;
    struct {
      struct uds_actor ecu_reset;
      struct uds_actor execute_scheduled_reset;
      uint8_t type;
    } ecu_reset;
    struct {
      uint16_t data_id;
      struct uds_actor read;
      struct uds_actor write;
      void *user_context;
    } data_identifier;
    struct {
      struct uds_actor read;
      struct uds_actor write;
    } memory;
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

// Include macro declarations after all types are defined
#include "ardep/uds_macro.h"  // IWYU pragma: keep

#endif  // ARDEP_UDS_H