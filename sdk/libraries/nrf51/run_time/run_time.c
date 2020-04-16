/* Copyright (c) 2016 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include "app_timer.h"
#include "run_time.h"



#define TIMER_CREATE(p_id, mode, handler)                                           \
{                                                                                   \
    uint32_t err_code;                                                              \
    err_code = app_timer_create(p_id, mode, handler);                               \
    APP_ERROR_CHECK(err_code);                                                      \
}                                                                                   \

#define TIMER_START(id, ms)                                                         \
{                                                                                   \
    uint32_t err_code;                                                              \
    err_code = app_timer_start(id, APP_TIMER_TICKS(ms, 0), NULL);                   \
    APP_ERROR_CHECK(err_code);                                                      \
}                                                                                   \

#define TIMER_STOP(id)                                                              \
{                                                                                   \
    uint32_t err_code;                                                              \
    err_code = app_timer_stop(id);                                                  \
    APP_ERROR_CHECK(err_code);                                                      \
}

#ifndef APP_TIMER_PRESCALER
#define  APP_TIMER_PRESCALER     0
#endif

#define  NS_PER_TICK             (1+APP_TIMER_PRESCALER)*30517
#define  RTC1_TICKS_MAX          16777216
#define  NS_PER_MS               1000000

APP_TIMER_DEF(run_time_timer);

static uint32_t m_run_time_ms = 0;

static void run_time_handler(void * p_contex)
{
    static uint32_t previous_tick = 0;
    uint32_t        current_tick = 0;
    uint32_t        tick_diff = 0;
    static uint32_t ns_remainder = 0;

    APP_ERROR_CHECK(app_timer_cnt_get(&current_tick));

    if (current_tick > previous_tick)
    {
        //ticks since last update
        tick_diff = current_tick - previous_tick;
    }
    else if (current_tick < previous_tick)
    {
        //RTC counter has overflown
        tick_diff = current_tick + (RTC1_TICKS_MAX - previous_tick);
    }

    uint32_t ns = tick_diff * NS_PER_TICK;

    m_run_time_ms += ns/NS_PER_MS;
    ns_remainder += ns % NS_PER_MS;

    if (ns_remainder >= NS_PER_MS)
    {
        m_run_time_ms += ns_remainder/NS_PER_MS;
        ns_remainder = ns_remainder % NS_PER_MS;
    }
    previous_tick = current_tick;
}

uint32_t RunTimeGet(void)
{
    run_time_handler(NULL);
    return m_run_time_ms;
}

uint32_t RunTimeElapsedGet(uint32_t past)
{
    uint32_t now = RunTimeGet();

    return (now - past);
}

void RunTimeInit(void)
{
    run_time_handler(NULL);
    TIMER_CREATE(&run_time_timer, APP_TIMER_MODE_REPEATED, run_time_handler);
    TIMER_START(run_time_timer, 2000);
}


