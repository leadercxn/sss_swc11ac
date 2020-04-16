/* Copyright (c) 2016 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "nrf.h"
#include "boards.h"
#include "nrf_gpio.h"
#include "ble_dtm.h"
//#include "uart.h"

#include "ble_dtm_test.h"

// Configuration parameters.
#define BITRATE  UART_BAUDRATE_BAUDRATE_Baud19200  /**< Serial bitrate on the UART */

// @note: The BLE DTM 2-wire UART standard specifies 8 data bits, 1 stop bit, no flow control.
//        These parameters are not configurable in the BLE standard.

/**@details Maximum iterations needed in the main loop between stop bit 1st byte and start bit 2nd
 * byte. DTM standard allows 5000us delay between stop bit 1st byte and start bit 2nd byte. 
 * As the time is only known when a byte is received, then the time between between stop bit 1st 
 * byte and stop bit 2nd byte becomes: 
 *      5000us + transmission time of 2nd byte.
 *
 * Byte transmission time is (Baud rate of 19200):
 *      10bits * 1/19200 = approx. 520 us/byte (8 data bits + start & stop bit).
 *
 * Loop time on polling UART register for received byte is defined in ble_dtm.c as:
 *   UART_POLL_CYCLE = 260 us
 *
 * The max time between two bytes thus becomes (loop time: 260us / iteration): 
 *      (5000us + 520us) / 260us / iteration = 21.2 iterations. 
 *
 * This is rounded down to 21. 
 *
 * @note If UART bit rate is changed, this value should be recalculated as well.
 */
#define MAX_ITERATIONS_NEEDED_FOR_NEXT_BYTE 21

/**@brief Function for UART initialization.
 */
static void uart_init(void)
{
    // Configure UART0 pins.
    nrf_gpio_cfg_output(_UART_TX_PIN);
    nrf_gpio_cfg_input(_UART_RX_PIN, NRF_GPIO_PIN_NOPULL);

    NRF_UART0->PSELTXD       = _UART_TX_PIN;
    NRF_UART0->PSELRXD       = _UART_RX_PIN;
    NRF_UART0->BAUDRATE      = BITRATE;

    // Clean out possible events from earlier operations
    NRF_UART0->EVENTS_RXDRDY = 0;
    NRF_UART0->EVENTS_TXDRDY = 0;
    NRF_UART0->EVENTS_ERROR  = 0;

    // Activate UART.
    NRF_UART0->ENABLE        = UART_ENABLE_ENABLE_Enabled;
    NRF_UART0->INTENSET      = 0;
    NRF_UART0->TASKS_STARTTX = 1;
    NRF_UART0->TASKS_STARTRX = 1;
}

/** @brief reference from uart.c
 *  
 */
static void uart_disable(void)
{

    NRF_UART0->TASKS_STOPTX = 1;
    NRF_UART0->TASKS_STOPRX = 1;
    NRF_UART0->ENABLE       = (UART_ENABLE_ENABLE_Disabled << UART_ENABLE_ENABLE_Pos);

    NRF_UART0->EVENTS_RXTO = 0;
    NRF_UART0->EVENTS_RXDRDY = 0;
    NRF_UART0->EVENTS_CTS = 0;
    NRF_UART0->EVENTS_ERROR = 0;
    nrf_gpio_pin_set(_UART_TX_PIN);
}

/**@brief Function for splitting UART command bit fields into separate command parameters for the DTM library.
*
 * @param[in]   command   The packed UART command.
 * @return      result status from dtmlib.
 */
static uint32_t dtm_cmd_put(uint16_t command)
{
    dtm_cmd_t      command_code = (command >> 14) & 0x03;
    dtm_freq_t     freq         = (command >> 8) & 0x3F;
    uint32_t       length       = (command >> 2) & 0x3F;
    dtm_pkt_type_t payload      = command & 0x03;
  
    // Check for Vendor Specific payload.
    if (payload == 0x03) 
    {
        /* Note that in a HCI adaption layer, as well as in the DTM PDU format,
           the value 0x03 is a distinct bit pattern (PRBS15). Even though BLE does not
           support PRBS15, this implementation re-maps 0x03 to DTM_PKT_VENDORSPECIFIC,
           to avoid the risk of confusion, should the code be extended to greater coverage. 
        */
        payload = DTM_PKT_VENDORSPECIFIC;
    }
    return dtm_cmd(command_code, freq, length, payload);
}

int ble_dtm_test(int argc, char **argv)
{
    printf("dtm start\r\n");
    
    uint32_t    current_time;
    uint32_t    dtm_error_code;
    uint32_t    msb_time          = 0;     // Time when MSB of the DTM command was read. Used to catch stray bytes from "misbehaving" testers.
    bool        is_msb_read       = false; // True when MSB of the DTM command has been read and the application is waiting for LSB.
    uint16_t    dtm_cmd_from_uart = 0;     // Packed command containing command_code:freqency:length:payload in 2:6:6:2 bits.
    uint8_t     rx_byte;                   // Last byte read from UART.
    dtm_event_t result;                    // Result of a DTM operation.

    uart_disable();
    uart_init();

    dtm_error_code = dtm_init();
    if (dtm_error_code != DTM_SUCCESS)
    {
        // If DTM cannot be correctly initialized, then we just return.
        return -1;
    }

    for (;;)
    {
        // Will return every timeout, 625 us.
        current_time = dtm_wait();  

        if (NRF_UART0->EVENTS_RXDRDY == 0)
        {
            // Nothing read from the UART.
            continue;
        }
        NRF_UART0->EVENTS_RXDRDY = 0;
        rx_byte                  = (uint8_t)NRF_UART0->RXD;

        if (!is_msb_read)
        {
            // This is first byte of two-byte command.
            is_msb_read       = true;
            dtm_cmd_from_uart = ((dtm_cmd_t)rx_byte) << 8;
            msb_time          = current_time;

            // Go back and wait for 2nd byte of command word.
            continue;
        }

        // This is the second byte read; combine it with the first and process command
        if (current_time > (msb_time + MAX_ITERATIONS_NEEDED_FOR_NEXT_BYTE))
        {
            // More than ~5mS after msb: Drop old byte, take the new byte as MSB.
            // The variable is_msb_read will remains true.
            // Go back and wait for 2nd byte of the command word.
            dtm_cmd_from_uart = ((dtm_cmd_t)rx_byte) << 8;
            msb_time          = current_time;
            continue;
        }

        // 2-byte UART command received.
        is_msb_read        = false;
        dtm_cmd_from_uart |= (dtm_cmd_t)rx_byte;

        if (dtm_cmd_put(dtm_cmd_from_uart) != DTM_SUCCESS)
        {
            // Extended error handling may be put here. 
            // Default behavior is to return the event on the UART (see below);
            // the event report will reflect any lack of success.
        }

        // Retrieve result of the operation. This implementation will busy-loop
        // for the duration of the byte transmissions on the UART.
        if (dtm_event_get(&result))
        {
            // Report command status on the UART.
            // Transmit MSB of the result.
            NRF_UART0->TXD = (result >> 8) & 0xFF;
            // Wait until MSB is sent.
            while (NRF_UART0->EVENTS_TXDRDY != 1)
            {
                // Do nothing.
            }
            NRF_UART0->EVENTS_TXDRDY = 0;

            // Transmit LSB of the result.
            NRF_UART0->TXD = result & 0xFF;
            // Wait until LSB is sent.
            while (NRF_UART0->EVENTS_TXDRDY != 1)
            {
                // Do nothing.
            }
            NRF_UART0->EVENTS_TXDRDY = 0;
        }
    }
}

