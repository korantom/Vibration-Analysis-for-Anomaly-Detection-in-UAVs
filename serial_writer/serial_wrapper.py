from enum import Enum
from queue import Queue
import serial
import time

import signal

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

        signal.signal(signal.SIGINT, self.SIGINT_handler)

    def SIGINT_handler(self, signal_received, frame):
        """on ctrl-c switch to interactive mode"""
        # TODO: will crash on consequtive calls

        self.ser.write("\n".encode(ENCODING))
        self.ser.write("\n".encode(ENCODING))

        self.wait_for_write_ready()

        while True:
            line_bytes = self.ser.readline()

            if line_bytes == bytes():
                continue

            try:
                line_str = line_bytes.decode(ENCODING)
            except:
                line_str = f"ERROR decoding {ENCODING}, unable to decode. {line_bytes}"
            print(line_str)

            if SHELL_PROMPT in line_str:
                while (cmd := input(SHELL_PROMPT)) == "":
                    pass
                if cmd == "exit":
                    exit(0)

                cmd += "\n"
                self.ser.write(cmd.encode(ENCODING))

    def check_shell_prompt(self, line: str):
        if (line != None) and (SHELL_PROMPT in line):
            self.shell_state = ShellState.WRITE_READY

    def clear(self):
        print(f"SerialWrapper.clear")
        self.data_queue = Queue()

    def check_if_ready(self):
        print(f"SerialWrapper.check_if_ready()")
        self.ser.write("\n".encode(ENCODING))
        # TODO: call once only or multiple times and clear input buffer
        # self.ser.write("\n".encode(ENCODING))
        time.sleep(0.5)
        self.wait_for_write_ready()
        # self.ser.reset_input_buffer()

    def write_command(self, cmd: str):
        print(f"SerialWrapper.write_command(): -> {cmd}")
        self.wait_for_write_ready()
        cmd += "\n"
        self.shell_state = ShellState.WRITEN
        self.ser.write(cmd.encode(ENCODING))
        self.ser.flush()

    def filter_data_str(self, line: str) -> str:
        # TODO: DATA_PREFIX in line but doesnt start? possible?

        if line.startswith(DATA_PREFIX):
            data = line.split(DATA_PREFIX)
            self.data_queue.put(data[1])
            return data[0]
        else:
            return line

    def wait_for_write_ready(self):
        print("SerialWrapper.wait_for_write_ready()")

        # assumes all data is dumped (written) by the time prompt appears
        # (i.e. single thread design, either doing ... or idle = console prompt ...)

        while self.shell_state != ShellState.WRITE_READY:

            # Read a '\n' terminated line (timeouts)
            line_bytes = self.ser.readline()

            # Skip if nothing read
            if line_bytes == bytes():
                continue

            # Try to decode read line
            try:
                line_str = line_bytes.decode(ENCODING)
            except:
                line_str = f"ERROR decoding {ENCODING}, unable to decode. {line_bytes}"
                # skip/continue/...?

            line_str = self.filter_data_str(line_str)
            if line_str in ["", None, "\r", "\n", "\r\n"]:
                continue

            self.check_shell_prompt(line_str)

            print(line_str)
