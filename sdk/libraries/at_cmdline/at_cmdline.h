#ifndef __AT_CMDLINE_H_
#define __AT_CMDLINE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define AT_CMDLINE_MAX_ARGS 	  32
#define AT_CMDLINE_MAX_ARG_LEN    1024

#define AT_CMDLINE_ERR_NONE	            0
#define AT_CMDLINE_ERR_INVALID_CMD      1    
#define AT_CMDLINE_ERR_INVALID_ARG	    2
#define AT_CMDLINE_ERR_ARGS_MAX         3

#define AT_CMD_OP_NONE                0
#define AT_CMD_OP_QUERY               1
#define AT_CMD_OP_SET                 2
#define AT_CMD_OP_EXEC                3

typedef struct 
{
     uint8_t argc;
     uint8_t option;
     char*	argv[AT_CMDLINE_MAX_ARGS];
} at_args_t;


typedef struct
{
    const char 	*cmd;
    int (*query)(int argc, char* argv[]);
    int (*set)(int argc, char* argv[]);
    int (*exec)(void);
} at_cmd_t;

int at_cmdline_handler(at_cmd_t *cmds, char *cmd_line);

#endif
