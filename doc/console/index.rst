.. _console:

Console Output
##############

.. contents::
   :local:
   :depth: 2

Fimrware Output
===============

Per default, every application build for the *ardep* board will include a cdc-acm driver.
This means, when connecting the board via usb to your pc, a new serial device should be visible in your system.
You can use this device to communicate with the board. The default baudrate is 115200.

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

Since the bootloader does not contain this driver, its output will not be redirected and instead be transmitted via UART on the UART-A pins.
You will need an external UART to USB adapter if you want to see the bootloader output on you pc.
