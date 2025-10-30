import logging
import time
from serial import Serial

try:
    from .serial_message import BootBannerFound, SerialMessage
except ImportError:
    from serial_message import BootBannerFound, SerialMessage  # type: ignore

log = logging.getLogger(__name__ + ".idle")


class IdleCheckError(Exception):
    """Raised when the tester or SUT does not report idle in time."""


class IdleChecker:
    MAX_ATTEMPTS = 10

    def __init__(self, port1: Serial, port2: Serial):
        self.port1 = port1
        self.port2 = port2

    def ensure_idle(self):
        self._wait_until_idle(self.port1)
        self._wait_until_idle(self.port2)

    def _wait_until_idle(self, serial_port: Serial):
        for attempt in range(1, self.MAX_ATTEMPTS + 1):
            serial_port.writelines([b"idle\n"])
            log.debug("Sent idle request on %s (attempt %d)", serial_port.port, attempt)
            should_retry = False

            while True:
                raw_line = serial_port.readline()
                if raw_line == b"":
                    raise IdleCheckError(
                        f"No response while waiting for idle on {serial_port.port}"
                    )

                try:
                    line = raw_line.strip().decode("ascii")
                except UnicodeDecodeError as err:
                    log.warning(
                        "Received non-ascii line from %s: %s (error: %s)",
                        serial_port.port,
                        raw_line,
                        err,
                    )
                    continue

                if line == "":
                    continue
                if line == "idle":
                    continue

                try:
                    message = SerialMessage(line)
                except BootBannerFound as banner:
                    log.debug(
                        'Ignoring boot banner on %s while waiting for idle: "%s"',
                        serial_port.port,
                        banner.banner,
                    )
                    continue
                except Exception as err:  # pylint: disable=broad-except
                    log.warning(
                        'Error parsing serial line "%s" on %s: %s',
                        line,
                        serial_port.port,
                        err,
                    )
                    continue

                payload = message.payload.lower()
                if payload == "idle true":
                    log.info("Serial %s is idle", serial_port.port)
                    return

                if payload == "idle false":
                    log.info("Serial %s busy (idle false)", serial_port.port)
                    should_retry = True
                    break

            if should_retry:
                if attempt == self.MAX_ATTEMPTS:
                    break
                time.sleep(1)
                continue

        raise IdleCheckError(
            f"{serial_port.port} did not report idle true after {self.MAX_ATTEMPTS} attempts"
        )
