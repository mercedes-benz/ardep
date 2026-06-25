.. _uds-lib:

UDS Library
###########

Overview
********

The UDS (Unified Diagnostic Services) library provides a high-level, event-driven API for implementing ISO 14229-1 compliant diagnostic services. Built on top of the :ref:`iso14229-lib`, it simplifies UDS server development by allowing you to register event handlers that respond to diagnostic requests.

Key Features
============

- **Event-Driven Architecture**: Register handlers for specific UDS services using callbacks
- **Static and Dynamic Registration**: Define handlers at compile-time via macros or register them at runtime
- **Flexible Handler System**: Each handler includes ``check`` and ``action`` functions for fine-grained control
- **Pre-built Default Handlers**: Common operations like memory access and ECU reset are provided out-of-the-box
- **Comprehensive Service Support**: Implements most UDS services defined in ISO 14229-1

Supported UDS Services
=======================

The following diagnostic services are currently implemented:

.. list-table::
    :header-rows: 1
    :widths: 50 50

    * - Service
      - SID
    * - Diagnostic Session Control
      - ``0x10``
    * - ECU Reset
      - ``0x11``
    * - Clear Diagnostic Information
      - ``0x14``
    * - Read DTC Information
      - ``0x19``
    * - Read Data By Identifier
      - ``0x22``
    * - Read Memory By Address
      - ``0x23``
    * - Security Access
      - ``0x27``
    * - Communication Control
      - ``0x28``
    * - Authentication
      - ``0x29``
    * - Dynamically Define Data Identifier
      - ``0x2C``
    * - Write Data By Identifier
      - ``0x2E``
    * - Input Output Control By Identifier
      - ``0x2F``
    * - Routine Control
      - ``0x31``
    * - Request Download
      - ``0x34``
    * - Request Upload
      - ``0x35``
    * - Transfer Data
      - ``0x36``
    * - Request Transfer Exit
      - ``0x37``
    * - Request File Transfer
      - ``0x38``
    * - Write Memory By Address
      - ``0x3D``
    * - Tester Present
      - ``0x3E``
    * - Control DTC Settings
      - ``0x85``
    * - Link Control
      - ``0x86``

Getting Started
***************

Core Concepts
=============

The UDS library operates on an event-driven model where diagnostic requests trigger events that are handled by registered event handlers.

Default UDS Instance
--------------------

The UDS library provides a **default instance** (``uds_default_instance``) that simplifies UDS server setup. When enabled via ``CONFIG_UDS_DEFAULT_INSTANCE=y`` (default), the library automatically:

- Initializes the UDS instance at application startup
- Configures CAN communication
- Registers common handlers (session control, ECU reset, optional link control)
- Starts the UDS thread

This approach eliminates most boilerplate code, allowing you to focus on implementing service-specific handlers.

**When to use the default instance:**

- Single UDS server applications
- Standard CAN addressing schemes
- Quick prototyping and testing

**When to create a custom instance:**

- Multiple UDS servers on different CAN interfaces
- Complex initialization requirements
- Non-standard transport layer configurations

Event Handlers
--------------

An **event handler** consists of:

1. **Check Function** (``uds_check_fn``): Validates whether the current ECU state allows handling the event with its provided arguments
   
   - Returns ``UDS_OK`` if the event can be processed
   - Returns a negative response code (``UDS_NRC_*``) if conditions aren't met
   - Sets ``apply_action`` to ``true`` to proceed with the action function

2. **Action Function** (``uds_action_fn``): Performs the actual diagnostic operation
   
   - Executes only if the check function approved
   - Returns ``UDS_PositiveResponse`` on success or ``UDS_NRC_*`` on error
   - Sets ``consume_event`` to ``true`` to stop event propagation or ``false`` to allow other handlers to process it

3. **Associated Data**: Context-specific information (e.g., data identifiers, memory addresses, user context)

Registration
------------

Event handlers are registered using:

- **Static Registration**: Use macros (e.g., ``UDS_REGISTER_*``) to define handlers at compile-time
- **Dynamic Registration**: Call ``instance.register_event_handler()`` at runtime (requires ``CONFIG_UDS_USE_DYNAMIC_REGISTRATION``)

Event Processing Flow
---------------------

When a diagnostic request arrives:

1. The underlying :ref:`iso14229-lib` generates a UDS event
2. The UDS library iterates through registered event handlers (static first, then dynamic)
3. For each handler:
   
   a. The ``check`` function is called
   b. If approved, the ``action`` function executes
   c. If ``consume_event`` is ``true``, iteration stops

4. If no handler processes the event, a negative response is sent to the client

Quick Start Example (Default Instance)
---------------------------------------

The following example demonstrates the simplest UDS server setup using the default instance:

.. code-block:: c

    #include <zephyr/logging/log.h>
    LOG_MODULE_REGISTER(uds_sample, LOG_LEVEL_DBG);

    #include <ardep/uds.h>

    // Define a data identifier and its associated data
    const uint16_t primitive_type_id = 0x50;
    uint16_t primitive_type = 5;

    UDSErr_t read_data_by_id_check(const struct uds_context *const context,
                                   bool *apply_action) {
        *apply_action = true;
        return UDS_OK;
    }

    UDSErr_t read_data_by_id_action(struct uds_context *const context,
                                    bool *consume_event) {
        UDSRDBIArgs_t *args = context->arg;
        *consume_event = true;

        // Convert to big-endian for network transmission
        uint16_t t = sys_cpu_to_be16(*(uint16_t *)context->registration->data_identifier.data);
        return args->copy(context->server, t, sizeof(t));
    }

    // Register handler for the default instance
    UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(
        &uds_default_instance,  // Use the default instance
        primitive_type_id,
        &primitive_type,
        read_data_by_id_check,
        read_data_by_id_action,
        NULL, NULL, NULL, NULL, NULL);

That's it! The default instance handles initialization, CAN setup, and thread management automatically.

Custom Instance Example
-----------------------

For advanced use cases requiring manual control, you can create and manage your own UDS instance.
Don't forget to disable the UDS default instance by setting ``CONFIG_UDS_DEFAULT_INSTANCE=n`` in your ``prj.conf``.

.. code-block:: c

    #include <zephyr/logging/log.h>
    LOG_MODULE_REGISTER(uds_sample, LOG_LEVEL_DBG);

    #include <zephyr/drivers/can.h>
    #include <ardep/uds.h>
    
    static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
    struct uds_instance_t my_instance;

    // ... (handler definitions as above)

    UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(
        &my_instance,  // Use custom instance
        primitive_type_id,
        &primitive_type,
        read_data_by_id_check,
        read_data_by_id_action,
        NULL, NULL, NULL, NULL, NULL);

    int main(void) {
        int err = 0;

        // Configure ISO-TP addressing
        UDSISOTpCConfig_t cfg = {
            .source_addr = 0x7E8,
            .target_addr = 0x7E0,
            .source_addr_func = 0x7DF,
            .target_addr_func = UDS_TP_NOOP_ADDR,
        };

        uds_init(&my_instance, &cfg, can_dev, NULL);

        if (!device_is_ready(can_dev)) {
            LOG_INF("CAN device not ready");
            return -ENODEV;
        }

        err = can_set_mode(can_dev, CAN_MODE_NORMAL);
        if (err) {
            LOG_ERR("Failed to set CAN mode: %d", err);
            return err;
        }

        err = can_start(can_dev);
        if (err) {
            LOG_ERR("Failed to start CAN device: %d", err);
            return err;
        }

        my_instance.iso14229.thread_start(&my_instance.iso14229);
    }

For more examples on how to setup the different UDS services with macros, see the code of the :ref:`uds-sample` and the documentation on the Macros in ``uds_macro.h``.

Default Instance Configuration
==============================

The default instance can be configured via Kconfig options:

**Enable/Disable Default Instance:**

.. code-block:: cfg

    # In prj.conf
    CONFIG_UDS_DEFAULT_INSTANCE=y  # Enabled by default

**CAN ID Configuration:**

You can either use static CAN IDs or provide them dynamically:

**Option 1a: Gearshift Address Provider (default)**

This is the default for ARDEP boards.
It uses the :ref:`gearshift-address-providers` module to determine CAN IDs based on gearshift position.


**Option 1b: External Address Provider**

.. code-block:: cfg

    # In prj.conf
    CONFIG_GEARSHIFT_UDS_ADDRESS_PROVIDER=n # needed on ardep boards to disable gearshift provider
    CONFIG_UDS_DEFAULT_INSTANCE_EXTERNAL_ADDRESS_PROVIDER=y

Then implement this function in your application:

.. code-block:: c

    UDSISOTpCConfig_t uds_default_instance_get_addresses(void) {
        UDSISOTpCConfig_t cfg = {
            .source_addr = /* your dynamic address */,
            .target_addr = /* your dynamic address */,
            .source_addr_func = /* your dynamic address */,
            .target_addr_func = /* your dynamic address */,
        };
        return cfg;
    }

**Option 2: Static CAN IDs**

.. code-block:: cfg

    # In prj.conf
    CONFIG_GEARSHIFT_UDS_ADDRESS_PROVIDER=n # needed on ardep boards to disable gearshift provider
    CONFIG_UDS_DEFAULT_INSTANCE_SOURCE_ADDRESS=0x7E8
    CONFIG_UDS_DEFAULT_INSTANCE_TARGET_ADDRESS=0x7E0
    CONFIG_UDS_DEFAULT_INSTANCE_FUNCTIONAL_SOURCE_ADDRESS=0x7DF
    CONFIG_UDS_DEFAULT_INSTANCE_FUNCTIONAL_TARGET_ADDRESS=0xFFFFFFFF


**Custom User Context:**

If your handlers need access to custom context data, override this weak function:

.. code-block:: c

    #include <ardep/uds.h>

    struct my_context {};
    static struct my_context ctx;

    void uds_default_instance_user_context(void **user_context) {
        *user_context = &ctx;
    }

**Disable Firmware Loader Switching:**

This is only needed if you either want to manage the programming session switch manually or if you dont use the firmware loader at all.

Note that this is enabled for the firmware loader to prevent boot-loops.

.. code-block:: cfg

    # In prj.conf
    CONFIG_UDS_DEFAULT_INSTANCE_DISABLE_SWITCH_TO_FIRMWARE_LOADER=y

ISO-TP Addressing
=================

The ``UDSISOTpCConfig_t`` structure describes the addressing scheme used on the CAN bus:
    
- ``source_addr``: The physical address the ECU listens to for "physical" requests (the address of the ECU). Default: ``0x7E8``

- ``target_addr``: The physical address the ECU uses in "physical" responses (the address of the Tester). Used for ECU → tester responses under physical addressing. Default: ``0x7E0``
      
- ``source_addr_func``: The functional/group request ID the ECU listens to for "functional" requests. Default: ``0x7DF``
      
- ``target_addr_func``: The ECU uses this address in "functional" responses. Usually set to ``UDS_TP_NOOP_ADDR`` (``0xFFFFFFFF``) as functional requests typically do not expect a response.


API Reference
*************

Event Handler Registration
===========================

This section describes the macros and functions available for registering handlers for each UDS service.

Read DTC Information (``0x19``)
--------------------------------

**Events**: ``UDS_EVT_ReadDTCInformation``

The Read DTC Information service supports multiple subfunctions. You can register handlers with different levels of granularity:

**Macros**:

- ``UDS_REGISTER_READ_DTC_INFO_HANDLER(_instance, _check, _act, _subfunc_id, _user_context)``
  
  Register a handler for a **specific subfunction** (e.g., ``UDS_READ_DTC_INFO_SUBFUNC__DTC_BY_STATUS_MASK``)

- ``UDS_REGISTER_READ_DTC_INFO_HANDLER_MANY(_instance, _check, _act, _user_context, ...)``
  
  Register a handler for **multiple subfunctions** by providing subfunction IDs as variadic arguments

- ``UDS_REGISTER_READ_DTC_INFO_HANDLER_ALL(_instance, _check, _act, _user_context)``
  
  Register a handler for **all subfunctions**

**Example**:

.. code-block:: c

    UDSErr_t my_check_dtc(const struct uds_context *ctx, bool *apply_action) {
        *apply_action = true;
        return UDS_OK;
    }

    UDSErr_t my_read_dtc(struct uds_context *ctx, bool *consume_event) {
        // Read and return DTC data
        *consume_event = true;
        return UDS_PositiveResponse;
    }

    UDS_REGISTER_READ_DTC_INFO_HANDLER(
        &instance,
        my_check_dtc,
        my_read_dtc,
        UDS_READ_DTC_INFO_SUBFUNC__DTC_BY_STATUS_MASK,
        NULL
    );

Memory Operations (``0x23``, ``0x3D``)
---------------------------------------

**Events**: ``UDS_EVT_ReadMemByAddr``, ``UDS_EVT_WriteMemByAddr``

**Macros**:

- ``UDS_REGISTER_MEMORY_HANDLER(_instance, _read_check, _read, _write_check, _write, _user_context)``
  
  Register **custom handlers** for memory read and write operations

- ``UDS_REGISTER_MEMORY_DEFAULT_HANDLER(_instance)``
  
  Register **default handlers** that support reading/writing RAM and Flash memory

**Default Handler Behavior**:

- Validates memory addresses are within RAM or Flash regions
- Performs bounds checking
- Uses safe memory access functions

**Example**:

.. code-block:: c

    // Use default handler for standard memory access
    UDS_REGISTER_MEMORY_DEFAULT_HANDLER(&instance);

    // Or register custom handler for specialized behavior
    UDS_REGISTER_MEMORY_HANDLER(
        &instance,
        my_mem_check_read,
        my_mem_action_read,
        my_mem_check_write,
        my_mem_action_write,
        &my_context
    );

ECU Reset (``0x11``)
--------------------

**Events**: ``UDS_EVT_EcuReset``, ``UDS_EVT_DoScheduledReset``

**Macros**:

- ``UDS_REGISTER_ECU_RESET_HANDLER(_instance, _reset_type, _ecu_reset_check, _ecu_reset, _do_scheduled_reset_check, _do_scheduled_reset, _user_context)``
  
  Register a **custom handler** for a specific reset type (e.g., ``ECU_RESET__HARD``)

- ``UDS_REGISTER_ECU_DEFAULT_HARD_RESET_HANDLER(_instance)``
  
  Register the **default handler** for hard reset operations

**Example**:

.. code-block:: c

    UDS_REGISTER_ECU_DEFAULT_HARD_RESET_HANDLER(&instance);

Data Identifier Operations (``0x22``, ``0x2E``, ``0x2F``)
----------------------------------------------------------

**Events**: ``UDS_EVT_ReadDataByIdent``, ``UDS_EVT_WriteDataByIdent``, ``UDS_EVT_IOControl``

These three services share the same data identifier space, so they use a common registration mechanism.

**Macros**:

- ``UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(_instance, _data_id, _data_ptr, _read_check, _read, _write_check, _write, _io_control_check, _io_control, _user_context)``

**Parameters**:

- ``_instance``: Pointer to the UDS server instance
- ``_data_id``: The 16-bit data identifier (DID)
- ``_data_ptr``: Pointer to the data buffer
- ``_read_check``, ``_read``: Check and action functions for read operations
- ``_write_check``, ``_write``: Check and action functions for write operations (set to ``NULL`` if not supported)
- ``_io_control_check``, ``_io_control``: Check and action functions for IO control operations (set to ``NULL`` if not supported)
- ``_user_context``: Optional user-defined context

**Example**:

.. code-block:: c

    static uint16_t vehicle_speed = 0;

    UDSErr_t read_speed_check(const struct uds_context *ctx, bool *apply) {
        *apply = true;
        return UDS_OK;
    }

    UDSErr_t read_speed_action(struct uds_context *ctx, bool *consume) {
        // Data is automatically copied from vehicle_speed
        *consume = true;
        return UDS_PositiveResponse;
    }

    UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(
        &instance,           // Instance
        0xF123,              // Custom DID
        &vehicle_speed,      // Data pointer
        read_speed_check,    // Read check
        read_speed_action,   // Read action
        NULL, NULL,          // No write support
        NULL, NULL,          // No IO control support
        NULL                 // User context
    );

Diagnostic Session Control (``0x10``)
--------------------------------------

**Events**: ``UDS_EVT_DiagSessCtrl``, ``UDS_EVT_SessionTimeout``

**Macros**:

- ``UDS_REGISTER_DIAG_SESSION_CTRL_HANDLER(_instance, _diag_session_ctrl_check, _diag_session_ctrl, _session_timeout_check, _session_timeout, _user_context)``

**Note**: This handler is **optional**. Session change requests succeed even without a custom handler. The library automatically manages session state.

**Accessing Current Session**:

.. code-block:: c

    struct uds_instance_t instance;
    uint8_t current_session = instance.iso14229.server.sessionType;

.. warning::

    If you use ``UDS_REGISTER_LINK_CONTROL_DEFAULT_HANDLER()``, it registers its own session handler. Ensure your custom handler doesn't consume session events if both are used.

Clear Diagnostic Information (``0x14``)
----------------------------------------

**Events**: ``UDS_EVT_ClearDiagnosticInfo``

**Macros**:

- ``UDS_REGISTER_CLEAR_DIAG_INFO_HANDLER(_instance, _check, _act, _user_context)``

Routine Control (``0x31``)
---------------------------

**Events**: ``UDS_EVT_RoutineCtrl``

**Macros**:

- ``UDS_REGISTER_ROUTINE_CONTROL_HANDLER(_instance, _routine_id, _check, _act, _user_context)``

The action function must handle all subfunctions:

- ``UDS_ROUTINE_CONTROL__START_ROUTINE`` (``0x01``)
- ``UDS_ROUTINE_CONTROL__STOP_ROUTINE`` (``0x02``)
- ``UDS_ROUTINE_CONTROL__REQUEST_ROUTINE_RESULTS`` (``0x03``)

Security Access (``0x27``)
---------------------------

**Events**: ``UDS_EVT_SecAccessRequestSeed``, ``UDS_EVT_SecAccessValidateKey``

**Macros**:

- ``UDS_REGISTER_SECURITY_ACCESS_HANDLER(_instance, _request_seed_check, _request_seed_act, _validate_key_check, _validate_key_act, _user_context)``

**Accessing Current Security Level**:

.. code-block:: c

    struct uds_instance_t instance;
    uint8_t security_level = instance.iso14229.server.securityLevel;

Communication Control (``0x28``)
---------------------------------

**Events**: ``UDS_EVT_CommCtrl``

**Macros**:

- ``UDS_REGISTER_COMMUNICATION_CONTROL_HANDLER(_instance, _check, _act, _user_context)``

Control DTC Setting (``0x85``)
-------------------------------

**Events**: ``UDS_EVT_ControlDTCSetting``

**Macros**:

- ``UDS_REGISTER_CONTROL_DTC_SETTING_HANDLER(_instance, _check, _act, _user_context)``

Dynamically Define Data Identifier (``0x2C``)
----------------------------------------------

**Events**: ``UDS_EVT_DynamicDefineDataId``

**Macros**:

- ``UDS_REGISTER_DYNAMICALLY_DEFINE_DATA_IDS_HANDLER(_instance, _check, _act, _user_context)``
  
  Register a **custom handler**

- ``UDS_REGISTER_DYNAMICALLY_DEFINE_DATA_IDS_DEFAULT_HANDLER(_instance)``
  
  Register the **default handler** (recommended)

.. important::

    Requires ``CONFIG_UDS_USE_DYNAMIC_REGISTRATION=y`` in your prj.conf

.. warning::

    Implementing this service requires managing internal UDS library structures. **Use the default handler unless you have specific requirements.**

Authentication (``0x29``)
--------------------------

**Events**: ``UDS_EVT_Auth``, ``UDS_EVT_AuthTimeout``

**Macros**:

- ``UDS_REGISTER_AUTHENTICATION_HANDLER(_instance, _auth_check, _auth_act, _timeout_check, _timeout_act, _user_context)``

**Managing Authentication State**:

Unlike session and security level, authentication state is **not stored internally**. You must manage it in your application.

**Recommended Approach**: Store authentication data in a user context:

.. code-block:: c

    struct my_auth_context {
        bool authenticated;
        uint8_t auth_level;
        // Additional authentication data
    };

    struct my_auth_context auth_ctx = { .authenticated = false };
    
    struct uds_instance_t instance;
    uds_init(&instance, &iso_tp_config, &can_dev, &auth_ctx);

    UDSErr_t my_auth_check(const struct uds_context *ctx, bool *apply_action) {
        struct my_auth_context *auth = 
            (struct my_auth_context *)ctx->instance->user_context;
        
        // Check authentication state
        if (auth->authenticated) {
            *apply_action = true;
            return UDS_OK;
        }
        return UDS_NRC_SECURITY_ACCESS_DENIED;
    }

Tester Present (``0x3E``)
--------------------------

**Events**: ``UDS_EVT_TesterPresent``

This service is handled automatically by the library. No custom event handlers are needed.

Link Control (``0x87``)
------------------------

**Events**: ``UDS_EVT_LinkControl``

**Macros**:

- ``UDS_REGISTER_LINK_CONTROL_HANDLER(_instance, _check, _act, _user_context)``
  
  Register a **custom handler**

- ``UDS_REGISTER_LINK_CONTROL_DEFAULT_HANDLER(_instance)``
  
  Register the **default handler** (recommended)

**Configuration**:

.. code-block:: cfg

    # In prj.conf
    CONFIG_UDS_USE_LINK_CONTROL=y
    CONFIG_UDS_DEFAULT_CAN_BITRATE=500000

.. important::

    - Requires ``CONFIG_UDS_USE_LINK_CONTROL=y``
    - Must set ``CONFIG_UDS_DEFAULT_CAN_BITRATE`` to your CAN interface's default baudrate because the default bitrate cannot be queried in code.

**Default Handler Behavior**:

- Handles baudrate transitions
- Automatically restores default baudrate on session timeout
- Registers its own session control handler

.. warning::

    The default link control handler registers a session event handler. If you use both this and a custom session handler, ensure your handler **does not consume** session events.

Data Transfer Services (``0x34``, ``0x35``, ``0x36``, ``0x37``, ``0x38``)
--------------------------------------------------------------------------

**Events**: ``UDS_EVT_RequestDownload``, ``UDS_EVT_RequestUpload``, ``UDS_EVT_TransferData``, ``UDS_EVT_RequestTransferExit``, ``UDS_EVT_RequestFileTransfer``

These services are handled internally by the library and **do not support custom handlers**.

These services read from and write to flash memory or the file system.
Note that they do not perform flash erase operations; any required erasure must be done beforehand (for example, via a routine).

**Configuration**:

.. code-block:: cfg

    # In prj.conf
    CONFIG_UDS_FILE_TRANSFER=y              # Required for file transfer (0x38)

Utility Functions
=================

``uds_get_isotp_config()``
--------------------------

Retrieves the current ISO-TP configuration from a UDS instance.

**Signature**:

.. code-block:: c

    int uds_get_isotp_config(struct uds_instance_t *inst,
                             UDSISOTpCConfig_t *iso_tp_config);

**Parameters**:

- ``inst``: Pointer to the UDS instance
- ``iso_tp_config``: Pointer to structure to receive the configuration

**Returns**:

- ``0`` on success
- ``-EINVAL`` if either parameter is NULL

**Example**:

.. code-block:: c

    UDSISOTpCConfig_t config;
    int err = uds_get_isotp_config(&uds_default_instance, &config);
    if (err == 0) {
        LOG_INF("Source address: 0x%03X", config.source_addr);
        LOG_INF("Target address: 0x%03X", config.target_addr);
    }

This function is useful when you need to query the current addressing configuration, for example when implementing custom CAN communication alongside UDS.

Dynamic Registration
====================

For scenarios where handlers need to be registered at runtime (e.g., based on configuration files, runtime conditions), the library supports dynamic registration.

**Prerequisites**:

.. code-block:: cfg

    # In prj.conf
    CONFIG_UDS_USE_DYNAMIC_REGISTRATION=y

**Registration Process**:

1. Create a ``struct uds_registration_t`` object with the desired configuration
2. Call ``instance.register_event_handler()``
3. Store the returned ``dynamic_id`` for later unregistration

**Example**:

.. code-block:: c

    struct uds_instance_t instance;
    
    // Create registration
    struct uds_registration_t reg = {
        .instance = &instance,
        .type = UDS_REGISTRATION_TYPE__DATA_IDENTIFIER,
        .data_identifier = {
            .data_id = 0xF190,
            .data = &my_vin_data,
            .read = {
                .check = my_read_check,
                .action = my_read_action
            },
            .user_context = NULL
        }
    };
    
    // Register dynamically
    uint32_t dynamic_id;
    struct uds_registration_t *reg_ptr;
    int ret = instance.register_event_handler(&instance, reg, &dynamic_id, &reg_ptr);
    
    if (ret == 0) {
        // Registration successful
    }
    
    // Later, unregister
    instance.unregister_event_handler(&instance, dynamic_id);

**Notes**:

- Examine the static registration macros in ``ardep/uds_macro.h`` for guidance on structuring registration objects
- Dynamic handlers are checked **after** static handlers during event processing
- Returns ``-ENOSPC`` if all dynamic IDs are exhausted (1 to UINT32_MAX)

Advanced Topics
***************

Internals and Architecture
===========================

The UDS library uses Zephyr's `Iterable Sections <https://docs.zephyrproject.org/4.2.0/kernel/iterable_sections/index.html>`_ for static event handlers and a singly-linked list for dynamic handlers.

**Event Processing Order**:

1. **Static handlers** (defined via macros) - stored in iterable sections
2. **Dynamic handlers** (registered at runtime) - stored in linked list
3. **Default negative response** - if no handler processed the event

**Performance Considerations**:

- Static handlers have O(n) lookup time but zero memory allocation overhead
- Dynamic handlers also have O(n) lookup but require heap allocation

Handler Interaction
===================

Multiple handlers can be registered for the same event type. The library processes them in order until one consumes the event.

**Event Consumption**:

- Set ``consume_event = true`` to stop further processing
- Set ``consume_event = false`` to allow subsequent handlers to process the event

**Use Cases for Non-Consuming Handlers**:

- Logging/monitoring without affecting normal processing

Best Practices
==============

1. **Use Default Handlers When Possible**
   
   The library provides tested implementations for common operations (memory access, ECU reset, link control). Use them unless you have specific requirements.

2. **Always Set consume_event**
   
   Explicitly set the ``consume_event`` flag in your action functions. Don't rely on default values.

3. **Validate in Check Functions**
   
   Perform all validation in the ``check`` function. The ``action`` function should assume preconditions are met.

4. **Return Appropriate NRCs**
   
   Use ISO 14229-1 defined negative response codes (``UDS_NRC_*``) to provide meaningful feedback to diagnostic clients.

5. **Manage State Carefully**
   
   For services like Authentication that don't store internal state, use the ``user_context`` to maintain necessary information.

6. **Test Session and Security Interactions**
   
   Many UDS services require specific diagnostic sessions or security levels. Ensure your handlers check these preconditions.

Troubleshooting
===============

**Handler Not Being Called**

- Verify the handler is registered (check return value of dynamic registration)
- Ensure ``check`` function returns ``UDS_OK`` and sets ``apply_action = true``
- Check if an earlier handler consumed the event
- For static handlers, verify the macro is at file scope (not in a function)

**Wrong NRC Returned**

- Check that your ``check`` function returns the correct NRC
- Verify your ``action`` function doesn't override the NRC on error paths
- Ensure you're not consuming events that should be handled by later handlers

**Link Control Issues**

- Verify ``CONFIG_UDS_DEFAULT_CAN_BITRATE`` matches your hardware configuration
- Check CAN driver supports runtime baudrate changes
- Ensure session handler doesn't consume events when using default link control

**Dynamic Registration Fails**

- Confirm ``CONFIG_UDS_USE_DYNAMIC_REGISTRATION=y``
- Check available heap memory
- Verify you're not exceeding UINT32_MAX registrations

Further Reading
===============

- :ref:`iso14229-lib` - Underlying ISO-TP and UDS protocol implementation
- `ISO 14229-1:2020 <https://www.iso.org/standard/72439.html>`_ - UDS specification
- `Zephyr Iterable Sections <https://docs.zephyrproject.org/latest/kernel/iterable_sections/index.html>`_ - Understanding static registration