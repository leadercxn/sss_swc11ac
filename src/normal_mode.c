#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ser_hal_transport.h"
#include "ser_conn_handlers.h"
#include "run_time.h"
#include "app_uart.h"
#include "app_scheduler.h"

#include "trace.h"

#include "boards.h"
#include "board_alarm_host_v100.h"
#include "alpha_radio_handler.h"

#include "normal_mode.h"




#if 0
/**
 *  初始化 timer
 */
static void timer_init(void)
{
    RunTimeInit();

}
#endif

/**
 *@brief 空闲状态 
 */
static void on_idle(void)
{
    ret_code_t ret_code = sd_app_evt_wait();
    ASSERT((ret_code == NRF_SUCCESS) || (ret_code == NRF_ERROR_SOFTDEVICE_NOT_ENABLED));
    UNUSED_VARIABLE(ret_code);
}










/**
 *@brief 常规模式 
 */
void normal_mode(void)
{
    uint32_t err_code = NRF_SUCCESS;

    trace_debug("normal_mode \n");
    //初始化定时器
    //timer_init();

    /* Force constant latency mode to control SPI slave timing */
    NRF_POWER->TASKS_CONSTLAT = 1;
    NRF_POWER->RESET = 1;

    //nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;
    SOFTDEVICE_HANDLER_INIT(NULL, NULL);

    // 初始化alpha芯片
    alpha_radio_init();

    /* Subscribe for BLE events. */
    /**
     * @fun 
     */
    err_code = softdevice_ble_evt_handler_set(ser_conn_ble_event_handle);
    APP_ERROR_CHECK(err_code);

    /* Open serialization HAL Transport layer and subscribe for HAL Transport events. */
    /**
     * @fun ser_hal_transport_open : 打开serialization的传输层(串口)
     * @fun ser_conn_hal_transport_event_handle : 注册传输层事件，把函数传输到底层
     */
    err_code = ser_hal_transport_open(ser_conn_hal_transport_event_handle);
    APP_ERROR_CHECK(err_code);


    trace_debug("enter loop \n");
    /* Enter main loop. */
    for (;;)
    {
        /* Process SoftDevice events. */
        app_sched_execute();

#if 0
        if (softdevice_handler_is_suspended())
        {
            // Resume pulling new events if queue utilization drops below 50%.
            if (app_sched_queue_space_get() > (SER_CONN_SCHED_QUEUE_SIZE >> 1))
            {
                softdevice_handler_resume();
            }
        }
#endif
        /* Process received packets.
         * We can NOT add received packets as events to the application scheduler queue because
         * received packets have to be processed before SoftDevice events but the scheduler queue
         * does not have priorities. */
        err_code = ser_conn_rx_process();
        APP_ERROR_CHECK(err_code);

        on_idle();
    }


}




