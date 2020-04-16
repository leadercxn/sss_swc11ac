/* Copyright (c) 2016 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include "trace.h"

#ifdef TRACE_ENABLE
#include "nrf_log.h"

void trace_init(void)
{
    (void)NRF_LOG_INIT();
}

void trace_dump(void * p_buffer, uint32_t len)
{
    uint8_t *p = (uint8_t *)p_buffer;
    for (uint32_t index = 0; index <  len; index++)
    {
        TRACE_PRINTF("%02X", p[index]);
    }
    TRACE_PRINTF("\r\n");
}

#endif // ENABLE_DEBUG_LOG_SUPPORT
