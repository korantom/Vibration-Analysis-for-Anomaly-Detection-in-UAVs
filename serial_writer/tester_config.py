from datetime import datetime
from dataclasses import dataclass, field
from dataclasses_json import dataclass_json
import os
from queue import Queue
from typing import Type, List

from serial_wrapper import *

################################################################################

# TODO: split config into subclasses
@dataclass_json
@dataclass
class TesterConfig:
    # paths/names config
    test_folder_path: str = "./Tests"
    test_folder_name: str = "TEST"
    test_config_file_name: str = "config.json"
    test_files_prefix: str = "T"

    # drone config
    test_voltage: int = 22
    test_mounting_setup_description: str = ""
    test_motor_count: int = 1
    test_motor_states: List[str] = field(
        default_factory=lambda: ["default"],
    )
    test_propeller_states: List[str] = field(
        default_factory=lambda: ["default"],
    )
    test_esc_config_description: str = "default"

    # testing config
    test_measurements_count: int = 10
    test_motor_throttle_values: List[int] = field(
        default_factory=lambda: [10, 15, 25, 50, 75, 90, 100, 0]
    )
    test_ramp_up_duration_sec: int = 2
    test_measurement_duration_sec: int = 4
    test_pause_duration_sec: int = 2

    # shell config/commands
    # TODO:
    # test_config_shell_commands: List[str] = field(
    #     default_factory=lambda: [
    #         "tester_init",
    #     ]
    # )

    # # TODO: ...
    # test_shell_command: str = "single_test_dump"

    ############################################################################
    def __post_init__(self):
        dt = datetime.now()
        dt_str = dt.strftime("%Y_%m_%d_%H_%M_%S")
        self.path = f"{self.test_folder_path}/{self.test_folder_name}_{dt_str}"

    ############################################################################


class Tester:
    def __init__(self, tester_config: Type[TesterConfig]):
        self.tester_config = tester_config
        self.serial_wrapper = SerialWrapper()
        self.tester_ready = False

    def get_test_file_name(self, test_index: int) -> str:
        throttle_count = len(self.tester_config.test_motor_throttle_values)

        prefix = self.tester_config.test_files_prefix
        throttle_prefix = "ABCDEFGH"[test_index % throttle_count]
        test_index_for_throttle = test_index // throttle_count

        test_file_name = f"{prefix}_{throttle_prefix}_{test_index_for_throttle}.csv"
        return test_file_name

    def write_config(self):
        # TODO: get commit hash

        # Assemble path
        path = self.tester_config.path + "/" + self.tester_config.test_config_file_name

        # Create directories
        os.makedirs(os.path.dirname(path), exist_ok=True)

        # Write config
        with open(path, "w") as f:
            f.write(self.tester_config.to_json())

    def write_test(self, file_name: str, data_queue: Queue):
        # Assemble path
        path = self.tester_config.path + "/" + file_name

        # Write data
        with open(path, "w") as f:
            while not data_queue.empty():
                line = data_queue.get()
                f.write(line)

    ############################################################################
    def tester_init(self):
        print("tester_init")
        if self.serial_wrapper.shell_state == ShellState.NOT_READY:
            self.serial_wrapper.wait_for_write_ready()
            self.serial_wrapper.write_command("tester_init")
            self.serial_wrapper.wait_for_write_ready()

            self.tester_ready = True

    ############################################################################

    def tester_start(self):
        if self.tester_ready == False:
            print("Tester not ready")
            return
        # prevent calling 2x conseq
        self.tester_ready = False

        throttle_count = len(self.tester_config.test_motor_throttle_values)
        test_count = throttle_count * (
            self.tester_config.test_measurements_count // throttle_count
        )

        for i in range(test_count):
            print(i)
            throttle = self.tester_config.test_motor_throttle_values[i % throttle_count]
            ramp_up_d = self.tester_config.test_ramp_up_duration_sec
            measurement_d = self.tester_config.test_measurement_duration_sec
            pause_d = self.tester_config.test_pause_duration_sec

            cmd = (
                f"{'single_test_dump'} {throttle} {ramp_up_d} {measurement_d} {pause_d}"
            )

            test_file_name = self.get_test_file_name(i)

            data_queue = Queue()

            ####################################################################

            if self.serial_wrapper.shell_state == ShellState.WRITE_READY:
                self.serial_wrapper.clear()

                self.serial_wrapper.write_command(cmd)
                self.serial_wrapper.wait_for_write_ready()

                if self.serial_wrapper.shell_state == ShellState.WRITE_READY:
                    print(f"copy data_queue")
                    data_queue = self.serial_wrapper.data_queue

            ####################################################################

            self.write_test(test_file_name, data_queue)

    def stop(self):
        cmd = f"{'single_test_dump'} {0} {1} {1} {10}"
        self.serial_wrapper.write_command(cmd)


def SIGINT_handler(signal_received, frame):
    print("SIGINT or CTRL-C detected. Exiting gracefully")
    tester.stop()
    exit(0)


if __name__ == "__main__":
    signal(SIGINT, SIGINT_handler)

    tester_config = TesterConfig()

    global tester
    tester = Tester(tester_config)

    tester.write_config()

    print("FLASH and RESET BLIP")

    tester.tester_init()

    while input("Enter 'START': ") != "START":
        time.sleep(1)

    print("Starting tester in 15sec")
    time.sleep(15)

    tester.tester_start()


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
    - waits until BLIP is RESET

5. power on the ESCs

6. Start test by typing START

"""
