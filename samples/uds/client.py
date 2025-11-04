#!/usr/bin/env -S uv run --script
# /// script
# dependencies = [
#    "can-isotp==2.0.3",
#    "udsoncan==1.21.2",
# ]
# ///
#
# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0


import struct
import time
from typing import Any
from argparse import ArgumentParser, Namespace

import isotp
import udsoncan
from udsoncan.client import Client, MemoryLocation
from udsoncan.connections import IsoTPSocketConnection
from udsoncan.exceptions import (
    NegativeResponseException,
)
from udsoncan.services import DiagnosticSessionControl, ECUReset
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
import pathlib


def print_headline(text: str, suppress_trailing_newline: bool = False):
    print(f"\n=== {text} ===\n", end="" if suppress_trailing_newline else "\n")


def print_sub_headline(text: str):
    print(f"\n\t--- {text} ---\n")


def print_indented(text: str, indent: int = 1):
    print("\t" * indent + text)


def change_session(client: Client):
    print("Changing to extended diagnostic session...")
    client.change_session(DiagnosticSessionControl.Session.extendedDiagnosticSession)
    print("\tSession change successful")


def data_by_identifier_string(client: Client):
    data_id: int = 0x0100
    print_sub_headline(f"Read a String from Data Identifier: 0x{data_id:04X}")
    data = client.read_data_by_identifier([data_id])
    print_indented(
        f'Reading data from identifier\t0x{data_id:04X}:\t"{data.service_data.values[data_id]}"',
        indent=2,
    )


def data_by_identifier_primitive(client: Client):
    data_id = 0x0050
    print_sub_headline(
        f"Read and write primitive data from Data Identifier: 0x{data_id:04X}"
    )
    data = client.read_data_by_identifier_first([data_id])
    print_indented(
        f"Reading data from identifier\t0x{data_id:04X}:\t0x{data:04X}", indent=2
    )

    write_data = 0x1234
    data = client.write_data_by_identifier(data_id, write_data)
    print_indented(
        f"Writing data to identifier\t0x{data_id:04X}:\t0x{write_data:04X}", indent=2
    )

    data = client.read_data_by_identifier_first([data_id])
    print_indented(
        f"Reading data from identifier\t0x{data_id:04X}:\t0x{data:04X}", indent=2
    )

    write_data = 0xBEEF
    data = client.write_data_by_identifier(data_id, write_data)
    print_indented(
        f"Writing data to identifier\t0x{data_id:04X}:\t0x{write_data:04X}", indent=2
    )

    data = client.read_data_by_identifier_first([data_id])
    print_indented(
        f"Reading data from identifier\t0x{data_id:04X}:\t0x{data:04X}", indent=2
    )


def data_by_id_io_control(client: Client):
    led_id = 0x0200
    print_sub_headline(
        f"Demonstrate IOControl with LED from Data Identifier: 0x{led_id:04X}"
    )
    data = client.read_data_by_identifier_first([led_id])
    print_indented(f"Reading LED state:\t\t\t\t\t\t\t0x{data:02X}", indent=2)

    write_data = 0x01
    data = client.write_data_by_identifier(led_id, write_data)
    print_indented(f"Turning LED ON, writing:\t\t\t\t\t\t0x{write_data:02X}", indent=2)

    time.sleep(1)

    io_control_param = 0x03  # short term adjustment
    io_control_data = 0x00
    client.io_control(led_id, control_param=io_control_param, values=[io_control_data])
    print_indented(
        f"Temporarily overriding LED state via IO control. Setting LED state:\t0x{io_control_data:02X}",
        indent=2,
    )

    time.sleep(1)

    io_control_data = 0x01
    client.io_control(led_id, control_param=io_control_param, values=[io_control_data])
    print_indented(
        f"Temporarily overriding LED state via IO control. Setting LED state:\t0x{io_control_data:02X}",
        indent=2,
    )

    time.sleep(1)

    io_control_param = 0x01  # reset to Default
    client.io_control(led_id, control_param=io_control_param)
    print_indented("Resetting LED state to default (OFF)", indent=2)

    time.sleep(1)

    io_control_param = 0x00  # return control to ECU
    client.io_control(led_id, control_param=io_control_param)
    print_indented("Returning control of LED back to the ECU", indent=2)

    time.sleep(1)

    write_data = 0x01
    data = client.write_data_by_identifier(led_id, write_data)
    print_indented(f"Turning LED ON, writing:\t\t\t\t\t\t0x{write_data:02X}", indent=2)

    time.sleep(1)

    write_data = 0x00
    data = client.write_data_by_identifier(led_id, write_data)
    print_indented(f"Turning LED OFF, writing:\t\t\t\t\t\t0x{write_data:02X}", indent=2)
    time.sleep(1)


def data_by_identifier(client: Client):
    print_headline("Data By Identifier", suppress_trailing_newline=True)

    data_by_identifier_string(client)
    data_by_identifier_primitive(client)
    data_by_id_io_control(client)


def read_write_memory_by_address(client: Client):
    print_headline("Memory By Address")

    address: int = 0x00001000
    memory_location = MemoryLocation(address=address, memorysize=16)

    data = client.read_memory_by_address(memory_location)
    print_indented(
        f"Reading 16 bytes:\t\t\t{' '.join(f'{b:02X}' for b in data.data)}", indent=1
    )
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
    print_indented(
        f"Writing 16 bytes:\t\t\t{' '.join(f'{b:02X}' for b in write_data)}", indent=1
    )

    data = client.read_memory_by_address(memory_location)
    print_indented(
        f"Reading updated 16 bytes:\t\t{' '.join(f'{b:02X}' for b in data.data)}",
        indent=1,
    )

    data = client.write_memory_by_address(
        memory_location,
        read_data,
    )
    print_indented(
        f"Restoring original 16 bytes:\t\t{' '.join(f'{b:02X}' for b in read_data)}",
        indent=1,
    )

    data = client.read_memory_by_address(memory_location)
    print_indented(
        f"Reading restored 16 bytes:\t\t{' '.join(f'{b:02X}' for b in data.data)}",
        indent=1,
    )


def dtc_information(client: Client):
    print_headline("Reading DTC information")

    print_sub_headline("Requesting DTC Snapshot Identification (unimplemented subFunc)")
    try:
        client.read_dtc_information(
            subfunction=0x03,
        )
    except NegativeResponseException as e:
        print_indented(
            f'Server refused our request with code "{e.response.code_name}" (0x{e.response.code:02X})',
            indent=2,
        )

    print_sub_headline("Requesting DTCs with status mask 0x84")
    dtc_data = client.read_dtc_information(
        subfunction=0x02,
        status_mask=0x84,
    )
    print_indented(f"DTC count: {dtc_data.service_data.dtc_count}", indent=2)
    for dtc in dtc_data.service_data.dtcs:
        print_indented(
            f"DTC: 0x{dtc.id:06X} with status 0x{dtc.status.get_byte_as_int():02X}",
            indent=3,
        )

    print_sub_headline("Clearing DTC information")
    client.clear_dtc(group=0xFFFFFF)

    print_sub_headline("Requesting DTCs with status mask 0x84 again")
    dtc_data = client.read_dtc_information(
        subfunction=0x02,
        status_mask=0x84,
    )
    print_indented(f"DTC count: {dtc_data.service_data.dtc_count}", indent=2)
    for dtc in dtc_data.service_data.dtcs:
        print_indented(
            f"DTC: 0x{dtc.id:06X} with status 0x{dtc.status.get_byte_as_int():02X}",
            indent=3,
        )

    time.sleep(1.5)

    print_sub_headline("Requesting DTCs with status mask 0x84 after delay")
    dtc_data = client.read_dtc_information(
        subfunction=0x02,
        status_mask=0x84,
    )
    print_indented(f"DTC count: {dtc_data.service_data.dtc_count}", indent=2)
    for dtc in dtc_data.service_data.dtcs:
        print_indented(
            f"DTC: 0x{dtc.id:06X} with status 0x{dtc.status.get_byte_as_int():02X}",
            indent=3,
        )

    # Control DTC Setting demonstration
    print_sub_headline("Control DTC Setting demonstration:")

    # First read current DTCs to see initial status
    print_indented("Reading initial DTC status", indent=2)
    try:
        dtc_data = client.read_dtc_information(
            subfunction=0x02,  # DTC by status mask
            status_mask=0xFF,  # All status bits
        )
        print_indented(
            f"Initial DTC count: {dtc_data.service_data.dtc_count}", indent=3
        )
        for dtc in dtc_data.service_data.dtcs:
            print_indented(
                f"DTC: 0x{dtc.id:06X} with status 0x{dtc.status.get_byte_as_int():02X}",
                indent=4,
            )
    except NegativeResponseException as e:
        print_indented(
            f"Failed to read DTCs: {e.response.code_name} (0x{e.response.code:02X})",
            indent=3,
        )

    # Wait a bit to see DTC status changes
    print()
    print_indented("Waiting 200ms to observe DTC status changes", indent=2)
    time.sleep(0.2)

    # Read DTCs again to see status increments
    try:
        dtc_data = client.read_dtc_information(
            subfunction=0x02,
            status_mask=0xFF,
        )
        print_indented(
            f"DTC count after 200ms: {dtc_data.service_data.dtc_count}", indent=3
        )
        for dtc in dtc_data.service_data.dtcs:
            print_indented(
                f"DTC: 0x{dtc.id:06X} with status 0x{dtc.status.get_byte_as_int():02X}",
                indent=4,
            )
    except NegativeResponseException as e:
        print_indented(
            f"Failed to read DTCs: {e.response.code_name} (0x{e.response.code:02X})",
            indent=3,
        )

    # Now freeze DTC updates using Control DTC Setting OFF
    print()
    print_indented(
        "Sending DTC Setting 'OFF' (0x02)\t-> freezing DTC status updates", indent=2
    )
    try:
        response = client.control_dtc_setting(0x02)  # (DTCSettingOff)

        if response.positive:
            print_indented(
                "DTC Setting 'OFF' successful\t-> DTC updates are now frozen", indent=3
            )
        else:
            print_indented(
                f"DTC Setting 'OFF' failed: {response.code_name} (0x{response.code:02X})",
                indent=3,
            )
    except NegativeResponseException as e:
        print_indented(f"Error sending DTC Setting OFF: {e}", indent=3)

    try:
        dtc_data = client.read_dtc_information(
            subfunction=0x02,
            status_mask=0xFF,
        )
        print_indented(
            f"DTC count directly after frozen: {dtc_data.service_data.dtc_count}",
            indent=3,
        )
        for dtc in dtc_data.service_data.dtcs:
            print_indented(
                f"DTC: 0x{dtc.id:06X} with status 0x{dtc.status.get_byte_as_int():02X}",
                indent=4,
            )
    except NegativeResponseException as e:
        print_indented(
            f"Failed to read DTCs: {e.response.code_name} (0x{e.response.code:02X})",
            indent=3,
        )

    # Wait and verify DTCs don't change
    print()
    print_indented("Waiting 200ms to verify DTC status is frozen", indent=2)
    time.sleep(0.2)

    try:
        dtc_data = client.read_dtc_information(
            subfunction=0x02,
            status_mask=0xFF,
        )
        print_indented(
            f"DTC count while frozen: {dtc_data.service_data.dtc_count}", indent=3
        )
        for dtc in dtc_data.service_data.dtcs:
            print_indented(
                f"DTC: 0x{dtc.id:06X} with status 0x{dtc.status.get_byte_as_int():02X}",
                indent=4,
            )
    except NegativeResponseException as e:
        print_indented(
            f"Failed to read DTCs: {e.response.code_name} (0x{e.response.code:02X})",
            indent=3,
        )

    # Resume DTC updates using Control DTC Setting ON
    print()
    print_indented(
        "Sending DTC Setting ON (0x01)\t\t-> resuming DTC status updates", indent=2
    )
    try:
        response = client.control_dtc_setting(0x01)  # DTCSettingOn
        if response.positive:
            print_indented(
                "DTC Setting ON successful\t-> DTC updates are now resumed", indent=3
            )
        else:
            print_indented(
                f"DTC Setting ON failed: {response.code_name} (0x{response.code:02X})",
                indent=3,
            )
    except NegativeResponseException as e:
        print_indented(f"Error sending DTC Setting ON: {e}", indent=3)

    # Wait and verify DTCs are updating again
    print()
    print_indented("Waiting 200ms to verify DTC status updates have resumed", indent=2)
    time.sleep(0.2)

    try:
        dtc_data = client.read_dtc_information(
            subfunction=0x02,
            status_mask=0xFF,
        )
        print_indented(
            f"DTC count after resume: {dtc_data.service_data.dtc_count}", indent=3
        )
        for dtc in dtc_data.service_data.dtcs:
            print_indented(
                f"DTC: 0x{dtc.id:06X} with status 0x{dtc.status.get_byte_as_int():02X}",
                indent=4,
            )
    except NegativeResponseException as e:
        print_indented(
            f"Failed to read DTCs: {e.response.code_name} (0x{e.response.code:02X})",
            indent=3,
        )

    print()
    print_indented("Control DTC Setting demonstration completed.", indent=2)


def ecu_reset(client: Client):
    # Send ECU reset request (hard reset)
    print_headline("Sending ECU hard reset request")
    client.ecu_reset(ECUReset.ResetType.hardReset)
    print_indented("ECU reset request sending successfully", indent=1)


def routine_control(client: Client):
    print_headline("Routine Control", suppress_trailing_newline=True)

    routine_id = 0x1234
    input_value = ~0xDEADBEEF & 0xFFFFFFFF

    print_sub_headline(f"Executing synchronous routine 0x{routine_id:04X}")
    try:
        response = client.routine_control(
            routine_id=routine_id,
            control_type=1,  # Start routine
            data=struct.pack(">I", input_value),  # Input as big endian (4 bytes)
        )

        # The first byte is the status, the next 4 bytes are the BE uint32 result
        result = struct.unpack(">I", response.service_data.routine_status_record)[0]
        print_indented(f"Routine input : 0x{input_value:08X}", indent=2)
        print_indented(f"Routine result: 0x{result:08X}", indent=2)
        print_indented("Routine executed successfully", indent=2)

    except NegativeResponseException as e:
        print_indented(
            f"Routine control failed: {e.response.code_name} (0x{e.response.code:02X})",
            indent=2,
        )
        raise
    except Exception as e:
        print_indented(f"Unexpected error during routine control: {e}", indent=2)
        raise

    print_sub_headline(f"Executing asynchronous routine 0x{routine_id:04X}")

    routine_id = 0x5678
    try:
        print_indented(f"Starting asynchronous routine 0x{routine_id:04X}", indent=2)
        client.routine_control(
            routine_id=routine_id,
            control_type=1,  # Start routine
        )
        print_indented(
            f"Asynchronous routine 0x{routine_id:04X} started successfully\n", indent=2
        )

        time.sleep(0.1)  # Wait a bit before requesting status

        print_indented(f"Stopping asynchronous routine 0x{routine_id:04X}", indent=2)
        client.routine_control(
            routine_id=routine_id,
            control_type=2,  # Stop routine
        )
        print_indented(
            f"Asynchronous routine 0x{routine_id:04X} stopped successfully\n", indent=2
        )

        print_indented(
            f"Requesting results from asynchronous routine 0x{routine_id:04X}...",
            indent=2,
        )
        response = client.routine_control(
            routine_id=routine_id,
            control_type=3,  # Request routine results
        )

        if response.service_data.routine_status_record:
            status_data = response.service_data.routine_status_record
            if len(status_data) >= 5:
                status = status_data[0]
                progress = struct.unpack(">I", status_data[1:5])[0]
                print_indented(
                    f"Routine status: {status}, Progress: {progress}", indent=2
                )
            else:
                print_indented(
                    f"Routine status data: {' '.join(f'{b:02X}' for b in status_data)}",
                    indent=2,
                )

    except NegativeResponseException as e:
        print_indented(
            f"Routine control failed: {e.response.code_name} (0x{e.response.code:02X})",
            indent=2,
        )
        raise
    except Exception as e:
        print_indented(f"Unexpected error during routine control: {e}", indent=2)
        raise


def security_access(client: Client):
    print_headline("Security Access")

    secure_data_id = 0x0300

    # First, try to read the secure data without authentication
    print_sub_headline(
        f"Trying to read secure data (ID: 0x{secure_data_id:04X}) without authentication"
    )
    try:
        data = client.read_data_by_identifier([secure_data_id])
        print_indented(
            f'Unexpected success: "{data.service_data.values[secure_data_id]}"',
            indent=2,
        )
    except NegativeResponseException as e:
        if e.response.code == 0x33:  # Security Access Denied
            print_indented(
                f"Access denied as expected: {e.response.code_name} (0x{e.response.code:02X})",
                indent=2,
            )
        else:
            print_indented(
                f"Unexpected error: {e.response.code_name} (0x{e.response.code:02X})",
                indent=2,
            )

    # Now perform security access authentication
    print_sub_headline("Performing security access authentication for level 1")

    try:
        # Step 1: Request seed for security level 1
        print_indented("Requesting seed for security level 1", indent=2)
        client.unlock_security_access(1)

        print_indented("Security access successful! Level 1 unlocked.", indent=2)

    except NegativeResponseException as e:
        print_indented(
            f"Security access failed: {e.response.code_name} (0x{e.response.code:02X})",
            indent=2,
        )
        return
    # pylint: disable=broad-except
    except Exception as e:
        print_indented(f"Unexpected error during security access: {e}", indent=2)
        return

    # Now try to read the secure data with authentication
    print_sub_headline(
        f"Trying to read secure data (ID: 0x{secure_data_id:04X}) with authentication",
    )
    try:
        data = client.read_data_by_identifier([secure_data_id])
        secure_content = data.service_data.values[secure_data_id]
        print_indented(f'Success! Secure data: "{secure_content}"', indent=2)
    except NegativeResponseException as e:
        print_indented(
            f"Still denied: {e.response.code_name} (0x{e.response.code:02X})", indent=2
        )
    # pylint: disable=broad-except
    except Exception as e:
        print_indented(f"Unexpected error: {e}", indent=2)

    print_indented("Security access demonstration completed.", indent=2)


def security_algorithm(_level: int, seed: bytes, _params: Any) -> bytes:
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
    print_headline("Authentication")

    data_id: int = 0x0150
    print_sub_headline(
        f"Try reading secure data with id 0x{data_id:04X} without authentication"
    )
    try:
        data = client.read_data_by_identifier([data_id])
        print_indented(
            f'Reading data from identifier\t0x{data_id:04X} should have failed but succeeded with data "{data.service_data.values[data_id]}". Are we still authenticated?',
            indent=2,
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

    print_indented("Seed (hex):\t\t" + seed.hex(), indent=2)
    key: bytes = read_key(key_file)

    ct = encrypt_seed(seed, key)
    print_indented("Ciphertext (hex):\t" + ct.hex(), indent=2)

    client.authentication(
        authentication_task=6,
        proof_of_ownership_client=ct,
        algorithm_indicator=algo_indicator,
    )

    print_sub_headline("Authentication successful!")

    print_sub_headline(
        f"Try reading secure data with id 0x{data_id:04X} with authentication"
    )
    data = client.read_data_by_identifier([data_id])
    print_indented(
        f"Reading data from secured identifier\t0x{data_id:04X}:\t0x{data.service_data.values[data_id]:02X}",
        indent=2,
    )

    client.authentication(
        authentication_task=0,
    )

    print_sub_headline("De-Authentication successful!")


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

        print_headline("Demo finished")


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
