# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0


from argparse import ArgumentParser, Namespace

from random import shuffle
import struct
import time
from typing import Optional, List
import isotp
import udsoncan
from udsoncan.client import Client
from udsoncan.connections import IsoTPSocketConnection
from udsoncan.exceptions import (
    NegativeResponseException,
)
from udsoncan.services import DiagnosticSessionControl, ECUReset
import subprocess


def print_headline(text: str):
    print(f"=== {text} ===")


def print_indented(text: str, indent: int = 1):
    print("\t" * indent + text)


def change_session(client: Client):
    print_headline("Changing Session")

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
    with client.suppress_positive_response():
        client.routine_control(routine_id=0xFF00, control_type=1)

    print_indented("Waiting for erasure to be done...")

    for _ in range(5):
        time.sleep(1)
        try:
            client.tester_present()
            break
        except udsoncan.exceptions.TimeoutException:
            pass

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
    )

    block_size = 1024
    blocks = [
        firmware_data[i : i + block_size]
        for i in range(0, len(firmware_data), block_size)
    ]

    for idx, block in enumerate(blocks):
        print_indented(f"Transferring block {idx + 1}/{len(blocks)}...")

        # Reset the S3 timer by sending a TesterPresent every 5 blocks
        if idx > 0 and idx % 5 == 0:
            try:
                print_indented("Resetting timeout...", indent=2)
                client.tester_present()
            except Exception as e:
                print_indented(f"Device connectivity check failed: {e}", indent=2)
                raise RuntimeError(
                    "Device appears to have reset or disconnected"
                ) from e

        try:
            client.transfer_data(idx + 1, block)
        except udsoncan.exceptions.TimeoutException:
            print_indented(
                f"Timeout on block {idx + 1}. Device may have reset.", indent=2
            )
            print_indented(
                "Check server logs for reboot messages after flash write.", indent=2
            )
            raise

        # Small delay to allow the device to process
        time.sleep(0.05)

    # Requesting transfer exit
    print_indented("Requesting transfer exit...")
    client.request_transfer_exit()
    print_indented("Firmware download completed successfully")

def create_and_test_addresses(intf: str) -> List[isotp.Address]:
    addresses = []
    i = 0

    while i < 8:
        addr = isotp.Address(isotp.AddressingMode.Normal_11bits, rxid=0x7E0 + i, txid=0x7E8 + i)
        conn = IsoTPSocketConnection(intf, addr)

        config = dict(udsoncan.configs.default_client_config)


        try:
            with Client(conn, config=config, request_timeout=0.5) as client:
                client.tester_present()
                print(f"Connection {i} successful (RXID: 0x{addr._rxid:X}, TXID: 0x{addr._txid:X})")
        except Exception:
            print(f"Connection {i} failed (RXID: 0x{addr._rxid:X}, TXID: 0x{addr._txid:X})")
            break

        addresses.append(addr)
        i += 1

    return addresses


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
    pristine: bool = args.pristine
    upgrade: bool = args.upgrade

    if upgrade:
        print("Building client firmwares")
        for i in range(int(args.count)):
            print("Building firmware for client", i)
            cmd = ["west","build", "-d","builds/fw"+str(i)]
            if pristine:
                cmd += ["-p","auto","samples/uds_bus_sample","-b", args.board, "--sysbuild","--","-DSB_CONFIG_UDS_BASE_ADDR="+str(i)]
            print("Running command:", " ".join(cmd))
            a = subprocess.run(cmd, check=True)

    addresses = create_and_test_addresses(can)
    print(f"Discovered {len(addresses)} clients")

    if upgrade and len(addresses) > int(args.count):
        print(f"Error: Cannot upgrade all clients, discovered {len(addresses)} but only built for {args.count}")
        return

    config = dict(udsoncan.configs.default_client_config)
    config["data_identifiers"] = {
        0x1100: "<H",
        0x1101: "<H",
    }

    if upgrade:
        print("Upgrading clients")

        for addr in addresses:
            conn = IsoTPSocketConnection(can, addr)
            with Client(conn, config=config, request_timeout=2) as client:
                print(f"\n--- Upgrading client on RXID: 0x{addr._rxid:X}, TXID: 0x{addr._txid:X} ---")
                try_run(lambda: change_session(client))

                try_run(lambda: erase_slot0_memory_routine(client))
                try_run(lambda: firmware_download(client, "builds/fw"+str((addr._rxid - 0x7E0))+"/uds_bus_sample/zephyr/zephyr.signed.bin"))

                try_run(lambda: ecu_reset(client))
        print("\nAll clients upgraded")
        time.sleep(1)

    shuffle(addresses)

    print("Configuring client chain")

    first_reponder_receive_address = 0x001
    last_responder_send_address = 0x000

    client_cnt  = len(addresses)
    for i in range(1, client_cnt): # skip first client, this will be our master
        addr = addresses[i]
        conn = IsoTPSocketConnection(can, addr)
        with Client(conn, config=config, request_timeout=2) as client:
            # setup receive addresses
            if i == 1:
                client.write_data_by_identifier(0x1100, first_reponder_receive_address)
            else:
                client.write_data_by_identifier(0x1100, i + 1)

            # setup send addresses
            if i == client_cnt - 1:
                client.write_data_by_identifier(0x1101, last_responder_send_address)
            else:
                client.write_data_by_identifier(0x1101, i + 2)

    print("Client chain configured successfully")

    received_final_frame = None
    addr = addresses[0]
    conn = IsoTPSocketConnection(can, addr)
    with Client(conn, config=config, request_timeout=2) as client:
        # start master routine
        client.start_routine(routine_id=0x0000)

        # poll for completion and get final frame
        for i in range(5):
            a = client.get_routine_result(routine_id=0x0000)
            completed = False
            match a.service_data.routine_status_record[0]:
                case 0xff:
                    print("Routine still running...")
                case 0x00:
                    print("Routine completed successfully")
                    received_final_frame = a.service_data.routine_status_record[1:]
                    completed = True
                case other:
                    print(f"Routine failed with code: 0x{other:02X}")
                    completed = True

            if completed:
                break

            time.sleep(0.25)

    if received_final_frame is None:
        print("Did not receive final frame from master routine")
        return

    to_sign = received_final_frame[0]
    for i in range(1, len(addresses)): # master does not sign
        if received_final_frame[i] != (to_sign ^ (addresses[i]._txid & 0xff)):
            print(f"Signature mismatch on client {i}: expected 0x{to_sign ^ (i | 0xe0):02X}, got 0x{received_final_frame[i]:02X}")
            return
    print("All signatures verified successfully")


if __name__ == "__main__":
    parser = ArgumentParser(description="Firmware Loader Demo Client")
    parser.add_argument(
        "-c", "--can", default="vcan0", help="CAN interface (default: vcan0)"
    )
    parser.add_argument("-r", "--reset", action="store_true", help="Perform ECU reset")
    parser.add_argument("-u", "--upgrade",action="store_true", help="Upgrade clients")
    parser.add_argument("-p", "--pristine", action="store_true", help="Build pristine. Only used when --upgrade is set")
    parser.add_argument("-b", "--board", help="Board name, e.g. ardep@1 or ardep. Only used when --upgrade is set", default="ardep")
    parser.add_argument("-n", "--count", help="Number of clients to build. This is only used for building, not for discovery of UDS clients")

    parsed_args = parser.parse_args()

    main(parsed_args)
