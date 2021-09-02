#include <zephyr.h>
#include <shell/shell.h>
#include <sys/printk.h>

//----------------------------------------------------------------------------//
/* Static command without params */

static int cmd_hello_world(const struct shell *shell, size_t argc, char **argv)
{
    // when params are not intended to be used, stops compilation warnings
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(shell, "Hello World print");
    // shell_warn(shell, "Hello World warning");
    // shell_error(shell, "Hello World error");
    return 0;
}

SHELL_CMD_REGISTER(hello_world, NULL, "Prints Hello World", cmd_hello_world);

//----------------------------------------------------------------------------//
/* Static command with params */

static int cmd_print_params(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "argc = %d", argc);
    for (size_t cnt = 0; cnt < argc; cnt++)
    {
        shell_print(shell, "  argv[%d] = %s", cnt, argv[cnt]);
    }
    return 0;
}

SHELL_CMD_ARG_REGISTER(print_params, NULL, "Prints all passed params ", cmd_print_params, 0, 0);

//----------------------------------------------------------------------------//
/* Dictionary command */

int value = 0;

static int cmd_print_value(const struct shell *shell, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);
    shell_print(shell, "Value = %d", value);
    return 0;
}

static int cmd_set_value(const struct shell *shell, size_t argc, char **argv, void *data)
{
    int old_value = value;
    value = (int)data;
    shell_print(shell, "Value changed from %d to %s: %d", old_value, argv[0], value);
    cmd_print_value(shell, 0, NULL);
    return 0;
}

SHELL_SUBCMD_DICT_SET_CREATE(sub_value, cmd_set_value,
                             (low, 0), (med, 5), (high, 10));
SHELL_CMD_REGISTER(set_value, &sub_value, "Set value", NULL);
SHELL_CMD_REGISTER(print_value, NULL, "Print value", cmd_print_value);

void main(void)
{
    printk("Runnig shell\n");
}
