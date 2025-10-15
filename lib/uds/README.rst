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

**Configuration**:

.. code-block:: cfg

    # In prj.conf
    CONFIG_UDS_FILE_TRANSFER=y              # Required for file transfer (0x38)

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