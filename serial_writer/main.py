import serial
import time

from typing import Type, List
from signal import signal, SIGINT

from queue import Queue

################################################################################

DATA_PREFIX = "_data_: "
SHELL_PROMPT = "uart:~$"
PORT = "/dev/cu.usbmodemC1E9B7D03"
BAUDRATE = 115200
TIMEOUT = 1

global sigint_detected
sigint_detected = False


def init_serial() -> Type[serial.Serial]:
    ser = serial.Serial(
        port=PORT,
        baudrate=BAUDRATE,
        timeout=TIMEOUT,
    )

    # ser.open()

    time.sleep(1)

    return ser


################################################################################


def check_shell_prompt(line: str):
    return SHELL_PROMPT in line


################################################################################

data_queue = Queue()


def filter_data_str(line: str) -> str:
    if line.startswith(DATA_PREFIX):
        data = line.split(DATA_PREFIX)
        print(data)
        data_queue.put(data)
        return None
    else:
        return line


################################################################################


def main_interactive(ser: Type[serial.Serial]):
    prompt_ready = False
    while not sigint_detected:
        # Check how many bytes in buffer
        buffered_byte_count = ser.inWaiting()

        # Read 1 line
        line_bytes = ser.readline()
        try:
            line_str = line_bytes.decode("ascii")
        except:
            print("could not decod: ", line_bytes)

        # Check if time out, TODO: shoudl check probablly '\n' in line_str?
        if line_bytes != bytes():
            line_str = filter_data_str(line_str)
            print(line_str)

        # Check if prompt ready
        if check_shell_prompt(line_str):
            prompt_ready = True

        # Execute a command
        if prompt_ready and buffered_byte_count == 0:
            # print()
            # print('uart:~$:')

            cmd = input()
            cmd += "\r\n"  # ?

            prompt_ready = False
            ser.write(cmd.encode("ascii"))

    ser.close()


################################################################################

################################################################################


def SIGINT_handler(signal_received, frame):
    print("SIGINT or CTRL-C detected. Exiting gracefully")
    global sigint_detected
    sigint_detected = True


if __name__ == "__main__":
    signal(SIGINT, SIGINT_handler)

    # IFs
    # interactive mode
    # test mode and config provided ....
    # - additional flags? ...
    # - ...

    # while True: pass
    ser = init_serial()
    main_interactive(ser)

    # test_config = TestConfig()
    # print(f"{test_config=}")

    exit(0)
