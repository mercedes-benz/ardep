.. _iso14229-lib:

ISO14229 Library
################

Overview
********

The ISO14229 library provides a lightweight Zephyr-based wrapper around the `iso14229 library <https://github.com/driftregion/iso14229>`_ by driftregion. It enables implementation of ISO 14229-1 (Unified Diagnostic Services) compliant servers on Zephyr RTOS with minimal integration effort.

This library serves as the **low-level foundation** for the :ref:`uds-lib`, which provides a higher-level, event-driven API for building UDS servers. Most applications should use the UDS library instead of directly interfacing with ISO14229.

Key Features
============

- **Event-Driven Architecture**: Callback-based design for handling diagnostic requests
- **Minimal API Surface**: Simple initialization and callback registration
- **Optional Threading**: Built-in thread management eliminates the need for manual polling
- **ISO-TP Integration**: Handles CAN transport layer (ISO 15765-2) automatically

Architecture
============

The library consists of three main components:

1. **ISO-TP Layer**: Manages segmented CAN message transmission and reception
2. **UDS Server**: Processes diagnostic service requests and generates responses
3. **Event Callback System**: Notifies application code of incoming diagnostic events

.. code-block:: none

    ┌─────────────────────────────────────────┐
    │     Application (Event Handler)         │
    └──────────────────┬──────────────────────┘
                       │ Callbacks
    ┌──────────────────▼──────────────────────┐
    │    ISO14229 Zephyr Wrapper (this lib)   │
    │  ┌────────────────────────────────────┐ │
    │  │   UDS Server (iso14229 upstream)   │ │
    │  └────────────────────────────────────┘ │
    │  ┌────────────────────────────────────┐ │
    │  │     ISO-TP (CAN Transport)         │ │
    │  └────────────────────────────────────┘ │
    └──────────────────┬──────────────────────┘
                       │ CAN Frames
    ┌──────────────────▼──────────────────────┐
    │      Zephyr CAN Driver (can_dev)        │
    └─────────────────────────────────────────┘

When to Use This Library
========================

**Use ISO14229 library directly if:**

- You need fine-grained control over UDS event handling
- You're implementing custom or non-standard diagnostic protocols
- You want to minimize memory overhead

**Use the higher-level UDS library if:**

- You're building a standard ISO 14229-1 compliant server
- You want to register handlers for specific services/subfunctions
- You need built-in support for session management, security access, etc.

Example
*******

See :ref:`iso14229-sample`. The sample demonstrates:

- ISO-TP configuration
- CAN device initialization
- Event callback registration
- Session control handling

