.. _lin-commander-sample:

Lin Commander Sample
####################

This example demonstrates a simple LIN commander that reads and writes frames via the LIN bus.
 
It works together with the :ref:`lin-responder-sample` to simulate an led on the responder side that the commander toggles and polls its status.


Build and flash
===============

Build the application with:

.. code-block:: bash

  west build --board ardep samples/lin/commander

Then flash it using dfu-util:

.. code-block:: bash

  west ardep dfu


LIN Frames
==========

The LIN frame with the ID ``0x10`` (1 Byte in size) pulls the current status from the responder (``0x01`` means LED is on, ``0x00`` means LED is off).

The LIN frame with the ID ``0x11`` (1 Byte in size) sends the new status to the responder (``0x01`` to turn the LED on, ``0x00`` to turn it off).
