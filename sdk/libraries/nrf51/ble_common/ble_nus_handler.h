/* Copyright (c) 2016 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */
 
#ifndef __BLE_NUS_HANDLER_H_
#define __BLE_NUS_HANDLER_H_

#include <stdint.h>
#include "ble.h"
#include "ble_nus.h"

/**
 * @brief Data writting evnet handler type
 */
typedef void (*nus_on_write_handler_t)(uint8_t* p_data, uint16_t len);

/**
 * @brief Function for initializing the ble UART Service
 */
void ble_srv_nus_init(void);

/**
 * @brief Function for handling UART Service's BLE events.
 *
 * @param[i]  p_ble_evt  Event received from the SoftDevice.
 */
void on_ble_srv_nus_evt(ble_evt_t* p_ble_evt);

/**
 * @brief Function for sending data to the master device
 *
 * @param[i]  p_data  Data to be sent
 * @param[i]  len     Length of the data
 *
 * @retval NRF_SUCCESS              If data is sent successfully
 * @retval NRF_ERROR_BUSY           If sending procedule is busy
 * @retval NRF_ERROR_INVALID_STATE  If the ble connection is not established or the notification of RX characteristic is not enabled  
 * @retval NRF_ERROR_INVALID_PARAM  If data length is bigger than BLE_NUS_MAX_DATA_LEN
 * 
 */
uint32_t ble_srv_nus_data_send(uint8_t* p_data, uint16_t len);

/**
 * @brief Function for registering handler on data writing
 *
 * @param[i]  handler  Data writing handler
 */
void ble_srv_nus_handler_reg(nus_on_write_handler_t handler);

/**
 * @brief Function for registering handler on indication status change
 *
 * @param[i]  handler  Indication status change handler
 */
void ble_srv_nus_ind_handler_reg(ble_nus_ind_handler_t handler);

/**
 * @brief Function for setting UART Service's password
 *
 * @param[i]  passwd  Password to be setted
 * @param[i]  len     Length of the password
 */
void ble_srv_nus_set_passwd(uint8_t* passwd, uint8_t len);


#endif
