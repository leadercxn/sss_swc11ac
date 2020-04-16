/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "nordic_common.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "radio_config.h"

#include "app_uart.h"
#include "app_timer.h"
#include "timer.h"

#include "ble_ping.h"

#include "trace.h"

#define BLE_PING_STATE_IDLE 0
#define BLE_PING_STATE_RX 1
#define BLE_PING_STATE_TX 2

static ble_radio_config_t m_ping_tx_config;
static ble_radio_config_t m_ping_rx_config;

static uint32_t m_ping_rx_num = 0;
static uint32_t m_ping_tx_num = 0;

APP_TIMER_DEF(m_ble_ping_tx_timer);
APP_TIMER_DEF(m_ble_ping_timeout_timer);

static ble_radio_event_handler_t m_radio_event_handler;

static uint32_t m_rx_packet = 0;
static uint32_t m_tx_packet = 0;

static uint32_t m_ble_ping_state = BLE_PING_STATE_IDLE;

static void usage(void)
{
    printf("Available options:\r\n");
    printf(" -h print this help\r\n");
    printf(" -tp <int>   BLE TX_Power [-30,-20,-16,-12,-8,-4,0,4](dBm),default 4\r\n");
    printf(" -rp <int>   BLE RX_Power [-30,-20,-16,-12,-8,-4,0,4](dBm),default 4\r\n");
    printf(" -tc <uint>  tx_channel, default 0\r\n");
    printf(" -rc <uint>  rx_channel, default 0\r\n");
}

static void ble_ping_tx_timer_handler(void * p_context)
{
    m_tx_packet++;
    ble_radio_config_set(m_ping_tx_config);
    ble_radio_packet_set((uint8_t *)&m_tx_packet);
    ble_radio_tx_enable();
    m_ble_ping_state = BLE_PING_STATE_TX;
}

static void ble_ping_timeout_handler(void * p_context)
{
    NRF_RADIO->TASKS_DISABLE   = 1;
    while(NRF_RADIO->EVENTS_DISABLED == 0);
    NRF_RADIO->TASKS_TXEN = 0U;
    NRF_RADIO->TASKS_RXEN = 0U;

    printf("OnRxTimeout\r\n");
    printf("tx = %d, rx = %d\r\n", m_ping_tx_num, m_ping_rx_num);
    printf("lost rate = %f\r\n\r\n", ((float)m_ping_tx_num - (float)m_ping_rx_num) / (float)m_ping_tx_num * 100);

    TIMER_START(m_ble_ping_tx_timer, 500);
}

static void on_ble_radio_event_end_handler(void)
{
    int8_t rssi = 0;
    switch(m_ble_ping_state)
    {
        case BLE_PING_STATE_RX:
            TIMER_STOP(m_ble_ping_timeout_timer);

            printf("OnRxDone\r\n");
            if(ble_radio_crc_status_get())
            {
                m_ping_rx_num++;
                rssi = ble_radio_rssi_get();
                printf("TX_RSSI = %d, RX_RSSI = %d\r\n", m_rx_packet, rssi);
            }
            else
            {
                printf("Invalid crc\r\n");
            }

            printf("tx = %d, rx = %d\r\n", m_ping_tx_num, m_ping_rx_num);
            printf("lost rate = %f\r\n\r\n", ((float)m_ping_tx_num - (float)m_ping_rx_num) / (float)m_ping_tx_num * 100);

            TIMER_START(m_ble_ping_tx_timer, 500);
            break;

        case BLE_PING_STATE_TX:
            printf("OnTxDone\r\n");

            m_ping_tx_num++;

            ble_radio_config_set(m_ping_rx_config);
            ble_radio_packet_set((uint8_t *)&m_rx_packet);
            ble_radio_rx_enable();
            m_ble_ping_state = BLE_PING_STATE_RX;

            TIMER_START(m_ble_ping_timeout_timer, 500);
            break;

        default:
            break;
    }
}

int ble_ping(int argc, char ** argv)
{
    uint8_t  count = 0;
    int8_t   tx_power = 4;
    int8_t   tx_channel = 2;
    uint8_t  tx_mode = RADIO_MODE_MODE_Ble_1Mbit;

    int8_t    rx_power = 4;
    uint8_t   rx_channel = 0;
    uint8_t   rx_mode = RADIO_MODE_MODE_Ble_1Mbit;

    for(uint8_t i = 1; i < argc;)
    {
        if(strcmp(argv[i], "-h") == 0)
        {
            i++;
            usage();
            return 0;
        }
        else if(strcmp(argv[i], "-tp") == 0)
        {
            i++;
            count = sscanf(argv[i], "%d", (int *)&tx_power);
            if(count != 1)
            {
                trace_debug("error\r\n");
                printf("bleping: invalid tx_power arg\r\n");
                return 0;
            }
            if(tx_power != -30 && tx_power != -20 && tx_power != -16 && tx_power != -12 &&
               tx_power != -8 && tx_power != -4 && tx_power != 0 && tx_power != 4)
            {
                trace_debug("error\r\n");
                printf("bleping: invalid  tx_power arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-rp") == 0)
        {
            i++;
            count = sscanf(argv[i], "%d", (int *)&rx_power);
            if(count != 1)
            {
                printf("bleping: invalid rx_power arg\r\n");
                return 0;
            }
            if(rx_power != -30 && rx_power != -20 && rx_power != -16 && rx_power != -12 &&
               rx_power != -8 && rx_power != -4 && rx_power != 0 && rx_power != 4)
            {
                printf("bleping: invalid  rx_power arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-tc") == 0)
        {
            i++;
            tx_channel = atoi(argv[i]);
            i++;

        }
        else if(strcmp(argv[i], "-rc") == 0)
        {
            i++;
            rx_channel = atoi(argv[i]);
            i++;
        }
        else
        {
            printf("bleping: invalid arg\r\n");
            return 0;
        }
    }

    printf("tx_power   = %d\r\n", tx_power);
    printf("tx_channel   = %d\r\n", tx_channel);
    printf("rx_power   = %d\r\n", rx_power);
    printf("rx_channel   = %d\r\n", rx_channel);

    m_ping_rx_num = 0;
    m_ping_tx_num = 0;

    m_ping_tx_config.power   = tx_power;
    m_ping_tx_config.channel = tx_channel;
    m_ping_tx_config.mode    = tx_mode;

    m_ping_rx_config.power   = rx_power;
    m_ping_rx_config.channel = rx_channel;
    m_ping_rx_config.mode    = rx_mode;

    m_radio_event_handler.on_event_end_handler = on_ble_radio_event_end_handler;
    ble_radio_switch_init();
    ble_radio_init(&m_radio_event_handler);
    ble_radio_config_set(m_ping_tx_config);
    ble_radio_packet_set((uint8_t *)&m_tx_packet);
    ble_radio_tx_enable();
    m_ble_ping_state = BLE_PING_STATE_TX;

    TIMER_CREATE(&m_ble_ping_tx_timer, APP_TIMER_MODE_SINGLE_SHOT, ble_ping_tx_timer_handler);
    TIMER_CREATE(&m_ble_ping_timeout_timer, APP_TIMER_MODE_SINGLE_SHOT, ble_ping_timeout_handler);

    printf("bleping: start, ctrl+c to stop \r\n\r\n");
    while(true)
    {
        uint8_t c;
        while(app_uart_get(&c) == NRF_ERROR_NOT_FOUND)
        {
        }

        if(c == 0x03)   // ^C
        {
            ble_radio_disable();

            NVIC_ClearPendingIRQ(RADIO_IRQn);
            NVIC_DisableIRQ(RADIO_IRQn);

            TIMER_STOP(m_ble_ping_tx_timer);
            TIMER_STOP(m_ble_ping_timeout_timer);
            printf("bleping: end\r\n");
            break;
        }
    }
    return 0;
}
