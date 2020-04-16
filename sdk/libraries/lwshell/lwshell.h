#ifndef __LWSH_H
#define __LWSH_H

#ifdef __cplusplus
extern "C" {
#endif
	
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
	

#if defined(LWSH_MORE_ARGS)
    #define LWSH_MAX_ARGS 	    32
#else
    #define LWSH_MAX_ARGS 	    5
#endif    

#define LWSH_MAX_ARG_LEN    32
   
#define LWSH_ERR_NONE	        0
#define LWSH_ERR_CMD_UNKN       1    
#define LWSH_ERR_ARGS_LEN	    2
#define LWSH_ERR_ARGS_MAX       3

typedef struct 
{
     int    argc;
     char*	argv[LWSH_MAX_ARGS];
} lwsh_args_t;


typedef struct 
{
     const char 	*cmd;
     const char     *desc;
     int (*func)(int argc, char **argv);
} lwsh_cmd_t;

int lwsh_cmdline_handler(lwsh_cmd_t *cmds, char *cmd_line);

#ifdef _cplusplus
}
#endif

#endif
