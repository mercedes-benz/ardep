.. _hardware_test_sample:

Hardware Test Sample
####################

A sample used to ensure that the hardware is working correctly.

This requires 2 ARDEP boards to be connected on each pin of the board.

The other board needs to run the :ref:`hardware_tester_sample` sample.

Then execute the test with the tester script at */scripts/tester/tester.py*.


Flash and run the example
=========================

Build the application with:

.. code-block:: bash

    # From the root of the ardep repository
    west build --board ardep samples/hardware_test
    

Then flash it using dfu-util:

.. code-block:: bash
    
    west ardep dfu
