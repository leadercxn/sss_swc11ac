/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_uart.h"

#include "radio.h"
#include "sx1276.h"
#include "sx1276_board.h"
#include "nrf_delay.h"
#include "util.h"
#include "trace.h"
// #include "LoRaMacCrypto.h"

#include "shell_cmd.h"

#include "alpha_rx.h"

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
#define ALPHA_FIX_LENGTH_PAYLOAD_ON                  false
#define ALPHA_RX_IQ_INVERSION_ON                     false
#define ALPHA_TX_IQ_INVERSION_ON                     false

static uint16_t count;
static uint16_t m_rx_nb = 0;
static bool m_compute_mic = false;
static uint8_t m_nwkskey[16] = {0};

static uint8_t m_bw      = 0;
static uint8_t m_sf      = 11;
static uint8_t m_cr      = 1;
static uint8_t m_pl      = 8;
static uint8_t m_st      = 5;
static bool m_invert     = false;
static bool m_crc        = true;
static bool m_fix_payload = false;
static uint8_t m_fix_payload_len = 0;

/*
static void compute_mic(uint8_t *payload, uint16_t size, uint8_t *p_nwkskey)
{
    // uint8_t nwksky[] = {0x70,0x46,0x3f,0xf6,0xd4,0xc8,0xe1,0x4b,0x30,0x44,0x03,0xc6,0x08,0x87,0x29,0x70};
    uint32_t addr = 0;
    uint32_t downlink_count = 0;
    uint32_t mic_cal = 0;
    uint32_t mic_get = 0;
    bool mic_status = true;

    downlink_count = payload[6] + (payload[7] << 8);
    mic_get = (payload[size - 1] << 24) | (payload[size - 2] << 16) | (payload[size - 3] << 8) | payload[size - 4];
    addr = payload[1] | (payload[2] << 8) | (payload[3] << 16) | (payload[4] << 24);

    LoRaMacComputeMic(payload, size - 4, p_nwkskey, addr, 1, downlink_count, &mic_cal);

    if(mic_get != mic_cal)
    {
        mic_status = false;
    }

    printf( "ADDR = %08X, COUNT = %d, MIC = %d\r\n\r\n", addr, downlink_count, mic_status);
}
*/

static void OnRxDone(uint8_t * payload, uint16_t size, int16_t rssi, int8_t snr)
{
    count++;

    Radio.Sleep();

    printf("OnRxDone %03d, %d: ", count, size);
    print_dump(payload, size);
    printf("RSSI = %d, SNR = %d\r\n", rssi, snr);

    if(m_compute_mic)
    {
        // compute_mic(payload, size, m_nwkskey);
    }

    if((m_rx_nb != 0) && (count >= m_rx_nb))
    {
        printf("alpharx: done\r\n");
        return;
    }

    Radio.SetRxConfig(MODEM_LORA,
                      m_bw,
                      m_sf,
                      m_cr,
                      0,
                      m_pl,
                      m_st,
                      m_fix_payload,
                      m_fix_payload_len,
                      m_crc,
                      0,
                      0,
                      m_invert,
                      true);
    Radio.Rx(0);
}

static void OnRxTimeout(void)
{
    printf("OnRxTimeout\r\n");
}

static RadioEvents_t RxTestEvents =
{
    .TxDone = NULL,
    .RxDone = OnRxDone,
    .RxError = NULL,
    .TxTimeout = NULL,
    .RxTimeout = OnRxTimeout,
    .CadDone = NULL
};

static void usage(void)
{
    printf("Available options:\r\n");
    printf(" -h print this help\r\n");
    printf(" -f <float> target frequency in MHz, default 433\r\n");
    printf(" -b <uint>  bandwidth in kHz [0:125, 1:250, 2:500], default 0:125\r\n");
    printf(" -s <uint>  Spreading Factor [6-12], default 11\r\n");
    printf(" -c <uint>  Coding Rate [1:4/5, 2:4/6, 3:5/7, 4:4/8],default 1:4/5\r\n");
    printf(" -l <uint>  preamble length (symbols), default 8\r\n");
    printf(" -t <uint>  symbol timeout (symbols), default 5\r\n");
    printf(" -i         IQ invert\r\n");
    printf(" -sy <hex>  Cfg sync word, 0x34: public, 0x12: private\r\n");
    printf(" -nocrc     no crc\r\n");
    printf(" -fix <uint> fix payload len\r\n");
    printf(" -n   <uint> rx number\r\n");
    printf(" -key <hex> nwkskey to cal mic\r\n");
}

int alpha_rx(int argc, char ** argv)
{
    uint8_t arg_cnt = 0;
    double freq      = 433;
    bool sync       = false;
    uint8_t sync_word = 0x34;

    m_rx_nb = 0;
    uint32_t freq_int = 0;

    arg_cnt = 0;

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
            arg_cnt = sscanf(argv[i], "%lf", &freq);
            if(arg_cnt != 1)
            {
                trace_debug("error\r\n");
                printf("alpharx: invalid arg\r\n");
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
                printf("alpharx: invalid arg\r\n");
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
                printf("alpharx: invalid arg\r\n");
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
                printf("alpharx: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-l") == 0)
        {
            i++;
            m_pl = atoi(argv[i]);
            i++;
        }
        else if(strcmp(argv[i], "-t") == 0)
        {
            i++;
            m_st = atoi(argv[i]);
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
            arg_cnt = sscanf(argv[i], "%hhx", &sync_word);
            if(arg_cnt != 1)
            {
                trace_debug("error\r\n");
                printf("alpharx: invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-nocrc") == 0)
        {
            i++;
            m_crc = false;
        }
        else if(strcmp(argv[i], "-fix") == 0)
        {
            i++;
            m_fix_payload_len = atoi(argv[i]);
            m_fix_payload = true;
            i++;
        }
        else if(strcmp(argv[i], "-n") == 0)
        {
            i++;
            m_rx_nb = atoi(argv[i]);
            i++;
        }
        else if(strcmp(argv[i], "-key") == 0)
        {
            i++;
            if(!hexstr_check(argv[i]))
            {
                printf("alpharx: invalid arg\r\n");
                return 0;
            }
            hexstr_decode(argv[i], m_nwkskey);
            m_compute_mic = true;
            i++;
        }
        else
        {
            trace_debug("error\r\n");
            printf("alpharx: invalid arg\r\n");
            return 0;
        }
    }
    freq_int = (uint32_t)(freq * 1e6);

    printf("freq    = %d\r\n", freq_int);
    printf("bw      = %d\r\n", m_bw);
    printf("sf      = %d\r\n", m_sf);
    printf("cr      = %d\r\n", m_cr);
    printf("preamble length  = %d symb\r\n", m_pl);
    printf("symbol timeout   = %d symb\r\n", m_st);
    printf("sync    = %d\r\n", sync);
    if(sync)
    {
        printf("sync_word = %02X\r\n", sync_word);
    }
    printf("invert  = %d\r\n", m_invert);
    printf("crc     = %d\r\n", m_crc);
    printf("fix len = %d\r\n", m_fix_payload_len);
    printf("nb      = %d\r\n", m_rx_nb);
    if(m_compute_mic)
    {
        printf("nwkskey = ");
        print_dump(m_nwkskey, sizeof(m_nwkskey));
    }

    Radio.Init(&RxTestEvents);
    if(sync == true)
    {
        Radio.SetModem(MODEM_LORA);
        Radio.Write(REG_LR_SYNCWORD, sync_word);
    }

    Radio.Sleep();

    Radio.SetChannel(freq_int);
    Radio.SetRxConfig(MODEM_LORA,
                      m_bw,
                      m_sf,
                      m_cr,
                      0,
                      m_pl,
                      m_st,
                      m_fix_payload,
                      m_fix_payload_len,
                      m_crc,
                      0,
                      0,
                      m_invert,
                      true);
    Radio.Rx(0);
    printf("alpharx: start, ctrl+c to stop\r\n");

    while(1)
    {
        uint8_t c;
        while(app_uart_get(&c) == NRF_ERROR_NOT_FOUND)
        {
        }

        if(c == 0x03)   // ^C
        {
            Radio.Sleep();
            printf("alpharx: end\r\n");
            break;
        }
    }
    return 0;
}

