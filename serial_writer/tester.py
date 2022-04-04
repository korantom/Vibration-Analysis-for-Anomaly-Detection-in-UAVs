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
        throttle = test_motor_throttle_values[test_index % throttle_count]

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

        throttle_count = len(self.tester_config.test_motor_throttle_values)
        test_count = throttle_count * (
            self.tester_config.test_measurements_count // throttle_count
        )

        for i in range(test_count):
            print("PERFORMING TEST {i}")

            # Preprare cmd, file_name, data_queue

            throttle = self.tester_config.test_motor_throttle_values[i % throttle_count]
            ramp_up_d = self.tester_config.test_ramp_up_duration_sec
            measurement_d = self.tester_config.test_measurement_duration_sec
            pause_d = self.tester_config.test_pause_duration_sec

            cmd = (
                f"{'tester_single_test_dump'} {throttle} {ramp_up_d} {measurement_d} {pause_d}"
            )

            test_file_name = self.get_test_file_name(i)

            data_queue = Queue()
            self.serial_wrapper.clear()

            # Wait ...
            self.serial_wrapper.wait_for_write_ready()

            # Write cmd (measure data)
            self.serial_wrapper.write_command(cmd)
            self.serial_wrapper.wait_for_write_ready()

            # copy data queue
            data_queue = self.serial_wrapper.data_queue

            # write data to file
            self.write_test(test_file_name, data_queue)


"""
TEST WORKFLOW/STEPS
0. Preliminaries:
    - ESCs have been calibrated
    - double check HW, is all mounted and safe
    - SD card should be wiped and inside the device
    - change logging priority to ERR/WRN
    - ...
1. ALL turned off

2. FLASH and RESET blib

4. start python test controller script
    - write to uart "\n" (or RESET BLIP) and wait until prompt ready for write
    - tester_init

5. power on the ESCs

6. python
    - start tester
    - ....

"""
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
