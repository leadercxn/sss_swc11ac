#ifndef __TIMER_H_
#define __TIMER_H_

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


#define TIMER_START_WITH_PARAM(id, ms, param)                                       \
{                                                                                   \
    uint32_t err_code;                                                              \
    err_code = app_timer_start(id, APP_TIMER_TICKS(ms, 0), param);                  \
    APP_ERROR_CHECK(err_code);                                                      \
}                                                                                   \


#define TIMER_STOP(id)                                                              \
{                                                                                   \
    uint32_t err_code;                                                              \
    err_code = app_timer_stop(id);                                                  \
    APP_ERROR_CHECK(err_code);                                                      \
}                                                                                   \


#endif
