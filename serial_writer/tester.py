import os
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


