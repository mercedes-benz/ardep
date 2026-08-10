# Hardware Source files
This directory includes all hardware source files for the ARDEP Mainboard and auxilary shields.

## KiCad Conversion
The initial project development was started in Altium designer. After the Hardware release of the ARDEP Mainboard rev 2.1 and the Power IO shield rev 1.0 in December 2025, the Altium source files were converted to KiCad.

From this point on, the KiCad source files are considered as the active project documents while the Altium source files are treated as legacy data and will no longer be actively maintained.

## OpenSCAD file
OpenSCAD enclosure models are stored in this hardware folder as source .scad files.
Use descriptive names such as ardep_enclosure.scad and prefer parametric dimensions/cutouts.
For each new model, include a short note with parameters and recommended print settings.
Generated STL files should only be committed when explicitly requested by maintainers.