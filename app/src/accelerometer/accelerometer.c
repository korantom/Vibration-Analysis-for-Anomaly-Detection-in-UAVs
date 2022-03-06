#include "accelerometer.h"

/* mutex + condvar */
K_MUTEX_DEFINE(accelerometer_mutex);     // mutex
K_CONDVAR_DEFINE(accelerometer_condvar); // condvar
static bool is_enabled = false;          // condition

/* define all variables and structs */
// ...

LOG_MODULE_REGISTER(accelerometer);

/* public interface ---------------------------------------------------------- */

void config_accelerometer()
{
    LOG_INF("config_accelerometer()");
}

void enable_accelerometer()
{
    /* will be called from another thread (shell, main, ...) */
    LOG_INF("enable_accelerometer()");
    LOG_INF("condvar signal");
    is_enabled = true;
    k_condvar_signal(&accelerometer_condvar);
}

void disable_accelerometer()
{
    /* will be called from another thread (shell, main, ...) */
    LOG_INF("disable_accelerometer()");
    is_enabled = false;
}

/* private functions --------------------------------------------------------- */

static void _accelerometer();

static void _accelerometer_loop()
{

    LOG_INF("thr: %p, Accelerometer thread started", k_current_get());

    while (1)
    {
        LOG_INF("mtx     locking");
        k_mutex_lock(&accelerometer_mutex, K_FOREVER);
        LOG_INF("mtx     locked");

        if (!is_enabled)
        {
            LOG_INF("condvar wait");
            k_condvar_wait(&accelerometer_condvar, &accelerometer_mutex, K_FOREVER);
            LOG_INF("condvar unblocked");
        }

        /* Perform (critical) service functionality ... */
        _accelerometer();

        LOG_INF("mtx     unlocking");
        k_mutex_unlock(&accelerometer_mutex);
        LOG_INF("mtx     unlocked");

        LOG_INF("yielding");
        k_yield();
    }
}

/** @brief Service functionality */
static void _accelerometer()
{
    LOG_INF("_accelerometer() ");
    k_msleep(1000);
}
/* thread creation ----------------------------------------------------------- */

K_THREAD_DEFINE(accelerometer_tid, ACCELEROMETER_STACK_SIZE,
                _accelerometer_loop, NULL, NULL, NULL,
                ACCELEROMETER_THREAD_PRIORITY, 0, 0);

static int _init_accelerometer(const struct device *_)
{
    LOG_INF("thr: %p, SYS_INIT() ACCELEROMETER_SYS_INIT_PRIORITY: %d", k_current_get(), ACCELEROMETER_SYS_INIT_PRIORITY);
    config_accelerometer();
    return 0;
}

SYS_INIT(_init_accelerometer, APPLICATION, ACCELEROMETER_SYS_INIT_PRIORITY);

/* shell commands ------------------------------------------------------------ */

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

SHELL_CMD_REGISTER(enable_accelerometer, NULL, "Enable accelerometer (signal conditional variable)", cmd_enable_accelerometer);
SHELL_CMD_REGISTER(disable_accelerometer, NULL, "Disable accelerometer (wait on conditional variable)", cmd_disable_accelerometer);
