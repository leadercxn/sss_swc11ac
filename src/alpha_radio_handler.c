#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "radio.h"
#include "sx1276.h"
#include "sx1276_board.h"
#include "nrf_delay.h"
#include "util.h"

#include "alpha_radio_handler.h"
#include "trace.h"


/**
 * @brief alpha发送完成回调
 */
void alpha_lora_txdone(void)
{
    trace_debug("alpha_lora_txdone ===>  \n"); //

}

/**
 * @brief alpha发送超时回调函数
 */
static void alpha_lora_tx_timeout(void)
{
    trace_debug("alpha_lora_tx_timeout ===>  \n"); //

}

/**
 * @brief alpha接收完成回调函数
 */
static void alpha_lora_rxdone(uint8_t * payload, uint16_t size, int16_t rssi, int8_t snr)
{
    trace_debug("alpha_lora_rxdone ===>  \n"); //
    trace_dump(payload , size);
    trace_debug("rssi = %d , SNR = %d  \n" , rssi , snr );
}

/**
 * @brief alpha接收超时回调函数
 */
static void alpha_lora_rx_timeout(void)
{
    trace_debug("alpha_lora_rx_timeout ===>  \n"); //

}

/**
 * @brief alpha接收错误回调
 */
static void alpha_lora_rx_error(void)
{
    trace_debug("alpha_lora_rx_error ===>  \n"); //

}






/**
 * @brief 事件列表 
 */
static RadioEvents_t RxTestEvents =
{
    .TxDone = alpha_lora_txdone,
    .RxDone = alpha_lora_rxdone,
    .RxError = alpha_lora_rx_error,
    .TxTimeout = alpha_lora_tx_timeout,
    .RxTimeout = alpha_lora_rx_timeout,
    .CadDone = NULL
};


/**
 * @brief alpha芯片的初始化
 */
void alpha_radio_init(void)
{
    Radio.Init(&RxTestEvents);      //初始化IO
    Radio.SetModem(MODEM_LORA);     //设置成lora模式
    Radio.Sleep();                  //进入睡眠模式
}


/**
 * @brief 设置lora进入接收模式
 */
void alpha_radio_rx( alpha_rx_param_t  alpha_rx_param )
{
    Radio.SetChannel( alpha_rx_param.freq );
    Radio.SetRxConfig(  alpha_rx_param.mode ,
                        alpha_rx_param.bandwidth ,
                        alpha_rx_param.datarate ,
                        alpha_rx_param.coderate ,
                        alpha_rx_param.bandwidthAfc,
                        alpha_rx_param.preambleLen,
                        alpha_rx_param.symbTimeout,
                        alpha_rx_param.fixLen,
                        alpha_rx_param.payloadLen,
                        alpha_rx_param.crcOn,
                        alpha_rx_param.FreqHopOn,
                        alpha_rx_param.HopPeriod,
                        alpha_rx_param.iqInverted,
                        alpha_rx_param.rxContinuous );
    Radio.Rx(0);
    trace_debug("alpha_radio_rx ===>  \n"); //
}

/**
 * @brief 设置lora进入发送模式
 */
void alpha_radio_tx( alpha_tx_param_t  alpha_tx_param )
{
    Radio.SetChannel( alpha_tx_param.freq );
    Radio.SetTxConfig(  alpha_tx_param.mode, 
                        alpha_tx_param.power, 
                        alpha_tx_param.fdev, 
                        alpha_tx_param.bandwidth,
                        alpha_tx_param.datarate, 
                        alpha_tx_param.coderate,
                        alpha_tx_param.preambleLen, 
                        alpha_tx_param.fixLen,
                        alpha_tx_param.crcOn, 
                        alpha_tx_param.FreqHopOn, 
                        alpha_tx_param.HopPeriod, 
                        alpha_tx_param.iqInverted, 
                        alpha_tx_param.timeout);
    Radio.Send( alpha_tx_param.tx_buff , alpha_tx_param.tx_buff_len );
    trace_debug("alpha_radio_tx ===>  \n"); //
}




















