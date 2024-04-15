.. _board_changelog:

Changelog
###########

.. contents::
   :local:
   :depth: 2

Rev 1.1
==============

See also:

- :download:`Schematics <ardep_board_rev_1_1_schematic.pdf>`
- :download:`Gerber File <ardep_board_rev_1_1_gerber.zip>`
- :download:`Altium Source Files <ardep_board_rev_1_1_sourcefiles.zip>`


Rev 1.0
==============

- Silkscreen adjustments
- Remove Jumpers underneath CMM chokes to prevent short circuits
- Change VIN Routing


..  figure:: rev_1_0.png
    :width: 600px

    Y=edit, R=removed, G=added

Rev 0.2
==============

Relevant for software development:

- Heartbeat Pinout
- CAN-Termination is no longer software configurable
- Swap PD14 and PD15 on Arduino header
- SWAP Pins along 32-pin header
- ADD Gearshift
- Change programming header pinout

Hardware:

- Combined Snapback ESD Protection
- Change input capacitors
- Change silkscreen

change the programming header position

..  figure:: rev_0_2.png
    :width: 600px

    change overview
