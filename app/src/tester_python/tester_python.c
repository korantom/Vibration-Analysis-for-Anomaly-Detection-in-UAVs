#include <logging/log.h>
#include <shell/shell.h>

#include <stdio.h>
#include <stdlib.h>

#include "../esc_pwm/esc_pwm.h"

#include "../accelerometer/accelerometer.h"
#include "../accelerometer/lis2dh12/lis2dh12.h"

LOG_MODULE_REGISTER(tester_python);
/* -------------------------------------------------------------------------- */
/**
 * TODO: ZEPHYR APP
 * - ...
 * TODO: PYTHON
 * - ...
 */
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
 * 3. tester_init()
 *   - ...
 *   - init pwm
 *   - arm pwm
 *   - ...
 *
 * 4. Run python test controller script
 *  - ...
 *
 * 5. power on the ESCs
 *
 * 6. ...
 *
 */

/* -------------------------------------------------------------------------- */

static bool tester_ready = false;

int tester_init(void)
{

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

    tester_ready = true;

    return 0;
}

/* -------------------------------------------------------------------------- */
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
        written = snprintf(out_str, sizeof(out_str) - 1, "%.6lf, %.6lf, %.6lf\r\n", x, y, z);

        // ...
        printk("_data_: %s\n", out_str);

        // FINISH
        int finish_ret = ring_buf_get_finish(&lis2dh12_ring_buf, ringbuf_aloc_size);
        if (finish_ret)
        {
            break;
        }
    }
    /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
}
/* -------------------------------------------------------------------------- */
// DUMP ALL AT ONCE
void single_test_dump(uint32_t throttle_percentage, uint32_t ramp_up_duration_sec, uint32_t test_duration_sec, uint32_t pause_duration_sec)
{
    if (tester_ready == false)
    {
        LOG_WRN("TESTER not ready, make sure tester_init() called and completed succesfully.\n");
        return;
    }

    // START MOTOR
    pwm_set_throttle(throttle_percentage);

    // RAMP UP PAUSE
    k_msleep(ramp_up_duration_sec * MSEC_PER_SEC);

    // ENABLE ACCELEROMETER
    enable_accelerometer();

    // TEST ... WAIT
    k_msleep(test_duration_sec * MSEC_PER_SEC);

    // DISABLE ACCELEROMETER
    disable_accelerometer();

    // STOP MOTOR
    pwm_set_throttle(0);

    // CHECK NO OVERRUN OR MISSING DATA
    if (fifo_overrun || ring_buffer_insufficient_memory)
    {
        LOG_ERR("fifo_overrun: %d, ring_buffer_insufficient_memory: %d", fifo_overrun, ring_buffer_insufficient_memory);
    }

    // DUMP
    dump_ring_buffer_to_console();

    k_msleep(pause_duration_sec * MSEC_PER_SEC);
}

/* -------------------------------------------------------------------------- */
// TODO: continuous write

/* -------------------------------------------------------------------------- */

static int cmd_single_test_dump(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    uint32_t throttle_percentage = atoi(argv[1]);
    uint32_t ramp_up_duration_sec = atoi(argv[2]);
    uint32_t test_duration_sec = atoi(argv[3]);
    uint32_t pause_duration_sec = atoi(argv[4]);

    single_test_dump(throttle_percentage, ramp_up_duration_sec, test_duration_sec, pause_duration_sec);
    return 0;
}

SHELL_CMD_ARG_REGISTER(single_test_dump, NULL, "...", cmd_single_test_dump, 5, 0);

static int cmd_tester_init(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    int res = tester_init();
    return 0;
}

SHELL_CMD_REGISTER(tester_init, NULL, "...", cmd_tester_init);
