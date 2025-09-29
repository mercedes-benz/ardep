#!/usr/bin/env -S uv run --script
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


import struct
import time
from typing import Any
from argparse import ArgumentParser, Namespace

import isotp
import udsoncan
from udsoncan import Baudrate
from udsoncan.client import Client
from udsoncan.connections import IsoTPSocketConnection
from udsoncan.exceptions import (
    NegativeResponseException,
)
from udsoncan.services import DiagnosticSessionControl
import subprocess


SESSION_NAME_MAP = {
    DiagnosticSessionControl.Session.defaultSession: "Default Session",
    DiagnosticSessionControl.Session.programmingSession: "Programming Session",
    DiagnosticSessionControl.Session.extendedDiagnosticSession: "Extended Diagnostic Session",
    DiagnosticSessionControl.Session.safetySystemDiagnosticSession: "Safety System Diagnostic Session",
}


def change_session(
    client: Client,
    session: DiagnosticSessionControl.Session,
    suppress_response: bool = False,
):
    session_name = SESSION_NAME_MAP.get(session, f"Unknown Session ({session})")
    print(f"\tChanging to session {session} ({session_name}) ...")
    if suppress_response:
        with client.suppress_positive_response(wait_nrc=True):
            client.change_session(session)
    else:
        client.change_session(session)
    print("\t\tSession change successful")


def read_data_by_identifier(client: Client):
    print("\tReading Data by Identifier...")
    data_id = 0x0050
    data = client.read_data_by_identifier_first([data_id])
    print(f"\t\tReading data from identifier\t0x{data_id:04X}:\t0x{data:04X}")


def link_control(client: Client, bitrate: int):
    print("\tLink Control...")

    print(f"\t\tRequesting bitrate transition to\t{bitrate}")
    response = client.link_control(control_type=0x01, baudrate=Baudrate(bitrate))
    if not response.positive:
        print("Negative response when requesting bitrate transition")
        return

    print(f"\t\tApplying bitrate to device: {bitrate}")

    with client.suppress_positive_response(wait_nrc=True):
        response = client.link_control(control_type=0x03)

        if (response is not None) and (not response.positive):
            print(
                "Negative response when applying bitrate. "
                "You might need to reset the device manually to fix the bitrate."
            )
            return

    client.conn.close()


def set_can_bitrate(can_device: str, bitrate: int):
    print("\t\tChanging Host bitrate")
    cmds = [
        ["sudo", "ip", "link", "set", "down", can_device],
        [
            "sudo",
            "ip",
            "link",
            "set",
            can_device,
            "type",
            "can",
            "bitrate",
            str(bitrate),
        ],
        ["sudo", "ip", "link", "set", "up", can_device],
    ]
    for cmd in cmds:
        print(f"\t\t\tExecuting: {' '.join(cmd)}")
        subprocess.run(cmd, check=True)


class CustomUint16Codec(udsoncan.DidCodec):
    def encode(self, *did_value: Any):
        return struct.pack(">H", *did_value)  # Big endian, 16 bit value

    def decode(self, did_payload: bytes):
        return struct.unpack(">H", did_payload)[0]  # decode the 16 bits value

    def __len__(self):
        return 2  # encoded payload is 2 byte long.


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
    default_bitrate = args.bitrate
    new_bitrate = args.changed_bitrate

    addr = isotp.Address(isotp.AddressingMode.Normal_11bits, rxid=0x7E0, txid=0x7E8)

    config = dict(udsoncan.configs.default_client_config)
    config["data_identifiers"] = {
        "default": ">H",  # Default codec is a struct.pack/unpack string. 16bits little endian
        0x0050: CustomUint16Codec,
    }

    print("Show read with default session after connecting...")
    conn = IsoTPSocketConnection(can, addr)
    with Client(conn, config=config, request_timeout=2) as client:
        try_run(
            lambda: change_session(
                client, DiagnosticSessionControl.Session.programmingSession
            )
        )

        try_run(lambda: read_data_by_identifier(client))
        try_run(lambda: link_control(client, new_bitrate))

    set_can_bitrate(can, new_bitrate)
    time.sleep(0.5)  # wait a bit for the CAN interface to come up again

    print("Now read on updated bitrate...")

    # Use a new connection, as the previous one was closed by link_control
    conn = IsoTPSocketConnection(can, addr)
    with Client(conn, config=config, request_timeout=2) as client:
        try_run(lambda: read_data_by_identifier(client))
        print("Changing into non default session should not affect bitrate...")
        try_run(
            lambda: change_session(
                client, DiagnosticSessionControl.Session.extendedDiagnosticSession
            )
        )
        try_run(lambda: read_data_by_identifier(client))

    print("Waiting for Session timeout as it resets the bitrate to default...")
    time.sleep(7)
    set_can_bitrate(can, default_bitrate)
    time.sleep(0.5)  # wait a bit for the CAN interface to come up again
    conn = IsoTPSocketConnection(can, addr)
    with Client(conn, config=config, request_timeout=2) as client:
        try_run(lambda: read_data_by_identifier(client))
        try_run(
            lambda: change_session(
                client, DiagnosticSessionControl.Session.programmingSession
            )
        )
        try_run(lambda: read_data_by_identifier(client))
        print(
            "Changing manually into default session should reset the bitrate to default..."
        )
        try_run(lambda: link_control(client, new_bitrate))

    set_can_bitrate(can, new_bitrate)
    time.sleep(0.5)  # wait a bit for the CAN interface to come up again
    conn = IsoTPSocketConnection(can, addr)
    with Client(conn, config=config, request_timeout=2) as client:
        try_run(lambda: read_data_by_identifier(client))
        print("Changing to default session should reset the bitrate...")
        try_run(
            lambda: change_session(
                client,
                session=DiagnosticSessionControl.Session.defaultSession,
                suppress_response=True,
            )
        )

    time.sleep(0.5)
    set_can_bitrate(can, default_bitrate)
    time.sleep(0.5)  # wait a bit for the CAN interface to come up again
    conn = IsoTPSocketConnection(can, addr)
    with Client(conn, config=config, request_timeout=2) as client:
        try_run(lambda: read_data_by_identifier(client))
        print("Changing bitrate in default session should result in an error")
        try:
            link_control(client, new_bitrate)
        except NegativeResponseException as e:
            print(f"\tCaught expected exception: {e}")

    print("\n=== Demo finished ===\n")


if __name__ == "__main__":
    parser = ArgumentParser(
        description=(
            "UDS ISO 14229 Link Control Demo Script\n\n"
            "Demonstrates UDS Link Control over CAN using ISO-TP.\n\n"
            "Requires root privileges and a physical CAN interface.\n\n"
            "Cannot be used with native_posix targets.\n\n"
            "Performs session change, bitrate transition, and data identifier read.\n\n"
        )
    )
    parser.add_argument(
        "-c", "--can", default="can0", help="CAN interface (default: can0)"
    )
    parser.add_argument(
        "-b",
        "--bitrate",
        type=int,
        default=500_000,
        help="CAN interface bitrate (default 500_000)",
    )
    parser.add_argument(
        "-m",
        "--changed-bitrate",
        type=int,
        default=250_000,
        help="CAN interface bitrate when changed by link control (default 250_000)",
    )
    parsed_args = parser.parse_args()

    main(parsed_args)
