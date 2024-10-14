.. _error_handling:

Error Handling
##############

.. contents::
   :local:
   :depth: 2

When encountering an error, you have several options, depending on the severity of the error.
Generally, it is helpful to see if your application gets started from the bootloader.
See the :ref:`console` documentation for more information about where you find the application and bootloader output.

If your application keeps running, you can just perform an update via dfu-util. See the :ref:`getting_started` guide for an example.
    
If you application keeps hangig itself up (e.g. segfaults) and a dfu is not possible, boot into the :ref:`bootloader_mode` and flash a new application from there.

If you can't get into bootloader mode, you should delete the whole flash memory and flash a new bootloader and application.
For this, you need to connect an external debugger on the SWD pins on the board.



To build the bootloader, you can use the following command:

    ``west ardep build-bootloader``
    
Then erase the flash memory of the ardep board.

Next, flash the bootloader. You can use a debugger of your choice or use the ``west flash`` command.
For an overview of the ``west flash`` command see `Zephyr's documentation on the west command <https://docs.zephyrproject.org/3.5.0/develop/west/build-flash-debug.html#flashing-west-flash>`_ and the *board.cmake* file under *boards/arm/ardep* for an overview of supported runners.

After flashing the bootloader, flash the application of your choice. See the :ref:`getting_started` guide and the :ref:`bootloader_mode` documentation for this.
