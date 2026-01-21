.. _bootloader:
   
Bootloader
##########

.. contents::
   :local:
   :depth: 2

Overview
++++++++

The ARDEP bootloader uses MCUBoot and a :ref:`firmware_loader` for firmware updates. This enables application upgrades either via UDS over CAN or USB DFU.

.. tabs::

    .. tab:: Ardep v2.0.0 and later

        The recommended upgrade mechanism is the on-board debugger using the `west flash` command or if needed using UDS over CAN.

    .. tab:: Ardep v1.0.0

        The recommended upgrade mechanism is USB DFU using the `west flash` command or if needed using UDS over CAN.


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

.. important::

    **Bootloader and firmware loader can ONLY be flashed via debugger:**
    
    - **v2.0.0 and later**: Must use the on-board debugger (OBD) - this is the **recommended and default** method
    - **v1.0.0**: Must use an external debug probe (e.g. J-Link, ST-Link)
    
    The firmware loader itself **cannot** be flashed via USB DFU or UDS DFU. Only **application** updates can use the firmware loader's DFU capabilities.

.. tabs::

    .. tab:: Ardep v2.0.0 and later

        The bootloader and firmware loader are flashed via the on-board debugger using the `west flash` command.

        Just run:

        .. code-block:: bash

            west flash

        This will flash the bootloader and firmware loader to the board.
        

    .. tab:: Ardep v1.0.0

        The bootloader and firmware loader are flashed with an external debug probe (e.g. J-Link) using the `west flash` command.

        For this connect the SWD Pins of the ARDEP board to the debug probe an run:

        .. code-block:: bash
           
            west flash -r {runner}

        The Pinout of the SWD connector is printed on the boards backside.

        See the `board.cmake` file under `boards/mercedes/ardep` for more info about supported debuggers/runners.


.. _bootloader_mode:

Firmware Loader Mode
++++++++++++++++++++

The firmware loader supports both **USB DFU** and **UDS DFU** update mechanisms for **application** updates. When you set the BOOT jumper, the bootloader enters the firmware loader, which provides both update methods.

.. important::

    The firmware loader can only update **applications**, not the bootloader or firmware loader itself. To update the bootloader or firmware loader, you must use the on-board debugger (v2) or external probe (v1).

It is helpful to see the Bootloader console output on ``UART-A`` for this.


Entering Firmware Loader Mode
==============================

To enter the firmware loader mode, pull the ``PE4`` pin (labeled *BOOT*) to a LOW state (by setting the jumper) while power-cycling or pushing the Reset button.

The firmware loader will be active and ready to accept **application** firmware updates via either USB DFU or UDS (CAN).

On ``UART-A`` you will see output indicating the bootloader has started the firmware loader.

.. note::

    ``UART-A`` is the output that is forwarded by the on-board debugger (on v2 boards) or available via the UART-A pins.

Upgrading the Application Firmware
===================================

You can upgrade **application** firmware using either method:

**Via On-board Debugger (v2)**

- Note that this method only works on ARDEP v2.0.0 and later boards with an on-board debugger.
- Build the application firmware you want to flash (assuming it is in the *build* directory)
- Set the BOOT jumper and reset the board
- Perform the upgrade with ``west flash``
- Wait for the upgrade to complete

**Via USB DFU:**

- Note that this method works on all ARDEP boards (v1 and v2), but the first option is generally better for v2+ boards
- Build the application firmware you want to flash (assuming it is in the *build* directory)
- Set the BOOT jumper and reset the board
- Perform the upgrade with ``west flash`` (for v2+ boards, set the runner: ``west flash --runner ardep``)
- Wait for the upgrade to complete

**Via UDS (CAN):**

- Use the UDS diagnostic services over CAN to request a programming session
- The firmware loader will handle the application update process
- See :ref:`firmware_loader` for details


.. _uds_bootloader:

UDS and USB DFU Support via Firmware Loader
++++++++++++++++++++++++++++++++++++++++++++

.. note::

    **All ARDEP boards** (v1 and v2): The unified firmware loader is now the **default** boot logic for all board versions. You can build any application with sysbuild and it will automatically include the firmware loader with support for both UDS DFU and USB DFU.
    
    **Upgrading from older ARDEP versions**: If your bootloader was built with an older ARDEP repository version that differentiated between board versions, you need to upgrade the bootloader using sysbuild (via OBD on v2 or external probe on v1) to get the unified firmware loader support.

For applications that require firmware updates over UDS (Unified Diagnostic Services) or USB DFU,
a :ref:`firmware_loader` is provided which can be built using sysbuild.

It is not recommended to build the firmware loader independently as the bootloader needs configuration options to handle the firmware loader correctly.

For demonstration purposes, build the :ref:`uds-sample` using sysbuild (which contains a ``sysbuild.conf``), which automatically builds the firmware loader:

.. code-block:: bash

    west build -b ardep samples/uds -p --sysbuild

If you wish to build your own application using the firmware loader, it is included by default when using sysbuild.

.. note::
    **Flashing considerations:**
    
    - **Bootloader and firmware loader**: Can ONLY be flashed via on-board debugger (v2) or external probe (v1)
    - **Applications**: Can be flashed via debugger OR via the firmware loader (USB DFU/UDS DFU)
    
    See :ref:`flashing_the_bootloader` for detailed instructions.

.. important::
    When using the default UDS instance (``CONFIG_UDS_DEFAULT_INSTANCE=y``, enabled by default on ARDEP boards), the firmware loader switch is handled automatically on programming session requests unless disabled with ``CONFIG_UDS_DEFAULT_INSTANCE_DISABLE_SWITCH_TO_FIRMWARE_LOADER=y``.
    
    If you create a custom UDS instance or disable the automatic switch, you must call ``uds_switch_to_firmware_loader_with_programming_session()`` in your diagnostic session control action handler when a programming session is requested.


To flash an application with the UDS firmware loader, see :ref:`ardep_uds`.

Sysbuild
++++++++

With sysbuild, multiple applications (i.e. bootloader, main application and firmware loader) are built together using just one build command.

More information is available in the official `Zephyr Sysbuild documentation <https://docs.zephyrproject.org/4.2.0/build/sysbuild/index.html>`_.

Sysbuild basics
===============

There are multiple things that differ from a normal west build:

First, there are multiple targets for using menuconfig, one for each "domain":

.. code-block:: bash

    # Main application menuconfig
    west build -t menuconfig

    # Bootloader menuconfig
    west build -t mcuboot_menuconfig

    # Firmware loader menuconfig
    west build -t firmware_loader_menuconfig

    # Sysbuild menuconfig
    west build -t sysbuild_menuconfig


Second, you can switch between domains when debugging by specifying the domain:

.. code-block:: bash

    # Debug main application
    west debug

    # Debug bootloader
    west debug --domain mcuboot

    # Debug firmware loader
    west debug --domain firmware_loader

Finally, when flashing, you can either specify no domain, which flashes all domains in series or you can specify a domain to flash only that domain:

.. code-block:: bash

    # Flash all domains (bootloader, firmware loader and main application)
    west flash

    # Flash only bootloader
    west flash --domain mcuboot

    # Flash only firmware loader
    west flash --domain firmware_loader

    # Flash only main application (when building sample/uds the sample-name would be uds)
    west flash --domain <sample-name>

.. important::
    Note that on ARDEP v1 boards, only the main application can be flashed using the ardep runner. This also means that the bootloader and firmware loader must be flashed using an external debug probe, see :ref:`flashing_the_bootloader`.
