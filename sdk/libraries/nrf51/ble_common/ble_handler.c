/* Copyright (c) 2016 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include "ble_handler.h"
#include "ble.h"
#include "app_error.h"
#include <stdint.h>
#include <string.h>
#include "app_util.h"
#include "nrf_gpio.h"
#include "ble_hci.h"
#include "ble_gap.h"


// TODO: 测试哪组连接参数更容易连接
#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(400, UNIT_1_25_MS)           /**< Minimum acceptable connection interval (0.4 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(650, UNIT_1_25_MS)           /**< Maximum acceptable connection interval (0.65 second). */
#define SLAVE_LATENCY                   0                                          /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)            /**< Connection supervisory timeout (4 seconds). */

#define INTERVAL_MAX                    5000
#define INTERVAL_MIN                    100

static bool m_is_adv = false;
uint32_t m_conn_handle = BLE_CONN_HANDLE_INVALID;
static int8_t m_tx_power = 0;
static float m_interval = 0;

ble_gap_adv_params_t     m_adv_param =
{
    .type           = BLE_GAP_ADV_TYPE_ADV_IND,
    .p_peer_addr    = NULL,
    .fp             = BLE_GAP_ADV_FP_ANY,
    .p_whitelist    = NULL,
    .interval       = 160,
    .timeout        = 0,
    .channel_mask   = {0, 0, 0}
};


void ble_handler_on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t err_code;
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            m_is_adv = false;
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            ble_advertising_start();
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            // Pairing not supported
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            // No system attributes have been stored.
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_TIMEOUT:
            m_is_adv = false;
            break;

        default:
            // No implementation needed.
            break;
    }
}

void ble_advertising_start(void)
{
    uint32_t err_code;

    if(!m_is_adv && (BLE_CONN_HANDLE_INVALID == m_conn_handle))
    {
        m_is_adv = true;
        m_adv_param.type = BLE_GAP_ADV_TYPE_ADV_IND;
        err_code = sd_ble_gap_adv_start(&m_adv_param);
        APP_ERROR_CHECK(err_code);
    }
}

void ble_advertising_stop(void)
{
    uint32_t err_code;

    if(m_is_adv && (BLE_CONN_HANDLE_INVALID == m_conn_handle))
    {
        m_is_adv = false;
        err_code = sd_ble_gap_adv_stop();
        APP_ERROR_CHECK(err_code);
    }
}

void ble_advertising_start_on_conn(void)
{
    //    uint32_t err_code;

    m_adv_param.type = BLE_GAP_ADV_TYPE_ADV_NONCONN_IND;

    /*连上后的广播间隔不能小于100ms, 也就是160*/
    if(m_adv_param.interval < 160)
    {
        m_adv_param.interval = 160;
    }

    sd_ble_gap_adv_start(&m_adv_param);

    /*这里不检查错误,避免万一出什么问题,就连不上*/
    // APP_ERROR_CHECK(err_code);

    /*改为原值*/
    m_adv_param.interval = (uint16_t)(m_interval * 16 / 10);
}

bool ble_advertising_status_get(void)
{
    return m_is_adv;
}

void ble_gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;

    //    err_code = sd_ble_gap_appearance_set(BLE_APPEARANCE_HEART_RATE_SENSOR_HEART_RATE_BELT);
    //    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));
    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}

void ble_address_set(uint8_t addr_type, uint8_t addr_cycle_mode, uint8_t *p_addr)
{
    uint32_t err_code;
    ble_gap_addr_t gap_addr;
    gap_addr.addr_type = addr_type;
    memcpy(gap_addr.addr, p_addr, 6);
    err_code = sd_ble_gap_address_set(addr_cycle_mode, &gap_addr);
    APP_ERROR_CHECK( err_code );
}

void ble_device_name_set(const char *p_name)
{
    uint32_t                err_code;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t*)p_name,
                                          strlen(p_name));
    APP_ERROR_CHECK(err_code);
}

#ifdef _USE_BLE_SWITCH
void ble_rf_switch_init(void)
{
    nrf_gpio_cfg_output(BLE_ANT_NORMAL);
    nrf_gpio_cfg_output(BLE_ANT_WEAK);

    nrf_gpio_pin_clear(BLE_ANT_NORMAL);
    nrf_gpio_pin_set(BLE_ANT_WEAK);
}
#endif

void ble_tx_power_set(int8_t txp)
{
#ifdef _USE_BLE_SWITCH
    if(txp < -30)
    {
        txp += 22;
        nrf_gpio_pin_clear(BLE_ANT_WEAK);
        nrf_gpio_pin_set(BLE_ANT_NORMAL);
    }
    else
    {
        nrf_gpio_pin_clear(BLE_ANT_NORMAL);
        nrf_gpio_pin_set(BLE_ANT_WEAK);
    }
#endif
    m_tx_power = txp;
    sd_ble_gap_tx_power_set(m_tx_power);
}

int8_t ble_tx_power_get(void)
{
    return m_tx_power;
}

void ble_interval_set(float intv)
{
    uint16_t interval = 0;

    m_interval = intv;

    interval = (uint16_t)(intv * 16 / 10);
    if(interval < 32)
    {
        /*Set to 20ms*/
        interval = 32;
    }
    m_adv_param.interval = interval;
}

void ble_advertising_timeout_set(uint16_t timeout)
{
    m_adv_param.timeout = timeout;
}

float ble_interval_get(void)
{
    return m_interval;
}

void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

bool ble_tx_power_check(int8_t txp)
{
    int8_t txp_array[NUM_OF_TX_POWER] = TXP_ARRAY;

    for(uint8_t i = 0; i < NUM_OF_TX_POWER; i++)
    {
        if(txp_array[i] == txp)
        {
            return true;
        }
    }
    return false;
}

bool ble_interval_check(float intv)
{
    if(intv > INTERVAL_MAX || intv < INTERVAL_MIN)
    {
        return false;
    }
    return true;
}

int8_t mrssi_get(int8_t txp)
{
    int8_t mrssi = -59;

    int8_t txp_array[NUM_OF_TX_POWER] = TXP_ARRAY;
    int8_t mrssi_array[NUM_OF_TX_POWER] = MRSSI_ARRAY;

    for(uint8_t i = 0; i < NUM_OF_TX_POWER; i++)
    {
        if(txp == txp_array[i])
        {
            mrssi = mrssi_array[i];
            break;
        }
    }
    return mrssi;
}

bool ble_data_check(uint8_t* p_data, uint8_t len)
{
    uint8_t length = 0;
    uint8_t type = 0;
    uint8_t data_len = 0;
    uint8_t index = 0;

    if(len > 31 || len < 2)
    {
        return false;
    }
    do
    {
        length = p_data[index++];
        if((length > (len - index)) || length < 1)
        {
            return false;
        }
        data_len = length - 1;
        type = p_data[index++];

        switch(type)
        {
            case BLE_GAP_AD_TYPE_FLAGS:
                if(1 != data_len)
                {
                    return false;
                }
                if(p_data[index] != 0x04 && p_data[index] != 0x05 && p_data[index] != 0x06)
                {
                    return false;
                }
                index += data_len;
                break;

            case BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE:
            case BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE:
                if((data_len % 2) != 0 || data_len == 0)
                {
                    return false;
                }
                index += data_len;
                break;

            case BLE_GAP_AD_TYPE_32BIT_SERVICE_UUID_MORE_AVAILABLE:
            case BLE_GAP_AD_TYPE_32BIT_SERVICE_UUID_COMPLETE:
                return false;

            case BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE:
            case BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_COMPLETE:
                if((data_len % 16) != 0 || data_len == 0)
                {
                    return false;
                }
                index += data_len;
                break;

            case BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME:
            case BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME:
                index += data_len;
                break;

            case BLE_GAP_AD_TYPE_TX_POWER_LEVEL:
                break;

            case BLE_GAP_AD_TYPE_CLASS_OF_DEVICE:
                return false;

            case BLE_GAP_AD_TYPE_SIMPLE_PAIRING_HASH_C:
                return false;

            case BLE_GAP_AD_TYPE_SIMPLE_PAIRING_RANDOMIZER_R:
                return false;

            case BLE_GAP_AD_TYPE_SECURITY_MANAGER_TK_VALUE:
                return false;

            case BLE_GAP_AD_TYPE_SECURITY_MANAGER_OOB_FLAGS:
                return false;

            case BLE_GAP_AD_TYPE_SLAVE_CONNECTION_INTERVAL_RANGE:
                return false;

            case BLE_GAP_AD_TYPE_SOLICITED_SERVICE_UUIDS_16BIT:
                if((data_len % 2) != 0 || data_len == 0)
                {
                    return false;
                }
                index += data_len;
                break;

            case BLE_GAP_AD_TYPE_SOLICITED_SERVICE_UUIDS_128BIT:
                if((data_len % 16) != 0 || data_len == 0)
                {
                    return false;
                }
                index += data_len;
                break;

            case BLE_GAP_AD_TYPE_SERVICE_DATA:
                if(data_len < 2)
                {
                    return false;
                }
                index += data_len;
                break;

            case BLE_GAP_AD_TYPE_PUBLIC_TARGET_ADDRESS:
                if((data_len % 6) != 0)
                {
                    return false;
                }
                index += data_len;
                break;

            case BLE_GAP_AD_TYPE_RANDOM_TARGET_ADDRESS:
                if((data_len % 6) != 0)
                {
                    return false;
                }
                index += data_len;
                break;

            case BLE_GAP_AD_TYPE_APPEARANCE:
                return false;

            case BLE_GAP_AD_TYPE_ADVERTISING_INTERVAL:
                return false;

            case BLE_GAP_AD_TYPE_LE_BLUETOOTH_DEVICE_ADDRESS:
                if((data_len % 6) != 0)
                {
                    return false;
                }
                index += data_len;
                break;

            case BLE_GAP_AD_TYPE_LE_ROLE:
                return false;

            case BLE_GAP_AD_TYPE_SIMPLE_PAIRING_HASH_C256:
                return false;

            case BLE_GAP_AD_TYPE_SIMPLE_PAIRING_RANDOMIZER_R256:
                return false;

            case BLE_GAP_AD_TYPE_SERVICE_DATA_32BIT_UUID:
                return false;

            case BLE_GAP_AD_TYPE_SERVICE_DATA_128BIT_UUID:
                return false;

            case BLE_GAP_AD_TYPE_URI:
                return false;

            case BLE_GAP_AD_TYPE_3D_INFORMATION_DATA:
                return false;

            case BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA:
                index += data_len;
                break;
        }
    }
    while(index < len);

    return true;
}

/**
 * Disconnecting the ble connection
 */
void ble_disconnect(void)
{
    if(BLE_CONN_HANDLE_INVALID != m_conn_handle)
    {
        uint32_t err_code;
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        APP_ERROR_CHECK(err_code);
    }
}

uint32_t ble_conn_handle_get(void)
{
    return m_conn_handle;
}

