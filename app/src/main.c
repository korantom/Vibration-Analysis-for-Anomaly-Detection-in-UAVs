#include <device.h>
#include <sys/printk.h>
#include <zephyr.h>

#include "common.h"

/* -------------------------------------------------------------------------- */

void main(void)
{
    printk("Main: Hello world\n");
}

/* -------------------------------------------------------------------------- */

static int _init_main(const struct device *_)
{
    printk("main.c thread: %p, SYS_INIT priority: %d\n", k_current_get(), MAIN_SYS_INIT_PRIORITY);
    return 0;
}

SYS_INIT(_init_main, APPLICATION, MAIN_SYS_INIT_PRIORITY);
