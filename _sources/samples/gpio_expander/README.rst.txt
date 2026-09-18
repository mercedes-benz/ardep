.. _gpio_expander:

GPIO Expander
#############

This firmware allows to configure, read and write all GPIOs over CAN.

Usage
==============================

#. Build and flash

    .. code-block:: bash

        west build -b ardep -p auto samples/gpio_expander
        west flash

#. Enable can interface

    .. code-block:: bash

        sudo ip link set can0 type can bitrate 500000
        sudo ip link set up can0

    There should be a can interface, linke a USB-CAN adapter connected.
    The CAN adapter should be connected to the CAN A of the board.
    Termination should be activated on both ends.

#. Read pin values

    .. code-block:: bash

        candump can0

    The frame with 0x003 provides the values of the pins

#. Confiure pins as output

    .. code-block:: bash

        cansend can0 001#0F00000000000000

    This configures the first 4 pins as output

#. Confiure pins as output

.. code-block:: bash

    cansend can0 001#0800000000000000

    This writes a high value to pin 3 and low to 0, 1, 2, if the first 4 pins are configured as output.

Protocol
---------------------------------

The order of the pins, depends on the pin definiton in the overlay.
Initially all pins are configured as input.

#. Config frame (client request)

    8 byte: 7 byte config + 1 byte reserved

    bit 0 → Input, bit 1 → Output

#. GPIO State frame (server update)

    8 byte: 7byte io States, 1 byte reserved

    bit 0 → IO Low, bit 1 → io High

#. Write GPIOs (client request)

    8 byte: 7byte io States, 1 byte reserved

    bit 0 → IO Low, bit 1 → io High

    Pins not configured as output are ignore, but value might be stored if pin is later configured as output.