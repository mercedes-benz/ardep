.. _config_jumpers:

Config Jumpers
###############

This sample shows how to read out the configs jumpers on the board.
It uses zephyr macros to get all the pins into an array.
The example then initializes the pins and reads out the values.


Flash and run the example
-------------------------

The commands assume that you are in the root of the repo, not the workspace.

.. code-block:: bash

    west build --board ardep samples/config_jumpers

Flash the app using dfu-util:

.. code-block:: bash

    west flash

Sample Output
=============

.. code-block:: console

    Reading input pins
    Jumper 0: 0
    Jumper 1: 0
    Jumper 2: 1
