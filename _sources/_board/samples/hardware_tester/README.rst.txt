.. _hardware_tester_sample:

Hardware Tester Sample
######################

See :ref:`hardware_test_sample`

Flash and run the example
=========================

Build the application with:

.. code-block:: bash

    # From the root of the ardep repository
    west build --board ardep samples/hardware_tester
    

Then flash it using dfu-util:

.. code-block:: bash
    
    west ardep dfu
