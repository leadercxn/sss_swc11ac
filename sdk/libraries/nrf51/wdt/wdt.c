#include "wdt.h"
#include "nrf51.h"
#include "nrf51_bitfields.h"
#include "app_scheduler.h"
#include "app_timer.h"


#define NRF_WDT_RR_VALUE       0x6E524635UL /* Fixed value, shouldn't be modified.*/

APP_TIMER_DEF(m_wdt_timer);

void wdt_init(uint32_t timeout)
{
    //    if(NRF_POWER->RESETREAS != POWER_RESETREAS_SREQ_Msk)/*when soft reset, wdt will not reset*/
    {
        NRF_WDT->CONFIG         = (WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos) |
                                  (WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos);

        NRF_WDT->CRV            = (timeout * 32768) / 1000;
        NRF_WDT->RREN          |= 0x1UL;
    }
}

void wdt_start(void)
{
    NRF_WDT->TASKS_START    = 1;
    wdt_feed();
}

void wdt_feed(void)
{
    NRF_WDT->RR[0] = NRF_WDT_RR_VALUE;
}

static void wtd_handler(void* p_event_data, uint16_t event_size)
{
    wdt_feed();
}

static void wdt_timer_handler(void* p_context)
{
    app_sched_event_put(NULL, 0, wtd_handler);
}

void wdt_timer_init(void)
{
    uint32_t err_code;
    err_code = app_timer_create(&m_wdt_timer, 
                                APP_TIMER_MODE_REPEATED, 
                                wdt_timer_handler);
    APP_ERROR_CHECK(err_code);
}

void wdt_timer_start(uint32_t interval, uint32_t prescaler)
{
    uint32_t err_code;
    err_code = app_timer_start(m_wdt_timer,
                               APP_TIMER_TICKS(interval, prescaler), 
                               NULL);
    APP_ERROR_CHECK(err_code);
}

void wdt_timer_stop(void)
{
    uint32_t err_code;
    err_code = app_timer_stop(m_wdt_timer);
    APP_ERROR_CHECK(err_code);
}

