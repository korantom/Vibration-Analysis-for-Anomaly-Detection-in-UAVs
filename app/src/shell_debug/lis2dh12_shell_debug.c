#include <shell/shell.h>
#include "../accelerometer/lis2dh12/lis2dh12.h"

static int cmd_lis2dh12_start(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    lis2dh12_init();
    lis2dh12_config();
    lis2dh12_enable_interrupt();
    return 0;
}

static int cmd_lis2dh12_enable_fifo(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    lis2dh12_enable_fifo();
    return 0;
}

static int cmd_lis2dh12_read_fifo_dummy(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    lis2dh12_read_fifo_dummy(K_MSEC(1000));
    return 0;
}

static int cmd_lis2dh12_enable_and_read_fifo_dummy(const struct shell *shell, size_t argc, char **argv, void *data)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    lis2dh12_enable_fifo();
    for (int i = 0; i < (int)data; i++)
    {
        lis2dh12_read_fifo_dummy(K_MSEC(1000));
    }
    return 0;
}

SHELL_CMD_REGISTER(lis2dh12_start, NULL, "lis2dh12 init, config, enable_interrupt", cmd_lis2dh12_start);
SHELL_CMD_REGISTER(lis2dh12_enable_fifo, NULL, "lis2dh12_enable_fifo", cmd_lis2dh12_enable_fifo);
SHELL_CMD_REGISTER(lis2dh12_read_fifo_dummy, NULL, "lis2dh12_read_fifo_dummy", cmd_lis2dh12_read_fifo_dummy);

SHELL_SUBCMD_DICT_SET_CREATE(sub_lis2dh12_enable_and_read_fifo_dummy, cmd_lis2dh12_enable_and_read_fifo_dummy, (1, 1), (3, 3), (15, 15));
SHELL_CMD_REGISTER(lis2dh12_enable_and_read_fifo_dummy, &sub_lis2dh12_enable_and_read_fifo_dummy, "lis2dh12_enable_fifo, then perform read n times", NULL);
