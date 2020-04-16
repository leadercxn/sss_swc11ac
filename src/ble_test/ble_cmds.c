/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "shell_cmd.h"

#include "ble_ping.h"
#include "ble_ctx.h"
#include "ble_mtx.h"
#include "ble_cmds.h"
#include "ble_dtm_test.h"
#include "ble_radio_tx.h"
#include "ble_radio_rx.h"
#include "ble_pong.h"

static shell_cmd_t cmds[] =
{
    {
        .cmd  = "bleping",
        .desc = "ble ping test",
        .func = ble_ping,
    },

    {
        .cmd = "blepong",
        .desc = "ble radio pong test",
        .func = ble_pong_test,
    },

    {
        .cmd  = "blectx",
        .desc = "ble const carrier test",
        .func = ble_ctx_test,
    },

    {
        .cmd  = "blemtx",
        .desc = "ble modulated rf test",
        .func = ble_mtx_test,
    },

    {
        .cmd = "bletx",
        .desc = "ble radio tx test",
        .func = ble_radio_tx_test,
    },

    {
        .cmd = "blerx",
        .desc = "ble radio rx test",
        .func = ble_radio_rx_test,
    },

#if 0     //此测试存在bug，未移植前就存在           
    {
        .cmd = "bledtm",
        .desc = "ble direct test mode",
        .func = ble_dtm_test,
    },
#endif

};

void ble_cmds_register(void)
{
    shell_cmd_register(cmds, sizeof(cmds) / sizeof(cmds[0]));
}
