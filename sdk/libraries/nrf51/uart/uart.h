/* Copyright (c) 2009 Nordic Semiconductor. All Rights Reserved.
*
* The information contained herein is property of Nordic Semiconductor ASA.
* Terms and conditions of usage are described in detail in NORDIC
* SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
*
* Licensees are granted free, non-transferable use of the information. NO
* WARRANTY of ANY KIND is provided. This heading must NOT be removed from
* the file.
*
*/

#ifndef UART_H
#define UART_H

/*lint ++flb "Enter library region" */

#include <stdbool.h>
#include <stdint.h>

typedef void (*uart_rx_handler_t)(uint8_t p_data);

/** @file
* @brief Simple UART driver
*
*
* @defgroup nrf_drivers_uart Simple UART driver
* @{
* @ingroup nrf_drivers
* @brief UART driver
*/

/** @brief Function for reading a character from UART.
Execution is blocked until UART peripheral detects character has been received.
\return cr Received character.
*/
uint8_t uart_get(void);

/** @brief Function for reading a character from UART with timeout on how long to wait for the byte to be received.
Execution is blocked until UART peripheral detects character has been received or until the timeout expires, which even occurs first
\return bool True, if byte is received before timeout, else returns False.
@param timeout_ms maximum time to wait for the data.
@param rx_data pointer to the memory where the received data is stored.
*/
bool uart_get_with_timeout(uint32_t timeout_ms, uint8_t* rx_data);

/** @brief Function for sending a character to UART.
Execution is blocked until UART peripheral reports character to have been send.
@param cr Character to send.
*/
void uart_put(uint8_t cr);

/** @brief Function for sending a string to UART.
Execution is blocked until UART peripheral reports all characters to have been send.
Maximum string length is 254 characters including null character in the end.
@param str Null terminated string to send.
*/
void uart_putstring(const uint8_t* str);

/**@brief
 *
 * @param
 */
void uart_put_buffer(uint8_t *p_buf,uint16_t len);

/** @brief Function for configuring UART to use 38400 baud rate.
@param rts_pin_number Chip pin number to be used for UART RTS
@param txd_pin_number Chip pin number to be used for UART TXD
@param cts_pin_number Chip pin number to be used for UART CTS
@param rxd_pin_number Chip pin number to be used for UART RXD
@param hwfc Enable hardware flow control
*/
void uart_config(uint8_t rts_pin_number, uint8_t txd_pin_number, uint8_t cts_pin_number, uint8_t rxd_pin_number, bool hwfc);

/**@brief
 */
void uart_txd_pin_set(uint32_t pin);

/**@brief
 */
void uart_enable(void);

/**@brief
 */
void uart_disable(void);


/**@brief
 */
void uart_rx_int_enable(void);

void uart_rx_int_disable(void);


/**@brief
 *
 * @param
 */
void uart_baud_set(uint32_t rate);

/**@brief
 *
 * @retval
 */
uint32_t uart_baud_get(void);


/**@brief
 *
 * @param
 */
void uart_rx_handler_reg(uart_rx_handler_t handler);

/**
 *@}
 **/

/*lint --flb "Leave library region" */
#endif
