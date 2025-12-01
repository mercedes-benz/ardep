.. _uds-bus-sample:
   
UDS Bus Sample
##############

This sample demonstrates multiple UDS clients connected on a single CAN bus.
They use different physical addresses for communication.

This sample also demonstrates how the UDS clients can communicate to each other on the CAN bus via a small, custom protocol that lets each client "sign" a byte in a CAN message by each client.
For this, each client contains a *worker* functionality that listens for CAN frames on a specifig address (configurable via data identifier 0x1100 (2 bytes with little endian)) and responds (with the appended signature byte) on another configurable address (data identifier 0x1101).
Each client also contains an *controller* that is responsible for sending a first CAN frame to the first client in the chain, and then receiving the final response from the last client in the chain.

The controller uses a fixed send address of 0x001 and a fixed receive address of 0x000.


Building the Sample
===================

The sample should be built using sysbuild as the UDS addresses should be configured the same for the bootloader and application via the sysbuild configuration ``SB_CONFIG_UDS_BASE_ADDR``:

.. code-block:: bash
    west build -b ardep samples/uds_bus_sample --sysbuild -- -DSB_CONFIG_UDS_BASE_ADDR=0

Each ARDEP UDS client should be built with a different base address, all in a continuous range.
For example, for 3 clients, the first client should be built with ``SB_CONFIG_UDS_BASE_ADDR=0``, the second with ``SB_CONFIG_UDS_BASE_ADDR=1``, and the third with ``SB_CONFIG_UDS_BASE_ADDR=2``.


CAN IDs
=======

By using sysbuild, the CAN IDs are automatically configured based on the UDS base address. (see ``sysbuild.cmake`` in the sample directory for details).
The resulting CAN IDs are:
* UDS request ID: 0x7E0 + base address
* UDS response ID: 0x7E8 + base address
* CAN Logging ID: 0x100 + base address


Running the Sample
===================

To run the sample, first flash all ARDEPs with the build firmwares (each a different one as described above).
Then, run the client script:

.. code-block:: bash
    python3 client.py --can can0

If you wish to upgrade the firmwares before running the sample, use the ``--upgrade`` option, together with a set maximum count ``--count`` of clients to upgrade, a board name ``--board``, and optionally ``--pristine`` to build pristine firmwares:

.. code-block:: bash
    python3 client.py --can can0 --upgrade --count 3 --board ardep@1 --pristine

The client script will first optionally build the firmwares, then discover all connected UDS clients and optionally upgrade them. Then it will setup the signature CAN ID chain and run the master routine on one of the clients. Finally, it checks that the signature bytes are correct.
