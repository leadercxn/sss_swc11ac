#include "ble_serialization.h"
#include "ser_conn_alpha_event_encorder.h"
#include "alpha_ser_tx_enc.h"
#include "alpha_ser_rx_dec.h"
#include "alpha_radio_handler.h"
#include "user_cfg.h"

#include "sx1276.h"

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

    //解析rx数据包,并返回传参


    //处理函数


    //重新组织TX数据包数据结构
    err_code = alpha_alive_rsp_enc(err_code , p_tx_buf , p_tx_buf_len);
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    LOG_I("alpha items-list task: alive  success\n");
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



    //解析rx数据包,并返回传参


    //处理函数
    Radio.Sleep();                  //进入睡眠模式

    //重新组织TX数据包数据结构
    err_code = alpha_sleep_rsp_enc(err_code , p_tx_buf , p_tx_buf_len);
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    LOG_I("alpha items-list task: sleep success\n");
    return err_code;
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

    alpha_rx_param_t alpha_rx_param ;

    alpha_rx_param.mode = MODEM_LORA ;                                              //模式
    alpha_rx_param.bandwidthAfc =  0 ; 
    alpha_rx_param.fixLen = false ;
    alpha_rx_param.payloadLen = 0 ;
    alpha_rx_param.FreqHopOn = false ;
    alpha_rx_param.HopPeriod = 0 ;
    alpha_rx_param.rxContinuous = true ;

    //解析rx数据包,并返回传参
    err_code = alpha_open_rx_req_dec( p_rx_buf , rx_buf_len , &alpha_rx_param );        
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    //处理函数
    alpha_radio_rx( alpha_rx_param );

    //重新组织TX数据包数据结构
    err_code = alpha_open_rx_rsp_enc(err_code , p_tx_buf , p_tx_buf_len);
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    LOG_I("alpha items-list task: open rx success\n");
    return err_code;
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

    alpha_tx_param_t    alpha_tx_param ;

    alpha_tx_param.mode = MODEM_LORA ;
    alpha_tx_param.fdev = 0 ;
    alpha_tx_param.fixLen = 0 ;
    alpha_tx_param.FreqHopOn = false ;
    alpha_tx_param.HopPeriod = 0 ;

    //解析rx数据包,并返回传参
    alpha_open_tx_req_dec( p_rx_buf , rx_buf_len , &alpha_tx_param );
    
    //处理函数
    alpha_radio_tx( alpha_tx_param );

    //重新组织TX数据包数据结构
    err_code = alpha_open_tx_rsp_enc( err_code , p_tx_buf , p_tx_buf_len );
    SER_ASSERT(err_code == NRF_SUCCESS, err_code);

    LOG_I("alpha items-list task: open tx success\n");
    return err_code;
}







