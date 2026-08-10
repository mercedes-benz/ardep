.. _console:

Console Output
##############

.. contents::
   :local:
   :depth: 2

Firmware Output
===============

The application output is sent on ``UART-A``.

.. tabs::


    .. tab:: Ardep v2.0.0 and later

        The output of the application is additionally forwarded via the on-board debugger to a USB cdc-acm device on the host system.

    .. tab:: Ardep v1.0.0

       By default, every application built for the *ardep* board will include a USB cdc-acm driver.
       This means, when connecting the board via USB to your PC, a new serial device should be visible in your system.


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

Bootloader and Firmware Loader Output
======================================

The bootloader and firmware loader output is available on the ``UART-A`` interface.

.. tabs::

    .. tab:: Ardep v2.0.0 and later

       Just as the firmware's output, the bootloader and firmware loader output is also forwarded via the on-board debugger to a cdc-acm device on the host system.

    .. tab:: Ardep v1.0.0

       The bootloader and firmware loader do not contain the cdc-acm driver and only emit output via ``UART-A``.
       If you want to see the bootloader or firmware loader output on your PC, you will need an external UART to USB adapter connected to the ``UART-A`` pins.

.. note::
    
    The unified firmware loader is now used by default on all board versions, supporting both UDS DFU and USB DFU update mechanisms.
