from enum import Enum
from queue import Queue
import serial
import time

# TODO: move CONFIG
DATA_PREFIX = "_data_:"
SHELL_PROMPT = "uart:~$"
PORT = "/dev/cu.usbmodemC1E9B7D03"
BAUDRATE = 9600  # 115200,
READ_TIMEOUT = 0.5  # ...
DATA_BITS = serial.EIGHTBITS
ENCODING = "ascii"


class ShellState(Enum):
    UNKNOWN = 0
    WRITE_READY = 2
    WRITEN = 3


class SerialWrapper:
    """TODO: wrapper to enabel tester logic ... without impl detail ..."""

    def __init__(self):
        self.ser = serial.Serial(
            port=PORT,
            baudrate=BAUDRATE,
            timeout=READ_TIMEOUT,
            # write_timeout=, # TODO:
            bytesize=DATA_BITS,
        )

        self.shell_state = ShellState.UNKNOWN
        self.data_queue = Queue()

        # Wait for serial to open
        time.sleep(2)

    def check_shell_prompt(self, line: str):
        if (line != None) and (SHELL_PROMPT in line):
            self.shell_state = ShellState.WRITE_READY

    def clear(self):
        print(f"SerialWrapper.clear")
        self.data_queue = Queue()

    def write_command(self, cmd: str):
        print(f"SerialWrapper.write_command(): -> {cmd}")
        self.wait_for_write_ready()
        cmd += "\n"
        self.shell_state = ShellState.WRITEN
        self.ser.write(cmd.encode(ENCODING))
        self.ser.flush()
