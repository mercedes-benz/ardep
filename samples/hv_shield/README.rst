.. _hv_shield_sample:


HV Shield
#########

This firmware demonstrates the HV Shield driver using one HV (High voltage) GPIO and HV DAC.

Flash and run
=============

Build the firmware with:

.. code-block:: bash

  west build -b ardep samples/hv_shield --shield hv_shield


Flash it using dfu-util:

.. code-block:: bash

  west flash


Expected behavior
=================

The HV Shield puts out a slow sawtooth on AO36 and toggles D1
