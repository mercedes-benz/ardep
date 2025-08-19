.. _getting_started:


Getting Started
################

.. contents::
   :local:
   :depth: 2

Follow this guide to:

- Set up your first workspace
- Install the `Zephyr RTOS <https://zephyrproject.org/>`_ SDK
- Enable the dfu-util to perform Firmware upgrades
- Build and flash your first sample application

The instructions here closely follow the `Zephyr RTOS Getting Started Guide <https://docs.zephyrproject.org/4.2.0/develop/getting_started/index.html>`_ but are modified to setup an ARDEP workspace instead of a vanilla Zephyr workspace. [1]_


Install dependencies
*********************

Follow Zephyr's  `OS Update Section <https://docs.zephyrproject.org/4.2.0/develop/getting_started/index.html#select-and-update-os>`_ and then the `Install dependencies <https://docs.zephyrproject.org/4.2.0/develop/getting_started/index.html#install-dependencies>`_ section.

    .. note::

        **For Windows user:**
        
        - You might need to manually add the *7zip* installation folder to your *$PATH*. It is usually installed under ``C:\Program Files\7-Zip``. [2]_

        - One of your testers reported, that the ``python`` executable was actually called ``py`` on his device. You can check this with the ``python --version`` or ``py --version`` in a command prompt. If you see the version (e.g. `Python 3.13.7`) the executable is found.

Install the additional dependencies for the ardep board:

.. tabs::
   
   .. tab:: Linux

    .. code-block:: bash

        sudo apt update
        sudo apt install --no-install-recommends iproute2 dfu-util git-lfs

   .. tab:: Windows
   
        Install the latest `dfu-util <https://dfu-util.sourceforge.net/>`_ from the `release page <https://dfu-util.sourceforge.net/releases/>`_ (e.g. *dfu-util-X.YY-binaries.tar.xz*), extract the archive and ensure the executables are in your *$PATH*. [2]_

        Open a new *command prompt* or *powershell* and run ``dfu-util --version`` to check that the command is available.
        
    
Set up your workspace
*********************


We will clone the `ARDEP repository <https://github.com/mercedes-benz/ardep>`_, `Zephyr <https://github.com/zephyrproject-rtos/zephyr>`_ and all required `modules <https://docs.zephyrproject.org/4.2.0/develop/modules.html#modules>`_ into a new `west <https://docs.zephyrproject.org/4.2.0/develop/west/index.html#west>`_ workspace and install all required python dependencies in a `Python virtual environment <https://docs.python.org/3/library/venv.html>`_.

In the following, we use `ardep-workspace` as the name for the workspace and locate it in our *$HOME* directory, but you can choose any name and location you want.

   .. tabs::

        .. tab:: Linux
        
            #. Create a new virtual environment

                .. code-block:: bash

                    python3 -m venv ~/ardep-workspace/.venv

            #. Activate the virtual environment

                .. code-block:: bash

                    source ardep-workspace/.venv/bin/activate
                    
                .. note::    

                    Remember to do activate the virtual environment whenver you start working.
                
            #. Install west:

                .. code-block:: bash

                    pip install west
                    
            #. Get the ARDEP source code:

                .. code-block:: bash
                    
                    west init -m https://github.com/mercedes-benz/ardep.git ~/ardep-workspace 
                    
            #. Get the source code for Zephyr and all its dependencies:

                .. code-block:: bash
                    
                    cd ~/ardep-workspace
                    west update

            #. Export a `Zephyr CMake package <https://docs.zephyrproject.org/4.2.0/build/zephyr_cmake_package.html#cmake-pkg>`_. This allows CMake to automatically load boilerplate code required for building Zephyr applications.

                .. code-block:: bash
                
                    west zephyr-export

            #. Install python dependencies for other *west* commands:

                .. code-block:: bash
                
                    west packages pip # Lists all requirement.txt files that get installed
                    west packages pip --install # Actually install the packages
                    
                    
        .. tab:: Windows
        
            #. Open a command prompt or PowerShell as a **regular user**
            #. Create a new virtual environment
                           
                .. tabs::
                
                    .. tab:: Batchfile

                        .. code-block:: batch

                            cd %HOMEPATH%
                            python -m venv ardep-workspace\.venv   


                    .. tab:: PowerShell
                    
                        .. code-block:: powershell
                        
                            cd $Env:HOMEPATH
                            python -m venv ardep-workspace\.venv
                           

            #. Activate the virtual environment

                .. tabs::
                
                    .. tab:: Batchfile

                        .. code-block:: batch

                            ardep-workspace\.venv\Scripts\activate.bat


                    .. tab:: PowerShell
                    
                        .. code-block:: powershell

                            ardep-workspace\.venv\Scripts\Activate.ps1                        

                .. note::    

                    Remember to do activate the virtual environment whenver you start working.
                
            #. Install west:

                .. code-block:: bash

                    pip install west
                    
            #. Get the ARDEP source code:

                .. code-block:: bash
                    
                    west init -m https://github.com/mercedes-benz/ardep.git ardep-workspace 
                    
            #. Get the source code for Zephyr and all its dependencies:

                .. code-block:: bash
                    
                    cd ardep-workspace
                    west update

            #. Export a `Zephyr CMake package <https://docs.zephyrproject.org/4.2.0/build/zephyr_cmake_package.html#cmake-pkg>`_. This allows CMake to automatically load boilerplate code required for building Zephyr applications.

                .. code-block:: bash
                
                    west zephyr-export

            #. Install python dependencies for other *west* commands:

                .. code-block:: bash
                
                    west packages pip # Lists all requirement.txt files that get installed
                    west packages pip --install # Actually install the packages
                    
        
Install the Zephyr SDK
****************************

We will install the `Zephyr SDK <https://docs.zephyrproject.org/4.2.0/develop/toolchains/zephyr_sdk.html#toolchain-zephyr-sdk>`_ which contains the toolchain for every by Zephyr supported architectures. Additionally it contains host tools for Linux such as a custom QEMU and OpenOCD build for emulation, flashing and debugging.

   .. tabs::

        .. tab:: Linux
        
            Install the Zephyr SDK using ``west sdk install``.

                .. code-block:: bash
                
                    cd ~/ardep-workspace/zephyr
                    west sdk install
                    
                .. note::

                    See ``west sdk install --help`` for more command options (e.g. install location)

                    
        .. tab:: Windows
        
            Install the Zephyr SDK using ``west sdk install``.
                           
                .. tabs::
                
                    .. tab:: Batchfile

                        .. code-block:: batch

                            cd %HOMEPATH%\ardep-workspace\zephyr
                            west sdk install

                    .. tab:: PowerShell
                    
                        .. code-block:: powershell

                            cd $Env:HOMEPATH\ardep-workspace\zephyr
                            west sdk install
                           
                .. note::

                    See ``west sdk install --help`` for more command options (e.g. install location)

            
Enable the DFU-Util to perform firmware upgrades
************************************************
            

   .. tabs::

        .. tab:: Linux

            .. code-block:: bash

                west ardep create-udev-rule
                sudo udevadm control --reload-rules
                sudo udevadm trigger
                
            This rule allows ``dfu-util`` to access your ardep board without sudo privileges (required for firmware upgrades via ``dfu-util``).
            
            If your ardep board is already connected, unplug and replug it.
                           
                    
        .. tab:: Windows
        
            We need to install WinUSB drivers for the device in order to be able to use dfu-util.

            You can use the `Zadig <https://zadig.akeo.ie/>`_ tool to install the drivers.
            
            If you haven't connected your *ARDEP* board to host, connect it now.

            After starting *Zadig*, ensure the *List all devices* option is turned on in the Options menu.
            Then, in the dropdown menu, select *Ardep (Interface 0)* install the *WinUSB* driver. Then repeat the step for *Ardep (Interface 2)*.
            This allows us to set the device into DFU mode.
            
            .. image:: windows_install_usb_driver.png
               :alt: Installing WinUSB driver using Zadig
            
            We also need to install a driver for the DFU mode. For this, we need to build a sample application and unsuccessfully try to flash the firmware (see `Build your first app`_).
            
            After the initial flash command failed, select the *Ardep board* in the dropdown menu and install the *WinUSB* driver again.
            
            Now, flashing the app should succeed.


Build your first app 
********************

Build the :ref:`led_sample` with:


    .. tabs::

        .. tab:: Linux

            .. code-block:: bash

                cd ~/ardep-workspace/ardep
                west build --board ardep samples/led

        .. tab:: Windows
        
            .. tabs::

                .. tab:: Batchfile

                    .. code-block:: batch

                        cd %HOMEPATH%\ardep-workspace\ardep
                        west build --board ardep samples\led
                        
                .. tab:: PowerShell

                    .. code-block:: powershell

                        cd $Env:HOMEPATH\ardep-workspace\ardep
                        west build --board ardep samples\led

Flash the app using dfu-util:

    .. code-block:: bash

        west flash

.. [1] Tested on Ubuntu 24.04 and Windows (Version 24H2 Build 26100.5074), Zephyr SDK 0.17.2 and Zephyr RTOS 4.2.0

.. [2] See `here <https://www.computerhope.com/issues/ch000549.htm>`_ for a guide on how to add a folder to the *$PATH*