# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0
#
# pylint: disable=import-error

import logging

from serial_message import SerialMessage
from util import TestRunner

log = logging.getLogger(__name__ + ".lin")


class LinTester(TestRunner):
    serial_device_prefix: int = len("serial@40013800")
    serial_output: list[SerialMessage] = []
    evaluation_errors: list[str] = []
    evaluation_logs: list[str] = []
    serial_error_logs: list[str] = []

    def test(self):
        try:
            self.serial_output = self.run_test_until_stop("lin start", "lin stop")

            self.evaluate_results()

        except Exception as e:
            log.error("Error testing LIN's: %s", e)
            raise

    def _set_error(self, message: str):
        self.evaluation_errors.append(message)

    def _list_tester_received_messages(self) -> list[SerialMessage]:
        return [
            m
            for m in self.serial_output
            if m.device == "tester"
            and "serial@" in m.payload
            and "received" in m.payload
        ]

    def _list_sut_send_messages(self) -> list[SerialMessage]:
        return [
            m
            for m in self.serial_output
            if m.device == "sut" and "serial@" in m.payload and "sending" in m.payload
        ]

    def _separate_serial_error_logs(self):
        errors = [e for e in self.serial_output if e.log_level == "err"]
        self.serial_error_logs = errors

    def evaluate_results(self):
        self._separate_serial_error_logs()

        tester_received_msgs = self._list_tester_received_messages()
        sut_send_msgs = self._list_sut_send_messages()
        
        if len(tester_received_msgs) == 0:
            self._set_error("No messages received by tester")
            return

        while len(sut_send_msgs) > 0:
            device = sut_send_msgs[0].payload[: self.serial_device_prefix]

            send_msgs_for_device = [
                m
                for m in sut_send_msgs
                if m.payload[: self.serial_device_prefix] == device
            ]
            sut_send_msgs = [
                m
                for m in sut_send_msgs
                if m.payload[: self.serial_device_prefix] != device
            ]

            received_msgs_for_device = [
                m
                for m in tester_received_msgs
                if m.payload[: self.serial_device_prefix] == device
            ]
            tester_received_msgs = [
                m
                for m in tester_received_msgs
                if m.payload[: self.serial_device_prefix] != device
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

    def _assert_same_payload(
        self, device: str, send: list[SerialMessage], received: list[SerialMessage]
    ):
        for index, (s, r) in enumerate(zip(send, received)):
            if s.payload[-1] != r.payload[-1]:
                self._set_error(
                    f"{device}: Send message '{s.payload}' does not match received message '{r.payload}' at index {index}"
                )

    def _assert_frequency(self, device: str, received: list[SerialMessage]):
        for index, r in enumerate(received):
            if index == 0:
                continue

            last_timestamp = received[index - 1].time_ms
            current_timestamp = r.time_ms
            delta = current_timestamp - last_timestamp

            min_delta = 90
            max_delta = 110
            if delta < min_delta or delta > max_delta:
                self._set_error(
                    f"{device}: Message frequency is in interval [{min_delta}, {max_delta}] [ms], got {delta}ms at index {index}. (last: {last_timestamp}, current: {current_timestamp})"
                )

    def has_errors(self) -> bool:
        if self.evaluation_errors:
            return True
        if self.serial_error_logs:
            return True

        return False

    def serialize_information(self) -> str:
        msg: list[str] = []
        msg.append(f"LIN: {'ERROR' if self.has_errors() else 'OK'}")
        msg.extend([f"  {m}" for m in self.evaluation_logs])
        return "\n".join(msg)

    def serialize_logs(self) -> str:
        msg: list[str] = []
        msg.append("LIN Errors:")
        msg.extend([f"  {m}" for m in self.evaluation_errors])
        msg.append("LIN serial errors during test:")
        msg.extend([f"  {m}" for m in self.serial_error_logs])
        msg.append("LIN Log:")
        msg.extend([f"  {m}" for m in self.serial_output])

        return "\n".join(msg)
