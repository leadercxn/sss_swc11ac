#ifndef __ALPHA_RADIO_HANDLER_H
#define __ALPHA_RADIO_HANDLER_H


#include "radio.h"


/**
 * @brief lora接收设置参数结构
 */
typedef struct              
{
    RadioModems_t   mode ;          //lora所处的模式
    uint32_t        bandwidth;      //带宽
    uint32_t        datarate;       //速度
    uint8_t         coderate;       //编码率
    uint32_t        bandwidthAfc;   //AFC带宽 (仅FSK有效)
    uint16_t        preambleLen;    //前序长度
    uint16_t        symbTimeout;    //接收信号超时时间 (仅lora有效)
    bool            fixLen;         //是否定长
    uint8_t         payloadLen;     //数据长度，假如使用定长模式
    bool            crcOn;          //是否开启crc
    bool            FreqHopOn;      //是否开始包内调频
    uint8_t         HopPeriod;      //每一跳之间的符号数
    bool            iqInverted;     //反转的IQ信号
    bool            rxContinuous;   //是否持续接收模式

    uint32_t        freq;           //频点
} alpha_rx_param ;


/**
 * @brief lora发射设置参数结构
 */
typedef struct              
{
    RadioModems_t   mode ;          //lora所处的模式
    int8_t          power;          //发射功率
    uint32_t        fdev;           //频率偏差 (仅FSK有效)
    uint32_t        bandwidth;      //带宽
    uint32_t        datarate;       //传输速度
    uint8_t         coderate;       //编码率
    uint16_t        preambleLen;    //前导码长度
    bool            fixLen;         //是否定长
    bool            crcOn;          //是否开启crc
    bool            FreqHopOn;      //是否开始包内调频
    uint8_t         HopPeriod;      //每一跳之间的符号数
    bool            iqInverted;     //反转的IQ信号
    bool            rxContinuous;   //是否持续接收模式

    uint32_t        freq;           //频点
} alpha_rx_param ;






/**
 * @brief alpha芯片的初始化
 */
void alpha_radio_init(void);






#endif













