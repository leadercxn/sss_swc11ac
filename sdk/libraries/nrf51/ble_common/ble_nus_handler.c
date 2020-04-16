/* Copyright (c) 2016 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */
 
#include <string.h>
#include "ble_nus_handler.h"
#include "app_error.h"
#include "uart.h"
#include "app_scheduler.h"
#include "app_fifo.h"
#include "util.h"
#include "trace.h"

#define NUS_DATA_HEADER_LEN        2
#define NUS_DATA_LENGTH_OFFSET     0
#define NUS_DATA_OFFSET            2

#define NUS_STATE_IDLE             0
#define NUS_STATE_RECV             1
#define NUS_STATE_PROC             2

#define NUS_BUFF_SIZE           256

static uint8_t                  m_nus_state = NUS_STATE_IDLE;

static ble_nus_t                m_nus;

static uint8_t                  m_rx_buff[NUS_BUFF_SIZE];
static app_fifo_t               m_rx_fifo;
static uint16_t                 m_rx_exp_len = 0;

static uint8_t                  m_tx_buff[NUS_BUFF_SIZE];
static app_fifo_t               m_tx_fifo;
static bool                     m_tx_busy = false;

static bool                     m_is_locked = false;
static uint8_t                  m_passwd[16];
static bool                     m_is_passwd_set = false;

static nus_on_write_handler_t   m_on_write_handler = NULL;

static void uart_tx_sched_evt(void* p_event_data, uint16_t event_size)
{
    uint8_t buff[256];
    if(m_on_write_handler != NULL)
    {
        app_fifo_gets(&m_rx_fifo, buff, m_rx_exp_len);
        
        m_on_write_handler(buff, m_rx_exp_len);
    }
    app_fifo_flush(&m_rx_fifo);
    m_nus_state = NUS_STATE_IDLE;
    m_rx_exp_len = 0;
}

static void data_handler(ble_nus_t*           p_nus,
                         ble_nus_evt_type_t   evt_type,
                         uint16_t              value_handle,
                         uint8_t*              p_data,
                         uint16_t              length)
{
    uint32_t err_code;
  
    trace_info("nus data: ");
    trace_dump_i(p_data, length);
    
    ble_gatts_rw_authorize_reply_params_t reply =
    {
        .type                        = BLE_GATTS_AUTHORIZE_TYPE_WRITE,
        .params.write.gatt_status    = BLE_GATT_STATUS_SUCCESS,
        .params.write.update         = 1,
        .params.write.offset         = 0,
        .params.write.len            = 0,
        .params.write.p_data         = NULL
    };
   
    switch(evt_type)
    {

        case BLE_NUS_EVT_DATA_WRITE:
            if(m_is_locked)
            {
                trace_info("m_is_locked WRITE_NOT_PERMITTED\r\n");
                reply.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_WRITE_NOT_PERMITTED;
                break;
            }
            
            if(NUS_STATE_PROC == m_nus_state)
            {
                trace_info("NUS_STATE_PROC WRITE_NOT_PERMITTED\r\n");
                reply.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_WRITE_NOT_PERMITTED;
                break;
            }
            else if(NUS_STATE_IDLE == m_nus_state)
            {
                if(length < NUS_DATA_HEADER_LEN)
                {
                    trace_info("length error WRITE_NOT_PERMITTED\r\n");
                    reply.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_WRITE_NOT_PERMITTED;
                    break;
                }
                
                m_rx_exp_len = uint16_decode(&p_data[NUS_DATA_LENGTH_OFFSET]);
                
                if(m_rx_exp_len != 0 && m_rx_exp_len <= (NUS_BUFF_SIZE - NUS_DATA_HEADER_LEN))
                {
                    m_nus_state = NUS_STATE_RECV;
                    
                    app_fifo_puts(&m_rx_fifo, &p_data[NUS_DATA_HEADER_LEN], length - NUS_DATA_HEADER_LEN);
                }
                else
                {
                    trace_info("invalid m_rx_exp_len WRITE_NOT_PERMITTED\r\n");
                    reply.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_WRITE_NOT_PERMITTED;
                    break;
                }
            }
            else if(NUS_STATE_RECV == m_nus_state)
            {        
                app_fifo_puts(&m_rx_fifo, p_data, length);
            }

            reply.params.write.len = length;
            reply.params.write.p_data = p_data;

            if(fifo_length(&m_rx_fifo) >= m_rx_exp_len)
            {
                m_nus_state = NUS_STATE_PROC;
                app_sched_event_put(NULL, 0, uart_tx_sched_evt);
            }
            break;
        case BLE_NUS_EVT_PASSWORD_WRITE:
            if(m_is_locked)
            {
                if(memcmp(p_data, m_passwd , sizeof(m_passwd)) == 0)
                {
                    trace_info("unlocked\r\n");
                    m_is_locked = false;
                }
                else
                {
                    trace_info("invalid passwd WRITE_NOT_PERMITTED\r\n");
                    reply.params.write.gatt_status = BLE_GATT_STATUS_ATTERR_WRITE_NOT_PERMITTED;
                }
            }
            reply.params.write.len = length;
            reply.params.write.p_data = p_data;
            break;
        default:
            break;
    }

    err_code = sd_ble_gatts_rw_authorize_reply(p_nus->conn_handle, &reply);
    APP_ERROR_CHECK(err_code);
}

static void hvc_handler(ble_nus_t * p_nus, ble_gatts_char_handles_t   char_handle)
{
    uint32_t err_code;
    uint16_t send_len;
    uint8_t buff[20];
    send_len = fifo_length(&m_tx_fifo);
    
    if(send_len == 0)
    {        
        m_tx_busy = false;
        return;
    }
    
    if( send_len >= 20)
    {
        send_len = 20;
    }
    
    app_fifo_gets(&m_tx_fifo, buff, send_len);
    err_code = ble_nus_string_send(&m_nus, buff, send_len);
    if(err_code != NRF_SUCCESS)
    {
        app_fifo_flush(&m_tx_fifo);
        m_tx_busy = false;
    }
}

void ble_srv_nus_init(void)
{
    uint32_t err_code;
    ble_nus_init_t init;
    
    app_fifo_init(&m_tx_fifo, m_tx_buff, sizeof(m_tx_buff));
    app_fifo_init(&m_rx_fifo, m_rx_buff, sizeof(m_rx_buff));
    
    init.data_handler       = data_handler;
    init.hvc_handler        = hvc_handler;

    err_code = ble_nus_init(&m_nus, &init);
    APP_ERROR_CHECK(err_code);
}

static void on_ble_evt(ble_evt_t* p_ble_evt)
{
    switch(p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_DISCONNECTED:
            m_rx_exp_len = 0;
            m_nus_state = NUS_STATE_IDLE;
            
            m_tx_busy = false;
        
            app_fifo_flush(&m_tx_fifo);
            app_fifo_flush(&m_rx_fifo);
        
            if(m_is_passwd_set)
            {
                m_is_locked = true;
            }
            break;
    }
}

void on_ble_srv_nus_evt(ble_evt_t* p_ble_evt)
{
    ble_nus_on_ble_evt(&m_nus, p_ble_evt);
    on_ble_evt(p_ble_evt);
}

void ble_srv_nus_handler_reg(nus_on_write_handler_t handler)
{
    m_on_write_handler = handler;
}

void ble_srv_nus_ind_handler_reg(ble_nus_ind_handler_t handler)
{
    m_nus.ind_handler = handler;
}

uint32_t ble_srv_nus_data_send(uint8_t* p_data, uint16_t len)
{
    uint32_t err_code = NRF_SUCCESS;
    uint8_t buff[20] = {0};
    uint8_t send_len = 0;
    uint8_t len_buff[2];
    
    if(len == 0)
    {
        return err_code;
    }
    
    if(!m_tx_busy)
    {
        m_tx_busy = true;
        
        uint16_encode(len, len_buff);
        
        app_fifo_puts(&m_tx_fifo, len_buff, 2);
        app_fifo_puts(&m_tx_fifo, p_data, len);
        
        send_len = fifo_length(&m_tx_fifo);
        if( send_len > 20 )
        {
            send_len = 20;
        }
        app_fifo_gets(&m_tx_fifo, buff, send_len);
      
        err_code = ble_nus_string_send(&m_nus, buff, send_len);
        
        if(err_code != NRF_SUCCESS)
        {
            app_fifo_flush(&m_tx_fifo);
            m_tx_busy = false;
        }
    }
    else
    {        
        err_code = NRF_ERROR_BUSY;
    }
    return err_code;
}

void ble_srv_nus_set_passwd(uint8_t* passwd, uint8_t len)
{
    APP_ERROR_CHECK_BOOL(len == 16);
    memcpy(m_passwd, passwd, 16);
    if(!is_buff_empty(m_passwd, 16))
    {
        m_is_locked = true;
        m_is_passwd_set = true;
    }
    else
    {
        m_is_locked = false;
        m_is_passwd_set = false;
    }
}

