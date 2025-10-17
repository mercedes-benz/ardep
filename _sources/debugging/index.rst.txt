.. _debugging:

Debugging
#########

.. contents::
   :local:
   :depth: 2

The following section describes how to use an on chip debugger with ardep.

Connect a debug probe
=====================

Connect a debug probe to the SWD pins of the ARDEP board.

.. image:: swd_pinout.png
   :width: 600
   :alt: measurement

Pin one should only be connected to a voltage measuring pin (if required) not to power the board.

For example for an ST-Link V2 you can connect the following pins:

* SWDIO to pin 2
* SWCLK to pin 4
* GND to pin 5
* RST to pin 3

Flash using openocd
-------------------

To flash the board using openocd you can use the following command:

.. code-block:: bash

   west flash --runner openocd

Debug using openocd
-------------------

To debug the board using openocd you can use the following command:

.. code-block:: bash

   west debug --runner openocd

Flash and debug using JLINK
---------------------------

Use the sample commands as above, but replace `openocd` with `jlink`.

