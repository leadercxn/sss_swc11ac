/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#ifndef SHELL_CMD_H__
#define SHELL_CMD_H__

#include "queue.h"

#define SHELL_SUCCESS                   0
#define SHELL_ERROR_UNKNOWN_OPTION      1
#define SHELL_ERROR_INVALID_PARAM       2

#define SHELL_ERROR_INVALID_CMDLINE     100
#define SHELL_ERROR_TOOLONG_CMD         101
#define SHELL_ERROR_TOOMANY_ARGS        102
#define SHELL_ERROR_UNKNOWN_CMD         103

struct shell_cmd_s
{
    const char * cmd;
    const char * desc;

    int (* func)(int argc, char **argv);

    STAILQ_ENTRY(shell_cmd_s) next;
};

typedef struct shell_cmd_s shell_cmd_t;

void shell_cmd_register(shell_cmd_t * cmds, int size);

void shell_cmd_show(void);

int shell_cmd_execute(const char *text);

#endif
