/*
/ _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
\____ \| ___ |    (_   _) ___ |/ ___)  _ \
_____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
   (C)2013 Semtech

Description: LoRaMac classA device implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis, Gregory Cristian and Wael Guibene
*/
/******************************************************************************
  * @file    lora.c
  * @author  MCD Application Team
  * @version V1.1.0
  * @date    27-February-2017
  * @brief   lora API to drive the lora state Machine
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice,
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other
  *    contributors to this software may be used to endorse or promote products
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under
  *    this license is void and will automatically terminate your rights under
  *    this license.
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "LoRaMac.h"
#include "Region.h"
#include "RegionEU433.h"
#include "RegionEU868.h"
#include "RegionAS923.h"
#include "RegionIN865.h"
#include "RegionUS915-Hybrid.h"
#include "RegionAU915-Hybrid.h"
#include "RegionCN470.h"
#include "RegionCN779.h"
#include "RegionSE800.h"

#include "lora.h"

#include "timer_platform.h"
#include "util.h"
//include "typedef.h"

#include "trace.h"


#include "LoRaMacTest.h"

/*!
 * LoRaWAN ETSI duty cycle control enable/disable
 *
 * \remark Please note that ETSI mandates duty cycled transmissions. Use only for test purposes
 */
#define LORAWAN_DUTYCYCLE_ON                        false

#define USE_DEFAULT_CHANNEL_LINEUP          1

#if( USE_DEFAULT_CHANNEL_LINEUP == 1 )

#define EU868_LC4                { 867100000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define EU868_LC5                { 867300000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define EU868_LC6                { 867500000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define EU868_LC7                { 867700000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define EU868_LC8                { 867900000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }

#define EU433_LC4                { 433775000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define EU433_LC5                { 433975000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define EU433_LC6                { 434175000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define EU433_LC7                { 434375000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define EU433_LC8                { 434575000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }

#define AS923_LC3                { 923600000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define AS923_LC4                { 923800000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define AS923_LC5                { 924000000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define AS923_LC6                { 924200000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define AS923_LC7                { 924400000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define AS923_LC8                { 924600000, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }

#endif

typedef struct
{
    uint16_t    crc;
    uint8_t     sn[8];
    uint8_t     token[16];
    uint8_t     deveui[8];
    uint8_t     appeui[8];
    uint8_t     appkey[16];
    uint8_t     appskey[16];
    uint8_t     nwkskey[16];
    uint32_t    devaddr;
} uicr_param_layout_t;

typedef union
{
    uicr_param_layout_t db;
    uint32_t padding[CEIL_DIV(sizeof(uicr_param_layout_t), 4)];
} uicr_param_t;

/*!
 * LoRa State Machine states
 */
typedef enum eDevicState
{
    DEVICE_STATE_INIT,
    DEVICE_STATE_JOIN,
    DEVICE_STATE_JOINED,
    DEVICE_STATE_SEND,
    DEVICE_STATE_CYCLE,
    DEVICE_STATE_SLEEP
} DeviceState_t;

/**
* Lora Configuration
*/
typedef struct
{
    LoRaMacRegion_t region;
    ActivationType_t activation;
    bool duty_cycle;             /*< ENABLE if dutycyle is on, DISABLE otherwise */
    uint8_t DevEui[8];           /*< Device EUI */
    uint8_t AppEui[8];           /*< Application EUI */
    uint8_t AppKey[16];          /*< Application Key */
    int8_t TxDatarate;
    bool TxDatarateUpdate;
    uint8_t TxPower;
    bool TxPowerUpdate;
    bool AdrEnable;
    bool AdrEnableUpdate;
    int16_t Rssi;                /*< Rssi of the received packet */
    int8_t Snr;                  /*< Snr of the received packet */
    uint8_t confirmNbTrials;
    uint32_t joinNbTrials;
    McpsConfirm_t *McpsConfirm;  /*< pointer to the confirm structure */
} LoRaParam_t;

static LoRaParam_t LoRaParam =
{
    .DevEui = {0},
    .AppEui = {0},
    .AppKey = {0},

    .activation = ACTIVATION_ABP,
    .duty_cycle = false,
    .AdrEnable = false,

    .Rssi = 99,
    .Snr = 99,
    .confirmNbTrials = 0,
    .joinNbTrials = 0,
    .McpsConfirm = NULL,

    .TxDatarateUpdate = false,
    .TxPowerUpdate = false,
    .AdrEnableUpdate = false,
};


/*!
 * User application data buffer size
 */
#define LORAWAN_APP_DATA_BUFF_SIZE                           255

/*!
 * User application data
 */
static uint8_t AppDataBuff[LORAWAN_APP_DATA_BUFF_SIZE];

/*!
 * User application data structure
 */
static LoRaAppData_t AppData = { AppDataBuff,  0 , 0 };

/*!
 * Indicates if the node is sending confirmed or unconfirmed messages
 */
static bool IsTxConfirmed = false;

static bool Certification = false;
static bool CertificationStop = false;

static bool McpsReqRunning = false;

static bool Flag_MC_Ack = false;

/*!
 * Timer to handle the application data transmission duty cycle
 */
TIMER_DEF(TxcertifTimer);

static DeviceState_t DeviceState = DEVICE_STATE_INIT ;

static LoRaMacPrimitives_t LoRaMacPrimitives;
static LoRaMacCallback_t LoRaMacCallbacks;
static LoRaCallback_t *LoRaCallbacks;

/*!
 * Indicates if a new packet can be sent
 */
static bool NextTx = true;

/*!
 * LoRaWAN compliance tests support data
 */
struct ComplianceTest_s
{
    bool Running;
    uint8_t State;
    bool IsTxConfirmed;
    uint8_t AppPort;
    uint8_t AppDataSize;
    uint8_t *AppDataBuffer;
    uint16_t DownLinkCounter;
    bool LinkCheck;
    uint8_t DemodMargin;
    uint8_t NbGateways;
} ComplianceTest;

#if 0
static uint16_t crc16_compute(uint8_t * p_data, uint16_t size, uint16_t * p_crc)
{
    uint16_t i;
    uint16_t crc = (p_crc == NULL) ? 0xffff : *p_crc;

    for (i = 0; i < size; i++)
    {
        crc  = (unsigned char)(crc >> 8) | (crc << 8);
        crc ^= p_data[i];
        crc ^= (unsigned char)(crc & 0xff) >> 4;
        crc ^= (crc << 8) << 4;
        crc ^= ((crc & 0xff) << 4) << 1;
    }
    return crc;
}
#endif

/*!
 * \brief   Prepares the payload of the frame
 */
static void PrepareTxFrame()
{
    if (ComplianceTest.Running == true)
    {
        if (ComplianceTest.LinkCheck == true)
        {
            ComplianceTest.LinkCheck = false;
            AppData.BuffSize = 3;
            AppData.Buff[0] = 5;
            AppData.Buff[1] = ComplianceTest.DemodMargin;
            AppData.Buff[2] = ComplianceTest.NbGateways;
            ComplianceTest.State = 1;
        }
        else
        {
            switch (ComplianceTest.State)
            {
                case 4:
                    ComplianceTest.State = 1;
                    break;

                case 1:
                    AppData.BuffSize = 2;
                    AppData.Buff[0] = ComplianceTest.DownLinkCounter >> 8;
                    AppData.Buff[1] = ComplianceTest.DownLinkCounter;
                    break;
            }
        }
    }
}

/*!
 * \brief   Prepares the payload of the frame
 *
 * \retval  [0: frame could be send, 1: error]
 */
static bool SendFrame(void)
{
    McpsReq_t mcpsReq;
    LoRaMacTxInfo_t txInfo;

    MibRequestConfirm_t mibReq;

    trace_verbose("SendFrame\r\n");

    McpsReqRunning = true;
//zhangtieming
    // if (LoRaParam.TxPowerUpdate)
    // {
    //     LoRaParam.TxPowerUpdate = false;

    //     mibReq.Type = MIB_CHANNELS_TX_POWER;
    //     mibReq.Param.ChannelsTxPower = LoRaParam.TxPower;
    //     LoRaMacMibSetRequestConfirm(&mibReq);

    //     mibReq.Type = MIB_CHANNELS_DEFAULT_TX_POWER;
    //     mibReq.Param.ChannelsDefaultTxPower = LoRaParam.TxPower;
    //     LoRaMacMibSetRequestConfirm(&mibReq);
    //     mibReq.Type = MIB_CHANNELS_TX_POWER;
    //     mibReq.Param.ChannelsTxPower = level;
    //     status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    //     mibReq.Type = MIB_CHANNELS_DEFAULT_TX_POWER;
    //     mibReq.Param.ChannelsTxPower = level;
    //     status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);
    // }

    if (LoRaParam.AdrEnableUpdate)
    {
        LoRaParam.AdrEnableUpdate = false;

        mibReq.Type = MIB_ADR;
        mibReq.Param.AdrEnable = LoRaParam.AdrEnable;
        LoRaMacMibSetRequestConfirm(&mibReq);
    }
//zhangtieming
    // if (LoRaParam.TxDatarateUpdate)
    // {
    //     LoRaParam.TxDatarateUpdate = false;

    //     mibReq.Type = MIB_CHANNELS_DEFAULT_DATARATE;
    //     mibReq.Param.ChannelsDefaultDatarate = LoRaParam.TxDatarate;
    //     LoRaMacMibSetRequestConfirm(&mibReq);
    // }

    if (LoRaMacQueryTxPossible(AppData.BuffSize, &txInfo) != LORAMAC_STATUS_OK)
    {
        trace_warn("LoRaMacQueryTxPossible false\r\n");
        // Send empty frame in order to flush MAC commands
        mcpsReq.Type = MCPS_UNCONFIRMED;
        mcpsReq.Req.Unconfirmed.fBuffer = NULL;
        mcpsReq.Req.Unconfirmed.fBufferSize = 0;
        mcpsReq.Req.Unconfirmed.Datarate = LoRaParam.TxDatarate;
    }
    else
    {
        if (IsTxConfirmed == false)
        {
            mcpsReq.Type = MCPS_UNCONFIRMED;
            mcpsReq.Req.Unconfirmed.fPort = AppData.Port;
            mcpsReq.Req.Unconfirmed.fBuffer = AppData.Buff;
            mcpsReq.Req.Unconfirmed.fBufferSize = AppData.BuffSize;
            mcpsReq.Req.Unconfirmed.Datarate = LoRaParam.TxDatarate;
        }
        else
        {
            mcpsReq.Type = MCPS_CONFIRMED;
            mcpsReq.Req.Confirmed.fPort = AppData.Port;
            mcpsReq.Req.Confirmed.fBuffer = AppData.Buff;
            mcpsReq.Req.Confirmed.fBufferSize = AppData.BuffSize;
            mcpsReq.Req.Confirmed.NbTrials = LoRaParam.confirmNbTrials;
            mcpsReq.Req.Confirmed.Datarate = LoRaParam.TxDatarate;

            IsTxConfirmed = false;
        }
    }

    if (LoRaMacMcpsRequest(&mcpsReq) == LORAMAC_STATUS_OK)
    {
        LoRaParam.Snr = 99;
        LoRaParam.Rssi = 99;
        return false;
    }

    LoRaParam.Snr = 99;
    LoRaParam.Rssi = 99;

    McpsReqRunning = false;

    trace_warn("LoRaMacMcpsRequest error\r\n");
    return true;
}

void OnSendEvent(void)
{
    MibRequestConfirm_t mibReq;
    LoRaStatus_t status;

    trace_verbose("OnSendEvent\r\n");

    mibReq.Type = MIB_NETWORK_JOINED;
    status = (LoRaStatus_t)LoRaMacMibGetRequestConfirm(&mibReq);

    if (status == LORA_STATUS_OK)
    {
        if (mibReq.Param.IsNetworkJoined == true)
        {
            DeviceState = DEVICE_STATE_SEND;
            NextTx = true;
        }
        else
        {
            DeviceState = DEVICE_STATE_JOIN;
        }
    }
}

/*!
 * \brief   MCPS-Confirm event function
 *
 * \param   [IN] McpsConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void McpsConfirm(McpsConfirm_t *mcpsConfirm)
{
    trace_debug("McpsConfirm\r\n");

    LoRaTxConfirm_t txConfirm;

    LoRaParam.McpsConfirm = mcpsConfirm;

    txConfirm.Status = mcpsConfirm->Status;
    txConfirm.UpLinkFrequency = mcpsConfirm->UpLinkFrequency;
    txConfirm.TxDatarate = mcpsConfirm->Datarate;
    txConfirm.TxPower = mcpsConfirm->TxPower;
    txConfirm.UpLinkCounter = mcpsConfirm->UpLinkCounter;
    txConfirm.AckReceived = mcpsConfirm->AckReceived;
    txConfirm.Rssi = mcpsConfirm->Rssi;
    txConfirm.Snr = (int8_t)mcpsConfirm->Snr >> 2;
    txConfirm.RxIndication = mcpsConfirm->RxIndication;
    txConfirm.PacketType = LORA_PACKET_UNCONFIRMED;

    switch(mcpsConfirm->McpsRequest)
    {
        case MCPS_UNCONFIRMED:
        {
            // Check Datarate
            // Check TxPower
            txConfirm.PacketType = LORA_PACKET_UNCONFIRMED;
            break;
        }

        case MCPS_CONFIRMED:
        {
            // Check Datarate
            // Check TxPower
            // Check AckReceived
            // Check NbTrials
            txConfirm.PacketType = LORA_PACKET_CONFIRMED;
            break;
        }

        case MCPS_PROPRIETARY:
        {
            break;
        }

        default:
            break;
    }

    NextTx = true;

    if (!Certification)
    {
        if (Flag_MC_Ack == false)
        {
            if (LoRaCallbacks != NULL && LoRaCallbacks->OnTxDone != NULL)
            {
                LoRaCallbacks->OnTxDone(&txConfirm);
            }
        }
        else
        {
            Flag_MC_Ack = false;
        }
    }

    if (CertificationStop)
    {
        CertificationStop = false;
        Certification = false;
    }

    McpsReqRunning = false;
}

/*!
 * \brief   MCPS-Indication event function
 *
 * \param   [IN] mcpsIndication - Pointer to the indication structure,
 *               containing indication attributes.
 */
static void McpsIndication(McpsIndication_t *mcpsIndication)
{
    trace_debug("McpsIndication\r\n");

    LoRaRxIndication_t rxIndication;

    if (mcpsIndication->Status != LORAMAC_EVENT_INFO_STATUS_OK)
    {
        return;
    }

    LoRaParam.Snr = (int8_t)mcpsIndication->Snr >> 2;
    LoRaParam.Rssi = mcpsIndication->Rssi;

    rxIndication.Status = mcpsIndication->Status;
    rxIndication.DownLinkFrequency = mcpsIndication->DownLinkFrequency;
    rxIndication.RxDatarate = mcpsIndication->RxDatarate;
    rxIndication.DownLinkCounter = mcpsIndication->DownLinkCounter;
    rxIndication.RxData = mcpsIndication->RxData;
    rxIndication.Rssi = mcpsIndication->Rssi;
    rxIndication.Snr = (int8_t)mcpsIndication->Snr >> 2;

    switch (mcpsIndication->McpsIndication)
    {
        case MCPS_UNCONFIRMED:
        {
            break;
        }

        case MCPS_CONFIRMED:
        {
            break;
        }

        case MCPS_PROPRIETARY:
        {
            break;
        }

        case MCPS_MULTICAST:
        {
            break;
        }

        default:
            break;
    }

    // Check Multicast
    // Check Port
    // Check Datarate
    // Check FramePending
    // Check Buffer
    // Check BufferSize
    // Check Rssi
    // Check Snr
    // Check RxSlot

    if (ComplianceTest.Running == true)
    {
        ComplianceTest.DownLinkCounter++;
    }

    if (mcpsIndication->RxData == true)
    {
        trace_info("port = %d\n\r", mcpsIndication->Port);

        switch (mcpsIndication->Port)
        {
            case 224:
                if (ComplianceTest.Running == false)
                {
                    // Check compliance test enable command (i)
                    if ((mcpsIndication->BufferSize == 4) &&
                            (mcpsIndication->Buffer[0] == 0x01) &&
                            (mcpsIndication->Buffer[1] == 0x01) &&
                            (mcpsIndication->Buffer[2] == 0x01) &&
                            (mcpsIndication->Buffer[3] == 0x01))
                    {
                        IsTxConfirmed = false;
                        AppData.Port = 224;
                        AppData.BuffSize = 2;
                        ComplianceTest.DownLinkCounter = 0;
                        ComplianceTest.LinkCheck = false;
                        ComplianceTest.DemodMargin = 0;
                        ComplianceTest.NbGateways = 0;
                        ComplianceTest.Running = true;
                        ComplianceTest.State = 1;

                        MibRequestConfirm_t mibReq;
                        mibReq.Type = MIB_ADR;
                        mibReq.Param.AdrEnable = true;
                        LoRaMacMibSetRequestConfirm(&mibReq);

                        if (LORAMAC_REGION_EU868 == LoRaParam.region || LORAMAC_REGION_EU433 == LoRaParam.region)
                        {
                            LoRaMacTestSetDutyCycleOn(false);
                        }
                    }
                }
                else
                {
                    ComplianceTest.State = mcpsIndication->Buffer[0];
                    trace_info("ComplianceTest.State = %d\n\r", ComplianceTest.State);

                    switch (ComplianceTest.State)
                    {
                        case 0: // Check compliance test disable command (ii)
                        {
                            ComplianceTest.DownLinkCounter = 0;
                            ComplianceTest.Running = false;

                            MibRequestConfirm_t mibReq;
                            mibReq.Type = MIB_ADR;
                            mibReq.Param.AdrEnable = LoRaParam.AdrEnable;
                            LoRaMacMibSetRequestConfirm(&mibReq);

                            if (LORAMAC_REGION_EU868 == LoRaParam.region || LORAMAC_REGION_EU433 == LoRaParam.region)
                            {
                                LoRaMacTestSetDutyCycleOn(LORAWAN_DUTYCYCLE_ON);
                            }
                        }
                        break;

                        case 1: // (iii, iv)
                            AppData.BuffSize = 2;
                            break;

                        case 2: // Enable confirmed messages (v)
                            IsTxConfirmed = true;
                            ComplianceTest.State = 1;
                            break;

                        case 3:  // Disable confirmed messages (vi)
                            IsTxConfirmed = false;
                            ComplianceTest.State = 1;
                            break;

                        case 4: // (vii)
                            AppData.BuffSize = mcpsIndication->BufferSize;

                            AppData.Buff[0] = 4;

                            for (uint8_t i = 1; i < AppData.BuffSize; i++)
                            {
                                AppData.Buff[i] = mcpsIndication->Buffer[i] + 1;
                            }

                            break;

                        case 5: // (viii)
                        {
                            MlmeReq_t mlmeReq;
                            mlmeReq.Type = MLME_LINK_CHECK;
                            LoRaMacMlmeRequest(&mlmeReq);
                        }
                        break;

                        case 6: // (ix)
                        {
                            MlmeReq_t mlmeReq;

                            // Disable TestMode and revert back to normal operation

                            ComplianceTest.DownLinkCounter = 0;
                            ComplianceTest.Running = false;

                            MibRequestConfirm_t mibReq;
                            mibReq.Type = MIB_ADR;
                            mibReq.Param.AdrEnable = LoRaParam.AdrEnable;
                            LoRaMacMibSetRequestConfirm(&mibReq);

                            mlmeReq.Type = MLME_JOIN;

                            mlmeReq.Req.Join.DevEui = LoRaParam.DevEui;
                            mlmeReq.Req.Join.AppEui = LoRaParam.AppEui;
                            mlmeReq.Req.Join.AppKey = LoRaParam.AppKey;
                            mlmeReq.Req.Join.NbTrials = 3;

                            LoRaMacMlmeRequest(&mlmeReq);
                            DeviceState = DEVICE_STATE_SLEEP;
                        }
                        break;

                        case 7: // (x)
                        {
                            if (mcpsIndication->BufferSize == 3)
                            {
                                MlmeReq_t mlmeReq;
                                mlmeReq.Type = MLME_TXCW;
                                mlmeReq.Req.TxCw.Timeout = (uint16_t)((mcpsIndication->Buffer[1] << 8) | mcpsIndication->Buffer[2]);
                                LoRaMacMlmeRequest(&mlmeReq);
                            }

                            ComplianceTest.State = 1;
                        }
                        break;

                        default:
                            break;
                    }
                }

                break;

            default:
                //only recieved mac command
                if ((mcpsIndication->BufferSize == 0) && (mcpsIndication->MCNeedAck == true))
                {
                    AppData.Port = mcpsIndication->Port;
                    AppData.BuffSize = mcpsIndication->BufferSize;
                    memset(AppData.Buff, 0, sizeof(AppData.Buff));

                    LoRaCallbacks->OnRxData(&AppData, &rxIndication);
                    break;
                }
                else
                {
                    AppData.Port = mcpsIndication->Port;
                    AppData.BuffSize = mcpsIndication->BufferSize;
                    memcpy(AppData.Buff, mcpsIndication->Buffer, AppData.BuffSize);

                    if (LoRaCallbacks != NULL && LoRaCallbacks->OnRxData != NULL)
                    {
                        LoRaCallbacks->OnRxData(&AppData, &rxIndication);
                    }
                }

                break;
        }
    }
    else
    {
        if(LoRaCallbacks != NULL && LoRaCallbacks->OnRxData != NULL)
        {
            LoRaCallbacks->OnRxData(&AppData, &rxIndication);
        }
    }

    if (mcpsIndication->MCNeedAck == true)
    {
        LoRaUnconfirmedPacketSend(10, NULL, 0);
        Flag_MC_Ack = true;
    }

    trace_info("rssi = %d, snr = %d\n\r", LoRaParam.Rssi, LoRaParam.Snr);
}

/*!
 * \brief   MLME-Confirm event function
 *
 * \param   [IN] MlmeConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void MlmeConfirm(MlmeConfirm_t *mlmeConfirm)
{
    trace_debug("MlmeConfirm\r\n");

    switch (mlmeConfirm->MlmeRequest)
    {
        case MLME_JOIN:

            if (mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK)
            {
                trace_info("DEVICE_STATE_JOINED\r\n");
                // Status is OK, node has joined the network
                DeviceState = DEVICE_STATE_SLEEP;

                if (!Certification)
                {
                    if (LoRaCallbacks != NULL && LoRaCallbacks->OnJoined != NULL)
                    {
                        LoRaCallbacks->OnJoined();
                    }
                }
            }
            else
            {
                // Join was not successful.
                DeviceState = DEVICE_STATE_SLEEP;

                if (!Certification)
                {
                    if (LoRaCallbacks != NULL && LoRaCallbacks->OnJoinFailed != NULL)
                    {
                        LoRaCallbacks->OnJoinFailed();
                    }
                }
            }
            break;

        case MLME_LINK_CHECK:

            if (mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK)
            {
                // Check DemodMargin
                // Check NbGateways
                if (ComplianceTest.Running == true)
                {
                    ComplianceTest.LinkCheck = true;
                    ComplianceTest.DemodMargin = mlmeConfirm->DemodMargin;
                    ComplianceTest.NbGateways = mlmeConfirm->NbGateways;
                }
            }

            break;

        default:
            break;
    }

    NextTx = true;
}


void OnCertifTimer(void *p_context)
{
    trace_debug("OnCertifTimer\r\n");
    MibRequestConfirm_t mibReq;
    LoRaStatus_t status;

    mibReq.Type = MIB_NETWORK_JOINED;
    status = (LoRaStatus_t)LoRaMacMibGetRequestConfirm(&mibReq);

    if (status == LORA_STATUS_OK)
    {
        if (mibReq.Param.IsNetworkJoined == true)
        {
            trace_debug("Certif SEND\r\n");
            PrepareTxFrame();

            NextTx = SendFrame();

            DeviceState = DEVICE_STATE_SLEEP;
        }
        else
        {
            DeviceState = DEVICE_STATE_JOIN;
        }
    }

    if (Certification && !CertificationStop)
    {
        TIMER_START(&TxcertifTimer, 5000);
    }
}


void LoRaWANCertif(void)
{
    if (!Certification)
    {
        Certification = true;
        TIMER_START(&TxcertifTimer, 5000);
        LoRaJoin(8);
    }
}

void LoRaWANCertifStop(void)
{
    if (Certification)
    {
        TIMER_STOP(&TxcertifTimer);

        if (!McpsReqRunning)
        {
            Certification = false;
        }
        else
        {
            trace_info("McpsReqRunning\r\n");
            CertificationStop = true;
        }
    }
}

/**
 *  lora Join
 */
LoRaStatus_t LoRaJoin(uint32_t trials)
{
    LoRaStatus_t status;
#if 0
    // wanghuayuan
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_NETWORK_JOINED;
    status = (LoRaStatus_t)LoRaMacMibGetRequestConfirm(&mibReq);

    if ((status == LORA_STATUS_OK) && (mibReq.Param.IsNetworkJoined == true))
    {
        /* We're already joined */
        status = LORA_STATUS_OK;
    }
#endif
    if ((LoRaParam.activation == ACTIVATION_OTAA) && (NextTx != true))
    {
        trace_info("join LORA_STATUS_BUSY\r\n");
        /* we are joining */
        status = LORA_STATUS_BUSY;
    }
    else
    {
        LoRaParam.joinNbTrials = trials;
        DeviceState = DEVICE_STATE_JOIN;
        status = LORA_STATUS_OK;
    }

    return status;
}

LoRaStatus_t LoRaReqServerTime(void)
{
    MlmeReq_t mlmeReq;
    mlmeReq.Type = MLME_DEVICE_TIME;

    if ( NextTx == true )
    {
        trace_info("LoRaMacMlmeRequest\r\n");
        LoRaMacMlmeRequest( &mlmeReq );

        LoRaConfirmedPacketSend(2, 1, NULL, NULL);
    }
    else
    {
        DeviceState = DEVICE_STATE_SLEEP;
        return LORA_STATUS_BUSY;
    }

    return LORA_STATUS_OK;
}

/**
 *  lora Send
 */
LoRaStatus_t LoRaUnconfirmedPacketSend(uint8_t port, uint8_t *pBuff, uint16_t buffSize)
{
    if (port == 0 || port > 223)
    {
        return LORA_STATUS_PARAMETER_INVALID;
    }

    if (Certification || (Flag_MC_Ack == true))
    {
        return LORA_STATUS_BUSY;
    }

    OnSendEvent();

    if (DeviceState != DEVICE_STATE_SEND)
    {
        DeviceState = DEVICE_STATE_SLEEP;
        return LORA_STATUS_NO_NETWORK_JOINED;
    }

    if (buffSize > LORAWAN_APP_DATA_BUFF_SIZE)
    {
        DeviceState = DEVICE_STATE_SLEEP;
        return LORA_STATUS_PARAMETER_INVALID;
    }

    memcpy(AppData.Buff, pBuff, buffSize);
    AppData.BuffSize = buffSize;
    AppData.Port = port;

    IsTxConfirmed = false;

    if (NextTx == true)
    {
        PrepareTxFrame();
        NextTx = SendFrame();

        if (NextTx != 0)
        {
            /*
             * Data have not been sent.
             * main root causes: duty cycles, previous send is not completed (tx not done, rx windows not completed,...)
             */
            DeviceState = DEVICE_STATE_SLEEP;
            return LORA_STATUS_BUSY;
        }
    }

    return LORA_STATUS_OK;
}

LoRaStatus_t LoRaConfirmedPacketSend(uint8_t port, uint8_t trials, uint8_t *pBuff, uint16_t buffSize)
{
    if (port == 0 || port > 223)
    {
        return LORA_STATUS_PARAMETER_INVALID;
    }

    if (Certification || (Flag_MC_Ack == true))
    {
        return LORA_STATUS_BUSY;
    }

    OnSendEvent();

    if (DeviceState != DEVICE_STATE_SEND)
    {
        DeviceState = DEVICE_STATE_SLEEP;
        return LORA_STATUS_NO_NETWORK_JOINED;
    }

    if (buffSize > LORAWAN_APP_DATA_BUFF_SIZE)
    {
        DeviceState = DEVICE_STATE_SLEEP;
        return LORA_STATUS_PARAMETER_INVALID;
    }

    if (trials > 8)
    {
        DeviceState = DEVICE_STATE_SLEEP;
        return LORA_STATUS_PARAMETER_INVALID;
    }

    memcpy(AppData.Buff, pBuff, buffSize);
    AppData.BuffSize = buffSize;
    AppData.Port = port;
    IsTxConfirmed = true;
    LoRaParam.confirmNbTrials = trials;

    if (NextTx == true)
    {
        PrepareTxFrame();
        NextTx = SendFrame();

        if (NextTx != 0)
        {
            /*
             * Data have not been sent.
             * main root causes: duty cycles, previous send is not completed (tx not done, rx windows not completed,...)
             */
            DeviceState = DEVICE_STATE_SLEEP;
            return LORA_STATUS_BUSY;
        }
    }

    return LORA_STATUS_OK;
}


#if 0
static void LoRaDefNwkParamsInit(void)
{
    trace_info("LoRaDefNwkParamsInit\r\n");
    uint8_t *p_cipher_text;
    uint8_t clear_text[128];
    uicr_param_t *p_param;
    nrf_ecb_hal_data_t m_aes_ecb;

    p_cipher_text = (uint8_t *)0x10001080;

    // make aes-128-ecb key via {chipid, chipid}
    memcpy(m_aes_ecb.key, (uint8_t *)0x10000060, 8);
    memcpy(&m_aes_ecb.key[8], (uint8_t *)0x10000060, 8);

    // decrypt with aes-128-ecb.encrypt
    for (uint32_t i = 0; i < 128;)
    {
        memcpy(m_aes_ecb.cleartext, p_cipher_text + i, 16);
        sd_ecb_block_encrypt(&m_aes_ecb);
        memcpy(clear_text + i, m_aes_ecb.ciphertext, 16);
        i += 16;
    }

    p_param = (uicr_param_t *)clear_text;

    uint16_t crc = crc16_compute(p_param->db.sn, sizeof(uicr_param_t) - 2, NULL);
    if (crc == p_param->db.crc)
    {
#if 0
        // generate a hardfault
        *((uint8_t *)0x2000c000) = 0;
#endif
        trace_info("UICR DevEui= ");
        trace_dump_i(p_param->db.deveui, sizeof(p_param->db.deveui));
        trace_info("UICR AppEui= ");
        trace_dump_i(p_param->db.appeui, sizeof(p_param->db.appeui));
        trace_info("UICR AppKey= ");
        trace_dump_i(p_param->db.appkey, sizeof(p_param->db.appkey));
        trace_info("UICR NwkSKey= ");
        trace_dump_i(p_param->db.appskey, sizeof(p_param->db.appskey));
        trace_info("UICR AppSKey= ");
        trace_dump_i(p_param->db.nwkskey, sizeof(p_param->db.nwkskey));
        trace_info("UICR DevAddr = %08X\r\n", p_param->db.devaddr);

        memcpy(LoRaParam.DevEui, p_param->db.deveui, 8);
        memcpy(LoRaParam.AppEui, p_param->db.appeui, 8);
        memcpy(LoRaParam.AppKey, p_param->db.appkey, 16);

        LoRaAppSKeySet(p_param->db.appskey);
        LoRaNwkSKeySet(p_param->db.nwkskey);
        LoRaDevAddrSet(p_param->db.devaddr);
    }
}
#endif

void LoRaRadioInit(void)
{
    LoRaMacRadioInit();
}

/**
 *  lora Init
 */
void LoRaInit(LoRaMacRegion_t region, LoRaCallback_t *callbacks)
{
    MibRequestConfirm_t mibReq;
#if 0
    LoRaDefNwkParamsInit();
#endif
    /* init the DeviceState*/
    DeviceState = DEVICE_STATE_INIT;

    /* init the main call backs*/
    LoRaCallbacks = callbacks;

    LoRaMacPrimitives.MacMcpsConfirm = McpsConfirm;
    LoRaMacPrimitives.MacMcpsIndication = McpsIndication;
    LoRaMacPrimitives.MacMlmeConfirm = MlmeConfirm;

    if (LoRaCallbacks != NULL && LoRaCallbacks->BoardGetBatteryLevel != NULL)
    {
        LoRaMacCallbacks.GetBatteryLevel = LoRaCallbacks->BoardGetBatteryLevel;
    }

    if (LoRaCallbacks != NULL && LoRaCallbacks->OnCommandCallback != NULL)
    {
        LoRaMacCallbacks.MacCommandCallback = LoRaCallbacks->OnCommandCallback;
    }

    trace_verbose("LoRaMacInitialization\r\n");
    LoRaParam.region = region;
    LoRaMacInitialization(&LoRaMacPrimitives, &LoRaMacCallbacks, LoRaParam.region);

    mibReq.Type = MIB_CHANNELS_DEFAULT_TX_POWER;
    LoRaMacMibGetRequestConfirm(&mibReq);
    LoRaParam.TxPower = mibReq.Param.ChannelsDefaultTxPower;

    mibReq.Type = MIB_CHANNELS_DEFAULT_DATARATE;
    LoRaMacMibGetRequestConfirm(&mibReq);
    LoRaParam.TxDatarate = mibReq.Param.ChannelsDefaultDatarate;

    if (LORAMAC_REGION_EU868 == LoRaParam.region)
    {
#if( USE_DEFAULT_CHANNEL_LINEUP == 1 )
        LoRaMacChannelAdd(3, (ChannelParams_t)EU868_LC4);
        LoRaMacChannelAdd(4, (ChannelParams_t)EU868_LC5);
        LoRaMacChannelAdd(5, (ChannelParams_t)EU868_LC6);
        LoRaMacChannelAdd(6, (ChannelParams_t)EU868_LC7);
        LoRaMacChannelAdd(7, (ChannelParams_t)EU868_LC8);
#endif
        mibReq.Type = MIB_RX2_CHANNEL;
        mibReq.Param.Rx2Channel = (Rx2ChannelParams_t)
        {
            EU868_RX_WND_2_FREQ, EU868_RX_WND_2_DR
        };
        LoRaMacMibSetRequestConfirm(&mibReq);
    }
    else if (LORAMAC_REGION_EU433 == LoRaParam.region)
    {
#if( USE_DEFAULT_CHANNEL_LINEUP == 1 )
        LoRaMacChannelAdd(3, (ChannelParams_t)EU433_LC4);
        LoRaMacChannelAdd(4, (ChannelParams_t)EU433_LC5);
        LoRaMacChannelAdd(5, (ChannelParams_t)EU433_LC6);
        LoRaMacChannelAdd(6, (ChannelParams_t)EU433_LC7);
        LoRaMacChannelAdd(7, (ChannelParams_t)EU433_LC8);
#endif
        mibReq.Type = MIB_RX2_CHANNEL;
        mibReq.Param.Rx2Channel = (Rx2ChannelParams_t)
        {
            EU433_RX_WND_2_FREQ, EU433_RX_WND_2_DR
        };
        LoRaMacMibSetRequestConfirm(&mibReq);
    }
    else if (LORAMAC_REGION_AS923 == LoRaParam.region)
    {
#if( USE_DEFAULT_CHANNEL_LINEUP == 1 )
        LoRaMacChannelAdd(2, (ChannelParams_t)AS923_LC3);
        LoRaMacChannelAdd(3, (ChannelParams_t)AS923_LC4);
        LoRaMacChannelAdd(4, (ChannelParams_t)AS923_LC5);
        LoRaMacChannelAdd(5, (ChannelParams_t)AS923_LC6);
        LoRaMacChannelAdd(6, (ChannelParams_t)AS923_LC7);
        LoRaMacChannelAdd(7, (ChannelParams_t)AS923_LC8);
#endif
        mibReq.Type = MIB_RX2_CHANNEL;
        mibReq.Param.Rx2Channel = (Rx2ChannelParams_t)
        {
            AS923_RX_WND_2_FREQ, AS923_RX_WND_2_DR
        };
        LoRaMacMibSetRequestConfirm(&mibReq);
    }
    else if (LORAMAC_REGION_IN865 == LoRaParam.region)
    {
        mibReq.Type = MIB_RX2_CHANNEL;
        mibReq.Param.Rx2Channel = (Rx2ChannelParams_t)
        {
            IN865_RX_WND_2_FREQ, IN865_RX_WND_2_DR
        };
        LoRaMacMibSetRequestConfirm(&mibReq);
    }
    else if (LORAMAC_REGION_CN470 == LoRaParam.region)
    {
        mibReq.Type = MIB_RX2_CHANNEL;
        mibReq.Param.Rx2Channel = (Rx2ChannelParams_t)
        {
            CN470_RX_WND_2_FREQ, CN470_RX_WND_2_DR
        };
        LoRaMacMibSetRequestConfirm(&mibReq);
        uint16_t channel_mask[6] =
        {
            0x0000, // channels 0 ~ 15
            0x0000, // channels 16 ~ 31
            0x0000, // channels 32 ~ 47
            0x0000, // channels 48 ~ 63
            0x0000, // channels 64 ~ 79
            0x00ff  // channels 80 ~ 95
        };
        LoRaChannelMaskSet(channel_mask);
    }
    else if (LORAMAC_REGION_CN779 == LoRaParam.region)
    {
        mibReq.Type = MIB_RX2_CHANNEL;
        mibReq.Param.Rx2Channel = (Rx2ChannelParams_t)
        {
            CN779_RX_WND_2_FREQ, CN779_RX_WND_2_DR
        };
        LoRaMacMibSetRequestConfirm(&mibReq);
    }
    else if (LORAMAC_REGION_SE800 == LoRaParam.region)
    {
        trace_debug("region SE800");
        mibReq.Type = MIB_RX2_CHANNEL;
        mibReq.Param.Rx2Channel = (Rx2ChannelParams_t)
        {
            SE800_RX_WND_2_FREQ, SE800_RX_WND_2_DR
        };
        LoRaMacMibSetRequestConfirm(&mibReq);
    }

    LoRaMacTestSetDutyCycleOn(LoRaParam.duty_cycle);

    TIMER_CREATE(&TxcertifTimer, APP_TIMER_MODE_SINGLE_SHOT, OnCertifTimer);

    if (LoRaParam.activation == ACTIVATION_ABP)
    {
        mibReq.Type = MIB_NET_ID;
        mibReq.Param.NetID = 0;
        LoRaMacMibSetRequestConfirm(&mibReq);

        mibReq.Type = MIB_NETWORK_JOINED;
        mibReq.Param.IsNetworkJoined = true;
        LoRaMacMibSetRequestConfirm(&mibReq);
    }

    DeviceState = DEVICE_STATE_SLEEP;
}

/**
 *  lora class A state machine
 */
void LoRaFSM(void)
{
    switch (DeviceState)
    {
        case DEVICE_STATE_INIT:
        {
            trace_verbose("DEVICE_STATE_INIT\r\n");
            break;
        }

        case DEVICE_STATE_JOIN:
        {
            trace_verbose("DEVICE_STATE_JOIN\r\n");

            if (LoRaParam.activation == ACTIVATION_OTAA)
            {
                MlmeReq_t mlmeReq;

                mlmeReq.Type = MLME_JOIN;
                mlmeReq.Req.Join.DevEui = LoRaParam.DevEui;
                mlmeReq.Req.Join.AppEui = LoRaParam.AppEui;
                mlmeReq.Req.Join.AppKey = LoRaParam.AppKey;
                mlmeReq.Req.Join.NbTrials = LoRaParam.joinNbTrials;

                if (NextTx == true)
                {
                    LoRaMacMlmeRequest(&mlmeReq);
                }

                DeviceState = DEVICE_STATE_SLEEP;
            }
            else
            {
                if (!Certification)
                {
                    if (LoRaCallbacks != NULL && LoRaCallbacks->OnJoined != NULL)
                    {
                        LoRaCallbacks->OnJoined();
                    }
                }

                DeviceState = DEVICE_STATE_SLEEP;
            }

            break;
        }

        case DEVICE_STATE_SEND:
        {
            trace_debug("DEVICE_STATE_SEND\r\n");
            DeviceState = DEVICE_STATE_SLEEP;
            break;
        }

        case DEVICE_STATE_SLEEP:
        {
            // Wake up through events
            break;
        }

        default:
        {
            DeviceState = DEVICE_STATE_INIT;
            break;
        }
    }
}


DeviceState_t LoRaDeviceStateGet(void)
{
    return DeviceState;
}

/******************************************************************************/

uint8_t *LoRaDevEUIGet(void)
{
    return LoRaParam.DevEui;
}

LoRaStatus_t LoRaDevEUISet(uint8_t eui[8])
{
#if 0
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_NETWORK_JOINED;
    LoRaMacMibGetRequestConfirm(&mibReq);

    if (mibReq.Param.IsNetworkJoined)
    {
        return LORA_STATUS_STATE_INVALID;
    }
#endif

    memcpy(LoRaParam.DevEui, eui, sizeof(LoRaParam.DevEui));

    return LORA_STATUS_OK;
}

uint8_t *LoRaAppEUIGet(void)
{
    return LoRaParam.AppEui;
}

LoRaStatus_t LoRaAppEUISet(uint8_t eui[8])
{
#if 0
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_NETWORK_JOINED;
    LoRaMacMibGetRequestConfirm(&mibReq);

    if (mibReq.Param.IsNetworkJoined)
    {
        return LORA_STATUS_STATE_INVALID;
    }
#endif

    memcpy(LoRaParam.AppEui, eui, sizeof(LoRaParam.AppEui));

    return LORA_STATUS_OK;
}

uint8_t *LoRaAppKeyGet(void)
{
    return LoRaParam.AppKey;
}

LoRaStatus_t LoRaAppKeySet(uint8_t appkey[16])
{
#if 0
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_NETWORK_JOINED;
    LoRaMacMibGetRequestConfirm(&mibReq);

    if (mibReq.Param.IsNetworkJoined)
    {
        return LORA_STATUS_STATE_INVALID;
    }
#endif

    memcpy(LoRaParam.AppKey, appkey, sizeof(LoRaParam.AppKey));

    return LORA_STATUS_OK;
}

uint8_t *LoRaAppSKeyGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_APP_SKEY;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.AppSKey;
}

LoRaStatus_t LoRaAppSKeySet(uint8_t key[16])
{
    LoRaStatus_t status = LORA_STATUS_OK;
    MibRequestConfirm_t mibReq;

#if 0
    bool joined = false;

    mibReq.Type = MIB_NETWORK_JOINED;
    LoRaMacMibGetRequestConfirm(&mibReq);
    joined = mibReq.Param.IsNetworkJoined;

    if (joined)
    {
        return LORA_STATUS_STATE_INVALID;
    }
#endif

    mibReq.Type = MIB_APP_SKEY;
    mibReq.Param.AppSKey = key;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

uint8_t *LoRaNwkSKeyGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_NWK_SKEY;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.NwkSKey;
}

LoRaStatus_t LoRaNwkSKeySet(uint8_t key[16])
{
    LoRaStatus_t status = LORA_STATUS_OK;
    MibRequestConfirm_t mibReq;

#if 0
    bool joined = false;

    mibReq.Type = MIB_NETWORK_JOINED;
    LoRaMacMibGetRequestConfirm(&mibReq);
    joined = mibReq.Param.IsNetworkJoined;

    if (joined)
    {
        return LORA_STATUS_STATE_INVALID;
    }
#endif

    mibReq.Type = MIB_NWK_SKEY;
    mibReq.Param.NwkSKey = key;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

uint32_t LoRaDevAddrGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_DEV_ADDR;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.DevAddr;
}

LoRaStatus_t LoRaDevAddrSet(uint32_t addr)
{
    LoRaStatus_t status = LORA_STATUS_OK;
    MibRequestConfirm_t mibReq;

#if 0
    bool joined = false;

    mibReq.Type = MIB_NETWORK_JOINED;
    LoRaMacMibGetRequestConfirm(&mibReq);
    joined = mibReq.Param.IsNetworkJoined;

    if (joined)
    {
        return LORA_STATUS_STATE_INVALID;
    }
#endif

    mibReq.Type = MIB_DEV_ADDR;
    mibReq.Param.DevAddr = addr;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

LoRaStatus_t LoRaDeviceClassSet(DeviceClass_t device_class)
{
    MibRequestConfirm_t mibReq;
    LoRaStatus_t status = LORA_STATUS_OK;

    mibReq.Type = MIB_DEVICE_CLASS;
    mibReq.Param.Class = device_class;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

DeviceClass_t LoRaDeviceClassGet(void)
{
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_DEVICE_CLASS;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.Class;
}

LoRaStatus_t LoRaActivationSet(ActivationType_t activation)
{
    MibRequestConfirm_t mibReqSet;
    LoRaStatus_t status = LORA_STATUS_OK;

    if (activation > ACTIVATION_OTAA)
    {
        return LORA_STATUS_PARAMETER_INVALID;
    }

    if (LoRaParam.activation != activation)
    {
        mibReqSet.Type = MIB_NETWORK_JOINED;

        if (activation == ACTIVATION_OTAA)
        {
            mibReqSet.Param.IsNetworkJoined = false;
        }
        else
        {
            mibReqSet.Param.IsNetworkJoined = true;
        }

        status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReqSet);
        if (status == LORA_STATUS_OK)
        {
            LoRaParam.activation = activation;
        }
    }

    return status;
}

ActivationType_t LoRaActivationGet(void)
{
    return LoRaParam.activation;
}

LoRaStatus_t LoRaTxDatarateSet(uint32_t datarate)
{
    MibRequestConfirm_t mibReq;
    LoRaStatus_t status = LORA_STATUS_OK;

    if (!LoRaTxDatarateVerify(datarate))
    {
        return LORA_STATUS_PARAMETER_INVALID;
    }

    LoRaParam.TxDatarate = LoRaTxDatarateGet();

    if (LoRaParam.TxDatarate != datarate)
    {
        LoRaParam.TxDatarate = datarate;
//        LoRaParam.TxDatarateUpdate = true;

        mibReq.Type = MIB_CHANNELS_DATARATE;
        mibReq.Param.ChannelsDatarate = datarate;
        status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

        mibReq.Type = MIB_CHANNELS_DEFAULT_DATARATE;
        mibReq.Param.ChannelsDefaultDatarate = datarate;
        LoRaMacMibSetRequestConfirm(&mibReq);
    }

    return status;
}

uint32_t LoRaTxDatarateGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_CHANNELS_DATARATE;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.ChannelsDatarate;
}

uint32_t LoRaCurTxDatarateGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_CHANNELS_DATARATE;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.ChannelsDatarate;
}

uint32_t LoRaNbReqGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_CHANNELS_NB_REP;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.ChannelNbRep;
}

LoRaStatus_t LoRaNbReqSet(uint32_t nbReq)
{
    MibRequestConfirm_t mibReq;
    LoRaStatus_t status = LORA_STATUS_OK;

    mibReq.Type = MIB_CHANNELS_NB_REP;
    mibReq.Param.ChannelNbRep = nbReq;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_CHANNELS_DEFAULT_NB_REP;
    mibReq.Param.ChannelDefaultNbRep = nbReq;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

LoRaStatus_t LoRaTxPowerSet(uint8_t level)
{
    MibRequestConfirm_t mibReq;
    LoRaStatus_t status = LORA_STATUS_OK;

    if (!LoRaTxPowerVerify(level))
    {
        return LORA_STATUS_PARAMETER_INVALID;
    }

    LoRaParam.TxPower = LoRaTxPowerGet();

    if (LoRaParam.TxPower != level)
    {
        LoRaParam.TxPower = level;
//       LoRaParam.TxPowerUpdate = true;

        mibReq.Type = MIB_CHANNELS_TX_POWER;
        mibReq.Param.ChannelsTxPower = LoRaParam.TxPower;
        status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

        mibReq.Type = MIB_CHANNELS_DEFAULT_TX_POWER;
        mibReq.Param.ChannelsDefaultTxPower = LoRaParam.TxPower;
        status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);
    }

    return status;
}

uint32_t LoRaTxPowerGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_CHANNELS_TX_POWER;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.ChannelsTxPower;
}

LoRaStatus_t LoRaAdrStatusSet(bool enable)
{
    if (LoRaParam.AdrEnable != enable)
    {
        LoRaParam.AdrEnable = enable;
        LoRaParam.AdrEnableUpdate = true;
    }
    return LORA_STATUS_OK;
}

bool LoRaAdrStatusGet(void)
{
    return LoRaParam.AdrEnable;
}

LoRaStatus_t LoRaDutyCycleStatusSet(bool duty_cycle)
{
    LoRaParam.duty_cycle = duty_cycle;
    LoRaMacTestSetDutyCycleOn(duty_cycle);

    return LORA_STATUS_OK;
}

bool LoRaDutyCycleStatusGet(void)
{
    return LoRaParam.duty_cycle;
}

uint32_t LoRaRx1DatarateOffsetGet(void)
{
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_CHANNELS_DATARATE_OFFSET;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.ChannelsDatarateOffset;
}

LoRaStatus_t LoRaRx1DatarateOffsetSet(uint32_t datarateOffset)
{
    LoRaStatus_t status = LORA_STATUS_OK;
    MibRequestConfirm_t mibReq;

    if (!LoRaRx1DatarateOffsetVerify(datarateOffset))
    {
        return LORA_STATUS_PARAMETER_INVALID;
    }

    mibReq.Type = MIB_CHANNELS_DATARATE_OFFSET;
    mibReq.Param.ChannelsDatarateOffset = datarateOffset;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_CHANNELS_DEFAULT_DATARATE_OFFSET;
    mibReq.Param.ChannelsDefaultDatarateOffset = datarateOffset;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

LoRaStatus_t LoRaRx2FrequencySet(uint32_t freq)
{
    LoRaStatus_t status = LORA_STATUS_OK;
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_RX2_CHANNEL;
    LoRaMacMibGetRequestConfirm(&mibReq);

    mibReq.Param.Rx2Channel.Frequency = freq;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_RX2_CHANNEL_DEFAULT;
    mibReq.Param.Rx2ChannelDefault.Frequency = freq;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

uint32_t LoRaRx2FrequencyGet(void)
{
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_RX2_CHANNEL;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.Rx2Channel.Frequency;
}

LoRaStatus_t LoRaRx2DatarateSet(uint32_t datarate)
{
    LoRaStatus_t status = LORA_STATUS_OK;
    MibRequestConfirm_t mibReq;

    if (!LoRaRxDatarateVerify(datarate))
    {
        return LORA_STATUS_PARAMETER_INVALID;
    }

    mibReq.Type = MIB_RX2_CHANNEL;
    LoRaMacMibGetRequestConfirm(&mibReq);

    mibReq.Param.Rx2Channel.Datarate = datarate;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_RX2_CHANNEL_DEFAULT;
    mibReq.Param.Rx2ChannelDefault.Datarate = datarate;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

uint32_t LoRaRx2DatarateGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_RX2_CHANNEL;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.Rx2Channel.Datarate;
}

LoRaStatus_t LoRaRx1DelaySet(uint32_t delay)
{
    LoRaStatus_t status = LORA_STATUS_OK;
    MibRequestConfirm_t mibReq;

    if (delay < 1000 || delay > 15000)
    {
        return LORA_STATUS_PARAMETER_INVALID;
    }

    mibReq.Type = MIB_RECEIVE_DELAY_1;
    mibReq.Param.ReceiveDelay1 = delay;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_DEFAULT_RECEIVE_DELAY_1;
    mibReq.Param.DefaultReceiveDelay1 = delay;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    if (LORA_STATUS_OK != status)
    {
        return status;
    }

    mibReq.Type = MIB_RECEIVE_DELAY_2;
    mibReq.Param.ReceiveDelay2 = delay + 1000;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_DEFAULT_RECEIVE_DELAY_2;
    mibReq.Param.DefaultReceiveDelay2 = delay + 1000;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

uint32_t LoRaRx1DelayGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_RECEIVE_DELAY_1;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.ReceiveDelay1;
}

uint32_t LoRaRx2DelayGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_RECEIVE_DELAY_2;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.ReceiveDelay2;
}

LoRaStatus_t LoRaJoinRx1DelaySet(uint32_t delay)
{
    LoRaStatus_t status = LORA_STATUS_OK;
    MibRequestConfirm_t mibReq;

    if (delay < 1000 || delay > 15000)
    {
        return LORA_STATUS_PARAMETER_INVALID;
    }

    mibReq.Type = MIB_JOIN_ACCEPT_DELAY_1;
    mibReq.Param.JoinAcceptDelay1 = delay;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_JOIN_DEFAULT_ACCEPT_DELAY_1;
    mibReq.Param.JoinDefaultAcceptDelay1 = delay;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    if (LORA_STATUS_OK != status)
    {
        return status;
    }

    mibReq.Type = MIB_JOIN_ACCEPT_DELAY_2;
    mibReq.Param.JoinAcceptDelay2 = delay + 1000;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_JOIN_DEFAULT_ACCEPT_DELAY_2;
    mibReq.Param.JoinDefaultAcceptDelay2 = delay + 1000;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

uint32_t LoRaJoinRx1DelayGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_JOIN_ACCEPT_DELAY_1;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.JoinAcceptDelay1;
}

uint32_t LoRaJoinRx2DelayGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_JOIN_ACCEPT_DELAY_2;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.JoinAcceptDelay2;
}

LoRaStatus_t LoRaUplinkCounterSet(uint32_t count)
{
    LoRaStatus_t status = LORA_STATUS_OK;
    MibRequestConfirm_t mibReq;

    if (count == 0)
    {
        return LORA_STATUS_PARAMETER_INVALID;
    }

    mibReq.Type = MIB_UPLINK_COUNTER;
    mibReq.Param.UpLinkCounter = count;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

uint32_t LoRaUplinkCounterGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_UPLINK_COUNTER;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.UpLinkCounter;
}

LoRaStatus_t LoRaDownlinkCounterSet(uint32_t count)
{
    LoRaStatus_t status = LORA_STATUS_OK;
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_DOWNLINK_COUNTER;
    mibReq.Param.DownLinkCounter = count;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

uint32_t LoRaDownlinkCounterGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_DOWNLINK_COUNTER;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.DownLinkCounter;
}

bool LoRaNetworkJoinStatusGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_NETWORK_JOINED;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.IsNetworkJoined;
}

LoRaStatus_t LoRaNetworkJoinStatusSet(bool joinStatus)
{
    LoRaStatus_t status = LORA_STATUS_OK;
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_NETWORK_JOINED;
    mibReq.Param.IsNetworkJoined = joinStatus;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

LoRaStatus_t LoRaChannelMaskSet(uint16_t *pMask)
{
    LoRaStatus_t status = LORA_STATUS_OK;
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_CHANNELS_MASK;
    mibReq.Param.ChannelsMask = pMask;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);
    if (status != LORA_STATUS_OK)
    {
        return status;
    }

    mibReq.Type = MIB_CHANNELS_DEFAULT_MASK;
    mibReq.Param.ChannelsDefaultMask = pMask;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

uint16_t *LoRaChannelMaskGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_CHANNELS_MASK;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.ChannelsMask;
}

uint32_t LoRaMaxNbChannelsGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_MAX_NB_CHANNELS;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.MaxNbChannels;
}

ChannelParams_t *LoRaChannelsGet(void)
{
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_CHANNELS;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.ChannelList;
}

uint64_t LoRaLastTimeStampGet(void)
{
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_LAST_TIME_STRAMP;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.LastTimeStamp;
}

LoRaStatus_t LoRaChannelAdd(uint32_t id, ChannelParams_t param)
{
    LoRaStatus_t status;
    status = (LoRaStatus_t)LoRaMacChannelAdd(id, param);
    return status;
}

LoRaStatus_t LoRaChannelRemove(uint32_t id)
{
    LoRaStatus_t status;
    status = (LoRaStatus_t)LoRaMacChannelRemove(id);
    return status;
}

LoRaStatus_t LoRaMaxEripSet(uint32_t eirp)
{
    LoRaStatus_t status = LORA_STATUS_OK;
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_MAX_EIRP;
    mibReq.Param.MaxEirp = eirp;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_DEFAULT_MAX_EIRP;
    mibReq.Param.DefaultMaxEirp = eirp;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

uint32_t LoRaMaxEripGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_MAX_EIRP;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.MaxEirp;
}

int32_t LoRaSNRGet(void)
{
    return LoRaParam.Snr;
}

int32_t LoRaRSSIGet(void)
{
    return LoRaParam.Rssi;
}

bool LoRaIsAckReceived(void)
{
    if (LoRaParam.McpsConfirm == NULL)
    {
        return false;
    }
    else
    {
        return LoRaParam.McpsConfirm->AckReceived;
    }
}

LoRaStatus_t LoRaLbtStatusSet(bool enable)
{
    LoRaStatus_t status = LORA_STATUS_OK;
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_LBT_ENABLE;
    mibReq.Param.LbtEnable = enable;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

bool LoRaLbtStatusGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_LBT_ENABLE;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.LbtEnable;
}

LoRaStatus_t LoRaLbtThresholdSet(int32_t threshold)
{
    LoRaStatus_t status = LORA_STATUS_OK;
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_LBT_THRESHOLD;
    mibReq.Param.LbtThreshold = threshold;
    status = (LoRaStatus_t)LoRaMacMibSetRequestConfirm(&mibReq);

    return status;
}

int32_t LoRaLbtThresholdGet(void)
{
    MibRequestConfirm_t mibReq;

    mibReq.Type = MIB_LBT_THRESHOLD;
    LoRaMacMibGetRequestConfirm(&mibReq);

    return mibReq.Param.LbtThreshold;
}

/******************************************************************************/
int LoRaTxPowerLevelGet(int8_t txp)
{
    switch (LoRaParam.region)
    {
        case LORAMAC_REGION_EU433:
            for (size_t i = 0; i <= EU433_MIN_TX_POWER; i++)
            {
                if (txp == (EU433_DEFAULT_MAX_EIRP - i * 2))
                {
                    return i;
                }
            }

            break;

        case LORAMAC_REGION_EU868:
            for (size_t i = 0; i <= EU868_MIN_TX_POWER; i++)
            {
                if (txp == (EU868_DEFAULT_MAX_EIRP - i * 2))
                {
                    return i;
                }
            }

            break;

        case LORAMAC_REGION_US915_HYBRID:
            for (size_t i = 0; i <= US915_HYBRID_MIN_TX_POWER; i++)
            {
                if (txp == (US915_HYBRID_DEFAULT_MAX_EIRP - i * 2))
                {
                    return i;
                }
            }

            break;

        case LORAMAC_REGION_AU915_HYBRID:
            for (size_t i = 0; i <= AU915_HYBRID_MIN_TX_POWER; i++)
            {
                if (txp == TxPowersAU915_Hybrid[i])
                {
                    return i;
                }
            }

            break;

        case LORAMAC_REGION_AS923:
            for (size_t i = 0; i <= AS923_MIN_TX_POWER; i++)
            {
                if (txp == (AS923_DEFAULT_MAX_EIRP - i * 2))
                {
                    return i;
                }
            }

            break;

        case LORAMAC_REGION_IN865:
            for (size_t i = 0; i <= IN865_MIN_TX_POWER; i++)
            {
                if (txp == (IN865_DEFAULT_MAX_EIRP - i * 2))
                {
                    return i;
                }
            }

            break;

        case LORAMAC_REGION_CN470:
            for (size_t i = 0; i <= CN470_MIN_TX_POWER; i++)
            {
                if (txp == (CN470_DEFAULT_MAX_EIRP - i * 2))
                {
                    return i;
                }
            }
            break;

        case LORAMAC_REGION_CN779:
            for (size_t i = 0; i <= CN779_MIN_TX_POWER; i++)
            {
                if (txp == (CN779_DEFAULT_MAX_EIRP - i * 2))
                {
                    return i;
                }
            }
            break;
        case LORAMAC_REGION_SE800:
            for (size_t i = 0; i <= SE800_MIN_TX_POWER; i++)
            {
                if (txp == (SE800_DEFAULT_MAX_EIRP - i * 2))
                {
                    return i;
                }
            }

            break;

        default:
            break;
    }

    return -1;
}

bool LoRaTxPowerVerify(uint8_t level)
{
    uint8_t min_tx_power = 0;
    switch (LoRaParam.region)
    {
        case LORAMAC_REGION_EU433:
            min_tx_power = EU433_MIN_TX_POWER;
            break;

        case LORAMAC_REGION_EU868:
            min_tx_power = EU868_MIN_TX_POWER;
            break;

        case LORAMAC_REGION_US915_HYBRID:
            min_tx_power = US915_HYBRID_MIN_TX_POWER;
            break;

        case LORAMAC_REGION_AU915_HYBRID:
            min_tx_power = AU915_HYBRID_MIN_TX_POWER;
            break;

        case LORAMAC_REGION_AS923:
            min_tx_power = AS923_MIN_TX_POWER;
            break;

        case LORAMAC_REGION_IN865:
            min_tx_power = IN865_MIN_TX_POWER;
            break;

        case LORAMAC_REGION_CN470:
            min_tx_power = CN470_MIN_TX_POWER;
            break;

        case LORAMAC_REGION_CN779:
            min_tx_power = CN779_MIN_TX_POWER;
            break;
        case LORAMAC_REGION_SE800:
            min_tx_power = SE800_MIN_TX_POWER;
            break;

        default:
            return false;
    }

    if (level > min_tx_power)
    {
        return false;
    }

    return true;
}

bool LoRaTxDatarateVerify(uint8_t dr)
{
    uint8_t dr_max;
    uint8_t dr_min;

    switch (LoRaParam.region)
    {
        case LORAMAC_REGION_EU868:
            dr_max = EU868_TX_MAX_DATARATE;
            dr_min = EU868_TX_MIN_DATARATE;
            break;

        case LORAMAC_REGION_EU433:
            dr_max = EU433_TX_MAX_DATARATE;
            dr_min = EU433_TX_MIN_DATARATE;
            break;

        case LORAMAC_REGION_US915_HYBRID:
            dr_max = US915_HYBRID_TX_MAX_DATARATE;
            dr_min = US915_HYBRID_TX_MIN_DATARATE;
            break;

        case LORAMAC_REGION_AU915_HYBRID:
            dr_max = AU915_HYBRID_TX_MAX_DATARATE;
            dr_min = AU915_HYBRID_TX_MIN_DATARATE;
            break;

        case LORAMAC_REGION_AS923:
            dr_max = AS923_TX_MAX_DATARATE;
            dr_min = AS923_TX_MIN_DATARATE;
            break;

        case LORAMAC_REGION_IN865:
            dr_max = IN865_TX_MAX_DATARATE;
            dr_min = IN865_TX_MIN_DATARATE;
            break;

        case LORAMAC_REGION_CN470:
            dr_max = CN470_TX_MAX_DATARATE;
            dr_min = CN470_TX_MIN_DATARATE;
            break;

        case LORAMAC_REGION_CN779:
            dr_max = CN779_TX_MAX_DATARATE;
            dr_min = CN779_TX_MIN_DATARATE;
            break;

        case LORAMAC_REGION_SE800:
            dr_max = SE800_TX_MAX_DATARATE;
            dr_min = SE800_TX_MIN_DATARATE;
            break;

        default:
            return false;
    }

    if (dr > dr_max || dr < dr_min)
    {
        return false;
    }

    return true;
}

bool LoRaRxDatarateVerify(uint8_t dr)
{
    uint8_t dr_max;
    uint8_t dr_min;

    switch (LoRaParam.region)
    {
        case LORAMAC_REGION_EU868:
            dr_max = EU868_RX_MAX_DATARATE;
            dr_min = EU868_RX_MIN_DATARATE;
            break;

        case LORAMAC_REGION_EU433:
            dr_max = EU433_RX_MAX_DATARATE;
            dr_min = EU433_RX_MIN_DATARATE;
            break;

        case LORAMAC_REGION_US915_HYBRID:
            dr_max = US915_HYBRID_RX_MAX_DATARATE;
            dr_min = US915_HYBRID_RX_MIN_DATARATE;
            break;

        case LORAMAC_REGION_AU915_HYBRID:
            dr_max = AU915_HYBRID_RX_MAX_DATARATE;
            dr_min = AU915_HYBRID_RX_MIN_DATARATE;
            break;

        case LORAMAC_REGION_AS923:
            dr_max = AS923_RX_MAX_DATARATE;
            dr_min = AS923_RX_MIN_DATARATE;
            break;

        case LORAMAC_REGION_IN865:
            dr_max = IN865_RX_MAX_DATARATE;
            dr_min = IN865_RX_MIN_DATARATE;
            break;

        case LORAMAC_REGION_CN470:
            dr_max = CN470_RX_MAX_DATARATE;
            dr_min = CN470_RX_MIN_DATARATE;
            break;

        case LORAMAC_REGION_CN779:
            dr_max = CN779_RX_MAX_DATARATE;
            dr_min = CN779_RX_MIN_DATARATE;
            break;

        case LORAMAC_REGION_SE800:
            dr_max = SE800_RX_MAX_DATARATE;
            dr_min = SE800_RX_MIN_DATARATE;
            break;

        default:
            return false;
    }

    if (dr > dr_max || dr < dr_min)
    {
        return false;
    }

    return true;
}

bool LoRaRx1DatarateOffsetVerify(uint8_t drOffset)
{
    uint8_t drOffset_max;
    uint8_t drOffset_min;

    switch (LoRaParam.region)
    {
        case LORAMAC_REGION_EU868:
            drOffset_max = EU868_MAX_RX1_DR_OFFSET;
            drOffset_min = EU868_MIN_RX1_DR_OFFSET;
            break;

        case LORAMAC_REGION_EU433:
            drOffset_max = EU433_MAX_RX1_DR_OFFSET;
            drOffset_min = EU433_MIN_RX1_DR_OFFSET;
            break;

        case LORAMAC_REGION_US915_HYBRID:
            drOffset_max = US915_HYBRID_MAX_RX1_DR_OFFSET;
            drOffset_min = US915_HYBRID_MIN_RX1_DR_OFFSET;
            break;

        case LORAMAC_REGION_AU915_HYBRID:
            drOffset_max = AU915_HYBRID_MAX_RX1_DR_OFFSET;
            drOffset_min = AU915_HYBRID_MIN_RX1_DR_OFFSET;
            break;

        case LORAMAC_REGION_AS923:
            drOffset_max = AS923_MAX_RX1_DR_OFFSET;
            drOffset_min = AS923_MIN_RX1_DR_OFFSET;
            break;

        case LORAMAC_REGION_IN865:
            drOffset_max = IN865_MAX_RX1_DR_OFFSET;
            drOffset_min = IN865_MIN_RX1_DR_OFFSET;
            break;

        case LORAMAC_REGION_CN470:
            drOffset_max = CN470_MAX_RX1_DR_OFFSET;
            drOffset_min = CN470_MIN_RX1_DR_OFFSET;
            break;

        case LORAMAC_REGION_CN779:
            drOffset_max = CN779_MAX_RX1_DR_OFFSET;
            drOffset_min = CN779_MIN_RX1_DR_OFFSET;
            break;

        case LORAMAC_REGION_SE800:
            drOffset_max = SE800_MAX_RX1_DR_OFFSET;
            drOffset_min = SE800_MIN_RX1_DR_OFFSET;
            break;

        default:
            return false;
    }

    if (drOffset > drOffset_max || drOffset < drOffset_min)
    {
        return false;
    }

    return true;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

