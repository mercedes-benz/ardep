from runners.core import RunnerCaps, ZephyrBinaryRunner
import subprocess
import time

class ArdepRunner(ZephyrBinaryRunner):
    """Runner for ardep board using dfu-util for flashing"""

    def __init__(self, cfg, device=None):
        super().__init__(cfg)
        self._bin_file = cfg.bin_file
        self.device = device

    @classmethod
    def name(cls):
        return "ardep"

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands=({"flash"}))

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            "--device",
            help=f"Ardep usb device (see `lsusb`)",
            default=f"25e1:1b1e",
            metavar="DEVICE",
        )

    @classmethod
    def do_create(cls, cfg, args):
        return cls(cfg, device=args.device)

    def has_usb_device(self, device: str) -> bool:
        output = subprocess.run(
            ["dfu-util", "-l"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        output_lines = output.stdout.decode("utf-8").split("\n")
        for line in output_lines:
            if device in line:
                return True
        return False

    def do_run(self, command, **kwargs):
        if command != "flash":
            self.logger.critical(f"Unsupported command: {command}")
            exit(1)

        bin_file = self._bin_file

        if not self.has_usb_device(self.device):
            self.logger.error(f"Device {self.device} not found")
            exit(1)

        result = subprocess.Popen(
            [
                "dfu-util",
                "--alt",
                "1",
                "--device",
                f"{self.device}",
                "--download",
                f"{bin_file}",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            encoding="utf-8",
            text=True,
        )

        while result.poll() is None:
            print(f"flashing ...", end="\r")
            time.sleep(1)

        print("")
        if result.returncode != 0:
            subprocess_output = result.stdout.read()

            self.logger.error(f"dfu-util subprocess output:\n{subprocess_output}\n")
            self.logger.error(f"Failed to flash firmware to ardep")

            exit(result.returncode)
        
        result = subprocess.Popen(
            [
                "dfu-util",
                "--alt",
                "1",
                "--device",
                f"{self.device}",
                "--detach",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            encoding="utf-8",
            text=True,
        )

        while result.poll() is None:
            print(f"detaching ...", end="\r")
            time.sleep(1)
        
        print("")
        if result.returncode != 0:
            self.logger.error(f"dfu-util subprocess output:\n{result.stdout.read()}\n")
            self.logger.error(f"Failed to detach dfu-util from ardep")

            exit(result.returncode)

        self.logger.info(f"Successfully upgraded ardep firmware on usb device {self.device}")
        self.logger.info(f"The device applies the new firmware and boots in the next few seconds.")