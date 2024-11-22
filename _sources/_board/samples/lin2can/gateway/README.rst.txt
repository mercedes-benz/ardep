.. _lin2can-gateway-sample:

Lin2Can Gateway Sample
######################

This example can be used together with the :ref:`lin2can-uds-sample` to demonstrate UDS DFU via LIN by using this board as a CAN-LIN gateway.

All incoming CAN packages that can be converted to LIN packages (in the LIN2CAN format (see :ref:`lin2can-commander-sample`)) are converted and forwarded to the LIN bus. The same is happening in the other direction; all incoming LIN packages (in the LIN2CAN format) that can be converted are forwarded to the CAN bus.

Note that this example configures the LIN bus as a commander and schedules the LIN2CAN Frames every 10 ms for a stable isotp/uds connection.


Build and flash
===============

Build the application with:

.. code-block:: bash

  west build --board ardep samples/lin2can/gateway

Then flash it using dfu-util:

.. code-block:: bash

  west ardep dfu
