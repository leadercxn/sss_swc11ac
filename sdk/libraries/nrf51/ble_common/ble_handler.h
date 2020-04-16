/* Copyright (c) 2016 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */
 
#ifndef _BLE_HANDLER_H
#define _BLE_HANDLER_H

#include "nrf51.h"
#include "ble_strm_handler.h"
#include "app_error.h"
#include "nrf_soc.h"

//#include "sw_cfg.h"

#ifdef _USE_BLE_SWITCH

#define BLE_ANT_NORMAL                _BLE_ANT_NORMAL
#define BLE_ANT_WEAK                  _BLE_ANT_WEAK

#define NUM_OF_TX_POWER                 12
#define TXP_ARRAY                       {4,    0,     -4,    -8, -12,   -16,  -20,  -30, -34, -38, -42,  -52}
#define MRSSI_ARRAY                     {0xc1, 0xbe, 0xb9, 0xb5, 0xb1, 0xAD, 0xA9, 0x99, -93, -97, -101, -111 }

/**
 * @brief Function for initializing ble rf switch gpio
 */
void ble_rf_switch_init(void);

#else

#define NUM_OF_TX_POWER                 8
#define TXP_ARRAY                       {4,    0,     -4,    -8, -12,   -16,  -20,  -30}
#define MRSSI_ARRAY                     {0xc1, 0xbe, 0xb9, 0xb5, 0xb1, 0xAD, 0xA9, 0x99}

#define ble_rf_switch_init(...)

#endif


/**
 * @brief Function for handling the ble event
 *
 * @param[in]  p_ble_evt  Event received from SoftDevice
 */
void ble_handler_on_ble_evt(ble_evt_t * p_ble_evt);


/**
 * @brief Function for starting ble advertising
 */
void ble_advertising_start(void);

/**
 * @brief Function for stopping ble advertising
 */
void ble_advertising_stop(void);

/**
 * @brief Function for starting ble advertising after ble connection established 
 */
void ble_advertising_start_on_conn(void);

/**
 * @brief Function for getting ble advertising status
 * 
 * @retval true   Ble is advertising
 * @retval false  Ble is not advertising
 */
bool ble_advertising_status_get(void);

/**
 * @brief Function for initializing the ble gap params
 */
void ble_gap_params_init(void);

/**
 * @brief Function for getting ble advertising status.
 * 
 * @param[in]  addr_type        Address type, see @ref BLE_GAP_ADDR_TYPES at ble_gap.h.
 * @param[in]  addr_cycle_mode  Address cycle mode, see @ref BLE_GAP_ADDR_CYCLE_MODES at ble_gap.h.
 * @param[in]  p_addr           Address to set.
 */
void ble_address_set(uint8_t addr_type, uint8_t addr_cycle_mode, uint8_t *p_addr);

/**
 * @brief Function for setting ble device name
 *
 * @param[in]  p_name  Device name
 */
void ble_device_name_set(const char *p_name);

/**
 * @brief Function for entering low power mode
 */
void power_manage(void);

/**
 * @brief Function for getting MRSSI corresponding to tx power
 *
 * @param[in] txp   Ble tx power
 *
 * @retval MRSSI corresponding to tx power
 */
int8_t ble_mrssi_get(int8_t txp);

/**
 * @brief Function for setting ble tx power
 *
 * @param[in] txp   BLE tx power
 */
void ble_tx_power_set(int8_t txp);

/**
 * @brief Function for getting ble tx power
 *
 * @retval BLE tx power
 */
int8_t ble_tx_power_get(void);

/**
 * @brief Function for checking tx power is valid
 *
 * @param[in] txp   BLE tx power
 *
 * @retval true     The tx power is valid
 * @retval false    The tx power is invalid
 */
bool ble_tx_power_check(int8_t txp);

/**
 * @brief Function for setting ble advertising interval
 *
 * @param[i] intv   Advertising interval
 */
void ble_interval_set(float intv);

/**
 * @brief Function for getting ble advertising interval
 *
 * @retval Advertising interval
 */
float ble_interval_get(void);

/**
 * @brief Function for checking if the advertising interval is valid 
 *
 * @param[in] intv  Advertising interval to check
 *
 * @retval true     Is valid
 * @retval false    Is invalid 
 */
bool ble_interval_check(float intv);

/**
 * @brief Function for setting the advertising timeout value
 *
 * @param[in]  timeout  Timeout value in s
 */
void ble_advertising_timeout_set(uint16_t timeout);

/**
 * @brief Function for checking if the data is valid ble advertisment packet
 *
 * @param[i]  p_data    BLE advertisment data
 * @param[i]  len       Length of data
 *
 * @retval true     The advertisment data is valid
 * @retval false    The advertisment data is invalid
 */
bool ble_data_check(uint8_t* p_data, uint8_t len);

/**
 * @brief Function for disconnecting the ble connection
 */
void ble_disconnect(void);

/**
 * @brief Function for getting the ble connection handle
 *
 * @retval BLE connection handle
 */
uint32_t ble_conn_handle_get(void);


#endif
