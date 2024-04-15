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

The instructions here build on the `Zephyr RTOS guide <https://docs.zephyrproject.org/3.5.0/develop/getting_started/index.html>`_ focussing on the Ubuntu Linux Distribution and the ardep board. [1]_


Install dependencies
*********************

Follow the `Zephyr RTOS guide (section "Install dependencies") <https://docs.zephyrproject.org/3.5.0/develop/getting_started/index.html#install-dependencies>`_  to install all dependencies for the Zephyr RTOS.

Install the additional dependencies for the ardep board:

.. code-block:: bash

    sudo apt update
    sudo apt install --no-install-recommends iproute2 dfu-util
        
    
Install the Zephyr SDK
****************************

Follow `Zephyr's guide (section "Install Zephyr SDK") <https://docs.zephyrproject.org/3.5.0/develop/getting_started/index.html#install-the-zephyr-rtos-sdk>`_ .


Set up your workspace
****************************

Since you will install some python packages, we recommend using a virtual environment. We will use ``pipenv`` for this purpose.

#. Install pipenv
    .. code-block:: bash

        sudo apt install pipenv

#. Navigate to the directory where you want to create your workspace and activate a new virtual environment

    .. code-block:: bash

        cd ardep-workspace
        pipenv shell

#. Install Zephyr's metatool ``west``:

        .. code-block:: bash

            pip install west

#. Clone the ARDEP repository and initialize the workspace

    .. code-block:: bash

        git clone {REPOSITORY_URL} ardep
        cd ardep
        west init -l --mf ./west.yml .
        west update
        
#. Export a `Zephyr CMake Package <https://docs.zephyrproject.org/3.5.0/build/zephyr_cmake_package.html#cmake-pkg>`_

    .. code-block:: bash

        west zephyr-export
        
#. Install the required python dependencies:
    
    .. code-block:: bash

        pip install -r ../zephyr/scripts/requirements.txt
        pip install -r scripts/requirements.txt
        
#. Install and activate the ARDEP udev-rule:

    .. code-block:: bash

        west ardep create-udev-rule
        sudo udevadm control --reload-rules
        sudo udevadm trigger
        
    This rule allows ``dfu-util`` to access your ardep board without sudo privileges (required for firmware upgrades via ``dfu-util``).
    
    If your ardep board is already connected, unplug and replug it.


Build your first app 
********************

Build the :ref:`led_sample` with:

    .. code-block:: bash

        west build --board ardep samples/led

Flash the app using dfu-util:

    .. code-block:: bash

        west ardep dfu

.. [1] Tested on Ubuntu 20.04, Zephyr SDK 0.16.5 and Zephyr RTOS 3.5.0
