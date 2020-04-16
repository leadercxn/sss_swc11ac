#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include "lwshell.h"

#include "ble_radio_rx.h"
#include "trace.h"
//#include "uart.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_delay.h"
#include "radio_config.h"
#include "app_timer.h"
#include "app_util_platform.h"

static ble_radio_config_t rx_radio_config ;

static ble_radio_event_handler_t m_radio_event_handler;

uint32_t m_rx_packet;

static void usage(void)
{
    printf("Available options:\n");
    printf(" -h print this help\n");
    printf(" -c <uint>  rx_channel, default 0\n");
}

static void on_ble_radio_event_end_handler(void)
{
    int8_t rssi = 0;
    printf("OnRxDone \n");

    rssi = ble_radio_rssi_get();
    printf("RX freq = %d MHz, RSSI = %d\r\n", rx_radio_config.channel + 2400, rssi);

    ble_radio_rx_enable();
}

int ble_radio_rx_test(int argc, char *argv[])
{
    uint8_t   count = 0;
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
        else if(strcmp(argv[i], "-c") == 0)
        {
            i++;
            count = sscanf(argv[i], "%u", (unsigned int *)&rx_channel);
            if(count != 1)
            {
                trace_debug("error \n");
                printf("blerx: invalid arg \n");
                return 0;
            }
            i++;
        }
        else
        {
            printf("blerx: invalid arg \n");
            return 0;
        }
    }

    printf("rx_channel   = %d\n", rx_channel);

    rx_radio_config.channel = rx_channel;
    rx_radio_config.mode    = rx_mode;

    m_radio_event_handler.on_event_end_handler = on_ble_radio_event_end_handler;
    ble_radio_switch_init();
    ble_radio_init(&m_radio_event_handler);
    ble_radio_config_set(rx_radio_config);
    ble_radio_packet_set((uint8_t *)&m_rx_packet);
    ble_radio_rx_enable();

    printf("blerx: start, ctrl+c to stop \n\n");

    while(true)
    {
        int c = getchar();
        if(c == 0x03)
        {
            ble_radio_disable();

            NVIC_ClearPendingIRQ(RADIO_IRQn);
            NVIC_DisableIRQ(RADIO_IRQn);

            printf("blerx rest: end \n");
            break;
        }
    }
    return 0;
}
