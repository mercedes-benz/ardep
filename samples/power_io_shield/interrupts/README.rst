.. _power_io_shield_interrupts_sample:


Interrupts Sample
#################

This firmware demonstrates the Interrupt capabilities of the Power IO Shield by registering a few different interrupts and logging events

Flash and run
=============

Build the firmware with:

.. code-block:: bash

  west build -b ardep samples/power_io_shield/interrupts/


Flash it using dfu-util:

.. code-block:: bash

  west flash


Expected behavior
=================

A log is displayed when input pin 0 is detects a raising edge, input pin 1 detects a falling edge, input pin 2 detects any edge or as long as pin 3 is high
It also logs the state of all inputs and fault pins every second
