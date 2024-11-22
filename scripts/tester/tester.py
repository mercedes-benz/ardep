#!/bin/env python3
# pylint: disable=import-error
# pylint: disable=no-name-in-module


import argparse
import sys
from serial import Serial
import logging

from datetime import datetime
from uart import UartTester
from can import CanTester
from hardware_info import HardwareInformationFetcher
from gpio import GpioTester
from lin import LinTester

logger = logging.getLogger(__name__)


class Tester:
    port1: Serial = None
    port2: Serial = None

    hardware_info: HardwareInformationFetcher = None
    gpio: GpioTester = None
    uart: UartTester = None
    can: CanTester = None
    lin: LinTester = None

    def __init__(self, port1: Serial, port2: Serial):
        self.port1 = port1
        self.port2 = port2
        self.hardware_info = HardwareInformationFetcher(port1, port2)
        self.gpio = GpioTester(port1, port2)
        self.uart = UartTester(port1, port2)
        self.can = CanTester(port1, port2)
        self.lin = LinTester(port1, port2)

    def has_errors(self) -> bool:
        return (
            self.gpio.has_errors()
            or self.can.has_errors()
            or self.lin.has_errors()
            or self.uart.has_errors()
        )

    def run_tests(self):
        self.hardware_info.fetch()
        self.lin.test()
        self.uart.test()
        self.gpio.test()
        self.can.test()

    def serialize_evaluation_results(self) -> str:
        results = "\n".join(
            [
                self.hardware_info.serialize_information(),
                self.gpio.serialize_information(),
                self.uart.serialize_information(),
                self.can.serialize_information(),
                self.lin.serialize_information(),
            ]
        )

        return f"""
======================
||   TEST RESULTS   ||
======================
{results}
"""

    def serialize_logs(self) -> str:
        logs = "\n\n".join(
            [
                self.hardware_info.serialize_logs(),
                self.gpio.serialize_logs(),
                self.uart.serialize_logs(),
                self.can.serialize_logs(),
                self.lin.serialize_logs(),
            ]
        )

        return f"""
======================
||       LOGS       ||
======================
{logs}
"""


def parse_args() -> tuple[str, str]:
    parser = argparse.ArgumentParser(
        prog="Tester",
        description="Tests ARDEP boards",
    )

    parser.add_argument("serial_port1", default="/dev/ttyACM0")
    parser.add_argument("serial_port2", default="/dev/ttyACM1")

    args = parser.parse_args()
    port1: str = args.serial_port1
    port2: str = args.serial_port2

    return port1, port2


def clear_serial_buffer(serial_port: Serial):
    while serial_port.in_waiting != 0:
        serial_port.read(serial_port.in_waiting)


def main():
    logging.basicConfig(level=logging.DEBUG)

    port1, port2 = parse_args()

    with Serial(port1, 115200, timeout=10) as serial_port1:
        with Serial(port2, 115200, timeout=10) as serial_port2:
            clear_serial_buffer(serial_port1)
            clear_serial_buffer(serial_port2)

            tester = Tester(serial_port1, serial_port2)
            tester.run_tests()

            results = tester.serialize_evaluation_results()
            logs = tester.serialize_logs()

            time: str = datetime.now().isoformat()
            sut_id = tester.hardware_info.sut_id()
            filename = f"{time}_ardep_board_{sut_id}_hw_test_results.txt"

            with open(filename, "w", encoding="utf-8") as f:
                f.write(results)
                f.write(logs)

            print(results)
            print(f"Results saved to: {filename}")

            print("\n\n")
            if tester.has_errors():
                print("Test exited with errors!")
                sys.exit(1)
            else:
                print("Test successful")


if __name__ == "__main__":
    # execute only if run as the entry point into the program
    main()
