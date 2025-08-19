from argparse import ArgumentParser, Namespace
import subprocess
import time
import warnings
from .util import Util

from west import log


class DfuUtil:
    command: str = "dfu"
    _board_name: str = None
    _vid: str = None
    _pid: str = None
    _description: str = None
    _waiting_dots: int = 0
    _dfu_upgrade_time_seconds: int = 7

    def __init__(
        self,
        board_name: str,
        vid: str,
        pid: str,
    ):
        self._board_name = board_name
        self._vid = vid
        self._pid = pid
        self._description = """\
Performs a firmware upgrade with the dfu-util tool.
"""

    def add_args(self, parser: ArgumentParser):
        subcommand_parser: ArgumentParser = parser.add_parser(
            self.command,
            help="performs a firmware upgrade with the dfu-util tool",
        )

        subcommand_parser.add_argument(
            "--device",
            help=f"{self._board_name} usb device (see `lsusb`, default: {self._vid}:{self._pid})",
            default=f"{self._vid}:{self._pid}",
            metavar="DEVICE",
        )

        subcommand_parser.add_argument(
            "-d",
            "--build-dir",
            help="application build directory (default: build)",
            default="build",
            metavar="DIR",
        )

        subcommand_parser.add_argument(
            "-b",
            "--bootloader",
            help="set if upgrading in bootloader mode",
            action="store_true",
        )

    def run(self, args: Namespace):
        # Print deprecation warning in yellow
        log.wrn(
            "Using `west ardep dfu` is deprecated. Use the `west flash` command instead."
        )

        bootloader_mode: bool = args.bootloader
        device: str = self.get_dfu_usb_device(args.device)
        build_dir: str = args.build_dir
        bin_file: str = Util.get_zephyr_signed_bin(build_dir)
        if not self.has_usb_device(device):
            log.die(f"Device {args.device} not found", exit_code=1)

        Util.rebuild_application(build_dir)
        self.execute_dfu_util(device, bin_file, bootloader_mode)

    def get_dfu_usb_device(self, device: str) -> str:
        if ":" not in device:
            log.die(
                f"Invalid device format: {device}\nExpected string in format {self._vid}:{self._pid}",
                exit_code=1,
            )
        vid, pid = device.split(":")
        vid = vid.zfill(4)
        pid = pid.zfill(4)
        return f"{vid}:{pid}"

    def has_usb_device(self, device: str) -> bool:
        output = subprocess.run(
            ["dfu-util", "-l"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        output_lines = output.stdout.decode("utf-8").split("\n")
        device_line = next((line for line in output_lines if device in line), None)
        return device_line is not None

    def _get_dots(self) -> str:
        self._waiting_dots = (self._waiting_dots + 1) % 10
        return ("." * self._waiting_dots).ljust(10)

    def _wait_for_mcuboot_slot_swap(self):
        for _ in range(self._dfu_upgrade_time_seconds):
            print(f"Flashing Firmware {self._get_dots()}", end="\r")
            time.sleep(1)

        # Print a newline to not overwrite the above text
        print("")

    def execute_dfu_util(
        self, device: str, bin_file: str, bootloader_mode: bool = False
    ):
        result = subprocess.Popen(
            [
                "dfu-util",
                "--alt",
                "1",
                "--device",
                f"{device}",
                "--download",
                f"{bin_file}",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            encoding="utf-8",
            text=True,
        )

        while result.poll() is None:

            print(f"upgrading {self._get_dots()}", end="\r")
            time.sleep(1)

        # Print a newline to not overwrite the above text
        print("")

        if result.returncode != 0:
            subprocess_output = result.stdout.read()

            log.err(f"dfu-util subprocess output:\n{subprocess_output}\n")

            log.die(
                f"Failed to upgrade {self._board_name} firmware on usb device {device}",
                exit_code=2,
            )

        if bootloader_mode:
            self._wait_for_mcuboot_slot_swap()
            log.inf(
                "The board console should show a 'panic!' message now. A manual power cycle is required to start the new firmware."
            )
            return

        log.inf("Detaching the device...")
        time.sleep(1)

        result = subprocess.run(
            [
                "dfu-util",
                "--alt",
                "1",
                "--device",
                f"{device}",
                "--detach",
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )

        if result.returncode != 0:
            log.err(
                f"Failed to detach dfu-util from {self._board_name}\nPlease power-cycle manually to start the firmware upgrade\nWhen you upgraded in bootloader mode, wait for the 'panic!' message before power-cycling and use the --bootloader flag",
            )
            exit(result.returncode)

        self._wait_for_mcuboot_slot_swap()

        log.inf("done")
