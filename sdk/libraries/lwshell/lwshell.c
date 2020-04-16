#include "lwshell.h"

static int lwsh_arg_parser(char *cmd_line, lwsh_args_t *args)
{
    char *token;
    int argc = 0;
    
    token = strtok(cmd_line, " ");
    while( token != NULL ) 
    {
        if(strlen(token) > LWSH_MAX_ARG_LEN)
        {
            return LWSH_ERR_ARGS_LEN;
        }
        
        if(argc >= LWSH_MAX_ARGS)
        {
            return LWSH_ERR_ARGS_MAX;
        }
        args->argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    args->argc = argc;
    return LWSH_ERR_NONE;
}

int lwsh_cmdline_handler(lwsh_cmd_t *cmds, char *cmd_line)
{
    int i;
    int ret;
    int cmd_len;

    lwsh_args_t args;
    memset(&args, 0, sizeof(lwsh_args_t));

    for(i = 0; cmds[i].cmd != NULL; i++) 
    {
        cmd_len = strlen((char *)(cmds[i].cmd));

        if(strncmp((char *)(cmds[i].cmd), cmd_line, cmd_len) == 0) 
        {
            ret = lwsh_arg_parser(cmd_line, &args);
            if(LWSH_ERR_NONE != ret)
            {
                return ret;
            }
            return (cmds[i].func)(args.argc, args.argv);
        }
    }
    return LWSH_ERR_CMD_UNKN;
}
