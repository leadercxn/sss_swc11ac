#include "ble_serialization.h"
#include "ser_conn_alpha_event_encorder.h"
#include "alpha_ser_tx_enc.h"
#include "alpha_ser_rx_dec.h"
#include "user_cfg.h"

/**
 * @brief 访问alpha是否存在事件
 */
uint32_t conn_mw_alpha_alive_access(uint8_t const * const p_rx_buf,
                                    uint32_t              rx_buf_len,
                                    uint8_t * const       p_tx_buf,
                                    uint32_t * const      p_tx_buf_len)
{
    SER_ASSERT_NOT_NULL(p_rx_buf);
    SER_ASSERT_NOT_NULL(p_tx_buf);
    SER_ASSERT_NOT_NULL(p_tx_buf_len);

    uint32_t err_code = NRF_SUCCESS;

    //解析rx数据包


    //处理函数


    //重新组织TX数据包数据结构
    err_code = alpha_alive_rsp_enc(err_code , p_tx_buf , p_tx_buf_len);
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    LOG_I("alpha_alive tx enc success\n");

    return err_code;
}

/**
 * @brief alpha进入睡眠
 */
uint32_t conn_mw_alpha_sleep(   uint8_t const * const p_rx_buf,
                                uint32_t              rx_buf_len,
                                uint8_t * const       p_tx_buf,
                                uint32_t * const      p_tx_buf_len)
{
    SER_ASSERT_NOT_NULL(p_rx_buf);
    SER_ASSERT_NOT_NULL(p_tx_buf);
    SER_ASSERT_NOT_NULL(p_tx_buf_len);

    uint32_t err_code = NRF_SUCCESS;

    //解析rx数据包


    //处理函数


    //重新组织TX数据包数据结构

}

/**
 * @brief alpha进入接收模式
 */
uint32_t conn_mw_alpha_open_rx( uint8_t const * const p_rx_buf,
                                uint32_t              rx_buf_len,
                                uint8_t * const       p_tx_buf,
                                uint32_t * const      p_tx_buf_len)
{
    SER_ASSERT_NOT_NULL(p_rx_buf);
    SER_ASSERT_NOT_NULL(p_tx_buf);
    SER_ASSERT_NOT_NULL(p_tx_buf_len);

    uint32_t err_code = NRF_SUCCESS;

    //解析rx数据包


    //处理函数


    //重新组织TX数据包数据结构

}

/**
 * @brief alpha进入发射数据模式
 */
uint32_t conn_mw_alpha_open_tx( uint8_t const * const p_rx_buf,
                                uint32_t              rx_buf_len,
                                uint8_t * const       p_tx_buf,
                                uint32_t * const      p_tx_buf_len)
{
    SER_ASSERT_NOT_NULL(p_rx_buf);
    SER_ASSERT_NOT_NULL(p_tx_buf);
    SER_ASSERT_NOT_NULL(p_tx_buf_len);

    uint32_t err_code = NRF_SUCCESS;

    //解析rx数据包


    //处理函数


    //重新组织TX数据包数据结构

}







