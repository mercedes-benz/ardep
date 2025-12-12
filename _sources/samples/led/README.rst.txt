.. _led_sample:

LED Sample
###############

This sample simply toggles between the red and green onboard LEDs.


Flash and run the example
=========================

Build the application with:

.. code-block:: bash

    # From the root of the ardep repository
    west build --board ardep samples/led
    

Then flash it using dfu-util:

.. code-block:: bash
    
    west flash
