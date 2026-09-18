.. _power_io_shield_toggle_outputs_sample:


Toggle Outputs Sample
#####################

This firmware demonstrates the Power IO Shield's output capabilities by toggling the output pins as a binary counter.

Flash and run
=============

Build the firmware with:

.. code-block:: bash

  west build -b ardep samples/power_io_shield/toggle_outputs


Flash it using dfu-util:

.. code-block:: bash

  west flash


Expected behavior
=================

The Power IO Shield's output pins should count up in binary every second. The inputs and faults are also logged once every second

.. note::
  Don't forget to connect a suitable power supply to the Power IO Shield to see the outputs toggling. See :ref:`power_io_shield_voltage_supply` for more information
