# pylint: disable=import-error

import logging

from serial_message import SerialMessage
from util import TestRunner

log = logging.getLogger(__name__ + ".uart")


class UartTester(TestRunner):
    serial_device_prefix: int = len("serial@40013800")
    serial_output: list[SerialMessage] = []
    evaluation_errors: list[str] = []
    evaluation_logs: list[str] = []
    serial_error_logs: list[str] = []

    def test(self):
        try:
            self.serial_output = self.run_test_until_stop("uart start", "uart stop")

            self.evaluate_results()

        except Exception as e:
            log.error("Error testing UART's: %s", e)
            raise

    def _set_error(self, message: str):
        self.evaluation_errors.append(message)

    def _list_tester_send_messages(self) -> list[SerialMessage]:
        return [
            m
            for m in self.serial_output
            if m.device == "tester" and "serial@" in m.payload and "send" in m.payload
        ]

    def _list_tester_received_messages(self) -> list[SerialMessage]:
        return [
            m
            for m in self.serial_output
            if m.device == "tester"
            and "serial@" in m.payload
            and "received" in m.payload
        ]

    def _list_sut_echoed_messages(self) -> list[SerialMessage]:
        return [
            m
            for m in self.serial_output
            if m.device == "sut" and "serial@" in m.payload and "received" in m.payload
        ]

    def _separate_serial_error_logs(self):
        errors = [e for e in self.serial_output if e.log_level == "err"]
        self.serial_error_logs = errors

    def evaluate_results(self):
        self._separate_serial_error_logs()

        send_msgs = self._list_tester_send_messages()
        tester_received_msgs = self._list_tester_received_messages()
        sut_echoed_msgs = self._list_sut_echoed_messages()
        
        if len(send_msgs) == 0:
            self._set_error("No messages sent by tester")
            return

        while len(send_msgs) > 0:
            device = send_msgs[0].payload[: self.serial_device_prefix]

            send_msgs_for_device = [
                m for m in send_msgs if m.payload[: self.serial_device_prefix] == device
            ]
            send_msgs = [
                m for m in send_msgs if m.payload[: self.serial_device_prefix] != device
            ]

            tester_received_msgs_for_device = [
                m
                for m in tester_received_msgs
                if m.payload[: self.serial_device_prefix] == device
            ]
            tester_received_msgs = [
                m
                for m in tester_received_msgs
                if m.payload[: self.serial_device_prefix] != device
            ]

            sut_echoed_msgs_for_device = [
                m
                for m in sut_echoed_msgs
                if m.payload[: self.serial_device_prefix] == device
            ]
            sut_echoed_msgs = [
                m
                for m in sut_echoed_msgs
                if m.payload[: self.serial_device_prefix] != device
            ]

            has_different_length = self._assert_same_length(
                device,
                send_msgs_for_device,
                tester_received_msgs_for_device,
                sut_echoed_msgs_for_device,
            )

            if has_different_length:
                self._set_error(
                    f"{device}: Skipping equality check since number of messages do not match"
                )
                continue

            self._assert_same_characters(
                device,
                send_msgs_for_device,
                tester_received_msgs_for_device,
                sut_echoed_msgs_for_device,
            )

        if len(self.evaluation_errors) != 0:
            log.error("Errors found during evaluation: %d", len(self.evaluation_errors))
            self.evaluation_logs.append(
                f"Errors Found during evaluation: {len(self.evaluation_errors)}"
            )
        else:
            self.evaluation_logs.append("All tests passed")

    def _assert_same_length(
        self,
        device: str,
        send_msgs_for_device: list[SerialMessage],
        tester_received_msgs_for_device: list[SerialMessage],
        sut_echoed_msgs_for_device: list[SerialMessage],
    ) -> bool:
        has_different_length: bool = False

        if len(send_msgs_for_device) != len(sut_echoed_msgs_for_device):
            self._set_error(
                f"{device}: Number of sent messages ({len(send_msgs_for_device)}) does not match number of received messages ({len(sut_echoed_msgs_for_device)}) (on SUT)"
            )
            has_different_length = True

        if len(send_msgs_for_device) != len(tester_received_msgs_for_device):
            self._set_error(
                f"{device}: Number of sent messages ({len(send_msgs_for_device)}) does not match number of received messages ({len(tester_received_msgs_for_device)}) (on tester)"
            )
            has_different_length = True

        return has_different_length

    def _assert_same_characters(
        self,
        device,
        send_msgs_for_device,
        tester_received_msgs_for_device,
        sut_echoed_msgs_for_device,
    ):
        for i, _ in enumerate(send_msgs_for_device):
            if (
                send_msgs_for_device[i].payload[-1]
                != sut_echoed_msgs_for_device[i].payload[-1]
            ):
                self._set_error(
                    f"{device}: Character at index {i} sent by tester contained ({send_msgs_for_device[i].payload[-1]}) but does not match character received by SUT ({sut_echoed_msgs_for_device[i].payload[-1]})"
                )

            if (
                send_msgs_for_device[i].payload[-1]
                != tester_received_msgs_for_device[i].payload[-1]
            ):
                self._set_error(
                    f"{device}: Character {i} sent by tester ({send_msgs_for_device[i].payload[-1]}) does not match character received by tester ({tester_received_msgs_for_device[i].payload[-1]})"
                )

    def has_errors(self) -> bool:
        if self.evaluation_errors:
            return True
        if self.serial_error_logs:
            return True

        return False

    def serialize_information(self) -> str:
        msg: list[str] = []
        msg.append(f"UART: {'ERROR' if self.has_errors() else 'OK'}")
        msg.extend([f"  {m}" for m in self.evaluation_logs])
        return "\n".join(msg)

    def serialize_logs(self) -> str:
        msg: list[str] = []
        msg.append("UART Errors:")
        msg.extend([f"  {m}" for m in self.evaluation_errors])
        msg.append("UART serial errors during test:")
        msg.extend([f"  {m}" for m in self.serial_error_logs])
        msg.append("UART Log:")
        msg.extend([f"  {m}" for m in self.serial_output])

        return "\n".join(msg)
