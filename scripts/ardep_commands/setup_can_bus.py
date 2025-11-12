# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

from argparse import ArgumentParser, Namespace
import subprocess
from west import log
from .util import Util


class SetupCanBus:
    command: str = "setup-can-bus"
    _board_name: str = None
    _description: str = None

    def __init__(self, board_name: str):
        self._board_name = board_name
        self._description = f"""\
Sets up a CAN bus interface using `ip` to interface with the {self._board_name} board.
"""

    def add_args(self, parser: ArgumentParser):
        subcommand_parser: ArgumentParser = parser.add_parser(
            self.command,
            help=f"configures the given interface with the parameters used by the {self._board_name}",
        )

        subcommand_parser.add_argument(
            "interface",
            help="the interface to configure (e.g. can0)",
            metavar="INTERFACE",
        )

    def run(self, args: Namespace):
        interface: str = args.interface

        if not Util.has_can_interface(interface):
            log.die(f"Interface {interface} not found", exit_code=1)

        cmd = f"sudo ip link set dev {interface} down && sudo ip link set {interface} type can bitrate 500000 && sudo ip link set dev {interface} up"
        log.inf(f"setting up interface {interface}")
        rc = subprocess.call(cmd, shell=True)
        if rc != 0:
            log.die(f"failed to set up interface {interface}", exit_code=2)
