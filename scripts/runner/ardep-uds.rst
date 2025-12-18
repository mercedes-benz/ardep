.. _ardep_uds:

ARDEP UDS Runner
################

Using the ARDEP UDS runner, you can flash firmware images to ARDEPs with UDS support. ARDEP v2 device have this enabled by default and can be flashed over CAN using UDS protocol.
ARDEP v1 devices can also be flashed using UDS, but require to flash the UDS bootloader first using sysbuild, also see :ref:`uds_bootloader`.

Usage
+++++

.. important::

    Make sure not to use sysbuild while building the new firmware, as the runner can only flash the application binary, not the bootloader or firmware loader.


Example command:

.. code-block:: shell

    west flash --runner ardep-uds

If you want to use a different gearshift address, specify it with the ``--gearshift`` option (valid range: 0-7):

.. code-block:: shell

    west flash --runner ardep-uds --gearshift 2

If you want to use custom UDS CAN IDs, specify them with the ``--uds-source-id`` and ``--uds-target-id`` options:

.. code-block:: shell

    west flash --runner ardep-uds --uds-source-id 0x7E0 --uds-target-id 0x7E8

.. note::

    The **source** address of the UDS client (i.e., this ARDEP-UDS runner) must match the **target** address of the UDS server (i.e., the ARDEP device being flashed) and vice versa.

    Also note, that you cannot use both the ``--gearshift`` option and custom UDS addresses at the same time, as gearshift changes the UDS addresses automatically.

If the upload fails due to timeouts, you can try reducing the block size using the ``--block-size`` option (``128`` bytes is usually a safe choice):

.. code-block:: shell

    west flash --runner ardep-uds --block-size 128


UDS Flow
++++++++

The :ref:`firmware_loader` needs the following UDS services to be called in this order to flash new firmware, this runner implements this flow as following:

1. Start a programming session using `DiagnosticSessionControl` (0x10) with sub-function 0x02 (*Programming Session*).
2. Wait for the UDS server (the ARDEP device) to switch to the firmware loader. For this, use `TesterPresent` (0x3E) periodically until the server responds.
3. Erase slot0 (the application slot) using the custom erase routine with id ``0xFF00``. For this, use `RoutineControl` (0x31) with sub-function 0x01 (*Start Routine*) and routine id ``0xFF00``.
4. Wait for the erase to complete by waiting for the Server to respond again (use `TesterPresent` periodically) and finally check the routine status using `RoutineControl` with sub-function 0x03 (*Request Routine Results*). The expected payload is ``0x0000`` indicating success.
5. Upload the new firmware using `RequestDownload` (0x34), `TransferData` (0x36) and `RequestTransferExit` (0x37).
   The size of the TransferData blocks should be less than or equal 512 bytes, higher sizes might lead to timeouts.
   Check which size works best for your setup.
   Also note, that the `TransferData` block sequence counter should start at 1 and may wrap around after reaching 0xFF (back to a 0).
6. Finally, reset the device using `ECUReset` (0x11) with sub-function 0x01 (*Hard Reset*), which will make mcuboot boot into the newly flashed application, if the bootloader jumper is not set.

