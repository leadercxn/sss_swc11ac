#include "at_cmdline.h"
#include <ctype.h>

int at_arg_parser(char *cmd_line, at_args_t *args)
{
    char *token;
    char *substr;
    int cmd_len = 0;
    int cmd_line_len = 0;
    int argc = 0;
       
    cmd_line_len = strlen(cmd_line);
    
    if((strncmp(cmd_line, "AT", 2) != 0) &&
       (strncmp(cmd_line, "at", 2) != 0) )
    {
        return AT_CMDLINE_ERR_INVALID_CMD;
    }
    
    if((strcmp(cmd_line, "AT") == 0) ||
       (strcmp(cmd_line, "at") == 0))
    {
        args->argv[argc++] = cmd_line;
        args->option = AT_CMD_OP_EXEC;
        args->argc = argc;
        return AT_CMDLINE_ERR_NONE;
    }
    
    substr = strchr(&cmd_line[2], '?');
    if(substr != NULL)
    {
        if( (cmd_line_len - (substr - cmd_line)) > 1)
        {
            return AT_CMDLINE_ERR_INVALID_CMD;
        }
        cmd_len = substr-cmd_line-2;
        if(cmd_len > AT_CMDLINE_MAX_ARG_LEN)
        {
             return AT_CMDLINE_ERR_INVALID_CMD;
        }
        *substr = '\0';
        args->argv[argc++] = &cmd_line[2];
        args->option = AT_CMD_OP_QUERY;
        args->argc = argc;
        return AT_CMDLINE_ERR_NONE;
    }
    
    
    substr = strchr(cmd_line, '=');
    if(substr != NULL)
    {
        cmd_len = substr-cmd_line-2;
        if(cmd_len > AT_CMDLINE_MAX_ARG_LEN)
        {
             return AT_CMDLINE_ERR_INVALID_CMD;
        }
        *substr = '\0';
        args->argv[argc++] = &cmd_line[2];
        args->option = AT_CMD_OP_SET;
        
        token = strtok(substr+1, ",");
        while( token != NULL )
        {
            if(strlen(token) > AT_CMDLINE_MAX_ARG_LEN)
            {
                return AT_CMDLINE_ERR_INVALID_ARG;
            }
            if(argc >= AT_CMDLINE_MAX_ARGS)
            {
                return AT_CMDLINE_ERR_ARGS_MAX;
            }
            args->argv[argc++] = token;
            token = strtok(NULL, ",");
        }
        args->argc = argc;
        return AT_CMDLINE_ERR_NONE;
    }
     
    if(strlen(&cmd_line[2]) > AT_CMDLINE_MAX_ARG_LEN)
    {
        return AT_CMDLINE_ERR_INVALID_CMD;
    }
    args->argv[argc++] = &cmd_line[2];
    args->option = AT_CMD_OP_EXEC;
    args->argc = argc;
    
    return AT_CMDLINE_ERR_NONE;
}

int at_cmdline_handler(at_cmd_t *cmds, char *cmd_line)
{
    int ret;
//    int cmd_line_len;
    int cmd_len;

    at_args_t args;
    memset(&args, 0, sizeof(at_args_t));
      
//    cmd_line_len = strlen(cmd_line);
    
//    for(size_t i = 0; i < cmd_line_len; i++)
//    {
//        cmd_line[i] = toupper(cmd_line[i]);
//    }
    
    ret = at_arg_parser(cmd_line, &args);
    if(AT_CMDLINE_ERR_NONE != ret)
    {
        return ret;
    }
    
    cmd_len = strlen(args.argv[0]);
    for(size_t i = 0; i <cmd_len ; i++)
    {
        args.argv[0][i] = toupper(args.argv[0][i]);
    }
    
    for(size_t i = 0; cmds[i].cmd != NULL; i++) 
    {    
        if( 0 == strcmp((char *)(cmds[i].cmd), args.argv[0]) ) 
        {
            if(AT_CMD_OP_QUERY == args.option)
            {
                if(cmds[i].query == NULL)
                {
                    return AT_CMDLINE_ERR_INVALID_CMD;
                }
                return (cmds[i].query)(args.argc, args.argv);
            }
            else if(AT_CMD_OP_SET == args.option)
            {
                if(cmds[i].set == NULL)
                {
                    return AT_CMDLINE_ERR_INVALID_CMD;
                }
                return (cmds[i].set)(args.argc, args.argv);
            }
            else if(AT_CMD_OP_EXEC == args.option)
            {
                if(cmds[i].exec == NULL)
                {
                    return AT_CMDLINE_ERR_INVALID_CMD;
                }
                return (cmds[i].exec)();
            }
            else
            {
                return AT_CMDLINE_ERR_INVALID_CMD;
            }
        }
    }
    return AT_CMDLINE_ERR_INVALID_CMD;
}

