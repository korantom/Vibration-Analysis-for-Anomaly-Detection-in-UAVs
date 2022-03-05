#include <device.h>
#include <logging/log.h>
#include <sys/printk.h>
#include <zephyr.h>

#include "common.h"

/* -------------------------------------------------------------------------- */

LOG_MODULE_REGISTER(main);

/* -------------------------------------------------------------------------- */

void main(void)
{
    printk("Main: Hello world\n");
}

/* -------------------------------------------------------------------------- */

static int _init_main(const struct device *_)
{
    LOG_INF("thr: %p, SYS_INIT() MAIN_SYS_INIT_PRIORITY: %d\n", k_current_get(), MAIN_SYS_INIT_PRIORITY);

    return 0;
}

SYS_INIT(_init_main, APPLICATION, MAIN_SYS_INIT_PRIORITY);
