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
from udsoncan.services import DiagnosticSessionControl, ECUReset


def print_headline(text: str):
    print(f"=== {text} ===")


def print_indented(text: str, indent: int = 1):
    print("\t" * indent + text)


def change_session(client: Client):
    print_headline("Changing Session")

    print_indented("Changing to programming session")
    client.change_session(DiagnosticSessionControl.Session.programmingSession)
    print_indented("Session change successful")

    print_indented("Changing to extended diagnostic session")
    client.change_session(DiagnosticSessionControl.Session.extendedDiagnosticSession)
    print_indented("Session change successful")

    print_indented("Changing to programming session")
    client.change_session(DiagnosticSessionControl.Session.programmingSession)
    print_indented("Session change successful")


def ecu_reset(client: Client):
    # Send ECU reset request (hard reset)
    print("Sending ECU hard reset request...")
    client.ecu_reset(ECUReset.ResetType.hardReset)
    print("\tECU reset request sending successfully")


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
    reset: bool = args.reset

    addr = isotp.Address(isotp.AddressingMode.Normal_11bits, rxid=0x7E0, txid=0x7E8)
    conn = IsoTPSocketConnection(can, addr)

    config = dict(udsoncan.configs.default_client_config)

    with Client(conn, config=config, request_timeout=2) as client:
        try_run(lambda: change_session(client))

        if reset:
            try_run(lambda: ecu_reset(client))

        print("\n=== Demo finished ===\n")


if __name__ == "__main__":
    parser = ArgumentParser(description="Firmware Loader Demo Client")
    parser.add_argument(
        "-c", "--can", default="vcan0", help="CAN interface (default: vcan0)"
    )
    parser.add_argument("-r", "--reset", action="store_true", help="Perform ECU reset")
    parsed_args = parser.parse_args()

    main(parsed_args)
