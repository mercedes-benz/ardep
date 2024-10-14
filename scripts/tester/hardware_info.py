# pylint: disable=import-error

import logging
from util import TestRunner, pplist
from serial_message import SerialMessage

log = logging.getLogger(__name__ + ".hardware_info")


class HardwareInformation:
    message: SerialMessage = None
    id: str = None

    def __init__(self, message: SerialMessage):
        self.message = message


class HardwareInformationFetcher(TestRunner):
    serial_output: list[SerialMessage] = []
    hardwareInfo: list[HardwareInformation] = []

    def fetch(self):
        try:
            self.serial_output = self.run_test_until_stop("hwInfo start", "hwInfo stop")
        except Exception as e:
            print(f"Error reading hardware information: {e}")
            raise

        print(f"Serial Output: {self.serial_output}")

        hwInfoMessages: list[SerialMessage] = [
            m for m in self.serial_output if "id:" in m.payload
        ]

        if len(hwInfoMessages) != 2:
            log.error(
                "received %d hardware information messages. Expected 2!", len(hwInfoMessages)
            )
            log.error(
                "received hardware information messages: %s" ,pplist(hwInfoMessages)
            )

            # pylint: disable-next=broad-exception-raised
            raise Exception(
                f"Error reading hardware information. Expected 2 messages. Got {len(hwInfoMessages)}"
            )

        self.hardwareInfo = hwInfoMessages

    def serialize_information(self) -> str:
        msg: list[str] = []
        msg.append(f"SUT ID: {self.sut_id()}")
        msg.append(f"Tester ID: {self.tester_id()}")

        return "\n".join(msg)

    def serialize_logs(self) -> str:
        msg: list[str] = []
        msg.append("Hardware Information Log:")
        msg.extend([f"  {m}" for m in self.serial_output])
        return "\n".join(msg)

    def sut_id(self) -> str:
        sut_info = (
            self.hardwareInfo[0]
            if self.hardwareInfo[0].device.lower() == "sut"
            else self.hardwareInfo[1]
        )
        return sut_info.payload[len("id: ") :]

    def tester_id(self):
        tester_info = (
            self.hardwareInfo[0]
            if self.hardwareInfo[0].device.lower() == "tester"
            else self.hardwareInfo[1]
        )
        return tester_info.payload[len("id: ") :]
