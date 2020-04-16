#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <nrf_gpio.h>

#include "radio_config.h"
#include "radio.h"
#include "sx1276.h"
#include "sx1276_board.h"
#include "nrf_delay.h"
#include "util.h"
#include "trace.h"
#include "app_util.h"
#include "app_timer.h"
#include "timer.h"

#include "test_companion.h"

#define LORA_MAC_PUBLIC_SYNCWORD                    0x34
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false

#define BLE_PONG_STATE_IDLE 0
#define BLE_PONG_STATE_RX   1
#define BLE_PONG_STATE_TX   2

#define _PWM_TEST                30
#define LED_PIN  _PWM_TEST

static ble_radio_config_t m_pong_tx_config;
static ble_radio_config_t m_pong_rx_config;

static ble_radio_event_handler_t m_radio_event_handler;

static uint32_t m_rx_packet = 0;
static uint32_t m_tx_packet = 0;

static uint32_t m_ble_pong_state = BLE_PONG_STATE_IDLE;

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

APP_TIMER_DEF(m_led_timer);

/**
 * @brief lorawan TX done callback
 * 
 */
static void OnTxDone(void)
{
    printf("OnTxDone\r\n\r\n");

    static uint32_t count = 0;

    Radio.Sleep();


    if(count % 2 == 0)
    {
        // 发完之后, 得再发一次, ping端收到的RSSI才会正常
        Radio.SetChannel((uint32_t)(m_tx_freq * 1e6));
        SX1276SetTxConfig(MODEM_LORA,                 //mode
                           m_power,                    //power
                           0,                          //fdev
                           m_bw,                       //bw
                           7,                          //sf
                           m_cr,                       //cr
                           1,                          //preambleLen
                           LORA_FIX_LENGTH_PAYLOAD_ON, //fixLen
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
                          LORA_FIX_LENGTH_PAYLOAD_ON, //fixLen
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

/**
 * @brief lorawan TX timeout callback
 * 
 */
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
                      LORA_FIX_LENGTH_PAYLOAD_ON, //fixLen
                      0,                          //payloadLen
                      true,                       //crcOn
                      0,                          //FreqHopOn
                      0,                          //HopPeriod
                      false,                      //iqInverted
                      true);                      //rxContinuous

    Radio.Rx(0);   // Continuous Rx

}

/**
 * @brief lorawan RX timeout callback
 * 
 */
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
                      LORA_FIX_LENGTH_PAYLOAD_ON, //fixLen
                      0,                          //payloadLen
                      true,                       //crcOn
                      0,                          //FreqHopOn
                      0,                          //HopPeriod
                      false,                       //iqInverted
                      true);                      //rxContinuous

    Radio.Rx(0);   // Continuous Rx
}

/**
 * @brief lorawan RX done callback
 * 
 */
static void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
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
                      LORA_FIX_LENGTH_PAYLOAD_ON, //fixLen
                      false,                      //crcOn
                      0,                          //FreqHopOn
                      0,                          //HopPeriod
                      true,                   //iqInverted
                      3000);                      //timeout

    Radio.Send(m_tx_buff, m_tx_buff_len);
}

/**
 * @brief lorawan API
 */
static RadioEvents_t RadioEvents =
{
    .TxDone = OnTxDone,
    .RxDone = OnRxDone,
    .RxError = NULL,
    .TxTimeout = OnTxTimeout,
    .RxTimeout = OnRxTimeout,
    .CadDone = NULL
};

/**
 * @brief ble事件处理函数
 */
static void on_ble_radio_event_end_handler(void)
{
    int8_t rssi = 0;
    switch(m_ble_pong_state)
    {
        case BLE_PONG_STATE_RX:
            printf("OnRxDone\r\n");
            if(ble_radio_crc_status_get())
            {
                rssi = ble_radio_rssi_get();
                printf("data = %d, rssi = %d\r\n", m_rx_packet, rssi);

                m_tx_packet = rssi;

                ble_radio_config_set(m_pong_tx_config);
                ble_radio_packet_set((uint8_t *)&m_tx_packet);
                ble_radio_tx_enable();
                m_ble_pong_state = BLE_PONG_STATE_TX;
            }
            else
            {
                printf("Invalid crc\r\n");

                ble_radio_config_set(m_pong_rx_config);
                ble_radio_packet_set((uint8_t *)&m_rx_packet);
                ble_radio_rx_enable();
                m_ble_pong_state = BLE_PONG_STATE_RX;
            }
            break;

        case BLE_PONG_STATE_TX:
            printf("OnTxDone\r\n\r\n");

            ble_radio_config_set(m_pong_rx_config);
            ble_radio_packet_set((uint8_t *)&m_rx_packet);
            ble_radio_rx_enable();
            m_ble_pong_state = BLE_PONG_STATE_RX;
            break;

        default:
            break;
    }
}


static void usage(void)
{
    printf("Available options:\n");
    printf("  -h          print this help\n");
    printf("  -tf <float> tx frequency in MHz, default 506.7\n");
    printf("  -rf <float> rx frequency in MHz, default 486.5\n");
    printf("  -b  <uint>  bandwidth in kHz [0:125, 1:250, 2:500], default 0:125\n");
    printf("  -s  <uint>  Spreading Factor [7-12], default 11\n");
    printf("  -c  <uint>  Coding Rate [1:4/5, 2:4/6, 3:5/7, 4:4/8],default 1:4/5\n");
    printf("  -p  <int>   RF power (dBm),default 14\n");
    printf("  -l  <uint>  preamble length (symbols), default 8\n");
    printf("  -tp <int>   BLE tx power (dBm),default 4 \n");
    printf("  -tc <int>   BLE tx channel,default 0 \n");
    printf("  -rc <int>   BLE rx channel,default 2 \n");
    printf("  -sy         Cfg sync word\n");
//    printf(" -rfo use rfo output pin\n");
}

static void led_timer_handler(void *p_contex)
{
    nrf_gpio_pin_toggle(LED_PIN);
}

int test_companion(int argc, char **argv)
{
    uint8_t count = 0;
//    uint8_t pa_select = RF_PACONFIG_PASELECT_PABOOST;
    bool sync   = false;


    int8_t   ble_tx_power = 4;
    int8_t   ble_tx_channel = 0;
    uint8_t  ble_tx_mode = RADIO_MODE_MODE_Ble_1Mbit;

    int8_t   ble_rx_power = 4;
    uint8_t  ble_rx_channel = 2;
    uint8_t  ble_rx_mode = RADIO_MODE_MODE_Ble_1Mbit;


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
                printf("test_cmp invalid arg\r\n");
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
                printf("test_cmp invalid arg\r\n");
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
                printf("test_cmp invalid arg\r\n");
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
                printf("test_cmp invalid arg\r\n");
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
                printf("test_cmp invalid arg\r\n");
                return 0;
            }
            if(m_power > 20)
            {
                trace_debug("error\r\n");
                printf("test_cmp invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-tp") == 0) //ble tx power
        {
            i++;
            count = sscanf(argv[i], "%d", (unsigned int *)&ble_tx_power);
            if(count != 1)
            {
                trace_debug("error\r\n");
                printf("test_cmp invalid arg\r\n");
                return 0;
            }
            if(ble_tx_power > 4)
            {
                trace_debug("error\r\n");
               printf("test_cmp invalid arg\r\n");
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
                printf("test_cmp invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-tc") == 0) //ble tx channel
        {
            i++;
            ble_tx_channel = atoi(argv[i]);
            if(ble_tx_channel > 40 )
            {
                trace_debug("error\r\n");
                printf("test_cmp invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-rc") == 0) //ble rx channel
        {
            i++;
            ble_rx_channel = atoi(argv[i]);
            if(ble_rx_channel > 40)
            {
                trace_debug("error\r\n");
                printf("test_cmp invalid arg\r\n");
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
                printf("test_cmp invalid arg\r\n");
                return 0;
            }
            i++;
        }
        else if(strcmp(argv[i], "-sy") == 0)
        {
            i++;
            sync = true;
        }
//        else if(strcmp(argv[i], "-rfo") == 0)
//        {
//            i++;
//            pa_select = RF_PACONFIG_PASELECT_RFO;
//        }
        else
        {
            trace_debug("error\r\n");
            printf("test_cmp invalid arg\r\n");
            return 0;
        }
    }

    printf("subg tx freq = %d\r\n", (uint32_t)(m_tx_freq * 1e6));
    printf("subg rx freq = %d\r\n", (uint32_t)(m_rx_freq * 1e6));
    printf("subg bw      = %d\r\n", m_bw);
    printf("subg sf      = %d\r\n", m_sf);
    printf("subg power   = %d\r\n", m_power);
    printf("subg cr      = %d\r\n", m_cr);
    printf("subg pl      = %d symb\r\n", m_pl);
    printf("subg sync    = %d\r\n", sync);
//    printf("subg pacfg   = %02X\r\n", pa_select);

    printf("ble_tx_power   = %d\r\n", ble_tx_power);
    printf("ble_tx_channel = %d\r\n", ble_tx_channel);
    printf("ble_rx_channel = %d\r\n", ble_rx_channel);

    nrf_gpio_cfg_output(LED_PIN);
    TIMER_CREATE(&m_led_timer, APP_TIMER_MODE_REPEATED, led_timer_handler);
    TIMER_START(m_led_timer, 500);

    m_pong_tx_config.power   = ble_tx_power;
    m_pong_tx_config.channel = ble_tx_channel;
    m_pong_tx_config.mode    = ble_tx_mode;

    m_pong_rx_config.power   = ble_rx_power;
    m_pong_rx_config.channel = ble_rx_channel;
    m_pong_rx_config.mode    = ble_rx_mode;

    m_radio_event_handler.on_event_end_handler = on_ble_radio_event_end_handler;
    ble_radio_switch_init();
    ble_radio_init(&m_radio_event_handler);
    ble_radio_config_set(m_pong_rx_config);
    ble_radio_packet_set((uint8_t *)&m_rx_packet);                      //point to rx_data adress
    ble_radio_rx_enable();
    m_ble_pong_state = BLE_PONG_STATE_RX;


    Radio.Init(&RadioEvents);
//    Radio.SetPaSelect(pa_select);
    if(sync == true)
    {
        Radio.SetModem(MODEM_LORA);
        Radio.Write(REG_LR_SYNCWORD, LORA_MAC_PUBLIC_SYNCWORD);
    }

    Radio.SetChannel((uint32_t)(m_rx_freq * 1e6));

    Radio.SetRxConfig(MODEM_LORA,
                      m_bw,                       //bw
                      m_sf,                       //sf
                      m_cr,                       //cr
                      0,                          //bandwidthAfc
                      m_pl,                       //preambleLen
                      m_st,                       //symbTimeout
                      LORA_FIX_LENGTH_PAYLOAD_ON, //fixLen
                      0,                          //payloadLen
                      true,                       //crcOn
                      0,                          //FreqHopOn
                      0,                          //HopPeriod
                      false,                      //iqInverted
                      true);                      //rxContinuous
    Radio.Rx(0);   // Continuous Rx

    printf("test_cmp : start ,ctrl+c to stop  \n\n");

    while(1)
    {
        int c = getchar();
        if(c == 0x03)   // ^C
        {
            Radio.Sleep();

            ble_radio_disable();

            NVIC_ClearPendingIRQ(RADIO_IRQn);      //消除IRQ挂起状态
            NVIC_DisableIRQ(RADIO_IRQn);           //失能XX中断

            TIMER_STOP(m_led_timer);
            nrf_gpio_pin_clear(LED_PIN);
            printf("test_cmp end\n\n");
            break;
        }
    }

    return 0;
}

