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
import time
from typing import Any
from argparse import ArgumentParser, Namespace

import isotp
import udsoncan
from udsoncan import Baudrate
from udsoncan.client import Client, MemoryLocation
from udsoncan.connections import IsoTPSocketConnection
from udsoncan.exceptions import (
    NegativeResponseException,
)
from udsoncan.services import DiagnosticSessionControl, ECUReset
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
import pathlib, binascii


def change_session(client: Client):
    print("Changing to programming session...")
    client.change_session(DiagnosticSessionControl.Session.programmingSession)
    print("\tSession change successful")


def data_by_identifier(client: Client):
    print("Data By Identifier:")

    data_id: int = 0x0100
    data = client.read_data_by_identifier([data_id])
    print(
        f'\tReading data from identifier\t0x{data_id:04X}:\t"{data.service_data.values[data_id]}"'
    )

    data_id = 0x0050
    data = client.read_data_by_identifier_first([data_id])
    print(f"\tReading data from identifier\t0x{data_id:04X}:\t0x{data:04X}")

    write_data = 0x1234
    data = client.write_data_by_identifier(data_id, write_data)
    print(f"\tWriting data to identifier\t0x{data_id:04X}:\t0x{write_data:04X}")

    data = client.read_data_by_identifier_first([data_id])
    print(f"\tReading data from identifier\t0x{data_id:04X}:\t0x{data:04X}")

    write_data = 0xBEEF
    data = client.write_data_by_identifier(data_id, write_data)
    print(f"\tWriting data to identifier\t0x{data_id:04X}:\t0x{write_data:04X}")

    data = client.read_data_by_identifier_first([data_id])
    print(f"\tReading data from identifier\t0x{data_id:04X}:\t0x{data:04X}")

    led_id = 0x0200
    data = client.read_data_by_identifier_first([led_id])
    print(f"\tReading LED state:\t\t\t0x{data:02X}")

    write_data = 0x01
    data = client.write_data_by_identifier(led_id, write_data)
    print(f"\tTurning LED ON, writing:\t\t0x{write_data:02X}")

    time.sleep(1)

    io_control_param = 0x03  # short term adjustment
    io_control_data = 0x00
    client.io_control(led_id, control_param=io_control_param, values=[io_control_data])
    print(
        f"\tTemporarily overriding LED state via IO control. Setting LED state: 0x{io_control_data:02X}"
    )

    time.sleep(1)

    io_control_data = 0x01
    client.io_control(led_id, control_param=io_control_param, values=[io_control_data])
    print(
        f"\tTemporarily overriding LED state via IO control. Setting LED state: 0x{io_control_data:02X}"
    )

    time.sleep(1)

    io_control_param = 0x01  # reset to Default
    client.io_control(led_id, control_param=io_control_param)
    print("\tResetting LED state to default (OFF)")

    time.sleep(1)

    io_control_param = 0x00  # return control to ECU
    client.io_control(led_id, control_param=io_control_param)
    print("\tReturning control of LED back to the ECU")

    time.sleep(1)

    write_data = 0x01
    data = client.write_data_by_identifier(led_id, write_data)
    print(f"\tTurning LED ON, writing:\t\t0x{write_data:02X}")

    time.sleep(1)

    write_data = 0x00
    data = client.write_data_by_identifier(led_id, write_data)
    print(f"\tTurning LED OFF, writing:\t\t0x{write_data:02X}")
    time.sleep(1)


def read_write_memory_by_address(client: Client):
    print("Memory By Address:")

    address: int = 0x00001000
    memory_location = MemoryLocation(address=address, memorysize=16)

    data = client.read_memory_by_address(memory_location)
    print(f"\tReading 16 bytes:\t\t\t{' '.join(f'{b:02X}' for b in data.data)}")
    read_data = data.data

    write_data = bytes(
        [
            0x11,
            0x12,
            0x13,
            0x14,
            0x15,
            0x16,
            0x17,
            0x18,
            0x19,
            0x1A,
            0x1B,
            0x1C,
            0x1D,
            0x1E,
            0x1F,
            0x20,
        ]
    )
    data = client.write_memory_by_address(
        memory_location,
        write_data,
    )
    print(f"\tWriting 16 bytes:\t\t\t{' '.join(f'{b:02X}' for b in write_data)}")

    data = client.read_memory_by_address(memory_location)
    print(f"\tReading updated 16 bytes:\t\t{' '.join(f'{b:02X}' for b in data.data)}")

    data = client.write_memory_by_address(
        memory_location,
        read_data,
    )
    print(
        f"\tRestoring original 16 bytes:\t\t{' '.join(f'{b:02X}' for b in read_data)}"
    )

    data = client.read_memory_by_address(memory_location)
    print(f"\tReading restored 16 bytes:\t\t{' '.join(f'{b:02X}' for b in data.data)}")


def dtc_information(client: Client):
    print("Reading DTC information...")

    print("\tRequesting DTC Snapshot Identification (unimplemented subFunc)...")
    try:
        client.read_dtc_information(
            subfunction=0x03,
        )
    except NegativeResponseException as e:
        print(
            f'\t\tServer refused our request with code "{e.response.code_name}" (0x{e.response.code:02X})'
        )

    print("\tRequesting DTCs with status mask 0x84...")
    dtc_data = client.read_dtc_information(
        subfunction=0x02,
        status_mask=0x84,
    )
    print(f"\t\tDTC count: {dtc_data.service_data.dtc_count}")
    for dtc in dtc_data.service_data.dtcs:
        print(
            f"\t\t\tDTC: 0x{dtc.id:06X} with status 0x{dtc.status.get_byte_as_int():02X}"
        )

    print("\tClearing DTC information...")
    client.clear_dtc(group=0xFFFFFF)

    print("\tRequesting DTCs with status mask 0x84 again...")
    dtc_data = client.read_dtc_information(
        subfunction=0x02,
        status_mask=0x84,
    )
    print(f"\t\tDTC count: {dtc_data.service_data.dtc_count}")
    for dtc in dtc_data.service_data.dtcs:
        print(
            f"\t\t\tDTC: 0x{dtc.id:06X} with status 0x{dtc.status.get_byte_as_int():02X}"
        )

    time.sleep(1.5)

    print("\tRequesting DTCs with status mask 0x84 after delay...")
    dtc_data = client.read_dtc_information(
        subfunction=0x02,
        status_mask=0x84,
    )
    print(f"\t\tDTC count: {dtc_data.service_data.dtc_count}")
    for dtc in dtc_data.service_data.dtcs:
        print(
            f"\t\t\tDTC: 0x{dtc.id:06X} with status 0x{dtc.status.get_byte_as_int():02X}"
        )

    # Control DTC Setting demonstration
    print("\n\tControl DTC Setting demonstration:")

    # First read current DTCs to see initial status
    print("\t\tReading initial DTC status...")
    try:
        dtc_data = client.read_dtc_information(
            subfunction=0x02,  # DTC by status mask
            status_mask=0xFF,  # All status bits
        )
        print(f"\t\t\tInitial DTC count: {dtc_data.service_data.dtc_count}")
        for dtc in dtc_data.service_data.dtcs:
            print(
                f"\t\t\tDTC: 0x{dtc.id:06X} with status 0x{dtc.status.get_byte_as_int():02X}"
            )
    except NegativeResponseException as e:
        print(
            f"\t\t\tFailed to read DTCs: {e.response.code_name} (0x{e.response.code:02X})"
        )

    # Wait a bit to see DTC status changes
    print("\t\tWaiting 200ms to observe DTC status changes...")
    time.sleep(0.2)

    # Read DTCs again to see status increments
    try:
        dtc_data = client.read_dtc_information(
            subfunction=0x02,
            status_mask=0xFF,
        )
        print(f"\t\t\tDTC count after 200ms: {dtc_data.service_data.dtc_count}")
        for dtc in dtc_data.service_data.dtcs:
            print(
                f"\t\t\t\tDTC: 0x{dtc.id:06X} with status 0x{dtc.status.get_byte_as_int():02X}"
            )
    except NegativeResponseException as e:
        print(
            f"\t\t\tFailed to read DTCs: {e.response.code_name} (0x{e.response.code:02X})"
        )

    # Now freeze DTC updates using Control DTC Setting OFF
    print("\t\tSending DTC Setting OFF (0x02) - freezing DTC status updates...")
    try:
        response = client.control_dtc_setting(0x02)  # (DTCSettingOff)

        if response.positive:
            print("\t\t\tDTC Setting OFF successful - DTC updates are now frozen")
        else:
            print(
                f"\t\t\tDTC Setting OFF failed: {response.code_name} (0x{response.code:02X})"
            )
    except NegativeResponseException as e:
        print(f"\t\t\tError sending DTC Setting OFF: {e}")

    try:
        dtc_data = client.read_dtc_information(
            subfunction=0x02,
            status_mask=0xFF,
        )
        print(
            f"\t\t\tDTC count directly after frozen: {dtc_data.service_data.dtc_count}"
        )
        for dtc in dtc_data.service_data.dtcs:
            print(
                f"\t\t\t\tDTC: 0x{dtc.id:06X} with status 0x{dtc.status.get_byte_as_int():02X}"
            )
    except NegativeResponseException as e:
        print(
            f"\t\t\tFailed to read DTCs: {e.response.code_name} (0x{e.response.code:02X})"
        )

    # Wait and verify DTCs don't change
    print("\t\tWaiting 200ms to verify DTC status is frozen...")
    time.sleep(0.2)

    try:
        dtc_data = client.read_dtc_information(
            subfunction=0x02,
            status_mask=0xFF,
        )
        print(f"\t\t\tDTC count while frozen: {dtc_data.service_data.dtc_count}")
        for dtc in dtc_data.service_data.dtcs:
            print(
                f"\t\t\t\tDTC: 0x{dtc.id:06X} with status 0x{dtc.status.get_byte_as_int():02X}"
            )
    except NegativeResponseException as e:
        print(
            f"\t\t\tFailed to read DTCs: {e.response.code_name} (0x{e.response.code:02X})"
        )

    # Resume DTC updates using Control DTC Setting ON
    print("\t\tSending DTC Setting ON (0x01) - resuming DTC status updates...")
    try:
        response = client.control_dtc_setting(0x01)  # DTCSettingOn
        if response.positive:
            print("\t\t\tDTC Setting ON successful - DTC updates are now resumed")
        else:
            print(
                f"\t\t\tDTC Setting ON failed: {response.code_name} (0x{response.code:02X})"
            )
    except NegativeResponseException as e:
        print(f"\t\t\tError sending DTC Setting ON: {e}")

    # Wait and verify DTCs are updating again
    print("\t\tWaiting 200ms to verify DTC status updates have resumed...")
    time.sleep(0.2)

    try:
        dtc_data = client.read_dtc_information(
            subfunction=0x02,
            status_mask=0xFF,
        )
        print(f"\t\t\tDTC count after resume: {dtc_data.service_data.dtc_count}")
        for dtc in dtc_data.service_data.dtcs:
            print(
                f"\t\t\t\tDTC: 0x{dtc.id:06X} with status 0x{dtc.status.get_byte_as_int():02X}"
            )
    except NegativeResponseException as e:
        print(
            f"\t\t\tFailed to read DTCs: {e.response.code_name} (0x{e.response.code:02X})"
        )

    print("\t\tControl DTC Setting demonstration completed.")


def ecu_reset(client: Client):
    # Send ECU reset request (hard reset)
    print("Sending ECU hard reset request...")
    client.ecu_reset(ECUReset.ResetType.hardReset)
    print("\tECU reset request sending successfully")


def routine_control(client: Client):
    print("Routine Control:")

    routine_id = 0x1234
    input_value = ~0xDEADBEEF & 0xFFFFFFFF

    print(f"\tExecuting synchronous routine 0x{routine_id:04X}...")
    try:
        response = client.routine_control(
            routine_id=routine_id,
            control_type=1,  # Start routine
            data=struct.pack(">I", input_value),  # Input as big endian (4 bytes)
        )

        # The first byte is the status, the next 4 bytes are the BE uint32 result
        result = struct.unpack(">I", response.service_data.routine_status_record)[0]
        print(f"\t\tRoutine input : 0x{input_value:08X}")
        print(f"\t\tRoutine result: 0x{result:08X}")
        print("\t\tRoutine executed successfully")

    except NegativeResponseException as e:
        print(
            f"\t\tRoutine control failed: {e.response.code_name} (0x{e.response.code:02X})"
        )
        raise
    except Exception as e:
        print(f"\t\tUnexpected error during routine control: {e}")
        raise

    print(f"\tExecuting asynchronous routine 0x{routine_id:04X}...")

    routine_id = 0x5678
    try:
        print(f"\t\tStarting asynchronous routine 0x{routine_id:04X}...")
        client.routine_control(
            routine_id=routine_id,
            control_type=1,  # Start routine
        )
        print(f"\t\tAsynchronous routine 0x{routine_id:04X} started successfully\n")

        time.sleep(0.1)  # Wait a bit before requesting status

        print(f"\t\tStopping asynchronous routine 0x{routine_id:04X}...")
        client.routine_control(
            routine_id=routine_id,
            control_type=2,  # Stop routine
        )
        print(f"\t\tAsynchronous routine 0x{routine_id:04X} stopped successfully\n")

        print(f"\t\tRequesting results from asynchronous routine 0x{routine_id:04X}...")
        response = client.routine_control(
            routine_id=routine_id,
            control_type=3,  # Request routine results
        )

        if response.service_data.routine_status_record:
            status_data = response.service_data.routine_status_record
            if len(status_data) >= 5:
                status = status_data[0]
                progress = struct.unpack(">I", status_data[1:5])[0]
                print(f"\t\tRoutine status: {status}, Progress: {progress}")
            else:
                print(
                    f"\t\tRoutine status data: {' '.join(f'{b:02X}' for b in status_data)}"
                )

    except NegativeResponseException as e:
        print(
            f"\t\tRoutine control failed: {e.response.code_name} (0x{e.response.code:02X})"
        )
        raise
    except Exception as e:
        print(f"\t\tUnexpected error during routine control: {e}")
        raise


def security_access(client: Client):
    print("Security Access:")

    secure_data_id = 0x0300

    # First, try to read the secure data without authentication
    print(
        f"\tTrying to read secure data (ID: 0x{secure_data_id:04X}) without authentication..."
    )
    try:
        data = client.read_data_by_identifier([secure_data_id])
        print(f'\t\tUnexpected success: "{data.service_data.values[secure_data_id]}"')
    except NegativeResponseException as e:
        if e.response.code == 0x33:  # Security Access Denied
            print(
                f"\t\tAccess denied as expected: {e.response.code_name} (0x{e.response.code:02X})"
            )
        else:
            print(
                f"\t\tUnexpected error: {e.response.code_name} (0x{e.response.code:02X})"
            )

    # Now perform security access authentication
    print("\tPerforming security access authentication for level 1...")

    try:
        # Step 1: Request seed for security level 1
        print("\t\tRequesting seed for security level 1...")
        client.unlock_security_access(1)

        print("\t\tSecurity access successful! Level 1 unlocked.")

    except NegativeResponseException as e:
        print(
            f"\t\tSecurity access failed: {e.response.code_name} (0x{e.response.code:02X})"
        )
        return
    except Exception as e:
        print(f"\t\tUnexpected error during security access: {e}")
        return

    # Now try to read the secure data with authentication
    print(
        f"\tTrying to read secure data (ID: 0x{secure_data_id:04X}) with authentication..."
    )
    try:
        data = client.read_data_by_identifier([secure_data_id])
        secure_content = data.service_data.values[secure_data_id]
        print(f'\t\tSuccess! Secure data: "{secure_content}"')
    except NegativeResponseException as e:
        print(f"\t\tStill denied: {e.response.code_name} (0x{e.response.code:02X})")
    except Exception as e:
        print(f"\t\tUnexpected error: {e}")

    print("\tSecurity access demonstration completed.")


def security_algorithm(level: int, seed: bytes, params: Any) -> bytes:
    return bytes(~b & 0xFF for b in seed)


algo_indicator: bytes = bytes(
    [
        0x06,
        0x09,
        0x60,
        0x86,
        0x48,
        0x01,
        0x65,
        0x03,
        0x04,
        0x01,
        0x02,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    ]
)


def read_key(path: str) -> bytes:
    key = pathlib.Path(path).read_bytes()
    if len(key) != 16:
        raise ValueError(f"Expected 16 bytes, got {len(key)}")
    return key


def encrypt_seed(seed: bytes, key: bytes) -> bytes:
    cipher = Cipher(algorithms.AES(key), modes.ECB())
    encryptor = cipher.encryptor()
    ct = encryptor.update(seed) + encryptor.finalize()
    return ct


def authentication(client: Client, key_file: str):
    print("Authentication:")

    data_id: int = 0x0150
    print(
        f"\tTry reading secure data with id 0x{data_id:04X} without authentication..."
    )
    try:
        data = client.read_data_by_identifier([data_id])
        print(
            f'\t\tReading data from identifier\t0x{data_id:04X} should have failed but succeeded with data "{data.service_data.values[data_id]}". Are we still authenticated?'
        )
    except NegativeResponseException as e:
        if e.response.code == 0x22:  # Conditions Not Correct
            print(
                f"\t\tAccess denied as expected: {e.response.code_name} (0x{e.response.code:02X})"
            )
        else:
            raise

    response = client.authentication(
        authentication_task=5,
        communication_configuration=0,
        algorithm_indicator=algo_indicator,
    )

    seed: bytes = response.service_data.challenge_server

    print("\tSeed (hex):\t\t", seed.hex())
    key: bytes = read_key(key_file)

    ct = encrypt_seed(seed, key)
    print("\tCiphertext (hex):\t", ct.hex())

    client.authentication(
        authentication_task=6,
        proof_of_ownership_client=ct,
        algorithm_indicator=algo_indicator,
    )

    print("\tAuthentication successful!")

    print(f"\tTry reading secure data with id 0x{data_id:04X} with authentication...")
    data = client.read_data_by_identifier([data_id])
    print(
        f"\t\tReading data from secured identifier\t0x{data_id:04X}:\t0x{data.service_data.values[data_id]:02X}"
    )

    client.authentication(
        authentication_task=0,
    )

    print("\tDe-Authentication successful!")


class CustomUint16Codec(udsoncan.DidCodec):
    def encode(self, *did_value: Any):
        return struct.pack(">H", *did_value)  # Big endian, 16 bit value

    def decode(self, did_payload: bytes):
        return struct.unpack(">H", did_payload)[0]  # decode the 16 bits value

    def __len__(self):
        return 2  # encoded payload is 2 byte long.


class CustomUint8Codec(udsoncan.DidCodec):
    def encode(self, *did_value: Any):
        return struct.pack(">B", *did_value)  # Big endian, 8 bit value

    def decode(self, did_payload: bytes):
        return struct.unpack(">B", did_payload)[0]  # decode the 8 bits value

    def __len__(self):
        return 1  # encoded payload is 1 byte long.


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


class SecureDataCodec(udsoncan.DidCodec):
    def encode(self, *did_value: Any):
        value = did_value[0] if did_value else ""
        encoded = value.encode("ascii") if isinstance(value, str) else value
        return encoded

    def decode(self, did_payload: bytes):
        return did_payload.decode("ascii", errors="ignore").rstrip("\x00")

    def __len__(self):
        return 40  # "Secret Data Protected by Security Access" + null terminator


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
    aes_key_file = args.key_file

    addr = isotp.Address(isotp.AddressingMode.Normal_11bits, rxid=0x7E0, txid=0x7E8)
    conn = IsoTPSocketConnection(can, addr)

    config = dict(udsoncan.configs.default_client_config)
    config["data_identifiers"] = {
        "default": ">H",  # Default codec is a struct.pack/unpack string. 16bits little endian
        0x0050: CustomUint16Codec,
        0x0100: StringCodec,
        0x0150: CustomUint8Codec,
        0x0200: CustomUint8Codec,
        0x0300: SecureDataCodec,
    }

    config["input_output"] = {
        0x0200: "<B",  # Single Byte
    }

    config["security_algo"] = security_algorithm

    with Client(conn, config=config, request_timeout=2) as client:
        try_run(lambda: change_session(client))
        try_run(lambda: data_by_identifier(client))
        try_run(lambda: read_write_memory_by_address(client))
        try_run(lambda: dtc_information(client))
        try_run(lambda: routine_control(client))
        try_run(lambda: security_access(client))
        try_run(lambda: authentication(client, aes_key_file))

        if reset:
            try_run(lambda: ecu_reset(client))

        print("\n=== Demo finished ===\n")


if __name__ == "__main__":
    parser = ArgumentParser(description="UDS ISO 14229 Demo Script")
    parser.add_argument(
        "-c", "--can", default="vcan0", help="CAN interface (default: vcan0)"
    )
    parser.add_argument("-r", "--reset", action="store_true", help="Perform ECU reset")
    parser.add_argument(
        "-k",
        "--key-file",
        default=str(pathlib.Path(__file__).parent / "uds_aes_key.bin"),
        help="Path to AES key file used for authentication (default: uds_aes_key.bin in script directory)",
    )
    parsed_args = parser.parse_args()

    main(parsed_args)
