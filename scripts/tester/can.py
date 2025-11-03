# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0
#
# pylint: disable=import-error

import logging

from serial_message import SerialMessage
from util import TestRunner

log = logging.getLogger(__name__ + ".uart")


class CanTester(TestRunner):
    can_device_prefix: int = len("can@40006400")
    serial_output: list[SerialMessage] = []
    evaluation_errors: list[str] = []
    evaluation_logs: list[str] = []
    serial_error_logs: list[str] = []

    def test(self):
        try:
            self.serial_output = self.run_test_until_stop("can start", "can stop")

            self.evaluate_results()

        except Exception as e:
            log.error("Error testing CAN's: %s", e)
            raise

    def _set_error(self, message: str):
        self.evaluation_errors.append(message)

    def _list_tester_received_messages(self) -> list[SerialMessage]:
        return [
            m
            for m in self.serial_output
            if m.device == "tester" and "can@" in m.payload
        ]

    def _list_sut_send_messages(self) -> list[SerialMessage]:
        return [
            m for m in self.serial_output if m.device == "sut" and "can@" in m.payload
        ]

    def _separate_serial_error_logs(self):
        errors = [e for e in self.serial_output if e.log_level == "err"]
        self.serial_error_logs = errors

    def evaluate_results(self):
        self._separate_serial_error_logs()

        received_msgs = self._list_tester_received_messages()
        send_msgs = self._list_sut_send_messages()

        while len(received_msgs) > 0:
            device = received_msgs[0].payload[: self.can_device_prefix]

            received_msgs_for_device = [
                m
                for m in received_msgs
                if m.payload[: self.can_device_prefix] == device
            ]
            received_msgs = [
                m
                for m in received_msgs
                if m.payload[: self.can_device_prefix] != device
            ]

            send_msgs_for_device = [
                m for m in send_msgs if m.payload[: self.can_device_prefix] == device
            ]
            send_msgs = [
                m for m in send_msgs if m.payload[: self.can_device_prefix] != device
            ]

            has_different_length = len(received_msgs_for_device) != len(
                send_msgs_for_device
            )

            if has_different_length:
                self._set_error(
                    f"{device}: Skipping equality check since number of send message ({len(send_msgs_for_device)}) and received messages ({len(received_msgs_for_device)}) do not match"
                )
                continue

            self._assert_same_payload(
                device, send_msgs_for_device, received_msgs_for_device
            )

            self._assert_frequency(device, received_msgs_for_device)

        if len(self.evaluation_errors) != 0:
            log.error("Errors found during evaluation: %d", len(self.evaluation_errors))
            self.evaluation_logs.append(
                f"Errors Found during evaluation: {len(self.evaluation_errors)}"
            )
        else:
            self.evaluation_logs.append("All tests passed")

    def _assert_frequency(self, device: str, received: list[SerialMessage]):
        min_delta = 40
        max_delta = 60

        if len(received) < 2:
            self._set_error(
                f"Device {device}: can not assert frequency because Sample size is < 2 messages"
            )
            return

        for i in range(len(received)):
            if i == 0:
                continue
            previous = received[i - 1].time_ms
            current = received[i].time_ms
            delta = current - previous

            if delta > max_delta or delta < min_delta:
                self._set_error(
                    f"Frequency mismatch for device {device} at index {i}. Delta {delta}ms, previous timestamp: {previous}, current timestamp: {current}"
                )

    def _assert_same_payload(
        self, device: str, send: list[SerialMessage], received: list[SerialMessage]
    ):
        for i in range(len(send)):
            if send[i].payload != received[i].payload:
                self._set_error(
                    f"{device}: Payload mismatch at index {i}. Send {send[i].payload}, Received {received[i].payload}"
                )

    def has_errors(self) -> bool:
        if self.evaluation_errors:
            return True
        if self.serial_error_logs:
            return True

        return False

    def serialize_information(self) -> str:
        msg: list[str] = []
        msg.append(f"CAN: {'ERROR' if self.has_errors() else 'OK'}")
        msg.extend([f"  {m}" for m in self.evaluation_logs])
        return "\n".join(msg)

    def serialize_logs(self) -> str:
        msg: list[str] = []
        msg.append("CAN Errors:")
        msg.extend([f"  {m}" for m in self.evaluation_errors])
        msg.append("CAN serial errors during test:")
        msg.extend([f"  {m}" for m in self.serial_error_logs])
        msg.append("CAN Log:")
        msg.extend([f"  {m}" for m in self.serial_output])

        return "\n".join(msg)
