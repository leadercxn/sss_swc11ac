#ifndef __SER_CONN_ALPHA_EVENT_ENCORDER_H
#define __SER_CONN_ALPHA_EVENT_ENCORDER_H




/**
 * @brief 访问alpha是否存在事件
 */
uint32_t conn_mw_alpha_alive_access(uint8_t const * const p_rx_buf,
                                    uint32_t              rx_buf_len,
                                    uint8_t * const       p_tx_buf,
                                    uint32_t * const      p_tx_buf_len);








#endif






