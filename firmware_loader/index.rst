.. _firmware_loader:

UDS Firmware Loader
###################

This firmware loader is used to update the firmware on ARDEP devices.
The bootloader runs this firmware under certain conditions, such as when the *BOOT* labeled jumper is set during start up or when ``uds_switch_to_firmware_loader_with_programming_session()`` is called.

For this firmware loader to work as intended, use sysbuild to build the bootloader with this firmware loader.

A good starting point in using this loader is using the :ref:`uds-sample`.


Usage
+++++

This firmware registers an erase slot0 routine with id ``0xFF00`` which, if started via UDS, erases slot0 (the application slot).

Then, using `RequestDownload`, `TransferData`, `RequestTransferExit` the application can be updated. Finally, use an `ECUReset` to let mcuboot boot into the fresh application.


Building using sysbuild
+++++++++++++++++++++++

See :ref:`uds_bootloader` on building an application together with this firmware loader.

If you need to build this firmware loader by itself make sure, that the bootloader was built in sysbuild with the firmware loader boot logic enabled.
