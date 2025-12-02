# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0


import can
from argparse import ArgumentParser

def main(args):
    interface = args.interface
    id = args.id

    with can.Bus(channel=interface, interface="socketcan") as bus:
        bus.set_filters([{"can_id": id, "can_mask": 0x7FF}])
        while True:
            message = bus.recv()
            if message is None:
                continue

            print(str(message.data.decode()), end="")


if __name__ == "__main__":
    parser = ArgumentParser(description="CAN Log Receiver")
    parser.add_argument("-i", "--interface", type=str, default="can0", help="CAN interface to use (default: can0)")
    parser.add_argument("-id", "--id", type=lambda x: int(x, 0), default=0x100, help="CAN ID to listen to (default: 0x100)")
    args = parser.parse_args()
    main(args)
