# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

from argparse import ArgumentParser, Namespace
from os import path
import os
import subprocess
import tempfile
from west import log


class CreateUdevRule:
    command: str = "create-udev-rule"
    _command_name: str = None
    _udev_file_name: str = None
    _board_name: str = None
    _vid: str = None
    _pid: str = None
    _pid_dfu: str = None

    def __init__(
        self, command_name: str, board_name: str, vid: str, pid: str, pid_dfu: str
    ):
        self._command_name = command_name
        self._board_name = board_name
        self._udev_file_name: str = f"99-{self._command_name}.rules"
        self._vid = vid
        self._pid = pid
        self._pid_dfu = pid_dfu

    def add_args(self, parser: ArgumentParser):
        subcommand_parser: ArgumentParser = parser.add_parser(
            self.command,
            help=f"creates a udev rule to allow dfu-util access to {self._command_name} devices (may require `sudo`)",
        )

        subcommand_parser.add_argument(
            "-d",
            "--directory",
            help="destination directory for the udev rule (default: /etc/udev/rules.d)",
            default="/etc/udev/rules.d",
            metavar="DIRECTORY",
        )

        subcommand_parser.add_argument(
            "-f",
            "--force",
            help="forces an overwrite, if the file rule file already exists",
            action="store_true",
        )

    def run(self, args: Namespace):
        destination_directory: str = path.realpath(args.directory)
        rule_path: str = f"{destination_directory}/{self._udev_file_name}"

        if not path.exists(destination_directory):
            log.die(
                f"Destination directory does not exist: {destination_directory}",
                exit_code=1,
            )

        if (not args.force) and path.exists(rule_path):
            log.die(
                f"File already exists at: {rule_path}\nuse --force to overwrite",
                exit_code=2,
            )

        (fd, tmpfile) = tempfile.mkstemp()
        with open(fd, "w", encoding="utf-8") as f:
            f.write(
                f"""\
# {self._board_name}: Udev rule required by dfu-util to access {self._board_name} devices

ATTRS{{idVendor}}=="{self._vid}", ATTRS{{idProduct}}=="{self._pid}", MODE="664", GROUP="plugdev"
ATTRS{{idVendor}}=="{self._vid}", ATTRS{{idProduct}}=="{self._pid_dfu}", MODE="664", GROUP="plugdev"
    """
            )

        cmd = ["mv", tmpfile, rule_path]
        if not os.access(destination_directory, os.W_OK):
            cmd = ["sudo"] + cmd

        rc = subprocess.call(cmd)

        if rc != 0:
            log.die(f"Failed to create udev rule at {rule_path}", exit_code=3)

        log.inf("New rule was successfully created")

        if destination_directory.startswith("/etc/udev/rules.d"):
            log.inf(
                f"To activate the new rule, unplug the {self._board_name} and run:\n    sudo udevadm control --reload-rules && sudo udevadm trigger"
            )
