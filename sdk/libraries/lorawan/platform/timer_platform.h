#ifndef TIMER_PLATFORM_H_
#define TIMER_PLATFORM_H_

#include "app_timer.h"
#include "run_time.h"

#define TIMER_DEF(id)   APP_TIMER_DEF(id)

static __INLINE void TIMER_CREATE(app_timer_id_t const *      p_timer_id,
                                  app_timer_mode_t            mode,
                                  app_timer_timeout_handler_t timeout_handler)
{
    uint32_t err_code;
    err_code = app_timer_create(p_timer_id, mode, timeout_handler);
    APP_ERROR_CHECK(err_code);
}   
 
static __INLINE void TIMER_START(app_timer_id_t const * timer_id, uint32_t ms)
{                                                                                   
    uint32_t err_code;
    err_code = app_timer_start(*timer_id, APP_TIMER_TICKS((ms), 0), NULL);
    APP_ERROR_CHECK(err_code);
}

static __INLINE void TIMER_STOP(app_timer_id_t const * timer_id)
{                                                                                   
    uint32_t err_code;
    err_code = app_timer_stop(*timer_id);
    APP_ERROR_CHECK(err_code);
}

#define RUN_TIME_GET		  	        RunTimeGet
#define ELAPSED_TIME_GET    	        RunTimeElapsedGet

#endif
