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
void alpha_txdone(void)
{


}

/**
 * @brief alpha发送超时回调函数
 */
static void alpha_tx_timeout(void)
{
   
}

/**
 * @brief alpha接收完成回调函数
 */
static void alpha_rxdone(uint8_t * payload, uint16_t size, int16_t rssi, int8_t snr)
{

}

/**
 * @brief alpha接收超时回调函数
 */
static void alpha_rx_timeout(void)
{
   
}

/**
 * @brief alpha接收错误回调
 */
static void alpha_rx_error(void)
{
   
}






/**
 * @brief 事件列表 
 */
static RadioEvents_t RxTestEvents =
{
    .TxDone = alpha_txdone,
    .RxDone = alpha_rxdone,
    .RxError = alpha_rx_error,
    .TxTimeout = alpha_tx_timeout,
    .RxTimeout = alpha_rx_timeout,
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

    trace_debug("alpha radio init finish\n"); //
}



















