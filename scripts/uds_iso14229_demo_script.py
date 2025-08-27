import struct
from udsoncan.client import Client
from udsoncan.connections import IsoTPSocketConnection
from udsoncan.exceptions import (
    NegativeResponseException,
    InvalidResponseException,
    UnexpectedResponseException,
)
from udsoncan.services import DiagnosticSessionControl, RoutineControl, ECUReset
from argparse import ArgumentParser, Namespace

import isotp
import udsoncan


def write_read_memory(client: Client):
    client.change_session(DiagnosticSessionControl.Session.programmingSession)
    client.write_memory_by_address(
        udsoncan.MemoryLocation(
            address=0x12, memorysize=8, address_format=32, memorysize_format=32
        ),
        b"\x12\x34\x56\x78\x9a\xbc\xde\xf0",
    )

    addr = 2
    print(f"Reading memory address 0x{addr:02x} with size 0x34:")
    resp = client.read_memory_by_address(
        udsoncan.MemoryLocation(
            address=addr, memorysize=0x34, address_format=32, memorysize_format=32
        )
    )
    print(f"read response: {resp}")
    print(f"  data: {' '.join(f'{b:02x}' for b in resp.data)}")

    client.routine_control(1337, RoutineControl.ControlType.startRoutine)
    client.request_download(
        memory_location=udsoncan.MemoryLocation(
            address=0x12, memorysize=0x34, address_format=32, memorysize_format=32
        )
    )

    # ! important: iso14229 uses 1 as the first sequence number: https://github.com/driftregion/iso14229/blob/4a9394163d817b6ec55c3ad5c8e937bd51efeadf/src/server.c#L441
    client.transfer_data(sequence_number=1, data=b"\x12\x34\x56\x78")
    client.transfer_data(sequence_number=2, data=b"\x01\x02\x03\x04")

    client.request_transfer_exit()
    client.change_session(DiagnosticSessionControl.Session.defaultSession)


def read_data_by_identifier(client: Client):
    data = client.read_data_by_identifier([0x1234])
    print(f"Reading data from identifier 0x1234: {data.service_data.values[0x1234]}")


def ecu_reset(client: Client):
    # Send ECU reset request (hard reset)
    print("Sending ECU hard reset request...")
    client.ecu_reset(ECUReset.ResetType.hardReset)
    print("ECU reset request sent successfully")


class MyCustomCodec(udsoncan.DidCodec):
    def encode(self, val):
        return struct.pack(">H", val)  # Big endian, 16 bit value

    def decode(self, payload):
        return struct.unpack(">H", payload)[0]  # decode the 16 bits value

    def __len__(self):
        return 2  # encoded payload is 2 byte long.


def main(args: Namespace):
    can = args.can

    addr = isotp.Address(isotp.AddressingMode.Normal_11bits, rxid=0x7E0, txid=0x7E8)
    conn = IsoTPSocketConnection(can, addr)

    config = dict(udsoncan.configs.default_client_config)
    config["data_identifiers"] = {
        "default": ">H",  # Default codec is a struct.pack/unpack string. 16bits little endian
        0x1234: MyCustomCodec,
    }

    with Client(conn, config=config, request_timeout=2) as client:
        # try:
        #     write_read_memory(client)

        # except NegativeResponseException as e:
        #     print(
        #         f"Server refused our request for service {e.response.service.get_name()} "
        #         f'with code "{e.response.code_name}" (0x{e.response.code:02x})'
        #     )
        # except (InvalidResponseException, UnexpectedResponseException) as e:
        #     print(f"Server sent an invalid payload : {e.response.original_payload}")

        # try:
        #     ecu_reset(client)

        # except NegativeResponseException as e:
        #     print(
        #         f"Server refused our request for service {e.response.service.get_name()} "
        #         f'with code "{e.response.code_name}" (0x{e.response.code:02x})'
        #     )

        try:
            read_data_by_identifier(client)

        except NegativeResponseException as e:
            print(
                f"Server refused our request for service {e.response.service.get_name()} "
                f'with code "{e.response.code_name}" (0x{e.response.code:02x})'
            )


if __name__ == "__main__":
    parser = ArgumentParser(description="UDS ISO 14229 Demo Script")
    parser.add_argument(
        "-c", "--can", default="vcan0", help="CAN interface (default: vcan0)"
    )
    args = parser.parse_args()

    main(args)
