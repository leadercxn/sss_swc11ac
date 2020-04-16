#ifndef __SER_CONN_ALPHA_EVENT_ENCORDER_H
#define __SER_CONN_ALPHA_EVENT_ENCORDER_H




/**
 * @brief 访问alpha是否存在事件
 */
uint32_t conn_mw_alpha_alive_access(uint8_t const * const p_rx_buf,
                                    uint32_t              rx_buf_len,
                                    uint8_t * const       p_tx_buf,
                                    uint32_t * const      p_tx_buf_len);

/**
 * @brief alpha进入睡眠
 */
uint32_t conn_mw_alpha_sleep(   uint8_t const * const p_rx_buf,
                                uint32_t              rx_buf_len,
                                uint8_t * const       p_tx_buf,
                                uint32_t * const      p_tx_buf_len);

/**
 * @brief alpha进入接收模式
 */
uint32_t conn_mw_alpha_open_rx( uint8_t const * const p_rx_buf,
                                uint32_t              rx_buf_len,
                                uint8_t * const       p_tx_buf,
                                uint32_t * const      p_tx_buf_len);

/**
 * @brief alpha进入发射数据模式
 */
uint32_t conn_mw_alpha_open_tx( uint8_t const * const p_rx_buf,
                                uint32_t              rx_buf_len,
                                uint8_t * const       p_tx_buf,
                                uint32_t * const      p_tx_buf_len);


#endif






