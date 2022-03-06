#include "writer.h"

/* mutex + condvar */
K_MUTEX_DEFINE(writer_mutex);     // mutex
K_CONDVAR_DEFINE(writer_condvar); // condvar
static bool is_enabled = false;   // condition

/* define all variables and structs */
// ...

LOG_MODULE_REGISTER(writer_);

/* public interface ---------------------------------------------------------- */

void config_writer()
{
    LOG_INF("config_writer()");
}

void enable_writer()
{
    /* will be called from another thread (shell, main, ...) */
    LOG_INF("enable_writer()");
    LOG_INF("condvar signal");
    is_enabled = true;
    k_condvar_signal(&writer_condvar);
}

void disable_writer()
{
    /* will be called from another thread (shell, main, ...) */
    LOG_INF("disable_writer()");
    is_enabled = false;
}

/* private functions --------------------------------------------------------- */

static void _writer();

static void _writer_loop()
{

    LOG_INF("thr: %p, Writer thread started", k_current_get());

    while (1)
    {
        LOG_INF("mtx     locking");
        k_mutex_lock(&writer_mutex, K_FOREVER);
        LOG_INF("mtx     locked");

        if (!is_enabled)
        {
            LOG_INF("condvar wait");
            k_condvar_wait(&writer_condvar, &writer_mutex, K_FOREVER);
            LOG_INF("condvar unblocked");
        }

        /* Perform (critical) service functionality ... */
        _writer();

        LOG_INF("mtx     unlocking");
        k_mutex_unlock(&writer_mutex);
        LOG_INF("mtx     unlocked");

        LOG_INF("yielding");
        k_yield();
    }
}

/** @brief Service functionality */
static void _writer()
{
    LOG_INF("_writer() ");
    k_msleep(1000);
}
/* thread creation ----------------------------------------------------------- */

K_THREAD_DEFINE(writer_tid, WRITER_STACK_SIZE,
                _writer_loop, NULL, NULL, NULL,
                WRITER_THREAD_PRIORITY, 0, 0);

static int _init_writer(const struct device *_)
{
    LOG_INF("thr: %p, SYS_INIT() WRITER_SYS_INIT_PRIORITY: %d", k_current_get(), WRITER_SYS_INIT_PRIORITY);
    config_writer();
    return 0;
}

SYS_INIT(_init_writer, APPLICATION, WRITER_SYS_INIT_PRIORITY);

/* shell commands ------------------------------------------------------------ */
// TODO: move to a different file?
// TODO: only one toggle command with param on/off?

static int cmd_enable_writer(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    enable_writer();
    return 0;
}

static int cmd_disable_writer(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    disable_writer();
    return 0;
}

SHELL_CMD_REGISTER(enable_writer, NULL, "Enable writer (signal conditional variable)", cmd_enable_writer);
SHELL_CMD_REGISTER(disable_writer, NULL, "Disable writer (wait on conditional variable)", cmd_disable_writer);
