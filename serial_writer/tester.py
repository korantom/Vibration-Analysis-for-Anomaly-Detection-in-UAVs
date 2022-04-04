import os
import time
from queue import Queue
from typing import Type

from serial_wrapper import SerialWrapper
from tester_config import ShellState, TesterConfig


class Tester:
    """TODO: ..."""

    def __init__(self, tester_config: Type[TesterConfig]):
        self.tester_config = tester_config
        self.serial_wrapper = SerialWrapper()
        self.tester_ready = False

    def get_test_file_name(self, test_index: int) -> str:
        """Creates/Assambles a file name for given index, from test_config"""
        # Extract config
        test_motor_throttle_values = self.tester_config.test_motor_throttle_values
        throttle_count = len(test_motor_throttle_values)

        prefix = self.tester_config.test_files_prefix

        # throttle_prefix = "ABCDEFGH"[test_index % throttle_count]
        # instead of encoding as letter use explicitly the value
        throttle = throttle_count[test_index % throttle_count]

        test_index_for_throttle_value = test_index // throttle_count

        # Assemble file name
        test_file_name = (
            f"{prefix}_{throttle:03d}t_{test_index_for_throttle_value:04d}.csv"
        )
        return test_file_name

    def save_config(self):
        """Saves tester config to a json file."""

        # Assemble path
        path = self.tester_config.path + "/" + self.tester_config.test_config_file_name

        # Create directories
        os.makedirs(os.path.dirname(path), exist_ok=True)

        # Write config
        with open(path, "w") as f:
            f.write(self.tester_config.to_json())

    def write_test(self, file_name: str, data_queue: Queue):
        """Writes content of data queue to a file."""

        # Assemble path
        path = self.tester_config.path + "/" + file_name

        # Assumes directories exist

        # Write data
        with open(path, "w") as f:
            while not data_queue.empty():
                line = data_queue.get()
                f.write(line)

    ############################################################################

    def tester_init(self):
        print("tester.tester_init()")

        # TODO: or just wait for RESET
        if self.serial_wrapper.shell_state == ShellState.UNKNOWN:
            self.serial_wrapper.check_if_ready()
            self.serial_wrapper.wait_for_write_ready()

        # TODO: flush input buffer?

        if self.serial_wrapper.shell_state == ShellState.WRITE_READY:
            self.serial_wrapper.write_command("tester_init")
            self.serial_wrapper.wait_for_write_ready()
            self.tester_ready = True

        self.serial_wrapper.wait_for_write_ready()

    ############################################################################

    def tester_start(self):
        print("tester.tester_start()")
        if self.tester_ready == False:
            print(f"{self.tester_ready=}")
            return

        # prevent calling consequtively
        self.tester_ready = False



if __name__ == "__main__":
    tester_config = TesterConfig()

    global tester
    tester = Tester(tester_config)

    tester.save_config()

    print("FLASH and RESET BLIP")

    tester.tester_init()

    while input("Enter 'START': ") != "START":
        pass

    print("Starting tester in 15sec")
    time.sleep(15)

    tester.tester_start()
