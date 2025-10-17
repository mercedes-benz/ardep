.. _adc_sample:

ADC Sample
##########

This sample shows how to use the ADC peripheral to read an analog input.
The values that are printed on the console are from the arduino analog pins.
The values are ordered from A0 to A5.

Flash and run the example
-------------------------

The commands assume that you are in the root of the repo, not the workspace.

.. code-block:: bash

    west build --board ardep samples/adc

Flash the app using dfu-util:

.. code-block:: bash

    west flash

Sample Output
=============

.. code-block:: console

    ADC reading[0]:
    - adc@50000100, channel 2: 227 = 182 mV
    - adc@50000100, channel 4: 193 = 155 mV
    - adc@50000400, channel 12: 218 = 175 mV
    - adc@50000400, channel 5: 224 = 180 mV
    - adc@50000500, channel 4: 206 = 165 mV
    - adc@50000500, channel 3: 348 = 280 mV

HV Shield
=========

TODO
