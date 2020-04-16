#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include "lwshell.h"

#include "trace.h"
//#include "uart.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "radio_config.h"
#include "app_timer.h"
#include "timer.h"

static ble_radio_config_t tx_radio_config;

static uint32_t m_tx_packet = 0;

static ble_radio_event_handler_t m_radio_event_handler;

APP_TIMER_DEF(m_ble_tx_timer);

static void usage(void)
{
    printf("Available options: \n");
    printf(" -h print this help \n");
    printf(" -p <int>   BLE TX_Power [-30,-20,-16,-12,-8,-4,0,4](dBm),default 4 \n");
    printf(" -c <uint>  tx_channel, default 0 \n");
}

static void bletx_timer_handler(void *p_context)
{
    m_tx_packet++;
    printf("TX: %d\r\n", m_tx_packet);
    ble_radio_packet_set((uint8_t *)&m_tx_packet);
    ble_radio_tx_enable();
}

static void on_ble_radio_event_end_handler(void)
{
    printf("OnTxDone \n\n");
}

int ble_radio_tx_test(int argc, char **argv )
{
    static bool timer_created = false;

    uint8_t  count = 0;
    int8_t   tx_power = 0;
    int8_t   tx_channel = 0;
    uint8_t  tx_mode = RADIO_MODE_MODE_Ble_1Mbit;

    for(uint8_t i = 1; i < argc;)
    {
        if(strcmp(argv[i], "-h") == 0)
        {
            i++;
            usage();
            return false;
        }
        else if(strcmp(argv[i], "-p") == 0)
        {
            i++;
            count = sscanf(argv[i], "%d", (int *)&tx_power);
            if(count != 1)
            {
                trace_debug("error \n");
                printf("bletx: invalid arg \n");
                return false;
            }
            if(tx_power != -30 && tx_power != -20 && tx_power != -16 && tx_power != -12 &&
               tx_power != -8 && tx_power != -4 && tx_power != 0 && tx_power != 4)
            {
                trace_debug("error \n");
                printf("bletx: invalid arg \n");
                return false;
            }
            i++;
        }
        else if(strcmp(argv[i], "-c") == 0)
        {
            i++;
            tx_channel = atoi(argv[i]);
            i++;

        }
        else
        {
            printf("bletx: invalid arg \n");
            return false;
        }
    }

    printf("tx_power   = %d \n", tx_power);
    printf("tx_channel   = %d \n", tx_channel);

    tx_radio_config.power   = tx_power;
    tx_radio_config.channel = tx_channel;
    tx_radio_config.mode    = tx_mode;

    m_radio_event_handler.on_event_end_handler = on_ble_radio_event_end_handler;
    ble_radio_switch_init();
    ble_radio_init(&m_radio_event_handler);
    ble_radio_config_set(tx_radio_config);

    if(!timer_created)
    {
        timer_created = true;
        TIMER_CREATE(&m_ble_tx_timer, APP_TIMER_MODE_REPEATED, bletx_timer_handler);
    }

    // timer_start(m_ble_tx_timer, 500);
     TIMER_START(m_ble_tx_timer, 500);

    printf("bletx: start, ctrl+c to stop\n\n");
    while(true)
    {
        int c = getchar();
        if(c == 0x03)   // ^C
        {
            ble_radio_disable();

            NVIC_ClearPendingIRQ(RADIO_IRQn);
            NVIC_DisableIRQ(RADIO_IRQn);

            TIMER_STOP(m_ble_tx_timer);
            printf("bletx test: end \n");
            break;
        }
    }
    return false;
}

