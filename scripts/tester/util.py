# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0
#
# pylint: disable=import-error

import logging
from multiprocessing import Manager, Process
from serial import Serial
from serial_message import BootBannerFound, SerialMessage

log = logging.getLogger(__name__ + ".util")


class TestRunner:
    port1: Serial = None
    port2: Serial = None

    def __init__(self, port1: Serial, port2: Serial):
        self.port1 = port1
        self.port2 = port2

    def run_test_until_stop(
        self,
        start_message: str,
        stop_message: str,
    ) -> list[SerialMessage]:
        serial_output: list[SerialMessage] = Manager().list()
        p1 = Process(
            target=execute_test,
            args=(self.port1, serial_output, start_message, stop_message),
        )
        p2 = Process(
            target=execute_test,
            args=(self.port2, serial_output, start_message, stop_message),
        )

        try:
            p1.start()
            p2.start()

            p1.join()
            p2.join()
        except Exception as e:
            log.error("Error running test: %s", e)
            raise
        return [m for m in serial_output]


def run_test_until_stop(
    port1: Serial,
    port2: Serial,
    start_message: str,
    stop_message: str,
) -> list[SerialMessage]:
    serial_output: list[SerialMessage] = Manager().list()
    p1 = Process(
        target=execute_test,
        args=(port1, serial_output, start_message, stop_message),
    )
    p2 = Process(
        target=execute_test,
        args=(port2, serial_output, start_message, stop_message),
    )

    try:
        p1.start()
        p2.start()

        p1.join(timeout=30)
        p2.join(timeout=30)
    except Exception as e:
        log.error("Error running test: %s", e)
        raise

    print(f"Raw Output: {serial_output}")
    return [m for m in serial_output]


def execute_test(
    serial: Serial, messages: list[SerialMessage], start_message: str, stop_message: str
):
    log.debug(' started test with start message "%s" on %s', start_message, serial.port)
    start = f"{start_message}\n".encode("ascii")
    serial.writelines([start])
    while True:
        try:
            line = serial.readline().strip().decode("ascii")
            if line == "":
                # pylint: disable-next=broad-exception-raised
                raise Exception(f"Read empty line from serial {serial}")
            if line == start_message:
                continue
            message = SerialMessage(line)
        except BootBannerFound as bbf:
            log.info(
                'Boot banner found on serial %s: "%s"',
                serial.port,
                bbf.banner,
            )
            continue
        except Exception as e:
            log.error(
                'Error on test with start message: "%s" and serial %s',
                start_message,
                serial,
            )
            log.error("Error parsing message %s Error: %s", line, e)
            raise
        messages.append(message)
        if message.payload == stop_message:
            log.debug(
                'finished reading from %s. Received stop message "%s"',
                serial.port,
                stop_message,
            )
            break


def pplist(l: list):
    string = ""
    string += "[\n"
    string += "\n".join([f"  {e}" for e in l])
    string += "\n]"
    print(string)
