#include "ble_serialization.h"
#include "ser_alpha.h"

#include "alpha_ser_tx_enc.h"












/**
 * @brief alpha alive命令响应，打包TX数据包
 */
uint32_t alpha_alive_rsp_enc(uint32_t         return_code,
                             uint8_t * const  p_buf,
                             uint32_t * const p_buf_len)
{
    uint32_t index = 0;

    return op_status_enc( ALPHA_ALIVE_ACCESS, return_code, p_buf, p_buf_len, &index );
}

/**
 * @brief alpha sleep命令响应，打包TX数据包
 */
uint32_t alpha_sleep_rsp_enc(uint32_t         return_code,
                             uint8_t * const  p_buf,
                             uint32_t * const p_buf_len)
{
    uint32_t index = 0;

    return op_status_enc( ALPHA_CMD_SLEEP, return_code, p_buf, p_buf_len, &index );
}

/**
 * @brief alpha_open_rx 命令响应，打包TX数据包
 */
uint32_t alpha_open_rx_rsp_enc( uint32_t         return_code,
                                uint8_t * const  p_buf,
                                uint32_t * const p_buf_len)
{
    uint32_t index = 0;

    return op_status_enc( ALPHA_CMD_OPEN_RX, return_code, p_buf, p_buf_len, &index );
}

/**
 * @brief alpha_open_tx 命令响应，打包TX数据包
 */
uint32_t alpha_open_tx_rsp_enc( uint32_t         return_code,
                                uint8_t * const  p_buf,
                                uint32_t * const p_buf_len)
{
    uint32_t index = 0;

    return op_status_enc( ALPHA_CMD_OPEN_TX, return_code, p_buf, p_buf_len, &index );
}











