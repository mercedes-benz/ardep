.. _bootloader_mode:

Bootloader mode
###############

.. contents::
   :local:
   :depth: 2

In this mode, the ardep board does not load any firmware and waits for a firmware upgrade via the ``dfu-util`` tool.
This is handy if your firmware is broken and you can't update it from there.

It is helpful to see the Bootloader console output on ``UART-A`` for this.


Entering Bootloader mode
========================

To enter the bootloader mode, set the ``XXXXX`` Jumper to a HIGH/LOW state while power-cycling or pushing the Reset button.

When the red led light up permanently, the board is in bootloader mode.

On ``UART-A`` you will see the following output:

.. code-block::

    *** Booting Zephyr OS build zephyr-vx.y.z ***
    I: Starting bootloader
    I: Waiting for USB DFU

Upgrading the firmware
======================

- Build the firmware you want to flash (assuming it is in the *build* directory)
- Perform the upgrade with ``west ardep dfu --bootloader --build-dir build``
- Wait for the upgrade to complete:
    - If you can't see the bootloader console, wait >20 seconds.
    - If you can see the output, wait for the *panic!* message.
- Perform a manual power cycle. Your new firmware should be running now

