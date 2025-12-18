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

# Create addresses for all gearshift positions and test connectivity, returning only those that responded
def create_and_test_gearshift_addresses(intf: str) -> List[isotp.Address]:
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
    block_size: int = args.flash_blocksize

    if upgrade:
        print("Building client firmware")
        cmd = ["west","build"]
        if pristine:
            cmd += ["-p", "auto", "samples/uds_bus_sample", "-b", args.board]
        print("Running command:", " ".join(cmd))
        subprocess.run(cmd, check=True)

    addresses = create_and_test_gearshift_addresses(can)
    print(f"Discovered {len(addresses)} clients")

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
                cmd = ["west", "flash", "--runner", "ardep-uds", "--", "--iface", can, "--uds-source-id", f"0x{addr._rxid:X}", "--uds-target-id", f"0x{addr._txid:X}", "-b",str(block_size)]
                print("Running command:", " ".join(cmd))
                subprocess.run(cmd, check=True)
        print("\nAll clients upgraded")
        time.sleep(1)

    shuffle(addresses)

    print("Configuring client chain")

    first_reponder_receive_address = 0x001
    last_responder_send_address = 0x000

    client_cnt  = len(addresses)
    for i in range(1, client_cnt): # skip first client, this will be our controller
        addr = addresses[i]
        conn = IsoTPSocketConnection(can, addr)
        with Client(conn, config=config, request_timeout=2) as client:
            receive_address = i + 1
            send_address = i + 2

            # setup receive addresses
            if i == 1:
                receive_address = first_reponder_receive_address
            client.write_data_by_identifier(0x1100, receive_address)

            # setup send addresses
            if i == client_cnt - 1:
                send_address = last_responder_send_address
            client.write_data_by_identifier(0x1101, send_address)

    print("Client chain configured successfully")

    received_final_frame = None
    addr = addresses[0]
    conn = IsoTPSocketConnection(can, addr)
    with Client(conn, config=config, request_timeout=2) as client:
        # start controller routine
        print("Starting controller routine...")
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
        print("Did not receive final frame from controller routine")
        return

    to_sign = received_final_frame[0]
    for i in range(1, len(addresses)): # 1 because the controller does not sign
        received = received_final_frame[i]
        expected = to_sign ^ (addresses[i]._txid & 0xff)
        if received != expected:
            print(f"Signature mismatch on client {i}: expected 0x{expected:02X}, got 0x{received:02X}")
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
    parser.add_argument("-fb","--flash-blocksize", type=int, default=512, help="UDS Block size for flash transfer (default: 512 bytes)")

    parsed_args = parser.parse_args()

    main(parsed_args)
