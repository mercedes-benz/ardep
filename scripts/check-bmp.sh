#!/bin/bash
# ARDEP Black Magic Probe Detection

for dev in /dev/ttyBmpGdb /dev/ttyACM0 /dev/ttyACM1 /dev/ttyACM2 /dev/ttyACM3 /dev/ttyUSB0 /dev/ttyUSB1 /dev/ttyUSB2 /dev/ttyUSB3 /dev/tty.usbmodem*; do
    if [ -e "$dev" ]; then
        echo "OK: BMP at $dev"
        exit 0
    fi
done

echo "ERROR: No Black Magic Probe detected."
echo "Check USB connection and permissions."
exit 1