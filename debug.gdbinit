# ARDEP Black Magic Probe Auto-Detection
# Try common device paths, first match wins

target extended-remote /dev/ttyBmpGdb
target extended-remote /dev/ttyACM0
target extended-remote /dev/ttyACM1
target extended-remote /dev/ttyACM2
target extended-remote /dev/ttyACM3
target extended-remote /dev/ttyUSB0
target extended-remote /dev/ttyUSB1
target extended-remote /dev/ttyUSB2
target extended-remote /dev/ttyUSB3
target extended-remote /dev/tty.usbmodem*
target extended-remote \\.\COM3
target extended-remote \\.\COM4
target extended-remote \\.\COM5
target extended-remote \\.\COM6
target extended-remote \\.\COM7
target extended-remote \\.\COM8
target extended-remote \\.\COM9
target extended-remote \\.\COM10
target extended-remote \\.\COM11
target extended-remote \\.\COM12
target extended-remote \\.\COM13
target extended-remote \\.\COM14
target extended-remote \\.\COM15
target extended-remote \\.\COM16

monitor auto_scan
attach 1
load
break main
continue