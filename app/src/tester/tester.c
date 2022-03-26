#include <logging/log.h>
#include <shell/shell.h>

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
 * TODO: pass params through shell? or some congig macros?
 * - shell more tedious but shoudl force to write it down and avoid over writes...?
 * - config file easier but ...
 *
 *
 * params:
 * - FOLDER NAME
 *
 * - TEST DESCRIPTION (propeller, damage, ..., date)
 *
 * - NUMBER OF THROTTLE SPEEDS
 *   - THROTTLE SPEEDS
 * - DURATION OF EACH MEASUREMENT
 * - DURATION BETWEEN MEASUREMENTS
 *
 * - NUMBER OF SETS (1 run over all defined speeds)
 *
 * - // TODO: NAMING SCHEME
 * -
 */

/* -------------------------------------------------------------------------- */

/**
 * TEST WORKFLOW
 *
 * - Preliminaries:
 *   - ESCs have been calibrated
 *   - double check HW, is all mounted and safe
 *   - SD card should be cleaned and inside the device
 *   - change logging priority to ERR/WRN
 *   - ...
 *
 * 1. ALL turned off
 * 2. flash and reset blib
 * 3. test_config()
 *   - make sure no error
 *   - (will call pwm_calib, set up all needed etc...)
 * 4. power on the ESCs
 * 5. test_start()
 *
 */

/* -------------------------------------------------------------------------- */

static const uint32_t motor_speeds[] = {TEST_MOTOR_SPEEDS};
static int motor_speed_count = sizeof(motor_speeds) / sizeof(motor_speeds[0]);

static bool tester_ready = false;

/**
 * @brief Create a folder
 * @return   0 on success
 * @return < 0 on error
 */
int create_test_folder(const char *path)
{
    int res = disk_create_folder(path);
    if (res)
    {
        printk("Failed to create %s folder\n", path);
    }
    return res;
}

/* -------------------------------------------------------------------------- */
/** @brief Create a folder */
int save_config()
{
    // TODO: move into disk
    char path[100];
    snprintf(path, sizeof(path), "%s/%s/%s", DISK_MOUNT_PT, TEST_FOLDER_NAME, CONFIG_FILE_NAME);
    disk_open_file(path);

    static char config[1000];
    int written = snprintf(config, sizeof(config) - 1, TEST_DESCRIPTION);
    printk("writen: %d\n", written);
    disk_write_file(config, written);

    disk_close_file();

    return 0;
}
/* -------------------------------------------------------------------------- */

int tester_config(void)
{
    tester_ready = false;

    int res = 0;

    // CREATE TEST FOOLDER
    create_test_folder(TEST_FOLDER_NAME);
    if (res)
    {
        return -1;
    }

    // WRITE METADATA
    save_config();

    // INIT PWM
    res = pwm_init();
    if (res)
    {
        return -1;
    }

    // ARM ESC
    // pwm_arm();

    return 0;
}

/* -------------------------------------------------------------------------- */

/* create a test_path, test_path = ... + test_name */
void test_path_get(size_t t_num, size_t set_size, char *test_path)
{
    static const char test_types[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'};
    char test_type = test_types[t_num % set_size];
    int set_count = t_num / set_size;
    // TODO: BEWARE FILE/PATH? NAME CANT EXCEED SOME LENGTH 12?
    sprintf(test_path, "%s/P_%c_%04d.csv", TEST_FOLDER_NAME, test_type, set_count);
}

/* -------------------------------------------------------------------------- */

int tester_start(void)
{
    printk("tester_start: TEST_SET_COUNT = %d, MOTOR_SPEED_COUNT = %d\n\n", TEST_SET_COUNT, motor_speed_count);

    // CHECK CONFIG DONE AND OK

    char test_path[100] = "";
    uint32_t motor_speed = 0;

    for (size_t t = 0; t < (TEST_SET_COUNT - TEST_SET_COUNT % motor_speed_count); t++)
    {
        // STOP test if overrun/overflow occured
        if (fifo_overrun || ring_buffer_insufficient_memory)
        {
            printk("Stopping test: fifo_overrun: %d, ring_buffer_insufficient_memory: %d",
                   fifo_overrun, ring_buffer_insufficient_memory);

            pwm_set_throttle(0);
            return -1;
        }
        test_path_get(t, motor_speed_count, test_path);
        motor_speed = motor_speeds[t % motor_speed_count];
        printk("test: %04d ~ %s\n", t, test_path);

        // ENABLE WRITER
        enable_writer(test_path);

        // START MOTOR
        pwm_set_throttle(motor_speed);

        // RAMP UP PAUSE
        // k_yield();
        k_msleep(3000);

        // ENABLE ACCELEROMETER
        enable_accelerometer();

        // TEST ... WAIT
        k_msleep(6 * 1000);

        // DISABLE ACCELEROMETER
        disable_accelerometer();

        // DISABLE WRITER (GIVE ENOUGH TIME TO READ ALL OF BUFFER)
        k_msleep(2000);
        disable_writer();

        // STOP MOTOR? TODO: START,STOP, BREAK vs START, START, START
        pwm_set_throttle(0);
        // printk("gpio_sem take: limit=%d, count=%d\n", gpio_sem.limit, gpio_sem.count);
        k_msleep(2000);
    }
    return 0;
}

/* -------------------------------------------------------------------------- */

static int cmd_pwm_tester_start(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    tester_start();

    return 0;
}

SHELL_CMD_REGISTER(tester_start, NULL, "TODO:", cmd_pwm_tester_start);

static int cmd_tester_config(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    tester_config();

    return 0;
}

SHELL_CMD_REGISTER(tester_config, NULL, "tester_config: create test folder, write metadata, pwm init, ...", cmd_tester_config);

// TODO: REMOVE
#include <stdio.h>
#include <stdlib.h>
static int cmd_main(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    printk("%s", argv[1]);
    enable_writer(argv[1]);
    k_msleep(3000);
    enable_accelerometer();
    k_msleep(atoi(argv[2]) * 1000);
    disable_accelerometer();
    k_msleep(2000);
    disable_writer();

    return 0;
}
SHELL_CMD_ARG_REGISTER(main, NULL, "...", cmd_main, 3, 0);
