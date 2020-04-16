/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "shell_cmd.h"

#include "alpha_tx.h"
#include "alpha_rx.h"
#include "alpha_ping.h"
#include "alpha_pong.h"
#include "alpha_drx.h"
#include "alpha_ctx.h"
#include "alpha_fhtx.h"
#include "alpha_fhrx.h"
#include "alpha_cad.h"
#include "alpha_cmds.h"
#include "rssi_test.h"
#include "alpha_multi_channel_cad.h"

static shell_cmd_t cmds[] =
{
    {
        .cmd = "subgtx",
        .desc = "subg tx test",
        .func = alpha_tx,
    },

    {
        .cmd = "subgrx",
        .desc = "subg rx test",
        .func = alpha_rx,
    },

    {
        .cmd = "subgping",
        .desc = "subg ping test",
        .func = alpha_ping,
    },

    {
        .cmd = "subgpong",
        .desc = "subg pong test",
        .func = alpha_pong,
    },

    {
        .cmd = "subgdrx",
        .desc = "subg drx test",
        .func = alpha_drx_test,
    },

    {
        .cmd = "subgctx",
        .desc = "subg ctx test",
        .func = alpha_ctx_test,
    },

    {
        .cmd = "subgfhtx",
        .desc = "subg fhss tx test",
        .func = alpha_fhtx_test,
    },

    {
        .cmd = "subgfhrx",
        .desc = "subg fhss rx test",
        .func = alpha_fhrx_test,
    },

    {
        .cmd = "subgcad",
        .desc = "subg cad test",
        .func = alpha_cad_test,
    },

    {
        .cmd = "subgmccad",
        .desc = "subg multi channel cad test",
        .func = alpha_multi_channel_cad_test,
    },

    {
        .cmd = "rssi",
        .desc = "subg read rssi",
        .func = rssi_test,
    },
};

void alpha_cmds_register(void)
{
    shell_cmd_register(cmds, sizeof(cmds) / sizeof(cmds[0]));
}
