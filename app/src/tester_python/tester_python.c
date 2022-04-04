#include <shell/shell.h>
#include <sys/printk.h>

#include <stdio.h>
#include <stdlib.h>

#include "../esc_pwm/esc_pwm.h"

#include "../accelerometer/accelerometer.h"
#include "../accelerometer/lis2dh12/lis2dh12.h"

// LOG_MODULE_REGISTER(tester_python); // printk all

/* -------------------------------------------------------------------------- */

/**
 * TEST WORKFLOW/STEPS
 *
 * - 0. Preliminaries:
 *   - ESCs have been calibrated
 *   - double check HW, is all mounted and safe
 *   - SD card should be wiped and inside the device
 *   - change logging priority to ERR/WRN
 *   - ...
 *
 * 1. ALL turned off
 *
 * 2. FLASH and RESET blib
 *
 * 3. Run/Start python test controller script
 *   - 1) load tester config, create file structure, ...
 *   - 2) RESET blip? or uart_write("\r")? -> to get prompt = ready for write
 *
 *   - 3) uart_write( "tester_init" )
 *       - will init pwm
 *       - arm_esc (set pwm to low)
 *       - ...
 *   - 4) wait for prompt <= tester_init completed (will set tester_ready, if error wont allow to start tests)
 *
 * 4. power on the ESCs
 *
 * 3. in python script
 *   - 5) Start tests
 *        - in a FOR cycle uart_write('single_test throttle durations ....') for each test
 *            - filter all incoming for data ...
 *            - wait for prompt
 *            - write all filtered/buffered data
 *            - repeat
 */

/* -------------------------------------------------------------------------- */

static bool tester_ready = false;
#define DATA_PREFIX "_data_:"

/* -------------------------------------------------------------------------- */

/** @brief Init and arm pwm (set pwm to low), set tester_ready to true if no error */
int tester_init(void);

/** @brief prints entire content of ringbuffer to the console, line by line, each line prefixed with DATA_PREFIX */
void dump_ring_buffer_to_console();

/** @brief perfform 1 test (start motor, enable accel, wait, disable accel, stop motor, dump all)*/
void single_test_dump(uint32_t throttle_percentage, uint32_t ramp_up_duration_sec, uint32_t test_duration_sec, uint32_t pause_duration_sec);

// TODO: shell wrapper commands around each ...

/* -------------------------------------------------------------------------- */

static int cmd_tester_infinite_print(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    int i = 0;
    while (1)
    {
        k_msleep(2000);
        printk("foo %04d\n", i++);
    }
    return 0;
}

SHELL_CMD_REGISTER(tester_infinite_print, NULL, "...", cmd_tester_infinite_print);

/* -------------------------------------------------------------------------- */
