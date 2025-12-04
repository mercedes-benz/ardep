.. _can-log:

CAN Log Library
###############

Overview
********

The CAN Log library provides a log backend for logging messages over a CAN bus.

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

Receiving Logs
**************

The CAN frames that are sent via the ID configured with ``CONFIG_CAN_LOG_ID`` contain the raw log message data, 8 bytes per frame maximum.
To receive and decode these log messages, you can use the provided ``can_log_receiver.py`` script located in ``scripts/`` or use the ``west ardep can-log-receiver`` command. There you can setup your CAN interface via ``-i can0`` and the CAN ID to listen to via ``-id``.
