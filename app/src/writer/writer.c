#include "writer.h"
// #include "../accelerometer/lis2dh12/lis2dh12.h"
// #include "../service_template/service_template.h"

#include "disk/disk.h"

/* mutex + condvar */
K_MUTEX_DEFINE(writer_mutex);     // mutex used
K_CONDVAR_DEFINE(writer_condvar); // cond var
static bool is_enabled = false;   // condition

/* define all variables and structs */
// ...

/* public interface ---------------------------------------------------------- */

void config_writer()
{
    // ...
}

void enable_writer()
{
    /* will be called from another thread (shell, main, ...) */
    is_enabled = true;
    k_condvar_signal(&writer_condvar);
}

void disable_writer()
{
    /* will be called from another thread (shell, main, ...) */
    is_enabled = false;
}

/* private functions --------------------------------------------------------- */

static void _writer();

static void _writer_loop()
{

    printk("Writer Thread W: %p started\n", k_current_get());
    mount_disk();

    list_dir(DISK_MOUNT_PT);

    test_write();

    list_dir(DISK_MOUNT_PT);

    // unmount_disk();

    while (1)
    {
        printk("W: locking mutex\n");
        k_mutex_lock(&writer_mutex, K_FOREVER);
        printk("W: locked mutex\n");

        if (!is_enabled)
        {
            printk("W: disabled\n");
            printk("W: condvar wait\n");
            k_condvar_wait(&writer_condvar, &writer_mutex, K_FOREVER);
        }
        printk("W: enabled\n");

        /* Perform (critical) writer functionality ... */
        _writer();

        printk("W: unlocking mutex\n");
        k_mutex_unlock(&writer_mutex);
        printk("W: unlocked mutex\n");
        printk("\n");
        k_msleep(1000);
        k_yield();
    }
}

static void _writer()
{
    /* Service functionality */
    printk("W: _writer\n");

    test_write();
    list_dir(DISK_MOUNT_PT);

}
/* thread creation ----------------------------------------------------------- */

K_THREAD_DEFINE(writer_tid, WRITER_STACK_SIZE,
                _writer_loop, NULL, NULL, NULL,
                WRITER_THREAD_PRIORITY, 0, 0);

static int _init_writer(const struct device *_)
{
    printk("writer.c SYS_INIT, thread ?: %p\n", k_current_get());
    config_writer();
    return 0;
}

SYS_INIT(_init_writer, APPLICATION, 99);

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

SHELL_CMD_REGISTER(enable_writer, NULL, "Enable writer", cmd_enable_writer);
SHELL_CMD_REGISTER(disable_writer, NULL, "Disable writer", cmd_disable_writer);
