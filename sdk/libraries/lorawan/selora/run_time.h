#ifndef __RUN_TIME_H_
#define __RUN_TIME_H_

#include <stdint.h>

typedef struct
{
    uint32_t tv_s;
    uint32_t tv_ms;
}run_time_timespec_t;

uint32_t run_time_get(void);

void run_time_init(void);

void run_time_utc_set(uint32_t timestamp);

void run_time_timespec_get(run_time_timespec_t *p_timespec);

#endif
