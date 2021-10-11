#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/printk.h>

// Get a node identifier from aliases
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)

// TODO: should check DT_NODE_HAS_STATUS(LED0_NODE, okay)...

#define STACKSIZE 1024
#define PRIORITY 7

struct led
{
    struct gpio_dt_spec spec;
    const char *gpio_pin_name;
};

typedef struct blink_params
{
    uint32_t id;
    uint32_t count;
    uint32_t sleep_ms;
} blink_params;

static const struct led led0 = {
    .spec = GPIO_DT_SPEC_GET_OR(LED0_NODE, gpios, {0}),
    .gpio_pin_name = DT_PROP_OR(LED0_NODE, label, ""),
};

static const struct led led1 = {
    .spec = GPIO_DT_SPEC_GET_OR(LED1_NODE, gpios, {0}),
    .gpio_pin_name = DT_PROP_OR(LED1_NODE, label, ""),
};

static const struct led leds_list[] = {led0, led1};
static const blink_params blink_params_list[] = {
    {.id = 0, .count = 20, .sleep_ms = 1500},
    {.id = 1, .count = 30, .sleep_ms = 250},
};

int set_up_leds();
void toggle_led(const struct led *led, uint32_t sleep_ms, uint32_t id, bool on);
void blink(blink_params *params);

void main(void)
{
    printk("Hello World! %s\n", CONFIG_BOARD);

    if (set_up_leds())
    {
        return;
    }
}

int set_up_leds(void)
{
    for (size_t i = 0; i < sizeof(leds_list) / sizeof(struct led); i++)
    {

        const struct gpio_dt_spec *spec = &leds_list[i].spec;
        int ret;

        if (!device_is_ready(spec->port))
        {
            printk("Error: %s device is not ready\n", spec->port->name);
            return 1;
        }

        ret = gpio_pin_configure_dt(spec, GPIO_OUTPUT_ACTIVE);
        if (ret != 0)
        {
            printk("Error %d: failed to configure pin %d (LED '%s')\n",
                   ret, spec->pin, leds_list[i].gpio_pin_name);
            return 1;
        }
        gpio_pin_set_dt(spec, 0);
    }
    return 0;
}

void toggle_led(const struct led *led, uint32_t sleep_ms, uint32_t id, bool on)
{
    const struct gpio_dt_spec *spec = &led->spec;
    gpio_pin_set_dt(spec, (int)on);
    k_msleep(sleep_ms);
}

void blink(blink_params *params)
{
    bool is_on = false;
    for (uint32_t i = 0; i < params->count; i++)
    {
        toggle_led(&leds_list[params->id], params->sleep_ms, params->id, !is_on);
        is_on = !is_on;
    }
}

K_THREAD_DEFINE(t0_id, STACKSIZE, blink, &blink_params_list[0], NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(t1_id, STACKSIZE, blink, &blink_params_list[1], NULL, NULL, PRIORITY, 0, 0);
