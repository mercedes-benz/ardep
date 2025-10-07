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

import struct
import time
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


def erase_slot0_memory_routine(client: Client):
    print_headline("Erasing Slot0 Memory")
    print_indented("Starting memory erase routine...")
    # Assuming the ECU supports the RoutineControl service for memory erase
    client.routine_control(routine_id=0xFF00, control_type=1)

    print_indented("Waiting for erasure to be done...")
    time.sleep(1)

    print_indented("Requesting erasure results...")
    # Assuming the ECU supports the RoutineControl service for memory erase
    response = client.routine_control(routine_id=0xFF00, control_type=3)
    result = struct.unpack(">I", response.service_data.routine_status_record)[0]
    if result != 0:
        print_indented(f"Erasure failed with code: {result}")
        raise RuntimeError("Memory erasure failed")
    else:
        print_indented(f"Erasure results: {result}")


def firmware_download(client: Client, firmware_path: str):
    print_headline("Starting Firmware Download")
    with open(firmware_path, "rb") as firmware_file:
        firmware_data = firmware_file.read()

    # Assuming the ECU supports the Download service
    print_indented("Requesting download...")
    response = client.download(0x00000000, len(firmware_data))
    print_indented(
        f"Download request accepted, max block size: {response.max_number_of_bytes}"
    )

    # Sending data in chunks
    block_size = response.max_number_of_bytes
    for offset in range(0, len(firmware_data), block_size):
        chunk = firmware_data[offset : offset + block_size]
        print_indented(f"Sending block at offset {offset:#08x}...")
        client.transfer_data(chunk)
        print_indented("Block sent successfully")

    # Requesting transfer exit
    print_indented("Requesting transfer exit...")
    client.request_transfer_exit()
    print_indented("Firmware download completed successfully")


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
