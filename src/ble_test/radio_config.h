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
#ifndef RADIO_CONFIG_H
#define RADIO_CONFIG_H

#define DTM_PKT_PRBS9                   0x00                            /**< Bit pattern PRBS9. */
#define DTM_PKT_0X0F                    0x01                            /**< Bit pattern 11110000 (LSB is the leftmost bit). */
#define DTM_PKT_0X55                    0x02                            /**< Bit pattern 10101010 (LSB is the leftmost bit). */
#define DTM_PKT_VENDORSPECIFIC          0xFFFFFFFF                      /**< Vendor specific. Nordic: Continuous carrier test, or configuration. */

#define PACKET_BASE_ADDRESS_LENGTH  (4UL)                   //!< Packet base address length field size in bytes
#define PACKET_STATIC_LENGTH        (1UL)                   //!< Packet static length in bytes
#define PACKET_PAYLOAD_MAXSIZE      (PACKET_STATIC_LENGTH)  //!< Packet payload maximum size in bytes

typedef struct
{
    void (*on_event_end_handler)(void);
} ble_radio_event_handler_t;


typedef struct
{
    int8_t  power;
    uint8_t mode;
    uint8_t channel;
} ble_radio_config_t;

void ble_radio_switch_init(void);
void ble_radio_init(ble_radio_event_handler_t * p_handler);
void ble_radio_config_set(ble_radio_config_t config);
void ble_radio_dtm_rx_configure(void);
void ble_radio_tx_enable(void);
void ble_radio_rx_enable(void);
void ble_radio_disable(void);
void ble_radio_packet_set(uint8_t * p_packet);
bool ble_radio_crc_status_get(void);
int8_t ble_radio_rssi_get(void);

void ble_radio_const_carrier_configure(void);

#endif
