#include "service_template.h"

/* mutex + condvar */
K_MUTEX_DEFINE(service_mutex);     // mutex used
K_CONDVAR_DEFINE(service_condvar); // cond var
static bool is_enabled = false;    // condition

/* define all variables and structs */
// ...

/* public interface ---------------------------------------------------------- */

void config_service()
{
    // ...
}

void enable_service()
{
    /* will be called from another thread (shell, main, ...) */
    is_enabled = true;
    k_condvar_signal(&service_condvar);
}

void disable_service()
{
    /* will be called from another thread (shell, main, ...) */
    is_enabled = false;
}

/* private functions --------------------------------------------------------- */

static void _service();

static void _service_loop()
{

    printk("Service Thread S: %p started\n", k_current_get());

    while (1)
    {
        printk("S: locking mutex\n");
        k_mutex_lock(&service_mutex, K_FOREVER);
        printk("S: locked mutex\n");

        if (!is_enabled)
        {
            printk("S: disabled\n");
            printk("S: condvar wait\n");
            k_condvar_wait(&service_condvar, &service_mutex, K_FOREVER);
        }
        printk("S: enabled\n");

        /* Perform (critical) service functionality ... */
        _service();

        printk("S: unlocking mutex\n");
        k_mutex_unlock(&service_mutex);
        printk("S: unlocked mutex\n");
        printk("\n");

        k_yield();
    }
}

static void _service()
{
    /* Service functionality */
    printk("\tS: Service functionality\n");
    k_msleep(1000);
}
/* thread creation ----------------------------------------------------------- */

/**
 * TODO: should be in common.h or service.h
 * TODO: determine: stack_size, thread priority (with respect to main thread), sys_init priority
 */
#define SERVICE_STACK_SIZE 500
#define SERVICE_THREAD_PRIORITY -1
#define SERVICE_SYS_INIT_PRIORITY 32

K_THREAD_DEFINE(service_tid, SERVICE_STACK_SIZE,
                _service_loop, NULL, NULL, NULL,
                SERVICE_THREAD_PRIORITY, 0, 0);

static int _init_service(const struct device *_)
{
    printk("service.c SYS_INIT, thread ?: %p\n", k_current_get());
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

SHELL_CMD_REGISTER(enable_service, NULL, "Enable serivice", cmd_enable_service);
SHELL_CMD_REGISTER(disable_service, NULL, "Disable serice", cmd_disable_service);
