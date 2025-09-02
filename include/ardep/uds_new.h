#ifndef ARDEP_UDS_NEW_H
#define ARDEP_UDS_NEW_H

// Forward declaration to avoid include dependency issues
#include "ardep/iso14229.h"

#include <iso14229.h>

struct uds_new_instance_t;
struct uds_new_registration_t;

enum ecu_reset_type {
  ECU_RESET_HARD = 1,
  ECU_RESET_KEY_OFF_ON = 2,
  ECU_RESET_ENABLE_RAPID_POWER_SHUT_DOWN = 4,
  ECU_RESET_DISABLE_RAPID_POWER_SHUT_DOWN = 5,
  ECU_RESET_VEHICLE_MANUFACTURER_SPECIFIC_START = 0x40,
  ECU_RESET_VEHICLE_MANUFACTURER_SPECIFIC_END = 0x5F,
  ECU_RESET_SYSTEM_SUPPLIER_SPECIFIC_START = 0x60,
  ECU_RESET_SYSTEM_SUPPLIER_SPECIFIC_END = 0x7E,
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
   * @brief Event Handler specific context or user data
   */
  void *user_data;

  union {
    struct {
      uint16_t data_id;
      struct uds_new_actor read;
      struct uds_new_actor write;
      void *user_context;
    } data_identifier;
  };

  /**
   * @brief Pointer to the next dynamic registration
   *
   * @note: Only used for dynamic registration
   */
  struct uds_new_registration_t *next;
};

// clang-format off

#define _UDS_CAT(a, b) a##b
#define _UDS_CAT_EXPAND(a, b) _UDS_CAT(a, b)

/**
 * @brief Register a new static data identifier
 * 
 * @param _instance Pointer to associated the UDS server instance
 * @param _data_id The data identifier to register the handler for
 * @param data_ptr Custom context oder data the handle the event
 * @param _read_check Check if the `_read` action should be executed.
 * @param _read Execute a read for the event
 * @param _write_check Check if the `_write` action should be executed.
 * @param _write Execute a write for the event
 * @param _context Optional context provided by the user
 * 
 * @note: @p _write_check and @p _write are optional. Set to NULL for read-only
 *        data identifier
 */
#define UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC(                      \
  _instance,                                                          \
  _data_id,                                                           \
  data_ptr,                                                           \
  _read_check,                                                        \
  _read,                                                              \
  _write_check,                                                       \
  _write,                                                             \
  _context                                                            \
)                                                                     \
  _Static_assert(_read_check != NULL, "read_check cannot be NULL");   \
  _Static_assert(_read != NULL, "read action cannot be NULL");        \
  STRUCT_SECTION_ITERABLE(uds_new_registration_t,                     \
        _UDS_CAT_EXPAND(__uds_new_registration_id, _data_id)) = {     \
    .instance = _instance,                                            \
    .type = UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER,               \
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

#endif  // ARDEP_UDS_NEW_H