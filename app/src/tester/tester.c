#include <logging/log.h>
#include <shell/shell.h>

#include <stdio.h>
#include <stdlib.h>

#include "tester_config.h"

#include "../esc_pwm/esc_pwm.h"

#include "../writer/disk/disk.h"
#include "../writer/writer.h"

#include "../accelerometer/accelerometer.h"
#include "../accelerometer/lis2dh12/lis2dh12.h"
/* -------------------------------------------------------------------------- */

/**
 * Each test run should:
 * - create a new folder
 * - write all the metadata (params into a text file or csv)
 *
 * // TODO: pass params through shell? or some congig macros?
 *
 * // TODO: all config params
 *
 * // TODO: test files NAMING SCHEME
 */

/* -------------------------------------------------------------------------- */

/**
 * TEST WORKFLOW/STEPS
 *
 * - Preliminaries:
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
 * 3. tester_config()
 *   - create test folder
 *   - save config
 *   - ...
 *   - init pwm
 *   - ...
 *   - make sure no error
 *
 * 4. power on the ESCs
 *
 * 5. tester_start()
 *
 */

/* -------------------------------------------------------------------------- */

static const uint32_t motor_speeds[] = {TEST_MOTOR_SPEEDS};
static int motor_speed_count = sizeof(motor_speeds) / sizeof(motor_speeds[0]);

static bool tester_ready = false;
static int tester_counter = 0;
static char current_test_folder_name[128] = TEST_FOLDER_NAME;

/**
 * @brief Create a folder
 * @return   0 on success
 * @return < 0 on error
 */
int create_test_folder()
{
    snprintf(current_test_folder_name, sizeof(current_test_folder_name), "%s_%02d", TEST_FOLDER_NAME, tester_counter);
    int res = disk_create_folder(current_test_folder_name);

    if (res)
    {
        printk("Failed to create %s folder\n", current_test_folder_name);
    }
    return res;
}

/** @brief Save config into the test folder */
int save_config()
{
    // TODO: move into disk
    char path[100];
    snprintf(path, sizeof(path), "%s/%s/%s", DISK_MOUNT_PT, current_test_folder_name, CONFIG_FILE_NAME);
    disk_open_file(path);

    static char config[1000];
    int written = snprintf(config, sizeof(config) - 1, TEST_DESCRIPTION);
    // printk("writen: %d\n", written);
    disk_write_file(config, written);

    disk_close_file();

    return 0;
}
/* -------------------------------------------------------------------------- */

int tester_config(void)
{
    tester_ready = false;

    printk("motor speeds: ");
    for (int i = 0; i < motor_speed_count; i++)
    {
        printk("%d%%, ", motor_speeds[i]);
    }
    printk("\n");
    printk("prop states: %s\n", TEST_PROPELER_STATES);

    int res = 0;

    // CREATE TEST FOOLDER
    create_test_folder();
    if (res)
    {
        return -1;
    }

    // WRITE METADATA
    res = save_config();
    if (res)
    {
        return -1;
    }

    // INIT PWM
    res = pwm_init();
    if (res)
    {
        return -1;
    }

    // ARM ESC (set throttle to 0)
    pwm_arm();

    tester_counter++;
    tester_ready = true;

    return 0;
}

/* -------------------------------------------------------------------------- */

/* create a test_path, test_path = ... + test_name */
void test_path_get(size_t t_num, size_t set_size, char *test_path)
{
    int set_count = t_num / set_size;

    // TODO: BEWARE FILE/PATH? NAME CANT EXCEED SOME LENGTH 12?
    sprintf(test_path, "%s/%s_%03dt_%04d.csv", current_test_folder_name, TEST_FILE_PREFIX, motor_speeds[t_num % set_size], set_count);
}

/* -------------------------------------------------------------------------- */

int tester_start(void)
{
    // TODO: on fail/error write to test folder, completed tests, error cause, etc.
    if (tester_ready == false)
    {
        printk("TESTER not ready, make sure tester_cofig() called and completed succesfully\n.");
        return -1;
    }

    printk("tester_start: TEST_SET_COUNT = %d, MOTOR_SPEED_COUNT = %d, => total tests = %d\n\n", TEST_SET_COUNT, motor_speed_count, TEST_SET_COUNT * motor_speed_count);

    printk("motor speeds: ");
    for (int i = 0; i < motor_speed_count; i++)
    {
        printk("%d%%, ", motor_speeds[i]);
    }
    printk("\n");

    static char test_path[100] = "";
    uint32_t motor_speed = 0;

    for (size_t t = 0; t < TEST_SET_COUNT * motor_speed_count; t++)
    {
        // STOP test if overrun/overflow occured
        if (fifo_overrun || ring_buffer_insufficient_memory)
        {
            printk("Stopping test: fifo_overrun: %d, ring_buffer_insufficient_memory: %d",
                   fifo_overrun, ring_buffer_insufficient_memory);

            pwm_set_throttle(0);
            tester_ready = false;

            fifo_overrun = ring_buffer_insufficient_memory = false; // TODO: move to accel/lis2dh as function
            return -1;
        }

        /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

        test_path_get(t, motor_speed_count, test_path);
        motor_speed = motor_speeds[t % motor_speed_count];
        printk("test: %04d -> %s\n", t, test_path);

        /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

        // ENABLE WRITER
        enable_writer(test_path);

        // START MOTOR
        pwm_set_throttle(motor_speed);

        // RAMP UP PAUSE
        k_msleep(MSEC_PER_SEC * TEST_MEASUREMENT_PAUSE_DURATION_SEC);

        // ENABLE ACCELEROMETER
        enable_accelerometer();

        // TEST ... WAIT
        k_msleep(MSEC_PER_SEC * TEST_MEASUREMENT_DURATION_SEC);

        // DISABLE ACCELEROMETER
        disable_accelerometer();

        // DISABLE WRITER (GIVE ENOUGH TIME TO READ ALL OF BUFFER)
        k_msleep(MSEC_PER_SEC * TEST_MEASUREMENT_PAUSE_DURATION_SEC);
        disable_writer();

        // STOP MOTOR? TODO: START,STOP,BREAK vs START,INCREASE,INCREASE,...,STOP
        pwm_set_throttle(0);

        k_msleep(1000);
    }

    tester_ready = false;
    return 0;
}

/* -------------------------------------------------------------------------- */

static int cmd_tester_config(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    int res = tester_config();
    if (res == 0)
    {
        printk("tester_config completed\n");
    }
    else
    {
        printk("tester_config error\n");
    }

    return 0;
}

SHELL_CMD_REGISTER(tester_config, NULL, "tester_config: create test folder, write metadata, pwm init, ...", cmd_tester_config);

static int cmd_pwm_tester_start(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    int res = tester_start();

    if (res == 0)
    {
        printk("test completed\n");
    }
    else
    {
        printk("test error\n");
    }

    return 0;
}

SHELL_CMD_REGISTER(tester_start, NULL, "tester_start: ...", cmd_pwm_tester_start);

/* -------------------------------------------------------------------------- */

// TODO: REMOVE
// static int cmd_tester_debug(const struct shell *shell, size_t argc, char **argv)
// {
//     ARG_UNUSED(argc);
//     ARG_UNUSED(argv);
//     printk("%s", argv[1]);
//     enable_writer(argv[1]);
//     k_msleep(3000);
//     enable_accelerometer();
//     k_msleep(atoi(argv[2]) * 1000);
//     disable_accelerometer();
//     k_msleep(2000);
//     disable_writer();

//     return 0;
// }
// SHELL_CMD_ARG_REGISTER(tester_debug, NULL, "...", cmd_tester_debug, 3, 0);
