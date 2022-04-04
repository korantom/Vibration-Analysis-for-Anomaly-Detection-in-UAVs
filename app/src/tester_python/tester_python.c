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
static int cmd_tester_echo_arguments(const struct shell *shell, size_t argc, char **argv)
{

    k_msleep(100); // give enough time for echo, before following prints
    printk("argc: %d\n", argc);
    for (size_t i = 1; i < argc; i++)
    {
        printk("\t- argv[%d]: %d\n", i, atoi(argv[i]));
    }

    return 0;
}

SHELL_CMD_ARG_REGISTER(tester_echo_arguments, NULL, "echo up to 4 int arguments", cmd_tester_echo_arguments, 1, 4);

/* -------------------------------------------------------------------------- */

int tester_init(void)
{
    tester_ready = false;

    // INIT PWM
    int res = pwm_init();
    if (res)
    {
        return -1;
    }

    // ARM ESC (set throttle to 0)
    res = pwm_arm();
    if (res)
    {
        return -1;
    }

    // Safety wait before any test starts
    k_msleep(10 * MSEC_PER_SEC);

    tester_ready = true;

    return 0;
}

/* -------------------------------------------------------------------------- */

// TODO dump raw bytes, let python handle conversion?

void dump_ring_buffer_to_console()
{
    static char out_str[256];
    int written = 0;
    uint8_t *data;

    const double multiplier = 0.001197100830078125f;
    double x, y, z;

    /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

    while (1)
    {
        // CLAIM
        int ringbuf_aloc_size = ring_buf_get_claim(&lis2dh12_ring_buf, &data, 6);
        if (ringbuf_aloc_size != 6)
        {
            break;
        }

        // raw * (8*9.80665/1024/64)
        x = *(int16_t *)&data[0] * multiplier;
        y = *(int16_t *)&data[2] * multiplier;
        z = *(int16_t *)&data[4] * multiplier;
        // TODO: \r vs \r\n vs \n
        written = snprintf(out_str, sizeof(out_str) - 1, "%.6lf, %.6lf, %.6lf\n", x, y, z);

        // ...
        printk("%s%s\n", DATA_PREFIX, out_str);

        // FINISH
        int finish_ret = ring_buf_get_finish(&lis2dh12_ring_buf, ringbuf_aloc_size);
        if (finish_ret)
        {
            break;
        }
    }
}

/* -------------------------------------------------------------------------- */
