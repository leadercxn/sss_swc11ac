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
/** @file
* @addtogroup nrf_dev_radio_rx_example_main nrf_dev_radio_tx_example_main
* @{
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "trace.h"
#include "app_util_platform.h"
#include "util.h"

#include "radio_config.h"

/* These are set to zero as Shockburst packets don't have corresponding fields. */
#define PACKET_S1_FIELD_SIZE      (0UL)  /**< Packet S1 field size in bits. */
#define PACKET_S0_FIELD_SIZE      (0UL)  /**< Packet S0 field size in bits. */
#define PACKET_LENGTH_FIELD_SIZE  (0UL)  /**< Packet length field size in bits. */

#define DTM_HEADER_OFFSET        0                                         /**< Index where the header of the pdu is located. */
#define DTM_HEADER_SIZE          2                                         /**< Size of PDU header. */
#define DTM_PAYLOAD_MAX_SIZE     37                                        /**< Maximum payload size allowed during dtm execution. */
#define DTM_LENGTH_OFFSET        (DTM_HEADER_OFFSET + 1)                   /**< Index where the length of the payload is encoded. */
#define DTM_PDU_MAX_MEMORY_SIZE  (DTM_HEADER_SIZE + DTM_PAYLOAD_MAX_SIZE)  /**< Maximum PDU size allowed during dtm execution. */

#define PRBS9_CONTENT {0xff, 0xc1, 0xfb, 0xe8, 0x4c, 0x90, 0x72, 0x8b,  \
                       0xe7, 0xb3, 0x51, 0x89, 0x63, 0xab, 0x23, 0x23,  \
                       0x2,  0x84, 0x18, 0x72, 0xaa, 0x61, 0x2f, 0x3b,  \
                       0x51, 0xa8, 0xe5, 0x37, 0x49, 0xfb, 0xc9, 0xca,  \
                       0xc,  0x18, 0x53, 0x2c, 0xfd}                         /**< The PRBS9 sequence used as packet payload. */

typedef struct
{
    uint8_t content[DTM_HEADER_SIZE + DTM_PAYLOAD_MAX_SIZE];                 /**< PDU packet content. */
} pdu_type_t;

// static uint8_t const     m_prbs_content[]    = PRBS9_CONTENT;                /**< Pseudo-random bit sequence defined by the BLE standard. */

static uint32_t          m_address           = 0x71764129;
static uint8_t           m_crcConfSkipAddr   = 1;
static uint8_t           m_crcLength         = RADIO_CRCCNF_LEN_Three;
static uint8_t           m_packetHeaderLFlen = 8;                            /**< Length of length field in packet Header (in bits). */
static uint8_t           m_packetHeaderS0len = 1;                            /**< Length of S0 field in packet Header (in bytes). */
static uint8_t           m_packetHeaderS1len = 0;

static uint8_t           m_static_length     = 0;                            /**< Number of bytes sent in addition to the var.length payload. */
static uint32_t          m_balen             = 3;                            /**< Base address length in bytes. */
static uint32_t          m_endian            = RADIO_PCNF1_ENDIAN_Little;    /**< On air endianess of packet, this applies to the S0, LENGTH, S1 and the PAYLOAD fields. */
static uint32_t          m_whitening         = RADIO_PCNF1_WHITEEN_Disabled; /**< Whitening disabled. */

static uint32_t          m_crc_poly          = 0x0000065B;                   /**< CRC polynomial. */
static uint32_t          m_crc_init          = 0x00555555;                   /**< Initial value for CRC calculation. */

static pdu_type_t        m_pdu;

static ble_radio_event_handler_t * mp_event_handler = NULL;

static uint32_t swap_bits(uint32_t inp)
{
    uint32_t i;
    uint32_t retval = 0;

    inp = (inp & 0x000000FFUL);

    for(i = 0; i < 8; i++)
    {
        retval |= ((inp >> i) & 0x01) << (7 - i);
    }

    return retval;
}

static uint32_t bytewise_bitswap(uint32_t inp)
{
    return (swap_bits(inp >> 24) << 24)
           | (swap_bits(inp >> 16) << 16)
           | (swap_bits(inp >> 8) << 8)
           | (swap_bits(inp));
}

void ble_radio_switch_init(void)
{
#if defined(_USE_BLE_SWITCH)
    nrf_gpio_cfg_output(_BLE_ANT_NORMAL);
    nrf_gpio_cfg_output(_BLE_ANT_WEAK);

    nrf_gpio_pin_clear(_BLE_ANT_NORMAL);
    nrf_gpio_pin_set(_BLE_ANT_WEAK);
#endif
}

void ble_radio_disable(void)
{
    NRF_RADIO->SHORTS          = 0;
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TEST            = 0;
    NRF_RADIO->TASKS_DISABLE   = 1;
    while(NRF_RADIO->EVENTS_DISABLED == 0)
    {
        // Do nothing.
    }
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_TXEN = 0U;
    NRF_RADIO->TASKS_RXEN = 0U;
}

void ble_radio_const_carrier_configure(void)
{
    NRF_CLOCK->EVENTS_HFCLKSTARTED  = 0;
    NRF_CLOCK->TASKS_HFCLKSTART     = 1;

    ble_radio_disable();

    NRF_RADIO->TEST       = (RADIO_TEST_CONST_CARRIER_Enabled << RADIO_TEST_CONST_CARRIER_Pos) \
                            | (RADIO_TEST_PLL_LOCK_Enabled << RADIO_TEST_PLL_LOCK_Pos);
    NRF_RADIO->SHORTS     = RADIO_SHORTS_READY_START_Msk;
}

void ble_radio_dtm_rx_configure(void)
{
    NRF_CLOCK->EVENTS_HFCLKSTARTED  = 0;
    NRF_CLOCK->TASKS_HFCLKSTART     = 1;
    ble_radio_disable();

    if(((NRF_FICR->OVERRIDEEN) & FICR_OVERRIDEEN_BLE_1MBIT_Msk) == FICR_OVERRIDEEN_BLE_1MBIT_Override)
    {
        NRF_RADIO->OVERRIDE0 = NRF_FICR->BLE_1MBIT[0];
        NRF_RADIO->OVERRIDE1 = NRF_FICR->BLE_1MBIT[1];
        NRF_RADIO->OVERRIDE2 = NRF_FICR->BLE_1MBIT[2];
        NRF_RADIO->OVERRIDE3 = NRF_FICR->BLE_1MBIT[3];
        NRF_RADIO->OVERRIDE4 = NRF_FICR->BLE_1MBIT[4];
    }
    // Set the access address, address0/prefix0 used for both Rx and Tx address
    NRF_RADIO->PREFIX0    &= ~RADIO_PREFIX0_AP0_Msk;
    NRF_RADIO->PREFIX0    |= (m_address >> 24) & RADIO_PREFIX0_AP0_Msk;
    NRF_RADIO->BASE0       = m_address << 8;
    NRF_RADIO->RXADDRESSES = RADIO_RXADDRESSES_ADDR0_Enabled << RADIO_RXADDRESSES_ADDR0_Pos;
    NRF_RADIO->TXADDRESS   = (0x00 << RADIO_TXADDRESS_TXADDRESS_Pos) & RADIO_TXADDRESS_TXADDRESS_Msk;

    // Configure CRC calculation
    NRF_RADIO->CRCCNF = (m_crcConfSkipAddr << RADIO_CRCCNF_SKIP_ADDR_Pos) |
                        (m_crcLength << RADIO_CRCCNF_LEN_Pos);

    NRF_RADIO->PCNF0 = (m_packetHeaderS1len << RADIO_PCNF0_S1LEN_Pos) |
                       (m_packetHeaderS0len << RADIO_PCNF0_S0LEN_Pos) |
                       (m_packetHeaderLFlen << RADIO_PCNF0_LFLEN_Pos);

    NRF_RADIO->PCNF1 = (m_whitening          << RADIO_PCNF1_WHITEEN_Pos) |
                       (m_endian             << RADIO_PCNF1_ENDIAN_Pos)  |
                       (m_balen              << RADIO_PCNF1_BALEN_Pos)   |
                       (m_static_length      << RADIO_PCNF1_STATLEN_Pos) |
                       (DTM_PAYLOAD_MAX_SIZE << RADIO_PCNF1_MAXLEN_Pos);


    NRF_RADIO->TEST         = 0;
    NRF_RADIO->CRCPOLY      = m_crc_poly;
    NRF_RADIO->CRCINIT      = m_crc_init;
    NRF_RADIO->PACKETPTR    = (uint32_t)&m_pdu;                      // Setting packet pointer will start the radio
    NRF_RADIO->EVENTS_READY = 0;

    NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos |
                        RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos |
                        RADIO_SHORTS_ADDRESS_RSSISTART_Enabled << RADIO_SHORTS_ADDRESS_RSSISTART_Pos;


    NRF_RADIO->INTENSET = RADIO_INTENSET_END_Enabled << RADIO_INTENSET_END_Pos;

    NVIC_SetPriority(RADIO_IRQn, APP_IRQ_PRIORITY_LOW);
    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);
}

static void ble_radio_config(void)
{
    NRF_CLOCK->EVENTS_HFCLKSTARTED  = 0;
    NRF_CLOCK->TASKS_HFCLKSTART     = 1;

    ble_radio_disable();

    // Radio address config
    NRF_RADIO->PREFIX0 =
        ((uint32_t)swap_bits(0xC3) << 24) // Prefix byte of address 3 converted to nRF24L series format
        | ((uint32_t)swap_bits(0xC2) << 16) // Prefix byte of address 2 converted to nRF24L series format
        | ((uint32_t)swap_bits(0xC1) << 8)  // Prefix byte of address 1 converted to nRF24L series format
        | ((uint32_t)swap_bits(0xC0) << 0); // Prefix byte of address 0 converted to nRF24L series format

    NRF_RADIO->PREFIX1 =
        ((uint32_t)swap_bits(0xC7) << 24) // Prefix byte of address 7 converted to nRF24L series format
        | ((uint32_t)swap_bits(0xC6) << 16) // Prefix byte of address 6 converted to nRF24L series format
        | ((uint32_t)swap_bits(0xC4) << 0); // Prefix byte of address 4 converted to nRF24L series format

    NRF_RADIO->BASE0 = bytewise_bitswap(0x01234567UL);  // Base address for prefix 0 converted to nRF24L series format
    NRF_RADIO->BASE1 = bytewise_bitswap(0x89ABCDEFUL);  // Base address for prefix 1-7 converted to nRF24L series format

    NRF_RADIO->TXADDRESS   = 0x00UL;  // Set device address 0 to use when transmitting
    NRF_RADIO->RXADDRESSES = 0x01UL;  // Enable device address 0 to use to select which addresses to receive

    // Packet configuration
    NRF_RADIO->PCNF0 = (m_packetHeaderS1len     << RADIO_PCNF0_S1LEN_Pos) |
                       (m_packetHeaderS0len     << RADIO_PCNF0_S0LEN_Pos) |
                       (m_packetHeaderLFlen     << RADIO_PCNF0_LFLEN_Pos); //lint !e845 "The right argument to operator '|' is certain to be 0"

    // Packet configuration
    NRF_RADIO->PCNF1 = (m_whitening << RADIO_PCNF1_WHITEEN_Pos) |
                       (m_endian    << RADIO_PCNF1_ENDIAN_Pos)  |
                       (m_balen     << RADIO_PCNF1_BALEN_Pos)   |
                       (m_static_length << RADIO_PCNF1_STATLEN_Pos) |
                       (4               << RADIO_PCNF1_MAXLEN_Pos); //lint !e845 "The right argument to operator '|' is certain to be 0"

    // CRC Config
    NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Three << RADIO_CRCCNF_LEN_Pos); // Number of checksum bits

    NRF_RADIO->CRCINIT = m_crc_init;   // Initial value
    NRF_RADIO->CRCPOLY = m_crc_poly;  // CRC poly: x^16+x^12^x^5+1

    NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos |
                        RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos |
                        RADIO_SHORTS_ADDRESS_RSSISTART_Enabled << RADIO_SHORTS_ADDRESS_RSSISTART_Pos;

    NRF_RADIO->INTENSET = RADIO_INTENSET_END_Enabled << RADIO_INTENSET_END_Pos;

    NVIC_SetPriority(RADIO_IRQn, APP_IRQ_PRIORITY_LOW);
    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);
}

void ble_radio_config_set(ble_radio_config_t config)
{
    NRF_RADIO->TXPOWER    = config.power << RADIO_TXPOWER_TXPOWER_Pos;
    NRF_RADIO->MODE       = config.mode << RADIO_MODE_MODE_Pos;
    NRF_RADIO->FREQUENCY  = config.channel;
}

void ble_radio_init(ble_radio_event_handler_t * p_handler)
{
    ble_radio_config();
    mp_event_handler = p_handler;
}

void ble_radio_tx_enable(void)
{
    NRF_RADIO->TASKS_TXEN = 1;
}

void ble_radio_rx_enable(void)
{
    NRF_RADIO->TASKS_RXEN = 1;
}

void ble_radio_packet_set(uint8_t * p_packet)
{
    NRF_RADIO->PACKETPTR = (uint32_t)p_packet;
}

bool ble_radio_crc_status_get(void)
{
    return (NRF_RADIO->CRCSTATUS == 1U);
}

int8_t ble_radio_rssi_get(void)
{
    return -NRF_RADIO->RSSISAMPLE;
}

void RADIO_IRQHandler(void)
{
    if(NRF_RADIO->EVENTS_END == 1)
    {
        NRF_RADIO->EVENTS_END = 0;

        if(mp_event_handler != NULL && mp_event_handler->on_event_end_handler != NULL)
        {
            mp_event_handler->on_event_end_handler();
        }
    }
}
