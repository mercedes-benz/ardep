.. _getting_started:


Getting Started
################

.. contents::
   :local:
   :depth: 2

Follow this guide to:

- Install the `Zephyr RTOS <https://zephyrproject.org/>`_ SDK
- Set up your first workspace
- Build and flash your first sample application

The instructions here build on the `Zephyr RTOS guide <https://docs.zephyrproject.org/4.2.0/develop/getting_started/index.html>`_ focussing on the Ubuntu Linux Distribution and the ardep board. [1]_


Install dependencies
*********************

Follow the `Zephyr RTOS guide (section "Install dependencies") <https://docs.zephyrproject.org/4.2.0/develop/getting_started/index.html#install-dependencies>`_  to install all dependencies for the Zephyr RTOS.

Install the additional dependencies for the ardep board:

.. tabs::
   
   .. tab:: Linux

    .. code-block:: bash

        sudo apt update
        sudo apt install --no-install-recommends iproute2 dfu-util

   .. tab:: Windows
   
        Install the latest `dfu-util <https://dfu-util.sourceforge.net/>` from the `release-page <https://dfu-util.sourceforge.net/releases/>` and ensure the executables are in your PATH.

        run ``dfu-util --version`` to check that the command is available.
        
    
Install the Zephyr SDK
****************************

Follow `Zephyr's guide (section "Install Zephyr SDK") <https://docs.zephyrproject.org/4.2.0/develop/getting_started/index.html#install-the-zephyr-rtos-sdk>`_ .


Set up your workspace
****************************

Since you will install some python packages, we recommend using a virtual environment. We will use `uv <https://docs.astral.sh/uv/>` for this purpose.


#. Navigate to the directory where you want to create your workspace and activate a new virtual environment
   
   .. tabs::

        .. tab:: Linux

            .. code-block:: bash

                cd ardep-workspace
                uv venv
                source .venv/bin/activate
                
        .. tab:: Windows

            .. code-block:: powershell

                cd ardep-workspace
                uv venv
                .venv\Scripts\activate

#. Install Zephyr's metatool ``west``:

        .. code-block:: bash

            uv pip install west

#. Clone the ARDEP repository and initialize the workspace
   
   .. tabs::

        .. tab:: Remote Repository
        
            .. code-block:: bash
            
                west init -m  {REPOSITORY_URL} --mr main .
                cd ardep
        
        .. tab:: Local Repository

            .. code-block:: bash

                git clone {REPOSITORY_URL} ardep
                cd ardep
                west init -l --mf ./west.yml .
                west update
        
#. Install the required python dependencies:
    
    .. code-block:: bash

        uv pip install -r ../zephyr/scripts/requirements.txt
        uv pip install -r scripts/requirements.txt
        
        

#. Allow the dfu-util to connect with your device
   
   .. tabs::

        .. tab:: Linux
            Install and activate the ARDEP udev-rule:

                .. code-block:: bash

                    west ardep create-udev-rule
                    sudo udevadm control --reload-rules
                    sudo udevadm trigger
                    
                This rule allows ``dfu-util`` to access your ardep board without sudo privileges (required for firmware upgrades via ``dfu-util``).
                
                If your ardep board is already connected, unplug and replug it.
                

        .. tab:: Windows
            We need to install WinUSB drivers for the device in order to be able to use dfu-util.

            You can use the `Zadig <https://zadig.akeo.ie/>`_ tool to install the drivers.

            After installing and starting *Zadig*, ensure the *List all devices* option is turned on in the Options menu.
            Then, in the dropdown menu, select *Ardep (Interface 0)*, *Ardep board* or similar and install the *WinUSB* driver.
            This allows us to set the device into DFU mode.
            
            We also need to install a driver for the DFU mode. For this, we need build a sample application and unsuccessfully try to flash the firmware (see `Build your first app`_).
            
            After the initial flash command failed, select the *Ardep board* in the dropdown menu and install the *WinUSB* driver again.
            
            Now, flashing the app should succeed.



Build your first app 
********************

Build the :ref:`led_sample` with:

    .. code-block:: bash

        west build --board ardep samples/led

Flash the app using dfu-util:

    .. code-block:: bash

        west flash

.. [1] Tested on Ubuntu 24.04, Zephyr SDK 0.17.2 and Zephyr RTOS 4.2.0
