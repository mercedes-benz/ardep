.. _lin-commander-scheduler-sample:

Lin Commander Scheduler Sample
##############################

This example simulates the same behavior as the :ref:`lin-commander-sample` but uses a scheduler to schedule the LIN frames.


Build and flash
===============

Build the application with:

.. code-block:: bash

  west build --board ardep samples/lin/commander_scheduler

Then flash it using dfu-util:

.. code-block:: bash

  west ardep dfu
