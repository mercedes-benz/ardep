/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_UDS_H
#define ARDEP_UDS_H

#include "ardep/iso14229.h"
#include "iso14229.h"

#include <wchar.h>

#include <zephyr/sys/slist.h>

struct uds_instance_t;
struct uds_registration_t;

enum uds_diagnostic_session {
  UDS_DIAG_SESSION__DEFAULT = 0x01,
  UDS_DIAG_SESSION__PROGRAMMING = 0x02,
  UDS_DIAG_SESSION__EXTENDED = 0x03,
  UDS_DIAG_SESSION__SAFETY_SYSTEM = 0x04,
  UDS_DIAG_SESSION__VEHICLE_MANUFACTURER_SPECIFIC_START = 0x40,
  UDS_DIAG_SESSION__VEHICLE_MANUFACTURER_SPECIFIC_END = 0x5F,
  UDS_DIAG_SESSION__SYSTEM_SUPPLIER_SPECIFIC_START = 0x60,
  UDS_DIAG_SESSION__SYSTEM_SUPPLIER_SPECIFIC_END = 0x7E,

};

enum uds_ecu_reset_type {
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

enum uds_input_output_control_param {
  UDS_IO_CONTROL__RETURN_CONTROL_TO_ECU = 0x00,
  UDS_IO_CONTROL__RESET_TO_DEFAULT = 0x01,
  UDS_IO_CONTROL__FREEZE_CURRENT_STATE = 0x02,
  UDS_IO_CONTROL__SHORT_TERM_ADJUSTMENT = 0x03,
  UDS_IO_CONTROL__TACHOGRAPH_TEST_IDS_START = 0x100,
  UDS_IO_CONTROL__TACHOGRAPH_TEST_IDS_END = 0x1FF,
  UDS_IO_CONTROL__VEHICLE_MANUFACTURER_SPECIFIC_START = 0x200,
  UDS_IO_CONTROL__VEHICLE_MANUFACTURER_SPECIFIC_END = 0xDFFF,
  UDS_IO_CONTROL__OBD_TEST_IDS_START = 0xE000,
  UDS_IO_CONTROL__OBD_TEST_IDS_END = 0xE1FF,
  UDS_IO_CONTROL__EXECUTE_SPL = 0xE200,
  UDS_IO_CONTROL__DEPLOY_LOOP_ROUTINE_ID = 0xE201,
  UDS_IO_CONTROL__SAFETY_SYSTEM_ROUTINE_IDS_START = 0xE202,
  UDS_IO_CONTROL__SAFETY_SYSTEM_ROUTINE_IDS_END = 0xE2FF,
  UDS_IO_CONTROL__SYSTEM_SUPPLIER_SPECIFIC = 0xF000,
  UDS_IO_CONTROL__SYSTEM_SUPPLIER_SPECIFIC_END = 0xFEFF,
  UDS_IO_CONTROL__ERASE_MEMORY = 0xFF00,
  UDS_IO_CONTROL__CHECK_PROGRAMMING_DEPENDENCIES = 0xFF01,
};

enum uds_routine_control_subfunc {
  UDS_ROUTINE_CONTROL__START_ROUTINE = 0x01,
  UDS_ROUTINE_CONTROL__STOP_ROUTINE = 0x02,
  UDS_ROUTINE_CONTROL__REQUEST_ROUTINE_RESULTS = 0x03,
};

enum uds_dynamically_define_data_ids_subfunc {
  UDS_DYNAMICALLY_DEFINED_DATA_IDS__DEFINE_BY_DATA_ID = 0x01,
  UDS_DYNAMICALLY_DEFINED_DATA_IDS__DEFINE_BY_MEMORY_ADDRESS = 0x02,
  UDS_DYNAMICALLY_DEFINED_DATA_IDS__CLEAR = 0x03,
};

enum uds_link_control_subfunc {
  UDS_LINK_CONTROL__VERIFY_MODE_TRANSITION_WITH_FIXED_PARAMETER = 0x01,
  UDS_LINK_CONTROL__VERIFY_MODE_TRANSITION_WITH_SPECIFIC_PARAMETER = 0x02,
  UDS_LINK_CONTROL__TRANSITION_MODE = 0x03,
  UDS_LINK_CONTROL__VEHICLE_MANUFACTURER_SPECIFIC_START = 0x40,
  UDS_LINK_CONTROL__VEHICLE_MANUFACTURER_SPECIFIC_END = 0x5F,
  UDS_LINK_CONTROL__SYSTEM_SUPPLIER_SPECIFIC_START = 0x60,
  UDS_LINK_CONTROL__SYSTEM_SUPPLIER_SPECIFIC_END = 0x7E,
};

enum uds_link_control_modifier {
  UDS_LINK_CONTROL_MODIFIER__PC_9600_BAUD = 0x01,
  UDS_LINK_CONTROL_MODIFIER__PC_1920_BAUD = 0x02,
  UDS_LINK_CONTROL_MODIFIER__PC_38400_BAUD = 0x03,
  UDS_LINK_CONTROL_MODIFIER__PC_57600_BAUD = 0x04,
  UDS_LINK_CONTROL_MODIFIER__PC_115200_BAUD = 0x05,
  UDS_LINK_CONTROL_MODIFIER__CAN_125000_BAUD = 0x10,
  UDS_LINK_CONTROL_MODIFIER__CAN_250000_BAUD = 0x11,
  UDS_LINK_CONTROL_MODIFIER__CAN_500000_BAUD = 0x12,
  UDS_LINK_CONTROL_MODIFIER__CAN_1000000_BAUD = 0x13,
  UDS_LINK_CONTROL_MODIFIER__PROGRAMMING_SETUP = 0x20,
};

/**
 * @brief Get the baudrate associated with the `enum uds_link_control_modifier`
 * entry
 *
 * @returns The baudrate in bit/s
 * @returns 0 if the modifier is unknown or
 *          `UDS_LINK_CONTROL_MODIFIER__PROGRAMMING_SETUP`
 */
uint32_t uds_link_control_modifier_to_baudrate(
    enum uds_link_control_modifier modifier);

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
   * @brief Pointer to the server parameter used in copy functions
   */
  UDSServer_t *server;
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
 * @returns UDS_OK on success
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
 * @brief Function to dynamically register a new event handler at runtime
 *
 * @param inst Pointer to the UDS server instance.
 * @param registration The registration information for the new event handler.
 * @param dynamic_id_out Pointer to store the assigned unique ID for this
 * registration.
 * @param registration_out Optional pointer to return a pointer to the allocated
 * registration object. Can be NULL if not needed.
 *
 * @returns 0 on success
 * @returns -ENOSPC if no free ID is available (all IDs from 1 to UINT32_MAX are
 * taken)
 * @returns <0 on other failures
 *
 */
typedef int (*register_event_handler_fn)(
    struct uds_instance_t *inst,
    struct uds_registration_t registration,
    uint32_t *dynamic_id_out,
    struct uds_registration_t **registration_out);

/**
 * @brief Function to unregister a dynamically registered event handler at
 * runtime
 *
 * @param inst Pointer to the UDS server instance.
 * @param dynamic_id The unique ID of the registration to delete.
 *
 * @returns 0 on success
 * @returns -ENOENT if no registration with the given ID was found
 * @returns Error value of custom unregister function if provided
 */
typedef int (*unregister_event_handler_fn)(struct uds_instance_t *inst,
                                           uint32_t dynamic_id);

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

  const struct device *can_dev;

#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION
  /**
   * @brief Singly linked list of dynamic registrations
   */
  sys_slist_t dynamic_registrations;
  register_event_handler_fn register_event_handler;
  unregister_event_handler_fn unregister_event_handler;
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
  UDS_REGISTRATION_TYPE__CLEAR_DIAG_INFO,
  UDS_REGISTRATION_TYPE__ROUTINE_CONTROL,
  UDS_REGISTRATION_TYPE__SECURITY_ACCESS,
  UDS_REGISTRATION_TYPE__COMMUNICATION_CONTROL,
  UDS_REGISTRATION_TYPE__DYNAMIC_DEFINE_DATA_IDS,
  UDS_REGISTRATION_TYPE__CONTROL_DTC_SETTING,
  UDS_REGISTRATION_TYPE__UPLOAD_DOWNLOAD,
  UDS_REGISTRATION_TYPE__LINK_CONTROL,
  UDS_REGISTRATION_TYPE__AUTHENTICATION,
};

enum uds_dynamically_defined_data_type {
  UDS_DYNAMICALLY_DEFINED_DATA_TYPE__ID = 1,
  UDS_DYNAMICALLY_DEFINED_DATA_TYPE__MEMORY = 2,
};

struct uds_dynamically_defined_data {
  sys_snode_t node;
  enum uds_dynamically_defined_data_type type;
  union {
    struct {
      uint16_t id;
      uint8_t position;
      uint8_t size;
    } id;
    struct {
      void *memAddr;
      size_t memSize;
    } memory;
  };
};

struct dynamic_registration_id_sll_item {
  sys_snode_t node;
  uint32_t dynamic_registration_id;
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
      enum uds_ecu_reset_type type;
    } ecu_reset;
    /**
     * @brief Data for the Read/Write Data by ID event handler
     *
     * Handles *UDS_EVT_ReadDataByIdent*, *UDS_EVT_WriteDataByIdent* and
     * *UDS_EVT_IOControl* events
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
      /**
       * @brief Actor for *UDS_EVT_IOControl* events
       */
      struct uds_actor io_control;
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
    /**
     * @brief Data for the Control DTC Settings event handler
     *
     * Handles *UDS_EVT_ControlDTCSetting* events with all its sub-Functions
     */
    struct {
      /**
       * @brief User-defined context pointer
       */
      void *user_context;
      /**
       * @brief Actor for *UDS_EVT_ControlDTCSetting* events
       */
      struct uds_actor actor;
    } control_dtc_setting;
    /**
     * @brief Data for the Clear Diagnostic Information event handler
     *
     * Handles *UDS_EVT_ClearDiagnosticInfo* events
     */
    struct {
      /**
       * @brief User-defined context pointer
       */
      void *user_context;
      /**
       * @brief Actor for *UDS_EVT_ClearDiagnosticInfo* events
       */
      struct uds_actor actor;
    } clear_diagnostic_information;
    /**
     * @brief Data for the Routine Control event handler
     *
     * Handles *UDS_EVT_RoutineCtrl* events with all its sub-Functions
     */
    struct {
      /**
       * @brief User-defined context pointer
       */
      void *user_context;
      /**
       * @brief ID of the routine controlled by the actor
       */
      uint16_t routine_id;
      /**
       * @brief Actor for *UDS_EVT_RoutineCtrl* events
       */
      struct uds_actor actor;
    } routine_control;
    /**
     * @brief Data for the Security Access event handler
     *
     * Handles *UDS_EVT_SecAccessRequestSeed* and *UDS_EVT_SecAccessValidateKey*
     * events
     */
    struct {
      /**
       * @brief User-defined context pointer
       */
      void *user_context;
      /**
       * @brief Actor for *UDS_EVT_SecAccessRequestSeed* events
       */
      struct uds_actor request_seed;
      /**
       * @brief Actor for *UDS_EVT_SecAccessValidateKey* events
       */
      struct uds_actor validate_key;
    } security_access;
    /**
     * @brief Data for the Communication Control event handler
     *
     * Handles *UDS_EVT_CommCtrl* events
     */
    struct {
      /**
       * @brief User-defined context pointer
       */
      void *user_context;
      /**
       * @brief Actor for *UDS_EVT_CommCtrl* events
       */
      struct uds_actor actor;
    } communication_control;

    /**
     * @brief Data for the Dynamically Define Data ID event handler
     *
     * Handles *UDS_EVT_DynamicDefineDataId* events
     */
    struct {
      /**
       * @brief User-defined context pointer
       */
      void *user_context;

      /**
       * @brief list of `struct dynamic_registration_id_sll_item` to hold all
       * dynamic registration IDs created.
       */
      sys_slist_t dynamic_registration_id_list;

      /**
       * @brief Actor for *UDS_EVT_CommCtrl* events
       */
      struct uds_actor actor;
    } dynamically_define_data_ids;
    /**
     * @brief Data for the Link Control event handler
     *
     * Handles *UDS_EVT_LinkControl* events
     */
    struct {
      /**
       * @brief User-defined context pointer
       */
      void *user_context;
      /**
       * @brief Actor for *UDS_EVT_LinkControl* events
       */
      struct uds_actor actor;
    } link_control;
    /**
     * @brief Data for the Authentication event handler
     *
     * Handles *UDS_EVT_Auth* and *UDS_EVT_AuthTimeout* events
     */
    struct {
      /**
       * @brief User-defined context pointer
       */
      void *user_context;
      /**
       * @brief Actor for *UDS_EVT_Auth* events
       */
      struct uds_actor auth;
      /**
       * @brief Actor for *UDS_EVT_AuthTimeout* events
       */
      struct uds_actor timeout;
    } auth;
  };

#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION

  /**
   * @brief Singly linked list of `struct uds_registration_t` using zephyr's
   * sys_slist_t
   *
   * @note: Only used for dynamic registration
   */
  sys_snode_t node;

  /**
   * @brief Unique ID of this dynamic registration
   */
  uint32_t dynamic_registration_id;

  /**
   * @brief Function to unregister this registration
   *
   * A custom function can be provided to e.g. free additional resources when
   * this registration gets unregistered.
   *
   * @param this Pointer to this registration instance
   *
   * @returns 0 on success
   * @returns <0 on failure
   */
  int (*unregister_registration_fn)(struct uds_registration_t *this);
#endif  // CONFIG_UDS_USE_DYNAMIC_REGISTRATION
};

#if CONFIG_UDS_USE_LINK_CONTROL

/**
 * @brief Transitions the CAN controller to the specified baudrate
 *
 * @param can_dev the CAN device
 * @param baud_rate the target baudrate in bit/s
 * @return UDS_OK on success, otherwise an appropriate negative error code
 */
UDSErr_t uds_set_can_bitrate(const struct device *can_dev, uint32_t baud_rate);

/**
 * @brief Transitions the CAN controller to the default bitrate
 *
 * The default bitrate is configured via `CONFIG_UDS_DEFAULT_CAN_BITRATE`
 *
 * @param can_dev the CAN device
 * @return same as `uds_set_can_bitrate`
 */
UDSErr_t uds_set_can_default_bitrate(const struct device *can_dev);

#endif  // CONFIG_UDS_USE_LINK_CONTROL

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
 * @brief Default check function for the default dynamically define data IDs
 * handler
 */
UDSErr_t uds_check_default_dynamically_define_data_ids(
    const struct uds_context *const context, bool *apply_action);

/**
 * @brief Default action function for the default dynamically define data IDs
 * handler
 */
UDSErr_t uds_action_default_dynamically_define_data_ids(
    struct uds_context *const context, bool *consume_event);

/**
 * @brief Default check function for the default link control handler
 */
UDSErr_t uds_check_default_link_control(const struct uds_context *const context,
                                        bool *apply_action);

/**
 * @brief Default action function for the default link control handler
 */
UDSErr_t uds_action_default_link_control(struct uds_context *const context,
                                         bool *consume_event);

/**
 * @brief Default check function for  diagnostic session events for the
 * default link control handler
 */
UDSErr_t uds_check_default_link_control_change_diag_session(
    const struct uds_context *const context, bool *apply_action);

/**
 * @brief Default action function for diagnostic session events for the default
 * link control handler
 */
UDSErr_t uds_action_default_link_control_change_diag_session(
    struct uds_context *const context, bool *consume_event);

// Include macro declarations after all types are defined
#include "ardep/uds_macro.h"  // IWYU pragma: keep

#endif  // ARDEP_UDS_H