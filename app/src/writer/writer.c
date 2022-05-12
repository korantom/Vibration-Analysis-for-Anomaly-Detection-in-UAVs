#include "writer.h"
#include "disk/disk.h"
#include "../accelerometer/lis2dh12/lis2dh12.h"

/* mutex + condvar */
K_MUTEX_DEFINE(writer_mutex);     // mutex
K_CONDVAR_DEFINE(writer_condvar); // condvar
static bool is_enabled = false;   // condition

/* define all variables and structs */
static char path[100];

LOG_MODULE_REGISTER(writer_);

/* public interface ---------------------------------------------------------- */

void config_writer()
{
    LOG_INF("config_writer()");

    // disk_test_config();
    disk_mount();
    disk_list_dir(DISK_MOUNT_PT);
}

void enable_writer(const char *file_name)
{
    /* will be called from another thread (shell, main, ...) */
    LOG_INF("enable_writer()");

    if (is_enabled)
    {
        return;
    } // TODO:

    snprintf(path, sizeof(path), "%s/%s", DISK_MOUNT_PT, file_name);

    LOG_DBG("path = %s", log_strdup(path));

    disk_open_file(path);

    LOG_INF("writer_condvar signal");
    is_enabled = true;
    k_condvar_signal(&writer_condvar);
}

void disable_writer()
{
    /* will be called from another thread (shell, main, ...) */
    LOG_INF("disable_writer()");
    is_enabled = false;

    // TODO: wait for thread to finish writing
    disk_close_file();
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

    LOG_INF("ring_buf_sem take");
    k_sem_take(&ring_buf_sem, K_FOREVER);
    LOG_INF("ring_buf_sem taken");

    ////////////////////////////////////////////////////////////////////////////
    // TODO: make sure file is opened?
    // TODO: write raw bytes convert later?

    static char out_str[80];
    int byte_count = 3 * 2 * 16; // TODO: figure out max batch size to write
    uint8_t *data;

    // CLAIM
    int ringbuf_aloc_size = ring_buf_get_claim(&lis2dh12_ring_buf, &data, byte_count);
    LOG_INF("ring_buf_get_claim: claimed, alocated = %d, %d, alocated%%6=%d", byte_count, ringbuf_aloc_size, ringbuf_aloc_size % 6);

    const double multiplier = 0.001197100830078125f;
    double x, y, z;
    for (int i = 0; i < ringbuf_aloc_size; i += 6)
    {
        // raw * (8*9.80665/1024/64)
        x = *(int16_t *)&data[i] * multiplier;
        y = *(int16_t *)&data[i + 2] * multiplier;
        z = *(int16_t *)&data[i + 4] * multiplier;
        //

        int written = snprintf(out_str, sizeof(out_str) - 1, "%.6lf, %.6lf, %.6lf\r\n", x, y, z);
        disk_write_file(out_str, written);
    }

    // FINISH
    int finish_ret = ring_buf_get_finish(&lis2dh12_ring_buf, ringbuf_aloc_size);
    LOG_INF("RING BUFFER calimed, finished %d, %d bytes", ringbuf_aloc_size, finish_ret);

    ////////////////////////////////////////////////////////////////////////////

    disk_flush();
    ////////////////////////////////////////////////////////////////////////////
}

void dump_all(const char *file_name)
{
    if (is_enabled)
    {
        LOG_WRN("Writer service enabled cant open file.");
        return;
    }

    snprintf(path, sizeof(path), "%s/%s", DISK_MOUNT_PT, file_name);
    disk_open_file(path);

    static char out_str[80];
    int byte_count = 3 * 2 * 32;
    uint8_t *data;

    while (!ring_buf_is_empty(&lis2dh12_ring_buf))
    {

        // CLAIM
        int ringbuf_aloc_size = ring_buf_get_claim(&lis2dh12_ring_buf, &data, byte_count);
        LOG_INF("ring_buf_get_claim: claimed, alocated = %d, %d, alocated%%6=%d", byte_count, ringbuf_aloc_size, ringbuf_aloc_size % 6);

        const double multiplier = 0.001197100830078125f;
        double x, y, z;
        for (int i = 0; i < ringbuf_aloc_size; i += 6)
        {
            // raw * (8*9.80665/1024/64)
            x = *(int16_t *)&data[i] * multiplier;
            y = *(int16_t *)&data[i + 2] * multiplier;
            z = *(int16_t *)&data[i + 4] * multiplier;
            //

            int written = snprintf(out_str, sizeof(out_str) - 1, "%.6lf, %.6lf, %.6lf\r\n", x, y, z);
            disk_write_file(out_str, written);
        }
        // FINISH
        int finish_ret = ring_buf_get_finish(&lis2dh12_ring_buf, ringbuf_aloc_size);
        LOG_INF("RING BUFFER calimed, finished %d, %d bytes", ringbuf_aloc_size, finish_ret);
    }

    disk_close_file();
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
    // for (size_t cnt = 0; cnt < argc; cnt++)
    // {
    //     shell_print(shell, "  argv[%d] = %s", cnt, argv[cnt]);
    // }

    enable_writer(argv[1]);
    return 0;
}

static int cmd_disable_writer(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    disable_writer();
    return 0;
}

SHELL_CMD_ARG_REGISTER(enable_writer, NULL, "Enable writer (signal conditional variable), input filename", cmd_enable_writer, 2, 0);
SHELL_CMD_REGISTER(disable_writer, NULL, "Disable writer (wait on conditional variable)", cmd_disable_writer);
