.. _power_io_shield_multiple_sample:


Multiple shields sample
#######################

This firmware demonstrates how to use multiple Power IO Shields with the Ardep platform.
It configures 2 Power IO Shields, one set to addres 0 and the other to address 1.

Flash and run
=============

Build the firmware with:

.. code-block:: bash

  west build -b ardep samples/power_io_shield/multiple/


Flash it using dfu-util:

.. code-block:: bash

  west flash


Expected behavior
=================

Both Power IO Shields' outputs should count up in binary every second. The inputs and faults are also logged once every second.

.. note::
  Don't forget to connect a suitable power supply to the Power IO Shield to see the outputs toggling. See :ref:`power_io_shield_voltage_supply` for more information
