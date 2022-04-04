from datetime import datetime
from typing import List

from dataclasses import dataclass, field
from dataclasses_json import dataclass_json
from git import Repo

from serial_wrapper import *

################################################################################

# TODO: split config into subclasses
@dataclass_json
@dataclass
class TesterConfig:
    """Data class containing config of the test (data acquisition ...)."""

    # paths/names config
    test_folder_path: str = "./Tests"
    test_folder_name: str = "TEST"
    test_config_file_name: str = "config.json"
    test_files_prefix: str = "T"

    # drone config
    test_voltage: int = 20
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
    test_measurements_count: int = 20
    test_motor_throttle_values: List[int] = field(
        default_factory=lambda: [10, 15, 25, 50, 75, 90, 100, 0]
    )
    test_ramp_up_duration_sec: int = 2
    test_measurement_duration_sec: int = 8
    test_pause_duration_sec: int = 2

    # shell config/commands
    # TODO: as a list of commands to itterate over
    # test_config_shell_commands: List[str]
    # test_test_shell_command: str = "single_test_dump"

    # TODO:
    path: str = field(init=False)
    git_hash: str = field(init=False)

    ############################################################################

    def __post_init__(self):
        # Assembles/Create as uniqe path for given test (set of measurements)
        dt = datetime.now()
        dt_str = dt.strftime("%Y_%m_%d_%H_%M_%S")
        self.path = f"{self.test_folder_path}/{self.test_folder_name}_{dt_str}"

        # stores current commit hash
        self.git_hash = Repo("./").git.rev_parse("HEAD")


################################################################################

if __name__ == "__main__":

    tester_config = TesterConfig()

    print(f"{tester_config=}")
