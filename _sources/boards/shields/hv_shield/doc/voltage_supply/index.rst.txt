.. _hv_extension_shield_voltage_supply:

Voltage Supply
###################

.. contents::
   :local:
   :depth: 2

The High-Voltage shield has three main “supply levels“.
It uses 3,3V as a logic supply, A constant Bias Voltage for the MOSFETs within the Input circuit, and one high-voltage supply for each of the four IO-Banks.


Logic supply
=============

The 3,3 V logic supply is provided via the Arduino-like pin header of the Mainboard. 

High Voltage Output supply
===========================

The high voltage level of the output drivers can individually be configured for each of the four IO-Banks.
Using the Jumpers P1..P4 the user can select to supply each IO Bank either with the Mainboard's input-voltage (Vin) or via the IO-Banks reference pin REF0..3, which can be accessed via the spring terminal block.



..  figure:: hv_shield_io_bank_voltage_level.png
    :width: 600px
   
    Headers for selecting the IO bank’s voltage level

Please note, that the voltage level, selected for an IO bank, also sets its maximum allowable Input voltage.
Surpassing the IO-Banks supply voltage may lead to damage to the corresponding output drivers.
**When the mainboard is supplied only via USB, the used IO Banks must be connected to an appropriate reference voltage!**

..  figure:: hv_shield_io_banks_pinout.png
    :width: 600px
   
    IO Banks as indicated on the pinout legend of the HV-Shield

   
.. list-table::
    :widths: 30 30 30 30
    :header-rows: 1
   
    *   - IO Bank
        - Output Drivers
        - Digital IOs
        - Reference Pin
    *   - 0
        - 0,1
        - D1..8
        - REF0
    *   - 1
        - 2,3
        - D9..16
        - REF1
    *   - 2
        - 4,5
        - D17..24
        - REF2
    *   - 3
        - 6,7
        - D25..32
        - REF3

MOSFET Bias Voltage
=====================

It was determined that a suitable MOSFET bias voltage should be at around 4.2V.
This ensures that the output levels are within the tolerated range of the STM32 and are reliably detected as logic high. 

The HV-Shield offers two ways of setting the bias voltage.
Either by using a diode supply or a dedicated linear regulator.
The source can be altered by changing the bias selection solder jumper.
**Read the documentation carefully and ensure you know what you are doing before attempting to change this setting.**


..  figure:: hv_shield_mosfet_bias_voltage_circuit.png
    :width: 600px
   
    Circuit for generating the MOSFET bias voltage
   
..  figure:: hv_shield_mosfet_bias_section_voltage_jumper.png
    :width: 300px
   
    Bias selection solder jumper (defaults to diode supply)
   

Diode supply (default)
************************

In the default configuration, the ~4.2 V supply is generated from the mainboard's 5V rail utilizing a silicon diode to achieve a voltage drop of ~0.7 V which results in an adequate bias voltage.


Linear Regulator
********************

For fine-tuning and experimental purposes, an additional linear regulator has been implemented.
It is supplied via the mainboard's Input voltage and can therefore output voltages above 5V.
Please note, that this regulator will not work when the mainboard is only supplied via USB. 

In the default configuration, the Feedback network of this regulator is just partially populated, missing R6.
That way it will output a voltage of ~1.7V until it has been reconfigured.
For setting the output voltage, a potentiometer can be connected to the bottom side of the PCB, or the appropriate voltage divider can directly be calculated according to the `TPS7A4101DGNR’s datasheet <https://www.ti.com/lit/ds/symlink/tps7a4101.pdf?ts=1711547797794&ref_url=https%253A%252F%252Fwww.ti.com%252Fpower-management%252Flinear-regulators-ldo%252Fproducts.html>`_.


..  figure:: hv_shield_mosfet_linear_regulator.png
    :width: 600px
   
    Linear Regulator
