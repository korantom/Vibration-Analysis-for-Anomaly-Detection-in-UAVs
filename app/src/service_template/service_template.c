#include "service_template.h"

/* mutex + condvar */
K_MUTEX_DEFINE(service_mutex);     // mutex
K_CONDVAR_DEFINE(service_condvar); // condvar
static bool is_enabled = false;    // condition

/* define all variables and structs */
// ...

LOG_MODULE_REGISTER(service_template);

/* public interface ---------------------------------------------------------- */

void config_service()
{
    LOG_INF("config_service()");
}

void enable_service()
{
    /* will be called from another thread (shell, main, ...) */
    LOG_INF("enable_service()");
    LOG_INF("condvar signal");
    is_enabled = true;
    k_condvar_signal(&service_condvar);
}

void disable_service()
{
    /* will be called from another thread (shell, main, ...) */
    LOG_INF("disable_service()");
    is_enabled = false;
}

/* private functions --------------------------------------------------------- */

static void _service();

static void _service_loop()
{

    LOG_INF("thr: %p, Service thread started", k_current_get());

    while (1)
    {
        LOG_INF("mtx     locking");
        k_mutex_lock(&service_mutex, K_FOREVER);
        LOG_INF("mtx     locked");

        if (!is_enabled)
        {
            LOG_INF("condvar wait");
            k_condvar_wait(&service_condvar, &service_mutex, K_FOREVER);
            LOG_INF("condvar unblocked");
        }

        /* Perform (critical) service functionality ... */
        _service();

        LOG_INF("mtx     unlocking");
        k_mutex_unlock(&service_mutex);
        LOG_INF("mtx     unlocked");

        LOG_INF("yielding");
        k_yield();
    }
}

/** @brief Service functionality */
static void _service()
{
    LOG_INF("_service() ");
    k_msleep(1000);
}
/* thread creation ----------------------------------------------------------- */

K_THREAD_DEFINE(service_tid, SERVICE_STACK_SIZE,
                _service_loop, NULL, NULL, NULL,
                SERVICE_THREAD_PRIORITY, 0, 0);

static int _init_service(const struct device *_)
{
    LOG_INF("thr: %p, SYS_INIT() SERVICE_SYS_INIT_PRIORITY: %d", k_current_get(), SERVICE_SYS_INIT_PRIORITY);
    config_service();
    return 0;
}

SYS_INIT(_init_service, APPLICATION, SERVICE_SYS_INIT_PRIORITY);

/* shell commands ------------------------------------------------------------ */
// TODO: move to a different file?
// TODO: only one toggle command with param on/off?

static int cmd_enable_service(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    enable_service();
    return 0;
}

static int cmd_disable_service(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    disable_service();
    return 0;
}

SHELL_CMD_REGISTER(enable_service, NULL, "Enable service (signal conditional variable)", cmd_enable_service);
SHELL_CMD_REGISTER(disable_service, NULL, "Disable service (wait on conditional variable)", cmd_disable_service);
