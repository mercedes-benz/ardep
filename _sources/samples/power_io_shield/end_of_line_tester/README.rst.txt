.. _power_io_shield_end_of_line_tester:

End of line tester firmware
###########################

The end of line tester firmware is an application for the ARDEP Power IO Shield that can be used in manufacturing to test the functionality of the Power IO Shield.

Setup
*****

Connect all inputs to the corresponding outputs on the Power IO Shield. (I.e. connect input 0 to output 0, input 1 to output 1, etc.).

Do not forget to connect 12V power to the shield/ARDEP and set the power mux to the correct position to power the Power IO Shield.

Usage
*****

To use it, connect an Power IO Shield to an ARDEP board, flash this firmware, and run the ``host.py`` script from the ``samples/power_io_shield/end_of_line_tester/`` directory on a host computer connected to the ARDEP board via UART.
Specify the correct serial port via the ``--port`` argument.

The script will tell the firmware to start the test by sending ``START`` via UART and will then test all digital inputs and outputs of the Power IO Shield.

If the test passes a message ``End of line tester completed successfully`` is printed, otherwise an error message is shown.
