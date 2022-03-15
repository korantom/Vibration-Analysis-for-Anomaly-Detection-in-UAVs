#include "esc_pwm.h"
#include <stdio.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(esc_pwm);

/* -------------------------------------------------------------------------- */
struct pwm_node
{
    const struct device *device;
    int channel; // TODO: pin? channel? check DeviceTree
    int flags;
    // ...
};

static const struct pwm_node pwm_node0 = {
    .device = DEVICE_DT_GET(PWM0_CTLR),
    .channel = PWM0_CHANNEL,
    .flags = PWM0_FLAGS,
};

static const struct pwm_node pwm_node1 = {
    .device = DEVICE_DT_GET(PWM1_CTLR),
    .channel = PWM1_CHANNEL,
    .flags = PWM1_FLAGS,
};

static const struct pwm_node pwm_node2 = {
    .device = DEVICE_DT_GET(PWM2_CTLR),
    .channel = PWM2_CHANNEL,
    .flags = PWM2_FLAGS,
};

static const struct pwm_node pwm_node_list[] = {
    pwm_node0,
    pwm_node1,
    pwm_node2,
};

static int device_count = sizeof pwm_node_list / sizeof(struct pwm_node);

/* -------------------------------------------------------------------------- */

int pwm_init(void)
{
    LOG_INF("pwm_init(), %d devices found to init", device_count);

    for (int i = 0; i < device_count; i++)
    {
        const struct device *device = pwm_node_list[i].device;
        if (!device_is_ready(device))
        {
            LOG_ERR("PWM device[%d] (at %p) %s is not ready",
                    i, device, device->name);
            return -i;
        }
    }

    LOG_INF("pwm_init done, all %d devices ready", device_count);
    return 0;
}

/* -------------------------------------------------------------------------- */

int pwm_set(uint32_t pulse_width_usec)
{
    LOG_INF("pwm_set(%u)", pulse_width_usec);

    // TODO: check pulse width in range (?, MAX_PULSE_WIDTH_USEC)

    for (int i = 0; i < device_count; i++)
    {
        int res = pwm_pin_set_usec(pwm_node_list[i].device, pwm_node_list[i].channel,
                                   ESC_PERIOD_USEC, pulse_width_usec,
                                   pwm_node_list[i].flags);
        if (res)
        {
            LOG_ERR("PWM device[%d] (at %p) %s, unable to set pwm period, width = %d, %d, err: %d",
                    i, pwm_node_list[i].device, pwm_node_list[i].device->name, ESC_PERIOD_USEC, pulse_width_usec, res);
            return res;
        }
    }

    LOG_INF("pwm_set done");
    return 0;
}

/* -------------------------------------------------------------------------- */
int pwm_set_throttle(uint32_t percentage)
{
    uint32_t pulse_width_usec = MIN_PULSE_WIDTH_USEC + percentage * (MAX_PULSE_WIDTH_USEC - MIN_PULSE_WIDTH_USEC) / 100;

    LOG_INF("pwm_set_throttle(%u%%) = pwm_set(%u usec)", percentage, pulse_width_usec);

    return pwm_set(pulse_width_usec);
}
/* -------------------------------------------------------------------------- */

void pwm_arm()
{
    LOG_INF("pwm_arm()");

    // set pwm LOW
    pwm_set_throttle(0);

    // wait 2-5 sec
    LOG_INF("PWM_ARM_SLEEP %u ms", PWM_ARM_SLEEP_MSEC);
    k_msleep(PWM_ARM_SLEEP_MSEC);

    // ready to go?

    LOG_INF("pwm_arm done");
}

void pwm_calib()
{
    LOG_INF("pwm_calib()");

    // set pwm to new HIGH (at least MID or more)
    pwm_set_throttle(100);

    // wait 2-5 sec
    LOG_INF("PWM_CALIB_SLEEP %u ms", PWM_CALIB_SLEEP_MSEC);
    k_msleep(PWM_CALIB_SLEEP_MSEC);

    // set pwm new LOW
    pwm_set_throttle(0);

    LOG_INF("PWM_AFTER_CALIB_SLEEP %u ms", PWM_POST_CALIB_SLEEP_MSEC);
    k_msleep(PWM_POST_CALIB_SLEEP_MSEC);

    LOG_INF("pwm_calib done");
}

/* shell commands ------------------------------------------------------------ */

static int cmd_pwm_init(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    int res = pwm_init();
    printk("init -> %d\n", res);
    return 0;
}

static int cmd_pwm_arm(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    pwm_arm();

    return 0;
}

static int cmd_pwm_calib(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    pwm_calib();

    return 0;
}

SHELL_CMD_REGISTER(pwm_init, NULL, "Get all pwm device bindings, check if all devices are ready", cmd_pwm_init);
SHELL_CMD_REGISTER(pwm_arm, NULL, "Arm the ESC (!=Calibration)", cmd_pwm_arm);
SHELL_CMD_REGISTER(pwm_calib, NULL, "Calibrate the ESC", cmd_pwm_calib);

/* -------------------------------------------------------------------------- */

static int cmd_pwm_set(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    uint32_t pulse_width = atoi(argv[1]);

    printk("Seting pwm to pulse_width = %u\n", pulse_width);

    pwm_set(pulse_width);
    return 0;
}

SHELL_CMD_ARG_REGISTER(pwm_set, NULL, "Set pwm pulse width in USEC (micro seconds)", cmd_pwm_set, 2, 0);

/* -------------------------------------------------------------------------- */

static int cmd_pwm_set_throttle(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    uint32_t percantage = atoi(argv[1]);
    percantage = percantage > 100 ? 100 : percantage; // max 100
    percantage = percantage < 0 ? 0 : percantage;     // min 0 (uint => redundant)

    printk("Seting pwm to %u%%\n", percantage);

    pwm_set_throttle(percantage);
    return 0;
}

SHELL_CMD_ARG_REGISTER(pwm_set_throttle, NULL, "Set pwm pulse to x%% of range [LOW, HIGH]", cmd_pwm_set_throttle, 2, 0);
