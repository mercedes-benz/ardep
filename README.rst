ARDEP - Automotive Rapid Development Platform
##############################################

ARDEP is a powerful toolkit specifically designed for automotive developers based on the `Zephyr RTOS <https://www.zephyrproject.org/>`_.
It provides easy to use abstractions, features and tools to simplify the development process for automotive applications.

Getting Started
===============

See our `documentation <https://mercedes-benz.github.io/ardep/>`_  for more information on how to use ARDEP.

Follow our `Getting Started Guide <https://mercedes-benz.github.io/ardep/getting_started/index.html>`_ for a quick introduction


Create zephyr workspace
=======================

Create workspace from west.yml in this directory, e.g.


.. code-block:: console

    # create a workspace
    mkdir ardep-workspace
    # clone this repo into workspace
    cd ardep-workspace && git clone git@github.com:frickly-systems/ardep.git  ardep
    # init west workspace from west.yml
    cd ardep && west init -l --mf ./west.yml .
    # update workspace, fetches dependencies
    west update

Development bootloader
======================

Per default if the board is selected we build images to be used by the bootloader.
Those images are not signed (without signature validation).


Build the bootloader
--------------------

It is recommendet to use the `ardep` subcommand of `west` to build the bootloader.

.. code-block:: console
   
   west ardep build-bootloader


If you want to see the raw command, execute the above and look at the first lines of output. It should look something like this:

.. code-block:: console

    west build --pristine auto --board ardep --build-dir build \
        {...}/ardep-workspace/bootloader/mcuboot/boot/zephyr -- \
        -DEXTRA_CONF_FILE={...}/ardep-workspace/ardep/boards/arm/ardep/mcuboot.conf \
        -DEXTRA_DTC_OVERLAY_FILE={...}/ardep-workspace/ardep/boards/arm/ardep/mcuboot.overlay
