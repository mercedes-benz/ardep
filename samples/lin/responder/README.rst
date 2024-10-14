.. _lin-responder-sample:

Lin Responder Sample
#####################

This example simulates a LED controlled via LIN. The current status is stored in RAM an printed with Zephyr's logging


Build and flash
===============

Build the application with:

.. code-block:: bash

  west build --board ardep samples/lin/responder

Then flash it using dfu-util:

.. code-block:: bash

  west ardep dfu


LIN Frames
==========

Upon receiving a LIN header with the frame ID 0x10 the example will respond with the current LED status.

When receiving a LIN frame with the frame ID 0x11 the example will use the new state to control the LED and print a log message.

See :ref:`lin-commander-sample` for details of the frame payloads.
