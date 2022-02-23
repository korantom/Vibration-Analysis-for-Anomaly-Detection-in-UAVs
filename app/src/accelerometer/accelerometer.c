#include "accelerometer.h"
#include "lis2dh12/lis2dh12.h"

/* mutex + condvar */
K_MUTEX_DEFINE(accelerometer_mutex);     // mutex used
K_CONDVAR_DEFINE(accelerometer_condvar); // cond var
static bool is_enabled = false;          // condition

/* define all variables and structs */
// ...

/* public interface ---------------------------------------------------------- */

void config_accelerometer()
{
    lis2dh12_init();

    lis2dh12_config();

    lis2dh12_enable_interrupt();
}

void enable_accelerometer()
{
    /* will be called from another thread (shell, main, ...) */
    // if (enabled) { return; } // ?
    is_enabled = true;
    // TODO: empty ring buffer
    lis2dh12_enable_fifo(); // should be called before signaling?
    k_condvar_signal(&accelerometer_condvar);
    // lis2dh12_enable_fifo(); // or after?
}

void disable_accelerometer()
{
    /* will be called from another thread (shell, main, ...) */
    is_enabled = false;
}

/* private functions --------------------------------------------------------- */

static void _accelerometer_read();

static void _accelerometer_loop()
{

    printk("Accelerometer Thread A: %p started\n", k_current_get());

    while (1)
    {
        printk("A: locking mutex\n");
        k_mutex_lock(&accelerometer_mutex, K_FOREVER);
        printk("A: locked mutex\n");

        if (!is_enabled)
        {
            printk("A: disabled\n");
            printk("A: condvar wait\n");
            k_condvar_wait(&accelerometer_condvar, &accelerometer_mutex, K_FOREVER);
        }
        printk("A: enabled\n");

        /* Perform (critical) service functionality ... */
        _accelerometer_read();

        printk("A: unlocking mutex\n");
        k_mutex_unlock(&accelerometer_mutex);
        printk("A: unlocked mutex\n");
        printk("\n");

        k_yield();
    }
}

static void _accelerometer_read()
{
    /* Service functionality */
    printk("A: _accelerometer_read\n");

    int timout_res = lis2dh12_read_buffer(K_MSEC(500));
    if (timout_res <= 0)
    {
        printk("A: lis2dh12_read_buffer timeout %d\n", timout_res);
    }
}
/* thread creation ----------------------------------------------------------- */

K_THREAD_DEFINE(accelerometer_tid, ACCELEROMETER_STACK_SIZE,
                _accelerometer_loop, NULL, NULL, NULL,
                ACCELEROMETER_THREAD_PRIORITY, 0, 0);

static int _init_accelerometer(const struct device *_)
{
    printk("accelerometer.c SYS_INIT, thread ?: %p\n", k_current_get());
    config_accelerometer();
    return 0;
}

SYS_INIT(_init_accelerometer, APPLICATION, ACCELEROMETER_SYS_INIT_PRIORITY);

/* shell commands ------------------------------------------------------------ */
// TODO: move to a different file?

// TODO: only one toggle command with param on/off?

static int cmd_enable_accelerometer(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    enable_accelerometer();
    return 0;
}

static int cmd_disable_accelerometer(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    disable_accelerometer();
    return 0;
}

SHELL_CMD_REGISTER(enable_accelerometer, NULL, "Enable accelerometer", cmd_enable_accelerometer);
SHELL_CMD_REGISTER(disable_accelerometer, NULL, "Disable accelerometer", cmd_disable_accelerometer);
