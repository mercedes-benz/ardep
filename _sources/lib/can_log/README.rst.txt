.. _can-log:

CAN Log Library
###############

Overview
********

The CAN Log library provides a log backend for logging messages over a CAN bus.

It is enabled by default for the ARDEP boards.

Configuration
*************

To enable the CAN Log library, setup logging in your project and enable the CAN log backend by adding the following to your ``prj.conf``:

.. code-block:: ini

    CONFIG_CAN=y
    CONFIG_LOG=y

    CONFIG_CAN_LOG=y
    CONFIG_CAN_LOG_ID=0x100

    # Setup the preferred log mode and processing strategy
    CONFIG_LOG_MODE_DEFERRED=y
    CONFIG_LOG_PROCESS_THREAD=y

If your project does not start CAN itself, you also need to autostart it by adding ``CONFIG_CAN_LOG_AUTOSTART_BUS=y`` which will start CAN on the default bus during initialization.

Dynamic CAN ID
**************

For the ardep boards, the can id is provided by the :ref:`gearshift-address-providers` module by default.

You can disable this module by setting ``CONFIG_GEARSHIFT_CAN_LOG_ADDRESS_PROVIDER=n``

Custom Address Provider
***********************

If the gearshift can log address provider is disabled, you can provide your own CAN log ID by setting ``CONFIG_CAN_LOG_ADDRESS_PROVIDER_EXTERNAL=y`` and implementing the ``can_log_get_id()`` function in your code:

.. code-block:: c

    #include <ardep/can_log.h>

    uint16_t can_log_get_id() {
        // Return dynamically determined CAN ID
        // For example, read from hardware pins or configuration
        return my_calculated_id;
    }

When this function is implemented, it overrides the ``CONFIG_CAN_LOG_ID`` configuration value. The function is called during CAN log initialization to determine the CAN ID to use for log messages.

This is particularly useful in multi-node systems where each node needs a unique logging ID, or when the ID needs to be determined based on external factors like gearshift position or other methods of device addressing.

Receiving Logs
**************

The CAN frames that are sent via the ID configured with ``CONFIG_CAN_LOG_ID`` contain the raw log message data, 8 bytes per frame maximum.
To receive and decode these log messages, you can use the provided ``can_log_receiver.py`` script located in ``scripts/`` or use the ``west ardep can-log-receiver`` command. There you can setup your CAN interface via ``-i can0`` and the CAN ID to listen to via ``-id``.
