#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/pwm.h>

#define PWM_LED0_NODE DT_ALIAS(pwm_led0)

#if DT_NODE_HAS_STATUS(PWM_LED0_NODE, okay)
#define PWM_LABEL DT_PWMS_LABEL(PWM_LED0_NODE)
#define PWM_CTLR_NODE DT_PWMS_CTLR(PWM_LED0_NODE)
#define PWM_CHANNEL DT_PWMS_CHANNEL(PWM_LED0_NODE)
#define PWM_FLAGS DT_PWMS_FLAGS(PWM_LED0_NODE)
#else
#error "Unsupported board: pwm-led0 devicetree alias is not defined"
#define PWM_LABEL ""
#define PWM_CTLR_NODE DT_INVALID_NODE
#define PWM_CHANNEL 0
#define PWM_FLAGS 0
#endif

#define MIN_PERIOD_USEC (USEC_PER_SEC / 64U)
#define MAX_PERIOD_USEC USEC_PER_SEC

/*
SEC  - second		- 1
MSEC - millisecond  - 10^-3
USEC - microsecond  - 10^-6
NSEC - nanosecond   - 10^-9
*/

unsigned int pow_of_two(unsigned int exp)
{
	return 1 << exp;
}

void main(void)
{
	printk("PWM-based blinky\n");

	const struct device *pwm_dev;
	uint32_t max_period;
	uint32_t period;
	uint32_t pulse_width;

	/* ---------------------------------------------------------------------- */

	//a
	//pwm_dev = device_get_binding(PWM_LABEL); // runtime get of device -> have to check if NULL not returned
	// if (pwm_dev == NULL)
	// {
	// 	printk("Device not found.\n");
	// 	return;
	// }

	//b
	pwm_dev = DEVICE_DT_GET(PWM_CTLR_NODE); // won't compile if node doesn't exist
	// however unlike device_get_binding, DEVICE_DT_GET doesn't check readiness
	if (!device_is_ready(pwm_dev))
	{
		printk("Error: PWM device %s is not ready\n", pwm_dev->name);
		return;
	}

	/* ---------------------------------------------------------------------- */
	// calibration: find max possible period (min possible freq)
	// try pwm_pin_set_usec until it works or max_period is too small
	printk("Calibrating for channel %d...\n", PWM_CHANNEL);

	max_period = MAX_PERIOD_USEC;
	printk("initial max_period: %u usec\n", max_period);
	while (pwm_pin_set_usec(pwm_dev, PWM_CHANNEL, max_period, max_period / 2U, PWM_FLAGS))
	{
		max_period /= 2U;
		// max_period -= 1000U;
		printk("\t max_period/2 -> %u usec\n", max_period);
		if (max_period < (4U * MIN_PERIOD_USEC))
		{
			printk("Error: PWM device "
				   "does not support a period at least %u\n",
				   4U * MIN_PERIOD_USEC);
			return;
		}
	}
	printk("Done calibrating; maximum/minimum periods %u/%u usec\n", max_period, MIN_PERIOD_USEC);

	/* ---------------------------------------------------------------------- */
	// blinking example
	// period=max_period;
	// pulse_width=period/pow_of_two(3);
	// pwm_pin_set_usec(pwm_dev, PWM_CHANNEL, period, pulse_width, PWM_FLAGS);

	/* ---------------------------------------------------------------------- */
	// dimming example
	printk("Dimming example\n");
	period = 5000U;
	pulse_width = period;
	pwm_pin_set_usec(pwm_dev, PWM_CHANNEL, period, pulse_width, PWM_FLAGS);
	while (1)
	{
		for (size_t i = 2; i < 14; i++)
		{
			pulse_width = period / pow_of_two(i);
			pwm_pin_set_usec(pwm_dev, PWM_CHANNEL, period, pulse_width, PWM_FLAGS);
			k_sleep(K_MSEC(100U));
		}
		for (size_t i = 14; i > 1; i--)
		{
			pulse_width = period / pow_of_two(i);
			pwm_pin_set_usec(pwm_dev, PWM_CHANNEL, period, pulse_width, PWM_FLAGS);
			k_sleep(K_MSEC(100U));
		}
	}
	/* ---------------------------------------------------------------------- */
	// blinking dimmed led example
	// printk("Blinking dimmed example\n");
	// period = 5000U;
	// pulse_width = period / pow_of_two(10);
	// pwm_pin_set_usec(pwm_dev, PWM_CHANNEL, period, pulse_width, PWM_FLAGS);
	// bool led_is_on = true;
	// while (1)
	// {
	// 	if (led_is_on)
	// 		pwm_pin_set_usec(pwm_dev, PWM_CHANNEL, period, pulse_width, PWM_FLAGS);
	// 	else
	// 		pwm_pin_set_usec(pwm_dev, PWM_CHANNEL, period, 0, PWM_FLAGS);
	// 	led_is_on = !led_is_on;
	// 	k_msleep(500);
	// }
}
