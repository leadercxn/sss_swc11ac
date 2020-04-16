/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "nrf_delay.h"
#include "app_uart.h"
#include "app_timer.h"
#include "util.h"
#include "timer.h"

#include "radio.h"
#include "sx1276.h"
#include "sx1276_board.h"

#include "trace.h"

#include "shell_cmd.h"

#include "alpha_ping.h"

#define ALPHA_MAC_PUBLIC_SYNCWORD                    0x34

#define ALPHA_BANDWIDTH                              0         // [0: 125 kHz,
//  1: 250 kHz,
//  2: 500 kHz,
//  3: Reserved]
#define ALPHA_SPREADING_FACTOR                       11        // [SF7..SF12]
#define ALPHA_CODINGRATE                             1         // [1: 4/5,
//  2: 4/6,
//  3: 4/7,
//  4: 4/8]
#define ALPHA_SYMBOL_TIMEOUT                         5         // Symbols
#define ALPHA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define ALPHA_FIX_LENGTH_PAYLOAD_ON                  false
#define ALPHA_RX_IQ_INVERSION_ON                     false
#define ALPHA_TX_IQ_INVERSION_ON                     false

APP_TIMER_DEF(m_timer);                     // Normal task timer


static uint8_t m_buff[255];
static uint8_t m_buff_len = 21; // m_buff_len =20 时，会出现tx_rssi = 123xx 的bug
static uint16_t m_count   = 0;

static int m_repeat       = -1;
static int m_delay        = 0;

static double m_tx_freq    = 486.5;
static double m_rx_freq    = 506.7;
static uint8_t m_sf       = 7;
static int m_power        = 14;

static uint8_t m_bw       = 0;
static uint8_t m_cr       = 1;
static uint8_t m_pl       = 8;
static uint8_t m_st       = 5;
static uint8_t m_id       = 0;

static uint32_t m_tx_count = 0;
static uint32_t m_rx_count = 0;
static uint32_t m_err_count = 0;


static void OnTxDone(void)
{
    printf("OnTxDone\r\n");

    TIMER_STOP(m_timer);

    m_tx_count++;

    Radio.Sleep();

    Radio.SetChannel((uint32_t)(m_rx_freq * 1e6));

    Radio.SetRxConfig(MODEM_LORA, m_bw, m_sf,
                      m_cr, 0, m_pl,
                      m_st, false,
                      0, true, 0, 0, true, true);

    Radio.Rx(3000);   // Continuous Rx
    TIMER_START(m_timer, 3000);
}

static void OnTxTimeout(void)
{
    Radio.Send(m_buff, m_buff_len);
    TIMER_START(m_timer, 3000);
    printf("OnTxTimeout\r\n");
}

static void OnRxTimeout(void)
{
    printf("OnRxTimeout\r\n");
    Radio.Sleep();

    printf("tx = %d, rx = %d, err = %d\r\n", m_tx_count, m_rx_count, m_err_count);
    printf("lost rate = %f, err rate = %f\r\n\r\n", ((float)m_tx_count - (float)m_rx_count) / (float)m_tx_count * 100, (float)m_err_count / (float)m_tx_count * 100);


    if(m_tx_count < m_repeat || m_repeat == -1)
    {
        nrf_delay_ms(m_delay);

        m_buff[1] = m_tx_count >> 8;
        m_buff[2] = m_tx_count & 0xff;

        Radio.SetChannel((uint32_t)(m_tx_freq * 1e6));

        Radio.SetTxConfig(MODEM_LORA, m_power, 0, m_bw,
                          m_sf, m_cr,
                          m_pl, ALPHA_FIX_LENGTH_PAYLOAD_ON,
                          true, 0, 0, false, 3000);

        Radio.Send(m_buff, m_buff_len);
        TIMER_START(m_timer, 3000);
    }
}

static void OnRxDone(uint8_t * payload, uint16_t size, int16_t rssi, int8_t snr)
{
    printf("OnRxDone\r\n");
    trace_dump(payload, size);
    int16_t tx_rssi = 0;
    int8_t  tx_snr = 0;

    TIMER_STOP(m_timer);

    Radio.Sleep();
    uint8_t i = 0;

    if(
        (payload[0] == (m_id & 0xff)) &&
        (payload[1] == ((m_tx_count - 1) >> 8)) &&
        (payload[2] == ((m_tx_count - 1) & 0xff))
    )
    {
        for(i = 3; i < m_buff_len; i++)
        {
            if(payload[i] != i)
            {
                m_err_count++;
                break;
            }
        }
    }
    else
    {
        m_err_count++;
    }
    if(i == m_buff_len)
    {
        m_rx_count++;
    }
    tx_rssi |= payload[m_buff_len];
    tx_rssi |= (payload[m_buff_len + 1] & 0x00ff) << 8;
    tx_snr = payload[m_buff_len + 2];

    printf("RX_RSSI = %d, RX_SNR = %d\r\n", rssi, snr);
    printf("TX_RSSI = %d, TX_SNR = %d\r\n", tx_rssi, tx_snr);
    printf("tx = %d, rx = %d, err = %d\r\n", m_tx_count, m_rx_count, m_err_count);
    printf("lost rate = %f, err rate = %f\r\n\r\n", ((float)m_tx_count - (float)m_rx_count) / (float)m_tx_count * 100, (float)m_err_count / (float)m_tx_count * 100);
    nrf_delay_ms(10);

    if(m_tx_count < m_repeat || m_repeat == -1)
    {
        nrf_delay_ms(m_delay);

        m_buff[1] = m_tx_count >> 8;
        m_buff[2] = m_tx_count & 0xff;

        Radio.SetChannel((uint32_t)(m_tx_freq * 1e6));

        Radio.SetTxConfig(MODEM_LORA, m_power, 0, m_bw,
                          m_sf, m_cr,
                          m_pl, ALPHA_FIX_LENGTH_PAYLOAD_ON,
                          true, 0, 0, false, 3000);

        Radio.Send(m_buff, m_buff_len);
        TIMER_START(m_timer, 3000);
    }
}

static RadioEvents_t PingTestEvents =
{
    .TxDone = OnTxDone,
    .RxDone = OnRxDone,
    .RxError = NULL,
    .TxTimeout = OnTxTimeout,
    .RxTimeout = OnRxTimeout,
    .CadDone = NULL
};

static void ping_timer_handler(void * p_context)
{
    Radio.OnTimeout();
}

static void usage(void)
{
    printf("Available options:\n");
    printf(" -h print this help\n");
    printf(" -tf <float> tx frequency in MHz, default 433.5\n");
    printf(" -rf <float> rx frequency in MHz, default 433.5\n");
    printf(" -b <uint>  bandwidth in kHz [0:125, 1:250, 2:500], default 0:125\n");
    printf(" -s <uint>  Spreading Factor [7-12], default 11\n");
    printf(" -c <uint>  Coding Rate [1:4/5, 2:4/6, 3:5/7, 4:4/8],default 1:4/5\n");
    printf(" -p <int>   RF power (dBm),default 14\n");
    printf(" -l <uint>  preamble length (symbols), default 8\n");
    printf(" -sy        Cfg sync word\n");
    printf(" -t <uint>  pause between packets (ms),default 0\n");
    printf(" -r <uint>  repeat times, (-1 loop until stopped,default -1)\n");
    printf(" -bl <uint> tx buffer len, default 20\n");
    printf(" -id <uint> set a id, default 0\n");
    printf(" -rfo use rfo output pin\n");
}

int alpha_ping(int argc, char ** argv)
{
    uint8_t count = 0;
    uint8_t pa_select = RF_PACONFIG_PASELECT_PABOOST;

    bool sync     = false;
    m_id     = 0;
    m_repeat = -1;
    m_delay  = 0;


    m_tx_count = 0;
    m_rx_count = 0;
    m_err_count = 0;

    for(uint8_t i = 1; i < argc;)
    {
        trace_debug("arg = %s\r\n", argv[i]);
        if(strcmp(argv[i], "-h") == 0)
        {
            i++;
            usage();
            return 0;
        }
        else if(strcmp(argv[i], "-f") == 0)
        {
            i++;
            count = sscanf(argv[i], "%lf", &m_tx_freq);
            if(count != 1)
            {
                trace_debug("error\r\n");
                printf("alphaping: invalid arg\r\n");
                return 0;
            }
            i++;
            m_rx_freq = m_tx_freq;
        }
        else if(strcmp(argv[i], "-tf") == 0)
        {
            i++;
            count = sscanf(argv[i], "%lf", &m_tx_freq);
            if(count != 1)
            {
                trace_debug("error\r\n");
                printf("alphaping: invalid arg\r\n");
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
                printf("alphaping: invalid arg\r\n");
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
                printf("alphaping: invalid arg\r\n");
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
                printf("alphaping: invalid arg\r\n");
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
                printf("alphaping: invalid arg\r\n");
                return 0;
            }
            if(m_power > 20)
            {
                trace_debug("error\r\n");
                printf("alphaping: invalid arg\r\n");
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
                printf("alphaping: invalid arg\r\n");
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
                printf("alphaping: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-t") == 0)
        {
            i++;
            count = sscanf(argv[i], "%u", (unsigned int *)&m_delay);
            if(count != 1)
            {
                trace_debug("error\r\n");
                printf("alphaping: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-r") == 0)
        {
            i++;
            count = sscanf(argv[i], "%d", (unsigned int *)&m_repeat);
            if(count != 1)
            {
                trace_debug("error\r\n");
                printf("alphaping: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-bl") == 0)
        {
            i++;
            m_buff_len = atoi(argv[i]);
            if(m_buff_len < 5 || m_buff_len > 51)
            {
                trace_debug("error\r\n");
                printf("alphaping: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-sy") == 0)
        {
            i++;
            sync = true;
        }
        else if(strcmp(argv[i], "-id") == 0)
        {
            i++;
            sscanf(argv[i], "%u", (unsigned int *)&m_id);
            i++;
        }
        else if(strcmp(argv[i], "-rfo") == 0)
        {
            i++;
            pa_select = RF_PACONFIG_PASELECT_RFO;
        }
        else
        {
            trace_debug("error\r\n");
            printf("alphaping: invalid arg\r\n");
            return 0;
        }
    }
    printf("tx_freq = %u\r\n", (uint32_t)(m_tx_freq * 1e6));
    printf("rx_freq = %u\r\n", (uint32_t)(m_rx_freq * 1e6));
    printf("bw      = %d\r\n", m_bw);
    printf("sf      = %d\r\n", m_sf);
    printf("power   = %d\r\n", m_power);
    printf("cr      = %d\r\n", m_cr);
    printf("pl      = %d symb\r\n", m_pl);
    printf("sync    = %d\r\n", sync);
    printf("delay   = %d\r\n", m_delay);
    printf("repeat  = %d\r\n", m_repeat);
    printf("tx len  = %d\r\n", m_buff_len);
    printf("id      = %02x\r\n", m_id);
    printf("pacfg   = %02X\r\n", pa_select);


    m_count = 0;
    m_buff[0] = m_id;
    m_buff[1] = m_count >> 8;
    m_buff[2] = m_count & 0xff;
    for(uint8_t i = 3; i < m_buff_len; i++)
    {
        m_buff[i] = i;
    }

    TIMER_CREATE(&m_timer, APP_TIMER_MODE_SINGLE_SHOT, ping_timer_handler);

    Radio.Init(&PingTestEvents);

    // Radio.SetPaSelect(pa_select);

    if(sync == true)
    {
        Radio.SetModem(MODEM_LORA);
        Radio.Write(REG_LR_SYNCWORD, ALPHA_MAC_PUBLIC_SYNCWORD);
    }

    Radio.Sleep();

    Radio.SetChannel((uint32_t)(m_tx_freq * 1e6));

    printf("alphaping: start, ctrl+c to stop\r\n");

    printf("TX: %d\r\n", m_count);
    Radio.SetTxConfig(MODEM_LORA, m_power, 0, m_bw,
                      m_sf, m_cr,
                      m_pl, ALPHA_FIX_LENGTH_PAYLOAD_ON,
                      true, 0, 0, false, 3000);

    Radio.Send(m_buff, m_buff_len);
    TIMER_START(m_timer, 3000);

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
            printf("alphaping: end\r\n");
            break;
        }
    }

    return 0;
}

