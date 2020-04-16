/*
 * Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 *
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "softdevice_handler_appsh.h"
#include "app_util.h"
#include "app_error.h"
#include "app_scheduler.h"
#include "app_timer_appsh.h"
#include "hardfault.h"
#include "nrf_delay.h"
#include "nrf_drv_gpiote.h"
#include "pstorage.h"

//#include "storage.h"
//#include "wdt.h"
#include "spi.h"
#include "trace.h"

#include "sw_cfg.h"

#include "factory_test.h"
#include "normal_mode.h"




void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t info)
{
#ifndef DEBUG
    printf("app error reset \n");
    NVIC_SystemReset();
#else
    error_info_t *p_info = (error_info_t *)info;
    trace_error("Fatal error: File: %s, Line: %d , error_code = %d \r\n",
                p_info->p_file_name,
                p_info->line_num,
                p_info->err_code);
    __disable_irq();
    UNUSED_VARIABLE(p_info);
    while(1);
#endif // DEBUG
}

/**@brief Function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num     Line number of the failing ASSERT call.
 * @param[in] p_file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}

/**@brief Function for initializing necessary hardware and bus
 */
static void hw_init(void)
{
    NRF_POWER->DCDCEN = POWER_DCDCEN_DCDCEN_Enabled << POWER_DCDCEN_DCDCEN_Pos ;        //使能DCDC转换，省电

    NRF_POWER->RESET  = POWER_RESET_RESET_Enabled << POWER_RESET_RESET_Pos ;            //上电复位

    // 初始化 trace 库
    trace_init();

    // 初始化 gpiote
    nrf_drv_gpiote_init();
}

/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in]   sys_evt   System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    if(sys_evt == NRF_EVT_FLASH_OPERATION_SUCCESS ||
       sys_evt == NRF_EVT_FLASH_OPERATION_ERROR)
    {
        pstorage_sys_event_handler(sys_evt);
    }
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;

    SOFTDEVICE_HANDLER_APPSH_INIT(NULL, true);

    ble_enable_params_t ble_enable_params;
    err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
               PERIPHERAL_LINK_COUNT,
               &ble_enable_params);
    APP_ERROR_CHECK(err_code);

    // Check the ram settings against the used number of links
    CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT, PERIPHERAL_LINK_COUNT);

    // Suport 2 base 128-bit uuids
    ble_enable_params.common_enable_params.vs_uuid_count = 1;

    ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
    // Enable BLE stack.
    err_code = softdevice_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);

    // 使能DCDC
    sd_power_dcdc_mode_set(NRF_POWER_DCDC_ENABLE);

    // 设置系统事件分发函数
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}



/**@brief Main
 */
int main(void)
{ 
    bool is_factory_mode = false ;
    // 初始化app timer, 分配内存空间, app_scheduler 方式
    APP_TIMER_APPSH_INIT(0, APP_TIMER_OP_QUEUE_SIZE, true);

    // 初始化app scheduler, 分配内存空间
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);

    // 初始化相关硬件
    hw_init();


    if( true == is_factory_mode )
    {
        //协议栈初始化
        ble_stack_init();
        
        //测试代码
        factory_test();
    }
    else
    {
        //正常模式
        normal_mode();
    }
    
    

}

/**@brief
 */
void HardFault_c_handler( uint32_t *p_stack_address )
{
#if defined(DEBUG)
    volatile HardFault_stack_t *HardFault_p_stack;

    HardFault_p_stack = (HardFault_stack_t*)p_stack_address;

    trace_error("HardFault_Handler\r\n");
    trace_error("r0  = %04X\r\n", HardFault_p_stack->r0);
    trace_error("r1  = %04X\r\n", HardFault_p_stack->r1);
    trace_error("r2  = %04X\r\n", HardFault_p_stack->r2);
    trace_error("r3  = %04X\r\n", HardFault_p_stack->r3);
    trace_error("r12 = %04X\r\n", HardFault_p_stack->r12);
    trace_error("lr  = %04X\r\n", HardFault_p_stack->lr);
    trace_error("pc  = %04X\r\n", HardFault_p_stack->pc);
    trace_error("psr = %04X\r\n", HardFault_p_stack->psr);
    /* Generate breakpoint if debugger is connected */
    __BKPT(0);
    while(1);
#else
    printf("hardfault reset \n");
    NVIC_SystemReset();
#endif
}
