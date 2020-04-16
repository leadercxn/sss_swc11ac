/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "ntopt.h"
#include "ntlibc.h"

#include "shell_cmd.h"

STAILQ_HEAD(, shell_cmd_s) m_cmds_queue = STAILQ_HEAD_INITIALIZER(m_cmds_queue);

void shell_cmd_register(shell_cmd_t * cmds, int size)
{
    for(int i = 0; i < size; i++)
    {
        STAILQ_INSERT_TAIL(&m_cmds_queue, &cmds[i], next);
    }
}

void shell_cmd_show(void)
{
    shell_cmd_t * p_cmd = NULL;

    STAILQ_FOREACH(p_cmd, &m_cmds_queue, next)
    {
        if(p_cmd != NULL)
        {
            printf("%-15s\t%s\r\n", p_cmd->cmd, p_cmd->desc);
        }
    }
}

static int ntopt_callback(int argc, char **argv, void *extobj)
{
    shell_cmd_t * p_cmd = NULL;

    if(argc == 0)
    {
        return SHELL_ERROR_INVALID_CMDLINE;
    }

    STAILQ_FOREACH(p_cmd, &m_cmds_queue, next)
    {
        if(ntlibc_strcmp((const char *)argv[0], p_cmd->cmd) == 0)
        {
            return p_cmd->func(argc, argv);
        }
    }
    return SHELL_ERROR_UNKNOWN_CMD;
}

int shell_cmd_execute(const char *text)
{
    return ntopt_parse(text, ntopt_callback, 0);
}
