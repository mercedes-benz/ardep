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

addr = isotp.Address(
    isotp.AddressingMode.Normal_11bits, rxid=2016, txid=2024
)
conn = IsoTPSocketConnection("vcan0", addr)


def do_something(client: Client):
    client.change_session(
        DiagnosticSessionControl.Session.programmingSession
    )
    client.write_memory_by_address(udsoncan.MemoryLocation(address=0x12, memorysize=8, address_format=32, memorysize_format=32), b'\x12\x34\x56\x78\x9A\xBC\xDE\xF0')
    client.read_memory_by_address(udsoncan.MemoryLocation(address=0x01, memorysize=0x34, address_format=32, memorysize_format=32))

    client.routine_control(
        1337, RoutineControl.ControlType.startRoutine
    )
    client.request_download(
        memory_location=udsoncan.MemoryLocation(
            address=0x12, memorysize=0x34, address_format=32, memorysize_format=32
        ))
    
    #! important: iso14229 uses 1 as the first sequence number: https://github.com/driftregion/iso14229/blob/4a9394163d817b6ec55c3ad5c8e937bd51efeadf/src/server.c#L441
    client.transfer_data(
        sequence_number=1, data=b'\x12\x34\x56\x78')
    client.transfer_data(
        sequence_number=2, data=b'\x01\x02\x03\x04')
    
    client.request_transfer_exit()
    client.change_session(
        DiagnosticSessionControl.Session.defaultSession)

with Client(conn, request_timeout=2) as client:
    try:
        do_something(client)

    except NegativeResponseException as e:
        print(
            'Server refused our request for service %s with code "%s" (0x%02x)'
            % (
                e.response.service.get_name(),
                e.response.code_name,
                e.response.code,
            )
        )
    except (InvalidResponseException, UnexpectedResponseException) as e:
        print(
            "Server sent an invalid payload : %s" % e.response.original_payload
        )
