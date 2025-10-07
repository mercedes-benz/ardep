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
from typing import Optional
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

    # print_indented("Changing to extended diagnostic session")
    # client.change_session(DiagnosticSessionControl.Session.extendedDiagnosticSession)
    # print_indented("Session change successful")

    # print_indented("Changing to programming session")
    # client.change_session(DiagnosticSessionControl.Session.programmingSession)
    # print_indented("Session change successful")


def ecu_reset(client: Client):
    # Send ECU reset request (hard reset)
    print("Sending ECU hard reset request...")
    client.ecu_reset(ECUReset.ResetType.hardReset)
    print("\tECU reset request sending successfully")


def erase_slot0_memory_routine(client: Client):
    print_headline("Erasing Slot0 Memory")
    print_indented("Starting memory erase routine...")
    # Assuming the ECU supports the RoutineControl service for memory erase
    with client.suppress_positive_response():
        client.routine_control(routine_id=0xFF00, control_type=1)

        print_indented("Waiting for erasure to be done...")

        for _ in range(5):
            time.sleep(1)
            client.tester_present()

    print_indented("Requesting erasure results...")
    # Assuming the ECU supports the RoutineControl service for memory erase
    try:
        response = client.routine_control(routine_id=0xFF00, control_type=3)
        result = struct.unpack(">I", response.service_data.routine_status_record)[0]
        if result != 0:
            print_indented(f"Erasure failed with code: {result}")
            raise RuntimeError("Memory erasure failed")
        else:
            print_indented(f"Erasure results: {result}")
    except udsoncan.exceptions.TimeoutException:
        print_indented("Ignoring timeout exceptions, as erase is working correctly")

    # Add a small delay to ensure any delayed responses are cleared
    print_indented("Waiting for communication to settle...")
    time.sleep(0.5)


def firmware_download(client: Client, firmware_path: str):
    for _ in range(2):
        client.tester_present()
        time.sleep(1)

    print_headline("Starting Firmware Download")
    with open(firmware_path, "rb") as firmware_file:
        firmware_data = firmware_file.read()

    # STM32G4 flash base address + slot0 offset
    slot0_address = 0x08000000 + 0x18000
    slot0_size = 192 * 1024  # 192KiB

    slot0_memory = udsoncan.MemoryLocation(
        memorysize=slot0_size, address=slot0_address, address_format=32
    )

    if len(firmware_data) > slot0_size:
        raise RuntimeError(
            f"Firmware size {len(firmware_data)} exceeds slot0 size {slot0_size}"
        )

    print_indented("Requesting download...")
    client.request_download(
        memory_location=slot0_memory,
        # dfi=udsoncan.DataFormatIdentifier.from_byte(0x00),
    )

    for _ in range(2):
        client.tester_present()
        time.sleep(1)

    block_size = 1 * 1024
    blocks = [
        firmware_data[i : i + block_size]
        for i in range(0, len(firmware_data), block_size)
    ]

    for idx, block in enumerate(blocks):
        print_indented(f"Transferring block {idx + 1}/{len(blocks)}...")
        client.transfer_data(idx + 1, block)
        time.sleep(1)

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
    firmware: Optional[str] = args.firmware

    addr = isotp.Address(isotp.AddressingMode.Normal_11bits, rxid=0x7E0, txid=0x7E8)
    conn = IsoTPSocketConnection(can, addr)

    config = dict(udsoncan.configs.default_client_config)

    with Client(conn, config=config, request_timeout=2) as client:
        try_run(lambda: change_session(client))

        if firmware is not None:
            try_run(lambda: erase_slot0_memory_routine(client))
            try_run(lambda: firmware_download(client, firmware))

        if reset:
            try_run(lambda: ecu_reset(client))

        print("\n=== Demo finished ===\n")


if __name__ == "__main__":
    parser = ArgumentParser(description="Firmware Loader Demo Client")
    parser.add_argument(
        "-c", "--can", default="vcan0", help="CAN interface (default: vcan0)"
    )
    parser.add_argument("-r", "--reset", action="store_true", help="Perform ECU reset")

    parser.add_argument(
        "-f", "--firmware", default=None, type=str, help="Path to the firmware.bin file"
    )
    parsed_args = parser.parse_args()

    main(parsed_args)
