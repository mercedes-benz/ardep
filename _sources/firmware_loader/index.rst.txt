.. _firmware_loader:

Firmware Loader
###############

This firmware loader is used to update **application** firmware on ARDEP devices. It supports both **UDS DFU** (Unified Diagnostic Services over CAN) and **USB DFU** (Device Firmware Update via USB), providing flexible firmware update mechanisms for all ARDEP board versions.

The bootloader runs this firmware under certain conditions, such as when the *BOOT* labeled jumper is set during start up,
when ``uds_switch_to_firmware_loader_with_programming_session()`` is called (or automatically with the UDS default instance), or when no valid application is flashed.

The firmware loader uses the UDS library's default instance for simplified UDS server setup, automatically handling CAN configuration and session management. It also provides USB DFU functionality for USB-based firmware updates.

.. important::

    Only applications can be updated via the firmware loader. Not the bootloader or the firmware loader itself.

For this firmware loader to work as intended, use sysbuild to build the bootloader with this firmware loader.

.. note::
    **Default for all boards**: This unified firmware loader is now the default boot logic for all ARDEP board versions (v1 and v2), eliminating the previous distinction between boards. Both UDS DFU and USB DFU are supported on all boards for application updates.
    
    **Upgrading from older ARDEP versions**: If you're using an older ARDEP repository version that differentiated between board versions, you must upgrade your bootloader via sysbuild using a debugger (OBD or external probe) to install the unified firmware loader.


Usage
+++++

The firmware loader supports two update mechanisms:

**UDS DFU (CAN-based updates):**

This firmware registers an *erase slot0* routine with id ``0xFF00`` which, if started via UDS, erases slot0 (the application slot).

Then, using `RequestDownload`, `TransferData`, `RequestTransferExit` the application can be updated. Finally, use an `ECUReset` to let mcuboot boot into the fresh application.

**USB DFU (USB-based updates):**

When the firmware loader is active and the device is connected via USB, it can accept firmware updates using the standard ``dfu-util`` tool.

- **v1 boards**: The default ``west flash`` runner uses dfu-util, so simply set the BOOT jumper, reset the board, and run ``west flash``
- **v2 boards**: To use USB DFU instead of the OBD run ``west flash --runner ardep``. Note that is usually better to use the on-board debugger (OBD) for flashing v2 boards.


Flashing firmware
+++++++++++++++++

If your application supports UDS firmware loading, you can use the :ref:`ardep_uds` runner to flash the new firmware.


Building using sysbuild
+++++++++++++++++++++++

See :ref:`uds_bootloader` on building an application together with this firmware loader.

The firmware loader is automatically included when building with sysbuild and provides both UDS and USB DFU support by default.

A good starting point in using this loader is using the :ref:`uds-sample`.
