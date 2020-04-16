/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "radio.h"
#include "sx1276.h"
#include "sx1276_board.h"
#include "nrf_delay.h"
#include "util.h"

#include "app_uart.h"
#include "app_timer.h"
#include "timer.h"

#include "shell_cmd.h"

#include "alpha_tx.h"

#include "trace.h"

#define ALPHA_MAC_PUBLIC_SYNCWORD                    0x34
#define ALPHA_BANDWIDTH                              0           // [0: 125 kHz,
//  1: 250 kHz,
//  2: 500 kHz,
//  3: Reserved]
#define ALPHA_SPREADING_FACTOR                       11          // [SF7..SF12]
#define ALPHA_CODINGRATE                             1           // [1: 4/5,
//  2: 4/6,
//  3: 4/7,
//  4: 4/8]
#define ALPHA_SYMBOL_TIMEOUT                         5           // Symbols
#define ALPHA_PREAMBLE_LENGTH                        8           // Same for Tx and Rx
#define ALPHA_FIX_LENGTH_PAYLOAD_ON                  true
#define ALPHA_RX_IQ_INVERSION_ON                     false
#define ALPHA_TX_IQ_INVERSION_ON                     false

APP_TIMER_DEF(m_timer);                     // Normal task timer

#define ALPHA_TX_TIMER_INTERVAL                      2000

static uint8_t m_buff[255] = { 0 };
static size_t m_buff_len = 20;
static uint16_t m_count   = 0;

static int m_repeat          = -1;
static int m_delay           = 0;

static uint8_t m_bw       = 0;
static uint8_t m_sf       = 11;
static uint8_t m_cr       = 1;
static int m_pl           = 8;
static int m_power        = 14;
static bool m_invert      = false;
static bool m_fix         = false;
static bool m_send_user_data = false;
static bool m_crc = true;

static void OnTxDone(void)
{
    TIMER_STOP(m_timer);

    Radio.Sleep();
    printf("OnTxDone\r\n\r\n");
    m_count++;

    if(!m_send_user_data)
    {
        m_buff[1] = m_count >> 8;
        m_buff[2] = m_count & 0xff;
    }
    nrf_delay_ms(10);

    nrf_delay_ms(m_delay);

    Radio.SetTxConfig(MODEM_LORA, m_power, 0, m_bw,
                      m_sf, m_cr,
                      m_pl, m_fix,
                      m_crc, 0, 0, m_invert, 3000000);

    /**
     * FSK mode
     */
    // Radio.SetTxConfig( MODEM_FSK,
    //                    m_power,
    //                    25000,
    //                    0,
    //                    50000,
    //                    0,
    //                    5,
    //                    false,
    //                    true,
    //                    0, 0, false, 3000000 );

    if(m_count < m_repeat || m_repeat == -1)
    {
        printf("TX: %d  , TX: %04x  \r\n", m_count , m_count );
        print_dump(m_buff, m_buff_len);                             //串口打印发送的数据串
        Radio.Send(m_buff, m_buff_len);
        TIMER_START(m_timer, ALPHA_TX_TIMER_INTERVAL);
    }
}

static void OnTxTimeout(void)
{
    printf("OnTxTimeout\r\n");
    m_count++;

    if(!m_send_user_data)
    {
        m_buff[1] = m_count >> 8;
        m_buff[2] = m_count & 0xff;
    }

    nrf_delay_ms(m_delay);

    if(m_count < m_repeat || m_repeat == -1)
    {
        printf("TX: %d\r\n", m_count);
        Radio.Send(m_buff, m_buff_len);
        TIMER_START(m_timer, ALPHA_TX_TIMER_INTERVAL);
    }
}

static RadioEvents_t TxTestEvents =
{
    .TxDone = OnTxDone,
    .RxDone = NULL,
    .RxError = NULL,
    .TxTimeout = OnTxTimeout,
    .RxTimeout = NULL,
    .CadDone = NULL
};

static void tx_timer_handler(void * p_context)
{
    Radio.OnTimeout();
}

static void usage(void)
{
    printf("Available options:\r\n");
    printf(" -h print this help\n");
    printf(" -f <float>  target frequency in MHz, default 433\n");
    printf(" -b <uint>   bandwidth in kHz [0:125, 1:250, 2:500], default 0:125\r\n");
    printf(" -s <uint>   Spreading Factor [6-12], default 11\r\n");
    printf(" -c <uint>   Coding Rate [1:4/5, 2:4/6, 3:5/7, 4:4/8],default 1:4/5\r\n");
    printf(" -p <int>    RF power (dBm),default 14\n");
    printf(" -l <uint>   preamble length (symbols), default 8\r\n");
    printf(" -i <uint>   IQ invert\r\n");
    printf(" -sy <hex>   Cfg sync word, 0x34: public, 0x12: private\r\n");
    printf(" -i          IQ invert\r\n");
    printf(" -t <uint>   pause between packets (ms),default 0\r\n");
    printf(" -r <uint>   repeat times, (-1 loop until stopped,default -1)\r\n");
    printf(" -bl <uint>  tx buffer len, default 20\r\n");
    printf(" -id <uint>  set a id, default 0\r\n");
    printf(" -fix        fix payload len\r\n");
    printf(" -rfo        use rfo output pin\r\n");
    printf(" -txt        text data to send\r\n");
    printf(" -bin        binary data to send\r\n");
    printf(" -nocrc      no crc\r\n");
}


int alpha_tx(int argc, char ** argv)
{
    uint8_t     count       = 0;
    double      freq        = 433;
    uint32_t    freq_int    = 0;
    bool        sync        = false;
    uint8_t     id          = 0;
    uint8_t     pa_select   = RF_PACONFIG_PASELECT_PABOOST;
    uint8_t     sync_word   = ALPHA_MAC_PUBLIC_SYNCWORD;

    m_repeat       = -1;
    m_delay        = 0;
    m_bw           = 0;
    m_sf           = 11;
    m_cr           = 1;
    m_pl           = 8;
    m_power        = 14;
    m_invert       = false;
    m_crc          = true;

    memset((char *)m_buff , 0 , sizeof(m_buff) );
    m_buff_len = 20;                                                //每一次重新配置后，数据包长度恢复原值

    m_send_user_data = false;

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
            count = sscanf(argv[i], "%lf", &freq);
            if(count != 1)
            {
                trace_debug("error\r\n");
                printf("alphatx: invalid arg\r\n");
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
                printf("alphatx: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-s") == 0)
        {
            i++;
            m_sf = atoi(argv[i]);
            if(m_sf > 12 || m_sf < 6)
            {
                trace_debug("error\r\n");
                printf("alphatx: invalid arg\r\n");
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
                printf("alphatx: invalid arg\r\n");
                return 0;
            }
            if(m_power > 20)
            {
                trace_debug("error\r\n");
                printf("alphatx: invalid arg\r\n");
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
                printf("alphatx: invalid arg\r\n");
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
                printf("alphatx: invalid arg\r\n");
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
                printf("alphatx: invalid arg\r\n");
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
                printf("alphatx: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-bl") == 0)
        {
            int payload_len = 0;
            i++;
            payload_len = atoi(argv[i]);
            if(payload_len < 5 || payload_len > 255)
            {
                trace_debug("error\r\n");
                printf("alphatx: invalid arg\r\n");
                return 0;
            }
            m_buff_len = payload_len;
            i++;
        }
        else if(strcmp(argv[i], "-i") == 0)
        {
            i++;
            m_invert = true;
        }
        else if(strcmp(argv[i], "-sy") == 0)
        {
            i++;
            sync = true;
            count = sscanf(argv[i], "%hhx", &sync_word);
            if(count != 1)
            {
                trace_debug("error\r\n");
                printf("alphatx: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-id") == 0)
        {
            i++;
            sscanf(argv[i], "%u", (unsigned int *)&id);
            i++;
        }
        else if(strcmp(argv[i], "-fix") == 0)
        {
            i++;
            m_fix = true;
        }
        else if(strcmp(argv[i], "-rfo") == 0)
        {
            i++;
            pa_select = RF_PACONFIG_PASELECT_RFO;
        }
        else if(strcmp(argv[i], "-bin") == 0)
        {
            i++;

            if(!hexstr_check(argv[i]))
            {
                printf("alphatx: invalid arg\r\n");
                return 0;
            }

            m_buff_len = (m_buff_len + 1) / 2;

            if(m_buff_len > 255)
            {
                printf("alphatx: invalid arg which is too long\r\n");
                return 0;
            }

            hexstr_decode(argv[i], m_buff);

            m_send_user_data = true;
            i++;
        }
        else if(strcmp(argv[i], "-txt") == 0)
        {
            i++;
            m_buff_len = strlen(argv[i]);
            if(m_buff_len > 255)
            {
                printf("alphatx: invalid arg which is too long\r\n");
                return 0;
            }

            if(m_buff_len != 0)
            {
                strcpy((char *)m_buff, argv[i]);
                m_send_user_data = true;
            }
            else
            {
                printf("alphatx: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-nocrc") == 0)
        {
            i++;
            m_crc = false;
        }
        else
        {
            trace_debug("error\r\n");
            printf("alphatx: invalid arg\r\n");
            return 0;
        }
    }
    freq_int = (uint32_t)(freq * 1e6);

    printf("freq    = %d\r\n", freq_int);
    printf("bw      = %d\r\n", m_bw);
    printf("sf      = %d\r\n", m_sf);
    printf("power   = %d\r\n", m_power);
    printf("cr      = %d\r\n", m_cr);
    printf("pl      = %d symb\r\n", m_pl);
    printf("sync    = %d\r\n", sync);
    if(sync)
    {
        printf("sync_word = %02X\r\n", sync_word);
    }
    printf("m_crc   = %d\r\n", m_crc);
    printf("invert  = %d\r\n", m_invert);
    printf("delay   = %d\r\n", m_delay);
    printf("repeat  = %d\r\n", m_repeat);
    printf("id      = %02x\r\n", id);
    printf("fix     = %d\r\n", m_fix);
    printf("pacfg   = %02X\r\n", pa_select);
    if(m_send_user_data)
    {
        printf("tx data = %s\r\n", m_buff);
    }
    printf("tx len  = %d\r\n", m_buff_len);

    m_count = 0;

    if(!m_send_user_data)                               //非用户制定数据，则会填充数据buffer
    {
        m_buff[0] = id;
        m_buff[1] = m_count >> 8;
        m_buff[2] = m_count & 0xff;
#if 0
        for(uint8_t i = 3; i < m_buff_len; i++)
        {
            m_buff[i] = i;
        }
#endif
        m_buff[m_buff_len - 1] = 0xaa;
    }

    TIMER_CREATE(&m_timer, APP_TIMER_MODE_SINGLE_SHOT, tx_timer_handler);

    Radio.Init(&TxTestEvents);

    // Radio.SetPaSelect(pa_select);

    if(sync == true)
    {
        Radio.SetModem(MODEM_LORA);
        Radio.Write(REG_LR_SYNCWORD, sync_word);
    }

    Radio.Sleep();

    Radio.SetChannel(freq_int);

    printf("alphatx: start, ctrl+c to stop\r\n");

    printf("TX: %d\r\n", m_count);
    Radio.SetTxConfig(MODEM_LORA, m_power, 0, m_bw,
                      m_sf, m_cr,
                      m_pl, m_fix,
                      m_crc, 0, 0, m_invert, 3000000);

    /**
     * FSK mode
     */
    // Radio.SetTxConfig(MODEM_FSK,
    //                   m_power,
    //                   25000,
    //                   0,
    //                   50000,
    //                   0,
    //                   5,
    //                   false,
    //                   true,
    //                   0, 0, false, 3000000);

    Radio.Send(m_buff, m_buff_len);
    TIMER_START(m_timer, ALPHA_TX_TIMER_INTERVAL);

    while(1)
    {
        uint8_t c;
        while(app_uart_get(&c) == NRF_ERROR_NOT_FOUND)
        {
        }

        if(c == 0x03) // ^C
        {
            TIMER_STOP(m_timer);
            Radio.Sleep();
            printf("alphatx: end\r\n");
            break;
        }
    }

    return 0;
}


