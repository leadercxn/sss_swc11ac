#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include "lwshell.h"

#include "ble_pong.h"
#include "trace.h"
//#include "uart.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "radio_config.h"
#include "app_timer.h"

#define BLE_PONG_STATE_IDLE 0
#define BLE_PONG_STATE_RX 1
#define BLE_PONG_STATE_TX 2

static ble_radio_config_t m_pong_tx_config;
static ble_radio_config_t m_pong_rx_config;

static ble_radio_event_handler_t m_radio_event_handler;

static uint32_t m_rx_packet = 0;
static uint32_t m_tx_packet = 0;

static uint32_t m_ble_pong_state = BLE_PONG_STATE_IDLE;

static void usage(void)
{
    printf("Available options: \n");
    printf(" -h print this help \n");
    printf(" -tp <int>   BLE TX_Power [-30,-20,-16,-12,-8,-4,0,4](dBm),default 4 \n");
    printf(" -rp <int>   BLE RX_Power [-30,-20,-16,-12,-8,-4,0,4](dBm),default 4 \n");
    printf(" -tc <uint>  tx_channel, default 0 \n");
    printf(" -rc <uint>  rx_channel, default 0 \n");
}

static void on_ble_radio_event_end_handler(void)
{
    int8_t rssi = 0;
    switch(m_ble_pong_state)
    {
        case BLE_PONG_STATE_RX:
            printf("OnRxDone \n");
            if(ble_radio_crc_status_get())
            {
                rssi = ble_radio_rssi_get();
                printf("data = %d, rssi = %d \n", m_rx_packet, rssi);

                m_tx_packet = rssi;

                ble_radio_config_set(m_pong_tx_config);
                ble_radio_packet_set((uint8_t *)&m_tx_packet);
                ble_radio_tx_enable();
                m_ble_pong_state = BLE_PONG_STATE_TX;
            }
            else
            {
                printf("Invalid crc \n");

                ble_radio_config_set(m_pong_rx_config);
                ble_radio_packet_set((uint8_t *)&m_rx_packet);
                ble_radio_rx_enable();
                m_ble_pong_state = BLE_PONG_STATE_RX;
            }
            break;

        case BLE_PONG_STATE_TX:
            printf("OnTxDone \n\n");

            ble_radio_config_set(m_pong_rx_config);
            ble_radio_packet_set((uint8_t *)&m_rx_packet);
            ble_radio_rx_enable();
            m_ble_pong_state = BLE_PONG_STATE_RX;
            break;

        default:
            break;
    }
}

int ble_pong_test(int argc, char **argv)
{
    uint8_t count = 0;
    int8_t   tx_power = 4;
    int8_t   tx_channel = 0;
    uint8_t  tx_mode = RADIO_MODE_MODE_Ble_1Mbit;

    int8_t    rx_power = 4;
    uint8_t   rx_channel = 2;
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
                trace_debug("error\n");
                printf("blepong: invalid tx_power arg\n");
                return 0;
            }
            if(tx_power != -30 && tx_power != -20 && tx_power != -16 && tx_power != -12 &&
               tx_power != -8 && tx_power != -4 && tx_power != 0 && tx_power != 4)
            {
                trace_debug("error \n");
                printf("blepong: invalid  tx_power arg \n");
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
                printf("blepong: invalid rx_power arg \n");
                return 0;
            }
            if(rx_power != -30 && rx_power != -20 && rx_power != -16 && rx_power != -12 &&
               rx_power != -8 && rx_power != -4 && rx_power != 0 && rx_power != 4)
            {
                printf("blepong: invalid  rx_power arg \n");
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
            printf("blepong: invalid arg\n");
            return 0;
        }
    }

    printf("tx_power   = %d \n", tx_power);
    printf("tx_channel   = %d \n", tx_channel);
    printf("rx_power   = %d \n", rx_power);
    printf("rx_channel   = %d \n", rx_channel);

    m_pong_tx_config.power   = tx_power;
    m_pong_tx_config.channel = tx_channel;
    m_pong_tx_config.mode    = tx_mode;

    m_pong_rx_config.power   = rx_power;
    m_pong_rx_config.channel = rx_channel;
    m_pong_rx_config.mode    = rx_mode;

    m_radio_event_handler.on_event_end_handler = on_ble_radio_event_end_handler;
    ble_radio_switch_init();
    ble_radio_init(&m_radio_event_handler);
    ble_radio_config_set(m_pong_rx_config);
    ble_radio_packet_set((uint8_t *)&m_rx_packet);
    ble_radio_rx_enable();
    m_ble_pong_state = BLE_PONG_STATE_RX;

    printf("blepong: start, ctrl+c to stop\n\n");
    while(true)
    {
        int c = getchar();
        if(c == 0x03)   // ^C
        {
            ble_radio_disable();

            NVIC_ClearPendingIRQ(RADIO_IRQn);
            NVIC_DisableIRQ(RADIO_IRQn);

            printf("blepong test: end\n");
            break;
        }
    }
    return 0;
}
