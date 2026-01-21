.. _error_handling:

Error Handling
##############


.. contents::
   :local:
   :depth: 2

.. note::

    The following instructions only apply to Ardep v1 since later versions have an :ref:`on_board_debugger` and the bootloader logs are accessible via the forwarded UART-A device.
    Additionally, you can flash the bootloader and the firmware via ``west flash`` using the on-board debugger.
    
    **All ARDEP boards** now use the unified firmware loader by default, which supports both USB DFU and UDS DFU for firmware updates.

When encountering an error, you have several options, depending on the severity of the error.
Generally, it is helpful to see if your application gets started from the bootloader.
See the :ref:`console` documentation for more information about where you find the application and bootloader output.

If your application keeps running, you can just perform an update via the firmware loader. See the :ref:`getting_started` guide for an example.
The unified firmware loader supports both USB DFU and UDS DFU update mechanisms.
    
If your application keeps hanging itself up (e.g. segfaults) and a firmware loader update is not possible, boot into the :ref:`bootloader_mode` and flash a new application from there.

If you can't get into bootloader mode, you should delete the whole flash memory and flash a new bootloader and application.
For this, you need to connect an external debugger on the SWD pins on the board (or use the on-board debugger on v2 boards).



To build the bootloader with the unified firmware loader, you can use sysbuild:

    ``west build -b ardep samples/led -p --sysbuild``
    
Then erase the flash memory of the ardep board.

Next, flash the bootloader. You can use a debugger of your choice or use the ``west flash`` command.
For an overview of the ``west flash`` command see `Zephyr's documentation on the west command <https://docs.zephyrproject.org/4.2.0/develop/west/build-flash-debug.html#flashing-west-flash>`_, ``west flash -h`` and the *board.cmake* file under *boards/mercedes/ardep* for an overview of supported runners.

**Important**: The bootloader and firmware loader can ONLY be flashed via on-board debugger (v2) or external debug probe (v1).

After flashing the bootloader, flash the application of your choice. See the :ref:`getting_started` guide and the :ref:`bootloader_mode` documentation for this.
