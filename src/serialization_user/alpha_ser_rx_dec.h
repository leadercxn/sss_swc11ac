#ifndef __ALPHA_SER_RX_DEC_H
#define __ALPHA_SER_RX_DEC_H


#include "alpha_radio_handler.h"


/**
 * @brief 解析串口发送过来控制alpha rx参数的函数
 */
uint32_t  alpha_open_rx_req_dec(uint8_t const * const           p_buf,
                                uint32_t                        packet_len,
                                alpha_rx_param_t *              alpha_rx_param );

/**
 * @brief 解析串口发送过来控制alpha tx参数的函数
 */
uint32_t  alpha_open_tx_req_dec(uint8_t const * const           p_buf,
                                uint32_t                        packet_len,
                                alpha_tx_param_t *              p_alpha_tx_param );






#endif






