from argparse import ArgumentParser, Namespace
from os import path
import subprocess
import sys

from west import log


class BuildBootloader:
    command: str = "build-bootloader"
    _board_name: str = None

    def __init__(self, board_name: str):
        self._board_name = board_name

    def add_args(self, parser: ArgumentParser):
        subcommand_parser: ArgumentParser = parser.add_parser(
            self.command,
            help=f"builds the bootloader for the {self._board_name}",
        )

        subcommand_parser.add_argument(
            "-d",
            "--build-dir",
            help="application build directory (default: build)",
            default="build",
            metavar="DIR",
        )

    def run(self, args: Namespace):
        build_dir: str = args.build_dir
        mcuboot_dir = f"{path.dirname(path.dirname(path.dirname(path.dirname(path.realpath(__file__)))))}/bootloader/mcuboot/boot/zephyr"
        board_dir = f"{path.dirname(path.dirname(path.dirname(path.realpath(__file__))))}/boards/arm/ardep"
        extra_conf_file = f"{board_dir}/mcuboot.conf"
        extra_overlay_file = f"{board_dir}/mcuboot.overlay"

        cmd = [
            "west",
            "build",
            "--pristine",
            "auto",
            "--board",
            "ardep",
            "--build-dir",
            f"{build_dir}",
            mcuboot_dir,
            "--",
            f"-DEXTRA_CONF_FILE={extra_conf_file}",
            f"-DEXTRA_DTC_OVERLAY_FILE={extra_overlay_file}",
        ]

        log.inf(f"Building bootloader with: {' '.join(cmd)}\n\n")

        result = subprocess.run(cmd, check=False)
        sys.exit(result.returncode)
