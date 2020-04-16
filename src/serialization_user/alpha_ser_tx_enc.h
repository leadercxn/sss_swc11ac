#ifndef __ALPHA_SER_TX_ENC_H
#define __ALPHA_SER_TX_ENC_H







/**
 * @brief alpha alive命令响应，打包TX数据包
 */
uint32_t alpha_alive_rsp_enc(uint32_t         return_code,
                             uint8_t * const  p_buf,
                             uint32_t * const p_buf_len);

/**
 * @brief alpha sleep命令响应，打包TX数据包
 */
uint32_t alpha_sleep_rsp_enc(uint32_t         return_code,
                             uint8_t * const  p_buf,
                             uint32_t * const p_buf_len);

/**
 * @brief alpha_open_rx 命令响应，打包TX数据包
 */
uint32_t alpha_open_rx_rsp_enc( uint32_t         return_code,
                                uint8_t * const  p_buf,
                                uint32_t * const p_buf_len);

/**
 * @brief alpha_open_tx 命令响应，打包TX数据包
 */
uint32_t alpha_open_tx_rsp_enc( uint32_t         return_code,
                                uint8_t * const  p_buf,
                                uint32_t * const p_buf_len);

#endif









