from os import path
import subprocess
from west import log


class Util:
    @staticmethod
    def get_zephyr_signed_bin(build_dir: str) -> str:
        bin_file = f"{build_dir}/zephyr/zephyr.signed.bin"
        realpath = path.realpath(bin_file)
        if not path.exists(bin_file):
            log.die(
                f"File not found: {bin_file}\nWhich expands to: {realpath}",
                exit_code=2,
            )
        return realpath

    @staticmethod
    def rebuild_application(build_dir: str):
        log.inf("check if application needs to be rebuilt:")
        result = subprocess.run(
            ["west", "build", "--pristine", "auto", "--build-dir", build_dir],
            check=False,
        )

        if result.returncode != 0:
            log.die(
                f"Failed to rebuild application in {build_dir}",
                exit_code=2,
            )

    @staticmethod
    def has_can_interface(interface: str) -> bool:
        output = subprocess.run(["ip", "link"], stdout=subprocess.PIPE, check=False)
        output_lines = output.stdout.decode("utf-8").split("\n")
        interface_line = next(
            (line for line in output_lines if interface in line), None
        )
        return interface_line is not None
