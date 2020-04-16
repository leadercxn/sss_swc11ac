/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "nrf51.h"

#include "shell_cmd.h"
#include "base_cmds.h"
#include "gpio_test.h"
#include "test_companion.h"


static int cmds_print(int argc, char *argv[]);
static int reset(int argc, char *argv[]);

static shell_cmd_t cmds[] =
{
    {
        .cmd = "?",
        .desc = "list all cmds",
        .func = cmds_print,
    },

    {
        .cmd = "reset",
        .desc = "reset the device",
        .func = reset,
    },

    {
        .cmd = "gpio",
        .desc = "gpio test",
        .func = gpio_test,
    },
    
    {
        .cmd = "testcmp",
        .desc = "test companion",
        .func = test_companion,
    },
};

static int cmds_print(int argc, char *argv[])
{
    shell_cmd_show();
    return 0;
}

static int reset(int argc, char *argv[])
{
    NVIC_SystemReset();
    return 0;
}

void base_cmds_register(void)
{
    shell_cmd_register(cmds, (sizeof(cmds) / sizeof(cmds[0])));
}
