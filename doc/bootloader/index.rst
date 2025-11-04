.. _bootloader:
   
Bootloader
##########

.. contents::
   :local:
   :depth: 2
   
Building the bootloader
+++++++++++++++++++++++

The bootloader can be built using sysbuild together with a sample for flashing. To reset the board to a delivery state, use 

.. code-block:: bash

    west build -b ardep samples/led -p --sysbuild

to build the :ref:`led_sample` and bootloader

If you want to build for a specific board version, change the `-b` argument accordingly:

.. code-block:: bash

    west build -b ardep@1.0.0 samples/led -p --sysbuild


Flashing the bootloader
+++++++++++++++++++++++


.. tabs::

    .. tab:: Ardep v2.0.0 and later

        The bootloader is flashed via the on-board debugger using the `west flash` command.

        Just run:

        .. code-block:: bash

            west flash

        This will flash the bootloader to the board.
        

    .. tab:: Ardep v1.0.0

        The bootloader is flashed with an external debug probe (e.g. J-Link) using the `west flash` command.

        For this connect the SWD Pins of the ARDEP board to the debug probe an run:

        .. code-block:: bash
           
            west flash -r {runner}

        The Pinout of the SWD connector is printed on the boards backside.

        See the `board.cmake` file under `boards/arm/ardep` for more info about supported debuggers/runners.


.. _bootloader_mode:

Bootloader mode
+++++++++++++++


.. note::

    This section only applies to Ardep v1 since later versions have an :ref:`on_board_debugger`.

    This means you can just build and flash the bootloader like any other firmware.

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
    

.. note::

    ``UART-A`` is the output that is forwarded by the on-board debugger.

Upgrading the firmware
======================

- Build the firmware you want to flash (assuming it is in the *build* directory)
- Perform the upgrade with ``west flash``
- Wait for the upgrade to complete
