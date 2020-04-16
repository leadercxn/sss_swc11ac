#include "storage.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "app_scheduler.h"
#include "pstorage.h"

void storage_init(void)
{
    uint32_t err_code;
    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);
}

void storage_access_wait(void)
{
    uint32_t count = 1;
    do
    {
        app_sched_execute();
        pstorage_access_status_get(&count);
    }
    while(count != 0);
}
