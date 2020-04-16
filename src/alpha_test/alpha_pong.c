/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_uart.h"
#include "app_util.h"
#include "app_timer.h"
#include "timer.h"

#include "radio.h"
#include "sx1276.h"
#include "sx1276_board.h"
#include "nrf_delay.h"
#include "util.h"
#include "trace.h"


#include "shell_cmd.h"

#include "alpha_pong.h"

#define ALPHA_MAC_PUBLIC_SYNCWORD                    0x34
#define ALPHA_FIX_LENGTH_PAYLOAD_ON                  false

APP_TIMER_DEF(m_timer);                     // Normal task timer

static double m_tx_freq    = 506.7;
static double m_rx_freq    = 486.5;
static uint8_t      m_sf       = 7;
static int          m_power    = 14;

static uint8_t      m_bw       = 0;
static uint8_t      m_cr       = 1;
static uint8_t      m_pl       = 8;
static uint8_t      m_st       = 5;

static uint8_t      m_tx_buff[256];
static uint16_t     m_tx_buff_len = 0;

static void OnTxDone(void)
{
    TIMER_STOP(m_timer);
    printf("OnTxDone\r\n");

    static uint32_t count = 0;

    Radio.Sleep();

    if(count % 2 == 0)
    {
        // 发完之后, 得再发一次, ping端收到的RSSI才会正常
        Radio.SetChannel((uint32_t)(m_tx_freq * 1e6));
        SX1276SetTxConfig(MODEM_LORA,               //mode
                          m_power,                    //power
                          0,                          //fdev
                          m_bw,                       //bw
                          7,                          //sf
                          m_cr,                       //cr
                          1,                          //preambleLen
                          ALPHA_FIX_LENGTH_PAYLOAD_ON, //fixLen
                          false,                      //crcOn
                          0,                          //FreqHopOn
                          0,                          //HopPeriod
                          true,                       //iqInverted
                          3000);                      //timeout
        SX1276Send(m_tx_buff, 0);
    }
    else
    {
        Radio.SetChannel((uint32_t)(m_rx_freq * 1e6));
        Radio.SetRxConfig(MODEM_LORA,
                          m_bw,                       //bw
                          m_sf,                       //sf
                          m_cr,                       //cr
                          0,                          //bandwidthAfc
                          m_pl,                       //preambleLen
                          m_st,                       //symbTimeout
                          ALPHA_FIX_LENGTH_PAYLOAD_ON, //fixLen
                          0,                          //payloadLen
                          true,                       //crcOn
                          0,                          //FreqHopOn
                          0,                          //HopPeriod
                          false,                      //iqInverted
                          true);                      //rxContinuous

        Radio.Rx(0);   // Continuous Rx
    }
    count++;
}

static void OnTxTimeout(void)
{
    printf("OnTxTimeout\r\n");
    Radio.Sleep();

    Radio.SetChannel((uint32_t)(m_rx_freq * 1e6));
    Radio.SetRxConfig(MODEM_LORA,
                      m_bw,                       //bw
                      m_sf,                       //sf
                      m_cr,                       //cr
                      0,                          //bandwidthAfc
                      m_pl,                       //preambleLen
                      m_st,                       //symbTimeout
                      ALPHA_FIX_LENGTH_PAYLOAD_ON, //fixLen
                      0,                          //payloadLen
                      true,                       //crcOn
                      0,                          //FreqHopOn
                      0,                          //HopPeriod
                      false,                      //iqInverted
                      true);                      //rxContinuous

    Radio.Rx(0);   // Continuous Rx

}

static void OnRxTimeout(void)
{
    printf("OnRxTimeout\r\n");
    Radio.Sleep();

    Radio.SetChannel((uint32_t)(m_rx_freq * 1e6));
    Radio.SetRxConfig(MODEM_LORA,
                      m_bw,                       //bw
                      m_sf,                       //sf
                      m_cr,                       //cr
                      0,                          //bandwidthAfc
                      m_pl,                       //preambleLen
                      m_st,                       //symbTimeout
                      ALPHA_FIX_LENGTH_PAYLOAD_ON, //fixLen
                      0,                          //payloadLen
                      true,                       //crcOn
                      0,                          //FreqHopOn
                      0,                          //HopPeriod
                      false,                       //iqInverted
                      true);                      //rxContinuous

    Radio.Rx(0);   // Continuous Rx
}



static void OnRxDone(uint8_t * payload, uint16_t size, int16_t rssi, int8_t snr)
{
    printf("OnRxDone\r\n");
    for(uint16_t i = 0; i < size; i++)
    {
        printf("%02x ", payload[i]);
    }
    printf("\r\n");
    Radio.Sleep();

    memcpy(m_tx_buff, payload, size);
    m_tx_buff[size] = (uint8_t)rssi;
    m_tx_buff[size + 1] = (uint8_t)(rssi >> 8);
    m_tx_buff[size + 2] = snr;
    m_tx_buff_len = size + 3;

    Radio.SetChannel((uint32_t)(m_tx_freq * 1e6));
    Radio.SetTxConfig(MODEM_LORA,                   //mode
                      m_power,                    //power
                      0,                          //fdev
                      m_bw,                       //bw
                      m_sf,                       //sf
                      m_cr,                       //cr
                      m_pl,                       //preambleLen
                      ALPHA_FIX_LENGTH_PAYLOAD_ON, //fixLen
                      false,                      //crcOn
                      0,                          //FreqHopOn
                      0,                          //HopPeriod
                      true,                   //iqInverted
                      3000);                      //timeout

    Radio.Send(m_tx_buff, m_tx_buff_len);
    TIMER_START(m_timer, 3000);
}

static RadioEvents_t RadioEvents =
{
    .TxDone = OnTxDone,
    .RxDone = OnRxDone,
    .RxError = NULL,
    .TxTimeout = OnTxTimeout,
    .RxTimeout = OnRxTimeout,
    .CadDone = NULL
};

static void pong_timer_handler(void * p_context)
{
    Radio.OnTimeout();
}

static void usage(void)
{
    printf("Available options:\n");
    printf(" -h print this help\n");
    printf(" -tf <float> tx frequency in MHz, default 433.5\n");
    printf(" -rf <float> rx frequency in MHz, default 433.5\n");
    printf(" -b <uint>   bandwidth in kHz [0:125, 1:250, 2:500], default 0:125\n");
    printf(" -s <uint>   Spreading Factor [7-12], default 11\n");
    printf(" -c <uint>   Coding Rate [1:4/5, 2:4/6, 3:5/7, 4:4/8],default 1:4/5\n");
    printf(" -p <int>    RF power (dBm),default 14\n");
    printf(" -l <uint>   preamble length (symbols), default 8\n");
    printf(" -sy         Cfg sync word\n");
    printf(" -rfo        Use rfo output pin\n");
}

int alpha_pong(int argc, char * argv[])
{
    uint8_t count = 0;
    uint8_t pa_select = RF_PACONFIG_PASELECT_PABOOST;

    bool sync   = false;

    for(uint8_t i = 1; i < argc;)
    {
        trace_debug("arg = %s\r\n", argv[i]);
        if(strcmp(argv[i], "-h") == 0)
        {
            i++;
            usage();
            return 0;
        }
        else if(strcmp(argv[i], "-tf") == 0)
        {
            i++;
            count = sscanf(argv[i], "%lf", &m_tx_freq);
            if(count != 1)
            {
                trace_debug("error\r\n");
                printf("alphapong: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-rf") == 0)
        {
            i++;
            count = sscanf(argv[i], "%lf", &m_rx_freq);
            if(count != 1)
            {
                trace_debug("error\r\n");
                printf("alphapong: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-b") == 0)
        {
            i++;
            m_bw = atoi(argv[i]);
            if(m_bw > 2)
            {
                trace_debug("error\r\n");
                printf("alphapong: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-s") == 0)
        {
            i++;
            m_sf = atoi(argv[i]);
            if(m_sf > 12 || m_sf < 7)
            {
                trace_debug("error\r\n");
                printf("alphapong: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-p") == 0)
        {
            i++;
            count = sscanf(argv[i], "%d", (unsigned int *)&m_power);
            if(count != 1)
            {
                trace_debug("error\r\n");
                printf("alphapong: invalid arg\r\n");
                return 0;
            }
            if(m_power > 20)
            {
                trace_debug("error\r\n");
                printf("alphapong: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-c") == 0)
        {
            i++;
            m_cr = atoi(argv[i]);
            if(m_cr > 4 || m_cr < 1)
            {
                trace_debug("error\r\n");
                printf("alphapong: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-l") == 0)
        {
            i++;
            count = sscanf(argv[i], "%u", (unsigned int *)&m_pl);
            if(count != 1)
            {
                trace_debug("error\r\n");
                printf("alphapong: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-sy") == 0)
        {
            i++;
            sync = true;
        }
        else if(strcmp(argv[i], "-rfo") == 0)
        {
            i++;
            pa_select = RF_PACONFIG_PASELECT_RFO;
        }
        else
        {
            trace_debug("error\r\n");
            printf("alphapong: invalid arg\r\n");
            return 0;
        }

    }
    printf("tx freq    = %d\r\n", (uint32_t)(m_tx_freq * 1e6));
    printf("rx freq    = %d\r\n", (uint32_t)(m_rx_freq * 1e6));
    printf("bw      = %d\r\n", m_bw);
    printf("sf      = %d\r\n", m_sf);
    printf("power   = %d\r\n", m_power);
    printf("cr      = %d\r\n", m_cr);
    printf("pl      = %d symb\r\n", m_pl);
    printf("sync    = %d\r\n", sync);
    printf("pacfg   = %02X\r\n", pa_select);

    TIMER_CREATE(&m_timer, APP_TIMER_MODE_SINGLE_SHOT, pong_timer_handler);

    Radio.Init(&RadioEvents);

    // Radio.SetPaSelect(pa_select);

    if(sync == true)
    {
        Radio.SetModem(MODEM_LORA);
        Radio.Write(REG_LR_SYNCWORD, ALPHA_MAC_PUBLIC_SYNCWORD);
    }

    Radio.SetChannel((uint32_t)(m_rx_freq * 1e6));

    Radio.SetRxConfig(MODEM_LORA,
                      m_bw,                       //bw
                      m_sf,                       //sf
                      m_cr,                       //cr
                      0,                          //bandwidthAfc
                      m_pl,                       //preambleLen
                      m_st,                       //symbTimeout
                      ALPHA_FIX_LENGTH_PAYLOAD_ON, //fixLen
                      0,                          //payloadLen
                      true,                       //crcOn
                      0,                          //FreqHopOn
                      0,                          //HopPeriod
                      false,                      //iqInverted
                      true);                      //rxContinuous

    Radio.Rx(0);   // Continuous Rx

    printf("start\r\n");

    while(1)
    {
        uint8_t c;
        while(app_uart_get(&c) == NRF_ERROR_NOT_FOUND)
        {
        }

        if(c == 0x03)   // ^C
        {
            TIMER_STOP(m_timer);
            Radio.Sleep();
            printf("alphapong: end\r\n");
            break;
        }
    }

    return 0;
}

