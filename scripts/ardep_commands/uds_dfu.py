# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

from udsoncan.client import Client
from udsoncan.connections import IsoTPSocketConnection
from udsoncan.exceptions import (
    NegativeResponseException,
    InvalidResponseException,
    UnexpectedResponseException,
)
from udsoncan.services import DiagnosticSessionControl, RoutineControl, ECUReset
from argparse import ArgumentParser, Namespace
from west import log

import isotp
import udsoncan

from .util import Util


class UdsDfu:
    command: str = "uds-dfu"
    _board_name: str = None
    _description: str = None

    def __init__(self, board_name: str):
        self._board_name = board_name
        self._description = """\
Perform a firmware upgrade via CAN using the UDS protocol.
"""

    def add_args(self, parser: ArgumentParser):
        subcommand_parser: ArgumentParser = parser.add_parser(
            self.command,
            help=f"flash {self._board_name} using UDS",
        )

        subcommand_parser.add_argument(
            "-i",
            "--interface",
            help="can interface (default: can0)",
            default="can0",
            metavar="CAN",
        )

        subcommand_parser.add_argument(
            "-d",
            "--build-dir",
            help="application build directory (default: build)",
            default="build",
            metavar="DIR",
        )
        subcommand_parser.add_argument(
            "--rxid", help="rxid (default: 0x180)", default=0x180, type=int
        )
        subcommand_parser.add_argument(
            "--txid", help="txid (default: 0x80)", default=0x80, type=int
        )

    def run(self, args: Namespace):
        log.dbg("build dir", args.build_dir)
        log.dbg("can interface", args.interface)
        log.dbg("rxid", args.rxid)
        log.dbg("txid", args.txid)

        binary = Util.get_zephyr_signed_bin(args.build_dir)

        addr = isotp.Address(
            isotp.AddressingMode.Normal_11bits, rxid=args.rxid, txid=args.txid
        )
        conn = IsoTPSocketConnection(args.interface, addr)

        with Client(conn, request_timeout=2) as client:
            try:
                client.change_session(
                    DiagnosticSessionControl.Session.programmingSession
                )

                self.write_image(client, binary)

                client.routine_control(1338, RoutineControl.ControlType.startRoutine)

                client.ecu_reset(ECUReset.ResetType.hardReset)

            except NegativeResponseException as e:
                log.err(
                    'Server refused our request for service %s with code "%s" (0x%02x)'
                    % (
                        e.response.service.get_name(),
                        e.response.code_name,
                        e.response.code,
                    )
                )
            except (InvalidResponseException, UnexpectedResponseException) as e:
                log.err(
                    "Server sent an invalid payload : %s" % e.response.original_payload
                )

    def write_image(self, client: Client, filename: str):
        # open flash context
        client.routine_control(1337, RoutineControl.ControlType.startRoutine)

        response = client.request_download(
            memory_location=udsoncan.MemoryLocation(
                address=0, memorysize=0x1000, address_format=32, memorysize_format=32
            )
        )

        log.inf("max block length is", response.service_data.max_length)

        max_block_length = (
            response.service_data.max_length - 2
        )  # subtract 2 for overhead

        log.inf("uploading...")

        with open(filename, "rb") as f:
            data = f.read()
            offset = 0
            block = 0
            while offset < len(data):
                for i in range(0, 10):
                    try:
                        client.transfer_data(
                            block, data[offset : offset + max_block_length]
                        )
                        break
                    except TimeoutError:
                        log.wrn("Timeout, retrying")

                    if i == 9:
                        raise TimeoutError("Failed to transfer data")

                offset += max_block_length
                block = (block + 1) % 0x100

                def message():
                    o = offset % 20_000
                    if o < 4_000:
                        return "Prove P==NP           "
                    elif o < 8_000:
                        return "Talk to a colleague   "
                    elif o < 12_000:
                        return "Look out of the window"
                    elif o < 16_000:
                        return "Recite Pi             "
                    else:
                        return "Get a coffee          "

                # print progress
                print(
                    f"progress: {str(offset).zfill(len(str(len(data))))}/{len(data)} bytes    In the meantime: {message()}",
                    end="\r",
                )

        client.request_transfer_exit()
