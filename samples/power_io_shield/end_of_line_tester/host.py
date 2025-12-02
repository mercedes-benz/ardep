from serial import Serial
from argparse import ArgumentParser

def main(args):
    port = Serial(args.port, baudrate=115200, timeout=1)

    port.write(b"START")
    try:
      while True:
          line = port.readline()
          if line:
              print(line.decode().strip())
              if "completed successfully" in line.decode():
                  print("Test completed successfully. Exiting.")
                  break
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    parser = ArgumentParser(description="End of Line Tester Host")
    parser.add_argument("-p", "--port", type=str, default="/dev/ttyACM0", help="Serial port to connect to (default: /dev/ttyACM0)")
    args = parser.parse_args()
    main(args)