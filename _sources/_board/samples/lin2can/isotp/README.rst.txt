.. _lin2can-isotp-sample:

Lin2Can IsoTP Sample
####################

This example demonstrates the usage of IsoTP over LIN.

To use the example use 2 boards, flash one with the isotp sample in responder configuration and the other one with the isotp sample in commander configuration (see below on how to do that).

The 2 boards will then transfer isotp packages over LIN bidirectionally.


Build and flash
===============

Build the application with for the responder with:

.. code-block:: bash

  west build --board ardep samples/lin2can/isotp

Or build the application with for the commander with:

.. code-block:: bash

  west build --board ardep samples/lin2can/isotp -DDTC_OVERLAY_FILE=commander.overlay

Then flash it using dfu-util:

.. code-block:: bash

  west ardep dfu

