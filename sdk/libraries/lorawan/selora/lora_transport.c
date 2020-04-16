#include "lora_transport.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "LoRaMac.h"
#include "app_error.h"
#include "app_timer.h"
#include "app_scheduler.h"

#ifdef __MODULE__
    #undef __MODULE__
    #define __MODULE__          "**lotr.c"
#endif

#include "debug_config.h"
#ifdef LORA_TRANSPORT_DEBUG
    #include "trace.h"
#else
    #include "trace_disable.h"
#endif

#define RANDR(min, max) ((int32_t)rand() % (max - min + 1) + min)

#define TIMER_CREATE(p_id, mode, handler)                                           \
{                                                                                   \
    uint32_t err_code;                                                              \
    err_code = app_timer_create(p_id, mode, handler);                               \
    APP_ERROR_CHECK(err_code);                                                      \
}                                                                                   \

#define TIMER_START(id, ms)                                                         \
{                                                                                   \
    uint32_t err_code;                                                              \
    err_code = app_timer_start(id, APP_TIMER_TICKS(ms, 0), NULL);                   \
    APP_ERROR_CHECK(err_code);                                                      \
}                                                                                   \

#define TIMER_STOP(id)                                                              \
{                                                                                   \
    uint32_t err_code;                                                              \
    err_code = app_timer_stop(id);                                                  \
    APP_ERROR_CHECK(err_code);                                                      \
}                                                                                   \

#define APP_PORT                2
#define TEST_PORT               224

#define LOTR_STATE_IDLE             0x00
#define LOTR_STATE_MESSAGE          0x01

#define TEST_PAYLOAD_LEN        6

static LoRaMacEvent_t       LoRaMacEvents;

static bool                     m_confirm = false;
static uint8_t                  m_confirm_cnt = 0;

static lotr_handlers_t          m_handlers;

static uint8_t                  m_rx_buff[51];
static uint8_t                  m_rx_len = 0;

static uint8_t                  m_tx_buff[51];
static uint8_t                  m_tx_len = 0;


//static uint8_t                  m_state = LOTR_STATE_IDLE;

static bool                     m_busy = false;
static bool                     m_txing_message = false;
static bool                     m_message_pedding = false;

static lotr_test_info_t         m_test_info;

static bool                     m_classb_report_pedding = false;

static bool                     m_tx_scheduling = false;

APP_TIMER_DEF(classb_rpt_rdm_timer);
APP_TIMER_DEF(send_delay_timer);


/**@brief
 *
 * @param
 *
 * @retval
 */
static void lotr_tx_sched_handler(void* p_event_data, uint16_t event_size)
{
    uint16_t mac_state = 0;
    
    m_tx_scheduling = false;
    
    mac_state = LoRaMacGetState();
    trace_debug("mac_state = %x\r\n", mac_state);
    if(mac_state != 0)
    {
        TIMER_START(send_delay_timer, 1000);
        return;
    }
    
    
    
    if(m_tx_len > 0 && m_confirm)
    {
        mac_state = LoRaMacSendConfirmedPacket(APP_PORT, m_tx_buff, m_tx_len, m_confirm_cnt);
    }
    else
    {
        mac_state = LoRaMacSendUnconfirmedPacket(APP_PORT, m_tx_buff, m_tx_len);
    }
    
    if(mac_state == 0)
    {
        m_busy = true;
    }
    else
    {
        TIMER_START(send_delay_timer, 1000);
    }
}

static void lotr_tx_schedule(void)
{
    // 这里用个标志控制起来, 是为了防止多次调度 lotr_tx_sched_handler
    if(!m_tx_scheduling)
    {
        m_tx_scheduling = true;
        app_sched_event_put(NULL, 0, lotr_tx_sched_handler);
    }
}

static void send_delay_handler(void *p_contxt)
{
    trace_debug("sched tx\r\n");
    lotr_tx_schedule();
}

uint32_t lotr_tx(uint8_t* fBuffer, uint16_t fBufferSize, uint8_t cfmCnt)
{
    uint32_t tx_limited_remain = 0;
    
    if(m_txing_message)
    {
        return LORA_TRANSPORT_ERROR_BUSY;
    }

    if(fBufferSize > LORA_TX_MAX_LEN)
    {
        return LORA_TRANSPORT_ERROR_INVALID_LEN;
    }
    
    tx_limited_remain = LoRaMacGetTxLimitedRemain();
    if(tx_limited_remain > 0)
    {
        trace_notice("tx_limited_remain = %d\r\n", tx_limited_remain);
        return LORA_TRANSPORT_ERROR_TX_LIMITED;
    }
    
    m_txing_message = true;
    
    memcpy(m_tx_buff, fBuffer, fBufferSize);
    m_tx_len = fBufferSize;
    
    m_confirm_cnt = cfmCnt;
    if(cfmCnt > 0)
    {
        m_confirm = true;;
    }
    else
    {
        m_confirm = false;
    }

    if(!m_busy)
    {
        trace_debug("sched tx\r\n");
        lotr_tx_schedule();
    }
    else
    {
        m_message_pedding = true;
    }
    
    return LORA_TRANSPORT_ERROR_NONE;
}

void lotr_test(void)
{
    uint8_t zero = 0;
    LoRaMacSendConfirmedPacket(TEST_PORT, &zero, 1, 0);
}

void lotr_test_with_data(uint8_t *p_data, uint16_t len)
{
    LoRaMacSendConfirmedPacket(TEST_PORT, p_data, len, 0);
}

/**@brief
 *
 * @param
 *
 * @retval
 */
static void lotr_rxed_sched_handler(void* p_event_data, uint16_t event_size)
{
    if(m_handlers.rxed_handler != NULL)
    {
        m_handlers.rxed_handler(m_rx_buff, m_rx_len);
    }
    memset(m_rx_buff, 0, sizeof(m_rx_buff));
    m_rx_len = 0;
}


/**@brief
 *
 * @param
 *
 * @retval
 */
static void lotr_txed_sched_handler(void* p_event_data, uint16_t event_size)
{
    uint8_t tx_state = *(uint8_t *)p_event_data;
#ifdef DEBUG
    if(LOTR_TX_STATE_DONEA == tx_state)
    {
        trace_info("LOTR_TX_STATE_DONEA\n");
    }
    else if(LOTR_TX_STATE_DONENA == tx_state)
    {
        trace_info("LOTR_TX_STATE_DONENA\n");
    }
    else if(LOTR_TX_STATE_DONE == tx_state)
    {
        trace_info("LOTR_TX_STATE_DONE\n");
    }
#endif

    if(m_handlers.txed_handler != NULL)
    {
        m_handlers.txed_handler(tx_state);
    }
}

static void lotr_tested_sched_handler(void* p_event_data, uint16_t event_size)
{
    if(m_handlers.test_handler != NULL)
    {
        m_handlers.test_handler(&m_test_info);
    }
}


static void app_port_handler(LoRaMacEventFlags_t* flags, LoRaMacEventAlohaInfo_t* info)
{
    bool mac_pedding = false;
    
    uint8_t   tx_state = LOTR_TX_STATE_NONE;
    
    if(1 == flags->Bits.Tx)
    {
        if(m_confirm)
        {
            if(info->TxAckReceived)
            {
                tx_state = LOTR_TX_STATE_DONEA;
            }
            else
            {
                tx_state = LOTR_TX_STATE_DONENA;
            }
        }
        else
        {
            tx_state = LOTR_TX_STATE_DONE;
        }
        
        if(flags->Bits.RxData == 1)
        {
            memcpy(&m_rx_buff, info->RxBuffer, info->RxBufferSize);
            m_rx_len = info->RxBufferSize;
                                 
            if(info->Pendding == 1)
            {
                mac_pedding = true;
                trace_debug("mac_pedding == true\r\n");
            }
            else
            {
                mac_pedding = false;
            }
            if(info->RxBufferSize != 0)
            {
                app_sched_event_put(NULL, 0, lotr_rxed_sched_handler);
            }
        }
    }

    trace_debug("mac_pedding = %d\r\n", mac_pedding);

    if(mac_pedding ||
       m_message_pedding ||
       m_classb_report_pedding)
    {        
        trace_info("sched tx\r\n");
        lotr_tx_schedule();

        mac_pedding = false;
        m_classb_report_pedding = false;
        m_message_pedding = false;
    }
    else
    {
        if(m_txing_message)
        {
            m_txing_message = false;
            app_sched_event_put(&tx_state, 1, lotr_txed_sched_handler);
            
            memset(m_tx_buff, 0, sizeof(m_tx_buff));
            m_tx_len = 0;
            m_confirm = false;
        }
    }
    m_busy = false;
}

/**@brief Function for processing data from test port
 *
 * @param
 */
static void test_port_handler(LoRaMacEventFlags_t* flags, LoRaMacEventAlohaInfo_t* info)
{
    trace_info("test_port_handler, rx_port = %d\r\n", info->RxPort);

    memset(&m_test_info, 0, sizeof(m_test_info));
    m_test_info.is_lost = true;
    m_test_info.up_freq    = info->TxFreq;
    m_test_info.up_dr      = info->TxDatarate;
    m_test_info.up_txp     = info->TxPower;

    if((1 == flags->Bits.Tx) &&
       (1 == flags->Bits.RxData) &&
       (TEST_PAYLOAD_LEN == info->RxBufferSize) &&
       (TEST_PORT == info->RxPort))
    {
        m_test_info.is_lost    = false;
        m_test_info.down_rssi   = info->RxRssi;
        m_test_info.down_snr    = (int8_t)info->RxSnr;

        memcpy(&m_test_info.up_snr, info->RxBuffer, 4);
        m_test_info.up_rssi = info->RxBuffer[4];
        m_test_info.down_txp = info->RxBuffer[5];

        m_test_info.down_freq  = info->RxFreq;
        m_test_info.down_dr    = info->RxDatarate;
    }
    app_sched_event_put(NULL, 0, lotr_tested_sched_handler);
}

/**@brief Mac event callback
 *
 * @param
 */
void lotr_on_mac_event(LoRaMacEventFlags_t* flags, LoRaMacEventAlohaInfo_t* info)
{
    uint8_t tx_port = info->TxPort;
    trace_debug("on_mac_event\r\n");
    
    if(1 == flags->Bits.JoinAccept)
    {

    }
    else
    {
        switch(tx_port)
        {
            case APP_PORT:
                app_port_handler(flags, info);
                break;

            case TEST_PORT:
                test_port_handler(flags, info);
                break;
            default:
                APP_ERROR_CHECK_BOOL(false);
                break;
        }
    }
}

void lotr_on_rx_event(LoRaMacEventRxInfo_t* p_info)
{
    trace_debug("lotr_on_rx_event\r\n");
    bool mac_pedding = false;
    
        
    if(1 == p_info->RxData)
    {
         memcpy(&m_rx_buff, p_info->RxBuffer, p_info->RxBufferSize);
         m_rx_len = p_info->RxBufferSize;
        
        if(p_info->Pendding == 1)
        {
            mac_pedding = true;
            trace_info("mac_pedding == true\r\n");
        }
        else
        {
            mac_pedding = false;
        }
        app_sched_event_put(NULL, 0, lotr_rxed_sched_handler);
    }
    
    if(mac_pedding)
    {
        mac_pedding = false;
        trace_info("sched tx\r\n");
        lotr_tx_schedule();
    }
    trace_info("rssi = %d, snr = %d\r\n", p_info->RxRssi, p_info->RxSnr);
}

void lotr_on_beacon_event(LoRaMacEventBeaconInfo_t info)
{
    uint32_t rdm_time = 0;
    trace_info("lotr_on_beacon_event\r\n");
    if(LORAMAC_BEACON_LOCKED == info)
    {
        rdm_time = RANDR( 0, 122 ) * 1000;
        TIMER_START(classb_rpt_rdm_timer, rdm_time);
    }
    else
    {
        TIMER_START(classb_rpt_rdm_timer, 1000);
    }
}

static void classb_rpt_rdm_handler(void * p_contxt)
{
    if(!m_busy)
    {
        trace_info("sched send report\r\n");
        lotr_tx_schedule();
    }
    else
    {
        trace_info("m_classb_report_pedding\r\n");
        m_classb_report_pedding = true;
    }
}


void lotr_init(lotr_handlers_t* p_handlers)
{    
    LoRaMacInit();
    
    LoRaMacEvents.AlohaEvent = lotr_on_mac_event;
    LoRaMacEvents.RxEvent = lotr_on_rx_event;
    LoRaMacEvents.BeaconEvent = lotr_on_beacon_event;
    
    LoRaMacEventHandlerRegister(&LoRaMacEvents);
        
    m_handlers.txed_handler = p_handlers->txed_handler;
    m_handlers.rxed_handler = p_handlers->rxed_handler;
    m_handlers.test_handler = p_handlers->test_handler;
    
    TIMER_CREATE(&classb_rpt_rdm_timer, APP_TIMER_MODE_SINGLE_SHOT, classb_rpt_rdm_handler);
    TIMER_CREATE(&send_delay_timer, APP_TIMER_MODE_SINGLE_SHOT, send_delay_handler);
}
