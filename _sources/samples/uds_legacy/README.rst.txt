.. _uds-sample-legacy:
   
UDS Sample (Legacy)
###################

.. note::

    The underlying UDS implementation is **deprecated**, please use the new implementation and the :ref:`uds sample <uds-sample>` instead.

This sample can be used to test the uds implementation draft.
The UDS implementation does not support all the services yet, but it is a good starting point.
The mainly supported use case is the firmware update.

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
            samples/uds_legacy

        ./build/zephyr/zephyr.exe

#. Run test client in another terminal

    .. code-block:: bash

        python samples/uds/scripts/test.py

Test with board
===============

#. Enable can interface

    .. code-block:: bash

        sudo ip link set can0 type can bitrate 500000
        sudo ip link set up can0