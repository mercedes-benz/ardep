# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

"""ardep.py

Extension for all commands regarding the ardep board"""

from argparse import ArgumentParser
from ardep_commands import (
    BuildBootloader,
    CreateUdevRule,
    DfuUtil,
    SetupCanBus,
    UdsDfu,
    CanLogReceiver
)
from west.commands import WestCommand  # your extension must subclass this
from west import log  # use this for user output

import argparse


class ArdepWestCommand(WestCommand):
    _board_name: str = "ardep"
    _vid: str = "25e1"
    _pid_dfu: str = "ffff"
    _pid: str = "1b1e"

    def __init__(self):
        super().__init__(
            "ardep",  # gets stored as self.name
            "commands related to the ardep baord",  # self.help
            # self.description:
            """\
This command is used together with the ardep board.

TODO: Add more details here.
            """,
        )
        self._build_bootloader = BuildBootloader(self._board_name)
        self._create_udev_rule = CreateUdevRule(
            self.name, self._board_name, self._vid, self._pid, self._pid_dfu
        )
        self._uds_dfu = UdsDfu(self._board_name)
        self._dfu_util = DfuUtil(
            self._board_name,
            self._vid,
            self._pid,
        )
        self._setup_can_bus = SetupCanBus(self._board_name)
        self._can_log_receiver = CanLogReceiver()

    def do_add_parser(self, parser_adder: ArgumentParser):
        parser: ArgumentParser = parser_adder.add_parser(
            self.name, help=self.help, description=self.description
        )

        subcommands: argparse._SubParsersAction[ArgumentParser] = parser.add_subparsers(
            dest="subcommand"
        )

        self._create_udev_rule.add_args(subcommands)
        self._setup_can_bus.add_args(subcommands)
        self._dfu_util.add_args(subcommands)
        self._uds_dfu.add_args(subcommands)
        self._build_bootloader.add_args(subcommands)
        self._can_log_receiver.add_args(subcommands)

        return parser

    def do_run(
        self, args: argparse.Namespace, unknown_args
    ):  # pylint: disable=arguments-renamed,unused-argument
        match args.subcommand:
            case self._build_bootloader.command:
                return self._build_bootloader.run(args)
            case self._create_udev_rule.command:
                return self._create_udev_rule.run(args)
            case self._setup_can_bus.command:
                return self._setup_can_bus.run(args)
            case self._dfu_util.command:
                return self._dfu_util.run(args)
            case self._uds_dfu.command:
                return self._uds_dfu.run(args)
            case self._can_log_receiver.command:
                return self._can_log_receiver.run(args)
            case _:
                log.inf("No subcommand specified")
                log.inf(f"See `west {self.name} -h` for help")
