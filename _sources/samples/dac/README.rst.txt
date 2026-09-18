.. _dac_sample:

DAC sample
##########

This sample shows how to use the DAC API to output a voltage on a pin.
The sample will output a see-saw voltage on the DAC pin.

Flash and run the example
-------------------------

The commands assume that you are in the root of the repo, not the workspace.

.. code-block:: bash

    west build --board ardep samples/dac

Flash the app using dfu-util:

.. code-block:: bash

    west flash

Other DAC pins
==============

To change the output pin to another pin, you have to modifiy the overlay in the baords directory.
There you only configure the DAC channel, the pins for the channel are configured in the board definition.

The usable pins for the DAC are: PA4, PA5, and PA6
