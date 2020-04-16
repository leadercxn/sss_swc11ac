/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_uart.h"

#include "gpio_test.h"
#include "nrf_delay.h"
#include "trace.h"
#include "nrf_gpio.h"
#include "app_timer.h"
#include "timer.h"

#define _UNUSED_PINS            {2,3,11,12,19,20,21,22,23,24,25,26,27,28,29,30,31}        //定义一些没用到的IO序号

APP_TIMER_DEF(m_gpio_timer);

static bool m_timer_created = false;

/**
 *  使用说明
 */
static void usage(void)
{
    printf("Available options: \n");
    printf(" -h     print this cmd help \n");
    printf(" gpio   start blink \n");
}

/**@brief  function for initializing unused gpio ping
 * 
 */
static void gpio_init(void)
{
    uint8_t gpio_array[] =  _UNUSED_PINS;
    for(uint8_t i = 0; i < sizeof(gpio_array); i++)
    {
        nrf_gpio_cfg_output(gpio_array[i]);
    }
}

/**@brief  function for freeing unused gpio ping
 * 
 */
static void gpio_deinit(void)
{
    uint8_t gpio_array[] =  _UNUSED_PINS;
    for(uint8_t i = 0; i < sizeof(gpio_array); i++)
    {
        nrf_gpio_cfg_default(gpio_array[i]);
    }
}

/**@brief   gpio_test timer handler
 * 
 */
static void gpio_timer_handler(void *p_context)
{
    uint8_t gpio_array[] =  _UNUSED_PINS;
    for(uint8_t i = 0; i < sizeof(gpio_array); i++)
    {
        nrf_gpio_pin_toggle(gpio_array[i]);
    }
}

/**@brief   gpio_test timer handler
 * 
 */
int gpio_test(int argc ,char **argv)
{
    for(uint8_t i = 1 ; i < argc ;)
    {
        trace_debug("arg = %s \n" , argv[i]);
        if( strcmp(argv[i] , "-h" ) == 0 )
        {
            i++ ; 
            usage();
            return false ;
        }
        else
        {
            printf("invalid arg \n");
            return false ;
        }        
    }

    if(!m_timer_created)
    {
        m_timer_created = true;
        TIMER_CREATE(&m_gpio_timer, APP_TIMER_MODE_REPEATED, gpio_timer_handler);
    }

    gpio_init();
    TIMER_START(m_gpio_timer, 500);
    printf("gpio test : start ,     ctrl+c to stop \n\n");

    while(true)
    {
        uint8_t c;
        while(app_uart_get(&c) == NRF_ERROR_NOT_FOUND)
        {
        }
        if(c == 0x03) // ^C
        {
            TIMER_STOP(m_gpio_timer);
            gpio_deinit();
            printf("gpio test : end\r\n");
            break;
        }
    }

    return false;
}


