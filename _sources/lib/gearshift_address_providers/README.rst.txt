.. _gearshift-address-providers:

Gearshift Address Providers
############################

Overview
********

The Gearshift Address Providers library enables dynamic CAN address configuration based on the ARDEP board's :ref:`gearshift` position. This allows multiple devices on the same CAN bus to automatically configure unique addresses without manual configuration or multiple firmware builds.

The gearshift address providers are enabled by default for all ARDEP boards, if UDS is enabled, which is also the default.

Key Features
============

- **UDS Address Provider**: Configure UDS CAN IDs based on gearshift position
- **CAN Log Address Provider**: Configure CAN logging ID based on gearshift position

How It Works
************

The library reads the gearshift position from GPIO pins on the ARDEP board during initialization. The gearshift position (0-7) is then used to calculate unique CAN addresses for UDS and logging services.

Gearshift Reading
=================

The gearshift position is read via the gearshift driver (see the code of the :ref:`gearshift_sample` for an example).
The gearshift position is read only once during boot.

Address Providers
*****************

UDS Address Provider
====================

When enabled (enabled by default on ARDEP boards), this provider automatically configures UDS CAN addresses based on gearshift position. It integrates seamlessly with the UDS library's default instance by implementing the external address provider interface (``CONFIG_UDS_DEFAULT_INSTANCE_EXTERNAL_ADDRESS_PROVIDER``).

The provider supplies physical source and target addresses dynamically based on gearshift position:

**Configuration**:

.. code-block:: cfg

    # In prj.conf
    CONFIG_GEARSHIFT_UDS_ADDRESS_PROVIDER_BASE_PHYS_SA=0x7E8
    CONFIG_GEARSHIFT_UDS_ADDRESS_PROVIDER_BASE_PHYS_TA=0x7E0

**Address Calculation**:

.. code-block:: none

    Physical Source Address = BASE_PHYS_SA + gearshift_position
    Physical Target Address = BASE_PHYS_TA + gearshift_position

**Example**:

For gearshift position 2 with default configuration:

- Physical Source Address: ``0x7E8 + 2 = 0x7EA``
- Physical Target Address: ``0x7E0 + 2 = 0x7E2``


CAN Log Address Provider
=========================

When enabled (enabled by default on ARDEP boards), this provider configures the CAN logging ID based on gearshift position:

**Configuration**:

.. code-block:: cfg

    # In prj.conf
    CONFIG_GEARSHIFT_CAN_LOG_ADDRESS_PROVIDER_BASE_ID=0x100

**Address Calculation**:

.. code-block:: none

    CAN Log ID = BASE_ID + gearshift_position

**Example**:

For gearshift position 2 with default configuration:

- CAN Log ID: ``0x100 + 2 = 0x102``

**Usage**:

The provider automatically integrates with the CAN log library:

.. code-block:: cfg

    CONFIG_CAN_LOG=y
    CONFIG_GEARSHIFT_CAN_LOG_ADDRESS_PROVIDER=y

The CAN log library will automatically call the gearshift address provider to determine the logging CAN ID.
