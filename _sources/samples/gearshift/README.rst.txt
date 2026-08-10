.. _gearshift_sample:

Gearshift Sample
################

This sample application reads the gearshift header configuration and logs the selected position every second.


Flash and run the example
=========================

Build the application with:

.. code-block:: bash

    # From the root of the ardep repository
    west build --board ardep samples/gearshift
    

Then flash it:

.. code-block:: bash
    
    west flash
