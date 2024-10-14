# pylint: disable=import-error

import logging
from util import TestRunner
from serial_message import SerialMessage

log = logging.getLogger(__name__ + ".gpio")


class GpioTester(TestRunner):
    serial_output: list[SerialMessage] = []
    serial_error_logs: list[str] = []
    evaluation_logs: list[str] = []
    evaluation_errors: list[str] = []

    def test(self):
        try:
            self.serial_output = self.run_test_until_stop("gpio start", "gpio stop")

            self.evaluate_results()

        except Exception as e:
            log.error("Error testing GPIO's: %s", e)
            raise

    def _separate_serial_error_logs(self):
        errors = [e for e in self.serial_output if e.log_level == "err"]
        self.serial_error_logs = errors

    def _set_error(self, message: str):
        self.evaluation_errors.append(message)

    def _list_sut_pin_on_messages(self) -> list[SerialMessage]:
        return [
            m
            for m in self.serial_output
            if m.device == "sut" and "gpio@" in m.payload and m.payload.endswith("on")
        ]

    def _list_tester_rising_edge_messages(self) -> list[SerialMessage]:
        return [
            m
            for m in self.serial_output
            if m.device == "tester" and "gpio@" in m.payload and "rising" in m.payload
        ]

    def _list_tester_falling_edge_messages(self) -> list[SerialMessage]:
        return [
            m
            for m in self.serial_output
            if m.device == "tester" and "gpio@" in m.payload and "falling" in m.payload
        ]

    def _evaluate_pin_toggle_count(self):
        sut_pin_msgs = self._list_sut_pin_on_messages()
        sut_len = len(sut_pin_msgs)
        
        if sut_len == 0:
            self._set_error("No pin toggles on SUT")
            return

        tester_rising_pin_msgs = self._list_tester_rising_edge_messages()
        rising_pin_len = len(tester_rising_pin_msgs)
        self.evaluation_logs.append(f"Found {rising_pin_len} rising edges on Tester")

        tester_falling_pin_msgs = self._list_tester_falling_edge_messages()
        falling_pin_len = len(tester_falling_pin_msgs)
        self.evaluation_logs.append(f"Found {falling_pin_len} falling edges on Tester")

        if sut_len != rising_pin_len:
            self.evaluation_logs.append(
                f"{sut_len} pin toggles on SUT != {rising_pin_len} rising edges on Tester"
            )
            self._set_error(
                f"Unmatching pin toggles on SUT ({sut_len}) and rising edges on Tester ({rising_pin_len})"
            )

        if rising_pin_len != falling_pin_len:
            self.evaluation_logs.append(
                f"{rising_pin_len} rising edges on tester != {falling_pin_len} falling edges on tester"
            )
            self._set_error(
                f"Unmatching rising edges ({rising_pin_len}) and falling edges ({falling_pin_len}) on Tester"
            )

    def _evaluate_rising_edge_timing(self):
        tester_rising_pin_msgs = self._list_tester_rising_edge_messages()

        has_error = False
        min_time = 17
        max_time = 23
        for index, msg in enumerate(tester_rising_pin_msgs):
            if index == 0:
                continue
            time_diff = msg.time_ms - tester_rising_pin_msgs[index - 1].time_ms
            if time_diff < min_time or time_diff > max_time:
                has_error = True
                self._set_error(
                    f'Time Gap ({time_diff}ms) between rising edge message: "{tester_rising_pin_msgs[index-1]}" and "{tester_rising_pin_msgs[index]}"'
                )

        if has_error:
            self.evaluation_logs.append(
                "Rising edge timing not in interval [{min_time}, {max_time}]"
            )
            return

    def _evaluate_port_order(self):
        port_pin_length = len("gpio@48000800 pin 1")
        tester_rising_pins = [
            m.payload[:port_pin_length]
            for m in self._list_tester_rising_edge_messages()
        ]
        tester_falling_pins = [
            m.payload[:port_pin_length]
            for m in self._list_tester_falling_edge_messages()
        ]
        sut_pins = [
            m.payload[:port_pin_length] for m in self._list_sut_pin_on_messages()
        ]

        port_order_error: bool = False

        for index, _ in enumerate(tester_rising_pins):
            if tester_rising_pins[index] != tester_falling_pins[index]:
                self._set_error(
                    f"Unmatching rising and falling pins on tester: {tester_rising_pins[index]} != {tester_falling_pins[index]} at index {index}"
                )
                port_order_error = True
            if tester_rising_pins[index] != sut_pins[index]:
                self._set_error(
                    f"Unmatching rising pins on tester and sut: {tester_rising_pins[index]} != {sut_pins[index]} at index {index}"
                )
                port_order_error = True
            if tester_falling_pins[index] != sut_pins[index]:
                self._set_error(
                    f"Unmatching falling pins on tester and sut: {tester_falling_pins[index]} != {sut_pins[index]} at index {index}"
                )
                port_order_error = True

        if port_order_error:
            self.evaluation_logs.append("Port order has ERRORs")
        else:
            self.evaluation_logs.append("Port order is OK")

    def evaluate_results(self):
        self._separate_serial_error_logs()

        self.evaluation_logs = []
        self.evaluation_errors = []

    def has_errors(self) -> bool:
        if self.evaluation_errors:
            return True
        if self.serial_error_logs:
            return True

        return False

    def serialize_information(self) -> str:
        msg: list[str] = []
        msg.append(f"GPIO: {'ERROR' if self.has_errors() else 'OK'}")
        if self.has_errors():
            msg.extend([f"  {m}" for m in self.evaluation_logs])
        else:
            msg.append("  All tests passed")
        return "\n".join(msg)

    def serialize_logs(self) -> str:
        msg: list[str] = []
        msg.append("GPIO Errors:")
        msg.extend([f"  {m}" for m in self.evaluation_errors])
        msg.append("GPIO serial errors during test:")
        msg.extend([f"  {m}" for m in self.serial_error_logs])
        msg.append("GPIO Log:")
        msg.extend([f"  {m}" for m in self.serial_output])

        return "\n".join(msg)
