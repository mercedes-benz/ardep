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


import struct
from typing import Any
from udsoncan.client import Client
from udsoncan.connections import IsoTPSocketConnection
from udsoncan.exceptions import (
    NegativeResponseException,
)
from udsoncan.services import DiagnosticSessionControl, ECUReset
from argparse import ArgumentParser, Namespace

import isotp
import udsoncan


def change_session(client: Client):
    print("Changing to programming session")
    client.change_session(DiagnosticSessionControl.Session.programmingSession)
    print("\tChange session successful")


def read_write_data_by_identifier(client: Client):
    print("Data By Identifier:")

    data_id: int = 0x0100
    data = client.read_data_by_identifier([data_id])
    print(
        f'\tRead data from identifier {data_id:04X}: "{data.service_data.values[data_id]}"\n'
    )

    data_id = 0x0050
    data = client.read_data_by_identifier_first([data_id])
    print(f"\tReading data from identifier 0x{data_id:04X}: 0x{data:04X}\n")

    write_data = 0x1234
    data = client.write_data_by_identifier(data_id, write_data)
    print(f"\tWritten data to identifier 0x{data_id:04X}: 0x{write_data:04X}")

    data = client.read_data_by_identifier_first([data_id])
    print(f"\tReading data from identifier 0x{data_id:04X}: 0x{data:04X}\n")

    write_data = 0xBEEF
    data = client.write_data_by_identifier(data_id, write_data)
    print(f"\tWritten data to identifier 0x{data_id:04X}: 0x{write_data:04X}")

    data = client.read_data_by_identifier_first([data_id])
    print(f"\tReading data from identifier 0x{data_id:04X}: 0x{data:04X}")


def ecu_reset(client: Client):
    # Send ECU reset request (hard reset)
    print("Sending ECU hard reset request...")
    client.ecu_reset(ECUReset.ResetType.hardReset)
    print("ECU reset request sent successfully")


class MyCustomCodec(udsoncan.DidCodec):
    def encode(self, *did_value: Any):
        return struct.pack(">H", *did_value)  # Big endian, 16 bit value

    def decode(self, did_payload: bytes):
        return struct.unpack(">H", did_payload)[0]  # decode the 16 bits value

    def __len__(self):
        return 2  # encoded payload is 2 byte long.


class StringCodec(udsoncan.DidCodec):
    def encode(self, *did_value: Any):
        value = did_value[0] if did_value else ""
        encoded = value.encode("ascii") if isinstance(value, str) else value
        # Ensure the data is exactly 15 bytes long, terminated with null byte
        if len(encoded) >= 15:
            return encoded[:14] + b"\x00"
        else:
            return encoded + b"\x00" * (15 - len(encoded))

    def decode(self, did_payload: bytes):
        return did_payload.decode("ascii", errors="ignore").rstrip("\x00")

    def __len__(self):
        return 15  # "Hello from UDS" + null terminator = 15 bytes


def try_run(runnable):
    try:
        runnable()

    except NegativeResponseException as e:
        print(
            f"Server refused our request for service {e.response.service.get_name()} "
            f'with code "{e.response.code_name}" (0x{e.response.code:02x})'
        )


def main(args: Namespace):
    can: str = args.can
    reset: bool = args.reset

    addr = isotp.Address(isotp.AddressingMode.Normal_11bits, rxid=0x7E0, txid=0x7E8)
    conn = IsoTPSocketConnection(can, addr)

    config = dict(udsoncan.configs.default_client_config)
    config["data_identifiers"] = {
        "default": ">H",  # Default codec is a struct.pack/unpack string. 16bits little endian
        0x0050: MyCustomCodec,
        0x0100: StringCodec,
    }

    with Client(conn, config=config, request_timeout=2) as client:
        try_run(lambda: change_session(client))
        try_run(lambda: read_write_data_by_identifier(client))

        if reset:
            try_run(lambda: ecu_reset(client))


if __name__ == "__main__":
    parser = ArgumentParser(description="UDS ISO 14229 Demo Script")
    parser.add_argument(
        "-c", "--can", default="vcan0", help="CAN interface (default: vcan0)"
    )
    parser.add_argument("-r", "--reset", action="store_true", help="Perform ECU reset")
    parsed_args = parser.parse_args()

    main(parsed_args)
