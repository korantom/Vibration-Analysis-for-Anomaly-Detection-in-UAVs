#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>

#include "common.h"

/* -------------------------------------------------------------------------- */

void main(void)
{
    printk("main\n");
}

static int _init_main(const struct device *_)
{
    printk("main.c SYS_INIT, thread: %p, priority: %d\n", k_current_get(), MAIN_SYS_INIT_PRIORITY);
    return 0;
}

SYS_INIT(_init_main, APPLICATION, MAIN_SYS_INIT_PRIORITY);
