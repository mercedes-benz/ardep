.. _uart_sample:

UART Sample
###########

This example demonstrates a basic uart input and output.

The uart can be connected through an USB-to-serial adapter to a PC.
The baud rate should be set to 115200.

Flash and run the example
-------------------------

The commands assume that you are in the root of the repo, not the workspace.

.. code-block:: bash

    west build --board ardep samples/uart

Flash the app using dfu-util:

    .. code-block:: bash

        west flash
    
