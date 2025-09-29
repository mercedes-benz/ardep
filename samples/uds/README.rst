.. _uds-sample:
   
UDS Sample
##########

This sample can be used to test the uds implementation.

Test with virtual CAN on posix
==============================

#. Add virtual can

    .. code-block:: bash

        sudo ip link add dev vcan0 type vcan
        sudo ip link set vcan0 mtu 72  
        sudo ip link set up vcan0
        sudo ip link property add dev vcan0 altname zcan0

#. Build for native sim and run

    .. code-block:: bash

        west build\
            --pristine always\
            --board native_sim/native/64\
            samples/uds

        ./build/zephyr/zephyr.exe

#. Run test client in another terminal

    .. code-block:: bash

        python samples/uds/client.py

Test with board
===============

#. Enable can interface

    .. code-block:: bash

        sudo ip link set can0 type can bitrate 500000
        sudo ip link set up can0

#. Run test client with *can0* interface

    .. code-block:: bash

        python samples/uds/client.py --can can0

Test Link Control functionality with board
==========================================

This sample only runs on a linux host as it requires updating the hosts can settings on the fly.
This is also the reason why root privileges are required to run the client
(See the ``set_can_bitrate`` function).


#. Enable can interface

    .. code-block:: bash

        sudo ip link set can0 type can bitrate 500000
        sudo ip link set up can0

#. Run test client with *can0* interface with root privileges.
   Make sure to source the python virtual environment again after elevating privileges so the clients dependencies are still available.

    .. code-block:: bash

        sudo -s
        source <path_to_your_venv>/bin/activate
        python samples/uds/client_link_control.py --can can0

If you get a series of message errors (e.g. on `Wireshark <https://www.wireshark.org/>`_), then you should:

    - Stop the client

    - Disconnect the board

    - Reconfigure the can interface

        .. code-block:: bash

            sudo ip link set down can0
            sudo ip link set can0 type can bitrate 500000
            sudo ip link set up can0