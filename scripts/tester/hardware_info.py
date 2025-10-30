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
                "received %d hardware information messages. Expected 2!",
                len(hwInfoMessages),
            )
            log.error(
                "received hardware information messages: %s", pplist(hwInfoMessages)
            )

            # pylint: disable-next=broad-exception-raised
            raise Exception(
                f"Error reading hardware information. Expected 2 messages. Got {len(hwInfoMessages)}"
            )

        self.hardwareInfo = hwInfoMessages
        self.assert_tester_and_sut_found()

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

    def assert_tester_and_sut_found(self):
        errors: list[tuple[str, Exception]] = []

        try:
            self.sut_id()
        except Exception as sut_error:  # pylint: disable=broad-except
            errors.append(("sut", sut_error))

        try:
            self.tester_id()
        except Exception as tester_error:  # pylint: disable=broad-except
            errors.append(("tester", tester_error))

        if not errors:
            return

        if len(errors) == 1:
            raise errors[0][1]

        combined = "; ".join(
            f"{device} lookup failed: {error}" for device, error in errors
        )

        log.error("Multiple hardware info lookup failures: %s", combined)

        # pylint: disable-next=broad-exception-raised
        raise Exception(f"Multiple hardware info lookup failures: {combined}")

    def sut_id(self) -> str:
        sut_info = self._single_device_message("sut")
        return sut_info.payload[len("id: ") :]

    def tester_id(self):
        tester_info = self._single_device_message("tester")
        return tester_info.payload[len("id: ") :]

    def _single_device_message(self, expected_device: str) -> SerialMessage:
        if not self.hardwareInfo:
            # pylint: disable-next=broad-exception-raised
            raise Exception("Hardware information has not been fetched yet")

        matches = [
            msg
            for msg in self.hardwareInfo
            if msg.device and msg.device.lower() == expected_device.lower()
        ]

        if len(matches) != 1:
            log.error(
                "expected exactly one %s hardware info message; got %d. Messages: %s",
                expected_device,
                len(matches),
                "; ".join(str(msg) for msg in self.hardwareInfo),
            )
            # pylint: disable-next=broad-exception-raised
            raise Exception(
                "Error reading hardware information. Expected exactly one "
                f"{expected_device} message. Got {len(matches)}"
            )

        return matches[0]
