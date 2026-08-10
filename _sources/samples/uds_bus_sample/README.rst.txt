.. _uds-bus-sample:

UDS Bus Sample
##############

This sample demonstrates multiple UDS clients connected on a single CAN bus using the default UDS instance feature.

The sample showcases:

- Multiple ECUs on a single CAN bus with gearbox addressing
- Custom inter-ECU communication protocol
- Firmware upgrade over UDS
- Routine control for orchestrating multi-ECU operations

Overview
========

The sample uses ``uds_default_instance`` with the standard UDS addresses. Each client contains:

1. **Worker functionality**: Listens for CAN frames on a configurable address (data identifier ``0x1100``) and responds with an appended signature byte on another address (data identifier ``0x1101``)

2. **Controller functionality**: Uses routine control (routine ID ``0x0000``) to orchestrate the signature chain by sending an initial CAN frame and receiving the final response

The controller uses fixed CAN addresses:
- Send address: ``0x001``
- Receive address: ``0x000``

Building the Sample
===================

Simply build the sample with sysbuild:

.. code-block:: bash

    west build -b ardep samples/uds_bus_sample --sysbuild

The sample now uses the default UDS instance with gearbox addressing, eliminating the need for per-client address configuration.

Running the Sample
==================

Flash all ARDEPs with the firmware and run the client script:

.. code-block:: bash

    python3 client.py --can can0

To upgrade firmwares before running the sample, use the ``--upgrade`` option:

.. code-block:: bash

    python3 client.py --can can0 --upgrade --board ardep@1 --pristine

The client script will:

1. Optionally build and upgrade the firmware
2. Discover all connected UDS clients
3. Configure the signature CAN ID chain
4. Execute the controller routine on one client
5. Verify that the signature bytes are correct
