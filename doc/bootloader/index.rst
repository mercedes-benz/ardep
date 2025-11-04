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


.. _flashing_the_bootloader:

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


.. _uds_bootloader:

UDS Bootloader
++++++++++++++

For UDS Applications where upgrading the firmware over UDS is required,
a :ref:`firmware_loader` is provided which can be built using sysbuild.

It is not recommended to build the firmware loader independently as the bootloader needs configuration options to handle the firmware loader correctly.

For demonstration purposes, build the :ref:`uds-sample` using sysbuild, which automatically builds the firmware loader:

.. code-block:: bash

    west build -b ardep samples/uds -p --sysbuild

.. note::
    On ARDEP v1 boards, the default dfu-util flasher will not work to flash the bootloader, see :ref:`flashing_the_bootloader`

If you wish to build your own application using the UDS Firmware loader, enable the sysbuild boot logic configuration like this:

.. code-block:: bash

    west build -b ardep <path-to-your-app> -p --sysbuild -- -DSB_CONFIG_BOOT_LOGIC_UDS_FIRMWARE_LOADER=y

Or more permanently by creating a ``sysbuild.conf`` file in your application folder and adding the following content:

.. code-block::

    SB_CONFIG_BOOT_LOGIC_UDS_FIRMWARE_LOADER=y

.. important::
    Do not forget to call ``uds_switch_to_firmware_loader_with_programming_session()`` in the diagnostic session control action handler, if a programming session is requested.


Sysbuild
++++++++

With sysbuild, multiple applications (i.e. bootloader, main application and sometimes firmware loader) are built together using just one build command.

There are multiple things that differ from a normal west build

First, there are multiple targets for using menuconfig, one for each "domain":

.. code-block:: bash

    # Main application menuconfig
    west build -t menuconfig

    # Bootloader menuconfig
    west build -t mcuboot_menuconfig

    # (If enabled) Firmware loader menuconfig
    west build -t firmware_loader_menuconfig

    # Sysbuild menuconfig (also contains the config option for enabling the firmware loader)
    west build -t sysbuild_menuconfig


Second, you can switch between domains when debugging by specifying the domain:

.. code-block:: bash

    # Debug main application
    west debug

    # Debug bootloader
    west debug --domain mcuboot

    # (If enabled) Debug firmware loader
    west debug --domain firmware_loader

Finally, when flashing, you can either specify no domain, which flashes all domains in series or you can specify a domain to flash only that domain:

.. code-block:: bash

    # Flash all domains (bootloader, firmware loader (if enabled) and main application)
    west flash

    # Flash only bootloader
    west flash --domain mcuboot

    # (If enabled) Flash only firmware loader
    west flash --domain firmware_loader

    # Flash only main application (when building sample/uds the sample-name would be uds)
    west flash --domain <sample-name>

.. important::
    Note that on ARDEP v1 boards, only the main application can be flashed using the ardep runner. This also means that the bootloader and (if needed) firmware loader must be flashed using an external debug probe, see :ref:`flashing_the_bootloader`.
