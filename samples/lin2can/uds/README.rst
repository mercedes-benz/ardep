.. _lin2can-uds-sample:

UDS Over LIN sample
###################

This example demonstrates DFU over LIN together with the :ref:`lin2can-gateway-sample`.

Note that this sample is configured as a LIN responder.


Hardware setup
==============

You will need 2 ARDEP Boards as well as 1 CAN to USB interface

.. code-block::

  +---------+  +---------+ +---------+
  | CAN2USB |  | ARDEP 1 | | ARDEP 2 |
  |         |  |         | |         |
  | USB CAN |  | CAN LIN | | CAN LIN |
  +--+---+--+  +--+---+--+ +------+--+
     |   |        |   |           |   
     |   +--------+   +-----------+   
     |                                
     |        +---------+             
     |        | U  HOST |             
     +--------+ S       |             
              | B       |             
              +---------+                           

Build and flash
===============

Build and flash the UDS application to the ARDEP 2 board in the diagram:

.. code-block:: bash

  west build --board ardep samples/lin2can/uds
  west ardep dfu

Build and flash the Gateway application to the ARDEP 1 board in the diagram:

.. code-block:: bash

  west build --board ardep samples/lin2can/gateway
  west ardep dfu


Usage
=====

#. Setup the can interface

  .. code-block:: bash
      
    sudo ip link set can0 type can bitrate 500000
    sudo ip link set up can0

#. Build a new application to flash via uds (note that the new application must also support UDS over LIN if you need to update the application again, in this case we just use the led sample which does not support UDS over LIN).
  
  .. code-block:: bash

    west build --board ardep samples/led --build-dir build-led

#. Flash the new application using UDS:

  .. code-block:: bash

    west ardep uds-dfu --build-dir build-led --interface can0
