# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

import can
from argparse import ArgumentParser

class CanLogReceiver:
    command: str = "can-log-receiver"
    _default_interface: str = "can0"
    _default_id: int = 0x100

    def add_args(self, parser: ArgumentParser):
        subcommand_parser: ArgumentParser = parser.add_parser(
            self.command,
            help="Receives and prints CAN log messages from the specified CAN interface",
        )

        subcommand_parser.add_argument(
            "-i",
            "--interface",
            type=str,
            default=self._default_interface,
            help=f"CAN interface to use (default: {self._default_interface})",
        )

        subcommand_parser.add_argument(
            "-id",
            "--id",
            type=lambda x: int(x, 0),
            default=self._default_id,
            help=f"CAN ID to listen to (default: {hex(self._default_id)})",
        )

    def run(self, args):
        interface = args.interface
        id = args.id
        print("Starting CAN Log Receiver on interface:", interface, "listening to ID:", hex(id))

        try:
            with can.Bus(channel=interface, interface="socketcan") as bus:
                bus.set_filters([{"can_id": id, "can_mask": 0x7FF}])
                while True:
                    message = bus.recv()
                    if message is None:
                        continue

                    print(str(message.data.decode()), end="")
        except KeyboardInterrupt:
            pass
