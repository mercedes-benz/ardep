#!/usr/bin/env -S uv run --script
# /// script
# dependencies = [
#    "can-isotp==2.0.3",
#    "udsoncan==1.21.2",
# ]
# ///
#
# Copyright (C) Frickly Systems GmbH
# Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0


from argparse import ArgumentParser, Namespace

import isotp
import udsoncan
from udsoncan.client import Client
from udsoncan.connections import IsoTPSocketConnection
from udsoncan.exceptions import (
    NegativeResponseException,
)
from udsoncan.services import DiagnosticSessionControl


def print_headline(text: str, suppress_trailing_newline: bool = False):
    print(f"\n=== {text} ===\n", end="" if suppress_trailing_newline else "\n")


def print_sub_headline(text: str):
    print(f"\n\t--- {text} ---\n")


def print_indented(text: str, indent: int = 1):
    print("\t" * indent + text)


def change_session(client: Client):
    print_headline("Changing sessions")

    print_indented("Switching to programming session")
    client.change_session(DiagnosticSessionControl.Session.programmingSession)

    print_indented("Switching to extended diagnostic session")
    client.change_session(DiagnosticSessionControl.Session.extendedDiagnosticSession)

    print_indented("Switching to custom session (0x16)")
    client.change_session(0x16)

    print_indented("Now wait a bit for the session timeout event.")


def try_run(runnable):
    try:
        runnable()

    except NegativeResponseException as e:
        print(
            f"Server refused our request for service {e.response.service.get_name()} "
            f'with code "{e.response.code_name}" (0x{e.response.code:02X})'
        )


def main(args: Namespace):
    can: str = args.can

    addr = isotp.Address(isotp.AddressingMode.Normal_11bits, rxid=0x7E0, txid=0x7E8)
    conn = IsoTPSocketConnection(can, addr)

    config = dict(udsoncan.configs.default_client_config)

    with Client(conn, config=config, request_timeout=2) as client:
        try_run(lambda: change_session(client))

        print_headline("Demo finished")


if __name__ == "__main__":
    parser = ArgumentParser(description="ISO 14229 Demo Client")
    parser.add_argument(
        "-c", "--can", default="vcan0", help="CAN interface (default: vcan0)"
    )
    parsed_args = parser.parse_args()

    main(parsed_args)
