.. _bootloader:
   
Bootloader
##########

.. contents::
   :local:
   :depth: 2
   
Building the bootloader
+++++++++++++++++++++++
   
The bootloader can be build with:

.. code-block:: bash
   
    west ardep build-bootloader

The first line of the output gives you the expanded `west build` command.


Flashing the bootloader
+++++++++++++++++++++++

The bootloader is flashed with an external debug probe (e.g. J-Link) using the `west flash` command.

For this connect the SWD Pins of the ARDEP board to the debug probe an run:

.. code-block:: bash
   
    west flash

The Pinout of the SWD connector is printed on the boards backside.

See the `board.cmake` file under `boards/arm/ardep` for more info about supported debuggers.


.. _bootloader_mode:

Bootloader mode
+++++++++++++++


In this mode, the ardep board does not load any firmware and waits for a firmware upgrade via the ``dfu-util`` tool.
This is handy if your firmware is broken and you can't update it from there.

It is helpful to see the Bootloader console output on ``UART-A`` for this.


Entering Bootloader mode
========================

To enter the bootloader mode, pull the ``PE4`` pin (labeld *BOOT*) to a LOW (setting the jumper) state while power-cycling or pushing the Reset button.

When the red led light up permanently, the board is in bootloader mode.

On ``UART-A`` you will see the following output:

.. code-block::

    *** Booting Zephyr OS build zephyr-vx.y.z ***
    I: Starting bootloader
    I: Waiting for USB DFU

Upgrading the firmware
======================

- Build the firmware you want to flash (assuming it is in the *build* directory)
- Perform the upgrade with ``west flash``
- Wait for the upgrade to complete
