.. _can_sample:

CAN sample
##########

This sample shows how to use the zephyr CAN API to send and receive CAN messages.
The sample uses both CANs of the board and sends a message on one CAN and receives it on the other.
To prepare the board for the sample, connect the CAN0 and CAN1 ports
and put the termination jumpers on both CAN ports.

Flash and run the example
-------------------------

The commands assume that you are in the root of the repo, not the workspace.

.. code-block:: bash

    west build --board ardep samples/can

The normal sample does not not use CAN-FD. To use CAN-FD, use the following command:

.. code-block:: bash

    west build --board ardep samples/can -- -DEXTRA_CONF_FILE=fd.conf

Flash the app using dfu-util:

    .. code-block:: bash

        west ardep dfu

Sample Output
=============

.. code-block:: console

    [00:00:00.000,000] <inf> app: Received CAN frame with ID 101

HV Shield
=========

TODO