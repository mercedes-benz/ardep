.. _lin2can-commander-sample:

Lin2Can Commander Sample
########################

This example demonstrates together with the :ref:`lin2can-responder-sample` the ``lin2can``  driver.

It sends and receives CAN frames transmitted over LIN.
 
LIN2CAN works by encoding the CAN Frame ID to the first 2 bits of the LIN frame using a lookup table (Kconfig options `CONFIG_LIN2CAN_CAN_ID{0-3}`).

This also means that the first 2 bits of the CAN Frame payload may not be used for data.

LIN2CAN is primarily meant to be used with isotp.


Build and flash
===============

Build the application with:

.. code-block:: bash

  west build --board ardep samples/lin2can/commander

Then flash it using dfu-util:

.. code-block:: bash

  west ardep dfu
