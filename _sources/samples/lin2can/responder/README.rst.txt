.. _lin2can-responder-sample:

Lin2Can Responder Sample
########################

This example is the counterpart for the :ref:`lin2can-commander-sample`.

It sends and receives CAN frames transmitted over LIN

See a more detailed description in the commander sample.


Build and flash
===============

Build the application with:

.. code-block:: bash

  west build --board ardep samples/lin2can/responder

Then flash it using dfu-util:

.. code-block:: bash

  west flash
