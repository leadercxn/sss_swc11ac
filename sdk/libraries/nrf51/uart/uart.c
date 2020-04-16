/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include <stdint.h>

#include "nrf.h"
#include "uart.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "nrf_soc.h"
#include "nrf_nvic.h"
#include "app_error.h"
#include "app_util_platform.h"
#include "nrf_assert.h"

#define CLK_PERIOD               63       // unit: ns
#define Min_Cycle_Per_Inst       1       // cycle count of one instruction
#define One_Loop_Inst            3        // instruction count of one loop (estimate)

#define CAL_TO(x)                (x * 1000000 / (CLK_PERIOD * Min_Cycle_Per_Inst * One_Loop_Inst))

static uint32_t             m_baudrate = 0;
static uint8_t              m_txd_pin_number;
static uart_rx_handler_t    m_uart_rx_handler = NULL;

bool wait_uart_ready(uint32_t exp_time)
{
    uint32_t temp = 0;
    while(NRF_UART0->EVENTS_RXDRDY != 1)
    {
        if(temp > exp_time)
            return false;

        temp = temp + 1;
    }

    return true;
}

uint8_t uart_get(void)
{
    while(NRF_UART0->EVENTS_RXDRDY != 1)
    {
        // Wait for RXD data to be received
    }

    NRF_UART0->EVENTS_RXDRDY = 0;
    return (uint8_t)NRF_UART0->RXD;
}

bool uart_get_with_timeout(uint32_t timeout_ms, uint8_t* rx_data)
{
    bool ret;
    
    ASSERT(rx_data != NULL);
    ASSERT(timeout_ms >= 0);

    uint32_t settime = CAL_TO(timeout_ms);

    if(wait_uart_ready(settime))
    {
        NRF_UART0->EVENTS_RXDRDY = 0;
        *rx_data = (uint8_t)NRF_UART0->RXD;

        ret = true;
    }
    else
    {
        ret = false;
    }

    return ret;
    
    /*
    while(NRF_UART0->EVENTS_RXDRDY != 1)
    {
        
        if(timeout_ms-- >= 0)
        {
            // wait in 1ms chunk before checking for status
            nrf_delay_us(1000);
        }
        else
        {
            ret = false;
            break;
        }
        
        
    }  // Wait for RXD data to be received

    if(timeout_ms >= 0)
    {
        // clear the event and set rx_data with received byte
        NRF_UART0->EVENTS_RXDRDY = 0;
        *rx_data = (uint8_t)NRF_UART0->RXD;
    }

    return ret;
    */
}

void uart_put(uint8_t cr)
{
    NRF_UART0->TXD = (uint8_t)cr;

    while(NRF_UART0->EVENTS_TXDRDY != 1)
    {
        // Wait for TXD data to be sent
    }

    NRF_UART0->EVENTS_TXDRDY = 0;
}

void uart_putstring(const uint8_t* str)
{
    uint_fast8_t i = 0;
    uint8_t ch;

    ASSERT(NULL != str);

    ch = str[i++];
    while(ch != '\0')
    {
        uart_put(ch);
        ch = str[i++];
    }
}

void uart_put_buffer(uint8_t *p_buf,uint16_t len)
{
    while(len--)
    {
        uart_put(*p_buf);
        p_buf++;
    }
}

void uart_config(uint8_t rts_pin_number,
                 uint8_t txd_pin_number,
                 uint8_t cts_pin_number,
                 uint8_t rxd_pin_number,
                 bool    hwfc)
{
    /** @snippet [Configure UART RX and TX pin] */
    m_txd_pin_number       = txd_pin_number;

    nrf_gpio_cfg_output(txd_pin_number);
    nrf_gpio_pin_set(txd_pin_number);
    nrf_gpio_cfg_input(rxd_pin_number, NRF_GPIO_PIN_NOPULL);

    NRF_UART0->PSELTXD     = txd_pin_number;
    NRF_UART0->PSELRXD     = rxd_pin_number;
    /** @snippet [Configure UART RX and TX pin] */
    if(hwfc)
    {
        nrf_gpio_cfg_output(rts_pin_number);
        nrf_gpio_cfg_input(cts_pin_number, NRF_GPIO_PIN_NOPULL);
        NRF_UART0->PSELCTS = cts_pin_number;
        NRF_UART0->PSELRTS = rts_pin_number;
        NRF_UART0->CONFIG  = (UART_CONFIG_HWFC_Enabled << UART_CONFIG_HWFC_Pos);
    }
}

void uart_txd_pin_set(uint32_t pin)
{
    m_txd_pin_number = pin;
    NRF_UART0->PSELTXD = m_txd_pin_number;
}

void uart_baud_set(uint32_t rate)
{
    uint32_t baud_rate  = UART_BAUDRATE_BAUDRATE_Baud115200;
    switch(rate)
    {
        case 1200:
            m_baudrate      = 1200;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud1200;
            break;
        case 2400:
            m_baudrate      = 2400;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud2400;
            break;
        case 4800:
            m_baudrate      = 4800;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud4800;
            break;
        case 9600:
            m_baudrate      = 9600;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud9600;
            break;
        case 14400:
            m_baudrate      = 14400;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud14400;
            break;
        case 19200:
            m_baudrate      = 19200;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud19200;
            break;
        case 28800:
            m_baudrate      = 28800;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud28800;
            break;
        case 38400:
            m_baudrate      = 38400;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud38400;
            break;
        case 57600:
            m_baudrate      = 57600;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud57600;
            break;
        case 76800:
            m_baudrate      = 76800;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud76800;
            break;
        case 115200:
            m_baudrate      = 115200;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud115200;
            break;
        case 230400:
            m_baudrate      = 230400;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud230400;
            break;
        case 250000:
            m_baudrate      = 250000;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud250000;
            break;
        case 460800:
            m_baudrate      = 460800;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud460800;
            break;
        case 921600:
            m_baudrate      = 921600;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud921600;
            break;
        case 1000000:
            m_baudrate      = 1000000;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud1M;
            break;
        default:
            m_baudrate      = 115200;
            baud_rate       = UART_BAUDRATE_BAUDRATE_Baud115200;
            break;
    }
    NRF_UART0->BAUDRATE = (baud_rate << UART_BAUDRATE_BAUDRATE_Pos);
}

uint32_t uart_baud_get(void)
{
    return m_baudrate;
}

void uart_enable(void)
{
    NRF_UART0->ENABLE        = (UART_ENABLE_ENABLE_Enabled << UART_ENABLE_ENABLE_Pos);
    NRF_UART0->TASKS_STARTTX = 1;
    NRF_UART0->TASKS_STARTRX = 1;
    NRF_UART0->EVENTS_RXDRDY = 0;
    NRF_UART0->EVENTS_TXDRDY = 0;
}

void uart_disable(void)
{

    NRF_UART0->TASKS_STOPTX = 1;
    NRF_UART0->TASKS_STOPRX = 1;
    NRF_UART0->ENABLE       = (UART_ENABLE_ENABLE_Disabled << UART_ENABLE_ENABLE_Pos);

    NRF_UART0->EVENTS_RXTO = 0;
    NRF_UART0->EVENTS_RXDRDY = 0;
    NRF_UART0->EVENTS_CTS = 0;
    NRF_UART0->EVENTS_ERROR = 0;
    nrf_gpio_pin_set(m_txd_pin_number);
}


void uart_rx_int_enable(void)
{
    uint32_t err_code;
    NRF_UART0->INTENSET = UART_INTENSET_RXDRDY_Enabled << UART_INTENSET_RXDRDY_Pos;

    err_code = sd_nvic_ClearPendingIRQ(UART0_IRQn);
    APP_ERROR_CHECK(err_code);

    err_code = sd_nvic_SetPriority(UART0_IRQn, APP_IRQ_PRIORITY_LOW);
    APP_ERROR_CHECK(err_code);

    err_code = sd_nvic_EnableIRQ(UART0_IRQn);
    APP_ERROR_CHECK(err_code);
}

void uart_rx_int_disable(void)
{
    uint32_t err_code;

    NRF_UART0->INTENSET = UART_INTENSET_RXDRDY_Disabled << UART_INTENSET_RXDRDY_Pos;

    err_code = sd_nvic_DisableIRQ(UART0_IRQn);
    APP_ERROR_CHECK(err_code);
}

void uart_rx_handler_reg(uart_rx_handler_t handler)
{
    m_uart_rx_handler = handler;
}

void UART0_IRQHandler(void)
{
    if(NRF_UART0->EVENTS_RXDRDY == 1)
    {
        NRF_UART0->EVENTS_RXDRDY = 0;

        uint8_t  ch =(uint8_t)NRF_UART0->RXD;
        if(m_uart_rx_handler != NULL)
        {
            m_uart_rx_handler(ch);
        }
    }
}
