.. _can_usb_bridge_sample:

CAN USB Bridge sample
#####################

This sample uses the application from the Cannectivity repository and just provides overlays for compatability with the ARDEP board.

It creates a CAN to USB bridge connecting *CAN A* of the ARDEP board to the USB Host.

The `Cannectivity repository <https://github.com/CANnectivity/cannectivity>`_ is automatically downloaded as a Zephyr module with ``west update`` on workspace setup. You can find it under ``{WORKSPACE_ROOT}/modules/lib/cannectivity``.

As a *one-time setup*, you need to install the Cannectivity UDEV rule (analogous to the one we installed in :ref:`getting_started` guide):

.. code-block::

    sudo cp {WORKSPACE_ROOT}/modules/lib/cannectivity/99-cannectivity.rules /etc/udev/rules.d/
    sudo udevadm control --reload-rules
    sudo udevadm trigger

.. note::

    Since the app manages the usb stack, there will be no USB-CDC-ACM device present to see the logs.
    Instead, they are redirected to UART-A (The same UART the bootloader uses).


Flash and run the example
-------------------------

Build the example with:

.. code-block::

    west build --board ardep ../modules/lib/cannectivity/app -- \
        -DFILE_SUFFIX=usbd_next \
        -DEXTRA_CONF_FILE=../../../../ardep/samples/can_usb_bridge/cannectivity.conf \
        -DEXTRA_DTC_OVERLAY_FILE=../../../../ardep/samples/can_usb_bridge/cannectivity.overlay
        
The ``FILE_SUFFIX`` selects the `prj_usbd_next.conf` file from the Cannectivity app folder, which enables the new USB stack.

The ``EXTRA_CONF_FILE`` and ``EXTRA_DTC_OVERLAY_FILE`` options specify the additional configuration and overlay files found in the samples directory.
The path is relative to the application directory.


Flash the app using dfu-util:

.. code-block:: bash

    west flash


After flashing, you should see an USB device named "Generic CANnectivity USB to CAN adapter" with a VID/PID of ``1209:ca01``.
Additionally you should also see a new can device on your host system.
Configure it as follows to enable it:

.. code-block:: bash
   
    # Replace can0 with the device name on your system

    sudo ip link set can0 type can bitrate 500000
    sudo ip link set up can0



.. warning::
   
    For users of the *ARDEPv1* board:

    Since we don't use the default USB PID/VID of the ARDEP board, the default ARDEP DFU will not work for firmware updates.
    Put the device in bootloader mode to flash another firmware.

    See :ref:`bootloader_mode` for more information.