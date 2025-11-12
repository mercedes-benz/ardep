.. _console:

Console Output
##############

.. contents::
   :local:
   :depth: 2

Fimrware Output
===============

.. tabs::


    .. tab:: Ardep v2.0.0 and later

        The output of the application is send on ``UART-A`` and additionally forwarded via the on-board debugger to a cdc-acm device on the host system.

    .. tab:: Ardep v1.0.0

       Per default, every application build for the *ardep* board will include a cdc-acm driver.
       This means, when connecting the board via usb to your pc, a new serial device should be visible in your system.


You can use the cdc-acm device to communicate with the board. The default baudrate is 115200.

.. tabs::

    .. tab:: Linux
    
        The serial device is usually displayed as */dev/ttyACMx*, */dev/ttyUSBx* or similar.

        Use a serial terminal emulator (e.g. *screen*, *minicom*, *picocom*) to connect to the device.
        
        .. code-block:: bash

            screen /dev/ttyACM0 115200

    .. tab:: Windows

        The serial device is usually displayed in your *Windows Device Manager* under *Ports (COM & LPT)* e.g. as *COM5*.

        Use *putty* or a similar terminal emulator to connect to the device.

        Connect to the COM port by selecting *Serial* in the *Connection type* section and entering the COM port number (e.g. *COM5*) in the *Serial line* field, setting the speed to 115200.

Bootloader Output
=================

The bootloader output is by default available on the ``UART-A`` interface.

.. tabs::

    .. tab:: Ardep v2.0.0 and later

       Just as the firmwares output, the bootloaders output is also forwarded via the on-board debugger to a cdc-acm device on the host system.

    .. tab:: Ardep v1.0.0

       The bootloader does not contain the cdc-acm driver and is only emitted via ``UART-A``.
       If you want to see the bootloader output on your pc, you will need an external UART to USB adapter connected to the ``UART-A`` pins.
