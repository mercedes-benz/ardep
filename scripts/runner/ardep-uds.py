# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

from runners.core import RunnerCaps, ZephyrBinaryRunner  # pylint: disable=import-error
import time


import udsoncan
from udsoncan.client import Client
from udsoncan.services import DiagnosticSessionControl, ECUReset
import isotp
from intelhex import IntelHex
import struct

class ArdepUDSRunner(ZephyrBinaryRunner):
    """Runner for ardep board using UDS for flashing"""

    can_interface: str
    uds_source_address: str
    uds_target_address: str
    gearshift: int | None
    block_size: int
    hex_file: IntelHex | None

    def __init__(self, cfg, can_interface, uds_source_address, uds_target_address, gearshift, block_size):
        super().__init__(cfg)
        self.hex_file = IntelHex(cfg.hex_file) if cfg.hex_file else None
        self.can_interface = can_interface
        self.uds_source_address = uds_source_address
        self.uds_target_address = uds_target_address
        self.gearshift = gearshift
        self.block_size = block_size

    @classmethod
    def name(cls):
        return "ardep-uds"

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands=({"flash"}))

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            "--iface",
            help="CAN interface to use for UDS flashing",
            default="can0",
        )
        parser.add_argument(
            "--uds-source-id",
            help="UDS source address, set this to the target address of the ARDEP",
            default="0x7E0",
        )
        parser.add_argument(
            "--uds-target-id",
            help="UDS target address, set this to the source address of the ARDEP",
            default="0x7E8",
        )
        parser.add_argument(
            "-g",
            "--gearshift",
            help="Set this to an integer 0-7 to use the default gearshift addressing scheme instead of custom source/target addresses",
            type=int,
            choices=range(0, 8),
        )
        parser.add_argument(
            "-b",
            "--block-size",
            help="Size of individual UDS TransferData blocks payload in bytes (default: 512)",
            type=int,
            default=512,
        )

    @classmethod
    def do_create(cls, cfg, args):
        return cls(
            cfg,
            can_interface=args.can_interface,
            uds_source_address=args.uds_source_address,
            uds_target_address=args.uds_target_address,
            gearshift=args.gearshift,
            block_size=args.block_size,
        )

    def read_and_split_firmware_into_blocks(self):
        base_address = self.hex_file.addresses()[0]
        firmware_data = bytes(self.hex_file.tobinarray(base_address))

        blocks = [
            firmware_data[i : i + self.block_size]
            for i in range(0, len(firmware_data), self.block_size)
        ]

        return blocks, base_address

    def get_isotp_address(self) -> isotp.Address:
        source_id = 0x7E0 + self.gearshift if self.gearshift is not None else int(self.uds_source_address, 0)
        target_id = 0x7E8 + self.gearshift if self.gearshift is not None else int(self.uds_target_address, 0)
        return isotp.Address(isotp.AddressingMode.Normal_11bits, rxid=source_id, txid=target_id)

    def create_client(self) -> Client:
        addr = self.get_isotp_address()
        conn = udsoncan.connections.IsoTPSocketConnection(self.can_interface, addr)
        config = dict(udsoncan.configs.default_client_config)
        return Client(conn, config=config, request_timeout=2)

    def test_connection(self, client: Client):
        print("Testing communication with ECU...")
        client.tester_present()

    def switch_to_programming_session(self, client: Client):
        print("Switching to programming session...")
        client.change_session(DiagnosticSessionControl.Session.programmingSession)

        print("Waiting for the device to come back online...")
        for _ in range(10):
            time.sleep(1)
            try:
                client.tester_present()
                break
            except udsoncan.exceptions.TimeoutException:
                pass

    def erase_slot0(self, client: Client):
        print("Erasing slot0 ...")
        client.start_routine(0xFF00) # Erase slot0 routine

        time.sleep(3)

        for _ in range(5):
            try:
                client.tester_present()
                break
            except udsoncan.exceptions.TimeoutException:
                pass
            time.sleep(1)

        response = client.get_routine_result(0xFF00)
        result = struct.unpack(">I", response.service_data.routine_status_record)[0]
        if result != 0:
            raise RuntimeError(f"Erase slot0 routine failed with code 0x{result:08X}")

        print("Slot0 erased successfully.")

    def upload_firmware(self, client: Client, blocks, base_address):
        print("Starting firmware transfer...")

        block_count = len(blocks)

        print(f"Requesting download of {block_count} blocks starting at address 0x{base_address:08X}...")
        address = udsoncan.MemoryLocation(memorysize=block_count * self.block_size, address=base_address, address_format=32)
        client.request_download(memory_location=address)

        for i in range(block_count):
            print(f"Transferring block {i + 1}/{block_count}...")
            client.transfer_data((i + 1) % 256, blocks.pop(0))
            client.tester_present()

            time.sleep(0.05)

        client.request_transfer_exit()

    def do_run(self, command, **kwargs):  # pylint: disable=unused-argument
        if self.hex_file == None:
            print("No hex file provided, please check that you are building a hex file")
            exit(1)

        blocks, base_address = self.read_and_split_firmware_into_blocks()

        with self.create_client() as client:
            self.test_connection(client)
            self.switch_to_programming_session(client)

            self.erase_slot0(client)
            self.upload_firmware(client, blocks, base_address)

            client.ecu_reset(ECUReset.ResetType.hardReset)

            print("Firmware transfer complete")
