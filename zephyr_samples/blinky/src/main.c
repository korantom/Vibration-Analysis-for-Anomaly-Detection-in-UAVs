#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#define SLEEP_TIME_MS 1000

/* Check the board.dts file to se all defined aliases. */
/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0) // Get a node identifier from /aliases

#if DT_NODE_HAS_STATUS(LED0_NODE, okay) // Check if the node identifier exist in device tree and has okay status
#define LED0 DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0 ""
#define PIN 0
#define FLAGS 0
#endif

/* example of how a node identifier is concatenated to create a valid/defined macro */
#define DUMMY(node_id, status) DT_CAT(node_id, _STATUS_##status)
#define DUMMY2(node_id, status) node_id##_STATUS_##status				  // won't work
#define DUMMY3(node_id, status) DT_CAT(node_id, DT_CAT(_STATUS_, status)) // won't work

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE_MACRO(str) QUOTE(str)

void main(void)
{
	// error: DT_ALIAS(led0) is expected to be used as param of another macro or concatenated with some macro to actually return a value
	// printk("%s\n", DT_ALIAS(led0));
	printk("%s\n", QUOTE(DT_ALIAS(led0)));
	// has to be nested in order to expand
	printk("%s\n", EXPAND_AND_QUOTE_MACRO(DT_ALIAS(led0)));

	// example using LED0_NODE identifier
	printk("%d\n", DT_NODE_HAS_STATUS(LED0_NODE, okay));

	printk("%d\n", DUMMY(LED0_NODE, okay)); // expands to DT_N_S_leds_S_led_0_STATUS_okay
	printk("%d\n", DT_N_S_leds_S_led_0_STATUS_okay);

	printk("%d\n", IS_ENABLED(1));
	printk("%d\n", IS_ENABLED(SOME_NOT_DEFINED_MACRO));

	/* ---------------------------------------------------------------------- */

	printk("sizeof(void *) * 8 = %u\n", sizeof(void *) * 8);
	printk("LED0 GPIO label %s\n", LED0);
	printk("LED0 GPIO PIN %d\n", PIN);
	printk("LED0 GPIO FLAGS %d\n", FLAGS);

	/* ---------------------------------------------------------------------- */

	const struct device *dev;
	bool led_is_on = true;
	int ret;

	// Get the device structure for a driver by name
	dev = device_get_binding(LED0);
	if (dev == NULL)
	{
		printk("Device not found.\n");
		return;
	}
	printk("Device found:\n");
	printk("\tname: %s\n", dev->name);
	printk("\tconfig at: %p \n", dev->config);
	printk("\tapi at: %p \n", dev->api);

	/* ---------------------------------------------------------------------- */

	ret = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
	if (ret < 0)
	{
		return;
	}

	// Blink led by seting a value on a pin every x ms
	while (1)
	{
		gpio_pin_set(dev, PIN, (int)led_is_on);
		led_is_on = !led_is_on;
		k_msleep(SLEEP_TIME_MS);
	}
}
