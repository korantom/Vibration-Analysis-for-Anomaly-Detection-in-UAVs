from enum import Enum
from queue import Queue
import serial
from signal import signal, SIGINT
import time
from typing import Type, List


DATA_PREFIX = "_data_: "
SHELL_PROMPT = "uart:~$"
PORT = "/dev/cu.usbmodemC1E9B7D03"
BAUDRATE = 115200
TIMEOUT = 2.0


class ShellState(Enum):
    NOT_READY = 0
    # READ_READY = 1
    WRITE_READY = 2
    WRITEN = 3


class SerialWrapper:
    def __init__(self):
        self.ser = serial.Serial(
            port=PORT,
            baudrate=BAUDRATE,
            timeout=TIMEOUT,
            write_timeout=2.0,
        )

        self.shell_state = ShellState.NOT_READY
        self.data_queue = Queue()

        time.sleep(1)

    def check_shell_prompt(self, line: str):
        if (line != None) and (SHELL_PROMPT in line):
            self.shell_state = ShellState.WRITE_READY

    def write_command(self, cmd: str):
        print(f"write_command({cmd})")
        self.wait_for_write_ready()
        cmd += "\r"
        self.shell_state = ShellState.WRITEN
        self.ser.write(cmd.encode("ascii"))
        self.ser.flush()

    # def read_data(self) -> Queue:
    #     pass

    def clear(self):
        print(f"clear")
        self.data_queue = Queue()

    def filter_data_str(self, line: str) -> str:
        if line.startswith(DATA_PREFIX):
            data = line.split(DATA_PREFIX)
            self.data_queue.put(data[1])
            return None
        else:
            return line

    def wait_for_write_ready(self):
        print("wait_for_write_ready")
        while self.shell_state != ShellState.WRITE_READY:
            # buffered_byte_count = self.ser.in_waiting
            # print(f"{buffered_byte_count=}")

            line_bytes = self.ser.readline()
            try:
                line_str = line_bytes.decode("ascii")
            except:
                line_str = "ERROR decoding ascii"
                print(line_bytes)

            if line_bytes != bytes():
                line_str = self.filter_data_str(line_str)
                if (
                    (line_str != "")
                    and (line_str != None)
                    and (line_str != "\r\n")
                    and (line_str != "\r")
                ):
                    print(line_str)

            self.check_shell_prompt(line_str)
