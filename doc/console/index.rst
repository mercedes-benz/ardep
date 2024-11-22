.. _console:

Console Output
##############

.. contents::
   :local:
   :depth: 2

Fimrware Output
===============

Per default, every application build for the *ardep* board will include a cdc-acm driver.
This means, when connecting the board via usb to your pc, a new serial device should be visible in your system (usually under */dev/ttyACM0* or similar).
You can use this device to communicate with the board. The default baudrate is 115200.

For example, you can use the *screen* command to connect to the board:

.. code-block:: bash

    screen /dev/ttyACM0 115200


Bootloader Output
=================

Since the bootloader does not contain this driver, its output will not be redirected and instead be transmitted via UART on the UART-A pins.
You will need an external UART to USB adapter if you want to see the bootloader output on you pc.
