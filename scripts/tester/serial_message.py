# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0
#
# pylint: disable=import-error

import logging

log = logging.getLogger(__name__ + ".serial_message")


class BootBannerFound(Exception):
    banner: str

    def __init__(self, banner: str):
        self.banner = banner


class SerialMessage:
    time_ms: int = None
    log_level: str = None
    device: str = None
    payload: str = None

    def __init__(self, message: str):
        try:
            if message.startswith("***") and message.endswith("***"):
                raise BootBannerFound(message)

            raw_timestamp, raw_level, raw_device, raw_message = message.split(" ", 3)
            self.time_ms = int(raw_timestamp.strip("[]"))
            self.log_level = raw_level.strip("<>")
            self.device = raw_device.strip(":")
            self.payload = raw_message.strip()
        except BootBannerFound:
            raise
        except Exception as e:
            log.error('Error parsing message "%s". Error: %s', message, e)
            raise

    def __str__(self):
        return f"{self.time_ms} {self.log_level} {self.device}: {self.payload}"

    def __repr__(self):
        return f"SerialMessage({self.time_ms}, {self.log_level}, {self.device}, {self.payload})"
