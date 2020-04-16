#ifndef LORA_TRANSPORT_H
#define LORA_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include "nrf.h"
#include "LoRaMac.h"

#define NETID                   0x000000

#define LORA_TX_MAX_LEN         37

#define LOTR_TX_STATE_NONE      0
#define LOTR_TX_STATE_FAIL      1
#define LOTR_TX_STATE_DONE      2
#define LOTR_TX_STATE_DONEA     3
#define LOTR_TX_STATE_DONENA    4

#define LOTR_RX_STATE_NONE      0
#define LOTR_RX_STATE_DATA      1

typedef struct
{
    float   up_snr;
    float   down_snr;

    int8_t  up_rssi;
    int8_t  down_rssi;

    int8_t  up_txp;
    int8_t  down_txp;

    uint32_t up_freq;
    uint32_t down_freq;

    uint8_t  up_dr;
    uint8_t  down_dr;

    bool    is_lost;
} lotr_test_info_t;

typedef struct
{
    uint32_t    freq;
    uint16_t    interval;
    uint8_t     dr;
    int8_t      txp;
} lotr_test_cfg_t;

enum lora_transport_error_t
{
    LORA_TRANSPORT_ERROR_NONE           = 0,
    LORA_TRANSPORT_ERROR_BUSY           = 1,
    LORA_TRANSPORT_ERROR_INVALID_LEN    = 2,
    LORA_TRANSPORT_ERROR_TX_LIMITED     = 3
};


typedef void (*lotr_rxed_handler_t)(uint8_t* p_data, uint16_t len);
typedef void (*lotr_txed_handler_t)(uint8_t state);
typedef void (*lotr_test_handler_t)(lotr_test_info_t* info);

typedef struct
{
    lotr_txed_handler_t txed_handler;
    lotr_rxed_handler_t rxed_handler;
    lotr_test_handler_t test_handler;
} lotr_handlers_t;

/**
 * Funciton for initializing lora transport layer
 *
 * @param  p_handlers  [description]
 */
void lotr_init(lotr_handlers_t* p_handlers);

/**
 * Function for lora transport layer to tx data
 *
 * @param   fBuffer      Data buffer to tx
 * @param   fBufferSize  Data len to tx
 * @param   cfmCnt       Confirm up packet retry count, 0 means unconfirm packet
 *
 * @return  LORA_TRANSPORT_ERROR_NONE           Successfully
 *          LORA_TRANSPORT_ERROR_BUSY           Busy
 *          LORA_TRANSPORT_ERROR_INVALID_LEN    Invalid data len    
 *          LORA_TRANSPORT_ERROR_TX_LIMITED     Lora Mac tx limited
 */
uint32_t lotr_tx(uint8_t* fBuffer, uint16_t fBufferSize, uint8_t cfmCnt);

/**
 * Function for lora transport layer to tx test pkt
 */
void lotr_test(void);

/**
 * Function for lora transport layer to tx test pkt
 */
void lotr_test_with_data(uint8_t *p_data, uint16_t len);


#endif
