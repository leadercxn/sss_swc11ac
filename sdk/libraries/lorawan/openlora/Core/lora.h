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
  * @file    lora.h
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
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __LORA_MAIN_H__
#define __LORA_MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

/*!
 * LoRa State Machine states
 */
typedef enum eActivationType
{
  /*!
   * @brief
   */
  ACTIVATION_ABP,
  /*!
   * @brief
   */
  ACTIVATION_OTAA
} ActivationType_t;

typedef enum ePacketType
{
  /*!
   * @brief
   */
  LORA_PACKET_UNCONFIRMED,
  /*!
   * @brief
   */
  LORA_PACKET_CONFIRMED
} LoRaPacketType_t;

/*!
 * Application Data structure
 */
typedef struct
{
  /*point to the LoRa App data buffer*/
  uint8_t* Buff;
  /*LoRa App data buffer size*/
  uint8_t BuffSize;
  /*Port on which the LoRa App is data is sent/ received*/
  uint8_t Port;

} LoRaAppData_t;

/*!
 * Command structure
 */
typedef enum eLoRaCommand
{
  /*!
   * LinkADRReq
   */
  LINK_ADR_REQ             = 0x03,
  /*!
   * DutyCycleReq
   */
  DUTY_CYCLE_REQ           = 0x04,
  /*!
   * RXParamSetupReq
   */
  RX_PARAM_SETUP_REQ       = 0x05,
  /*!
   * DevStatusReq
   */
  DEV_STATUS_REQ           = 0x06,
  /*!
   * NewChannelReq
   */
  NEW_CHANNEL_REQ          = 0x07,
  /*!
   * RXTimingSetupReq
   */
  RX_TIMING_SETUP_REQ      = 0x08,
  /*!
   * NewChannelReq
   */
  TX_PARAM_SETUP_REQ       = 0x09,
  /*!
   * DlChannelReq
   */
  DL_CHANNEL_REQ           = 0x0A,
  /*!
   * DeviceTimeReq
   */
  DEVICE_TIME_REQ          = 0x0D,

} LoRaCommand_t;

/*!
 * LoRaMAC Status
 */
typedef enum eLoRaStatus
{
  /*!
   * Service started successfully
   */
  LORA_STATUS_OK,
  /*!
   * Service not started - LoRaMAC is busy
   */
  LORA_STATUS_BUSY,
  /*!
   * Service unknown
   */
  LORA_STATUS_SERVICE_UNKNOWN,
  /*!
   * Service not started - invalid parameter
   */
  LORA_STATUS_PARAMETER_INVALID,
  /*!
   * Service not started - invalid frequency
   */
  LORA_STATUS_FREQUENCY_INVALID,
  /*!
   * Service not started - invalid datarate
   */
  LORA_STATUS_DATARATE_INVALID,
  /*!
   * Service not started - invalid frequency and datarate
   */
  LORA_STATUS_FREQ_AND_DR_INVALID,
  /*!
   * Service not started - the device is not in a LoRaWAN
   */
  LORA_STATUS_NO_NETWORK_JOINED,
  /*!
   * Service not started - payload lenght error
   */
  LORA_STATUS_LENGTH_ERROR,
  /*!
   * Service not started - payload lenght error
   */
  LORA_STATUS_MAC_CMD_LENGTH_ERROR,
  /*!
   * Service not started - the device is switched off
   */
  LORA_STATUS_DEVICE_OFF,
  /*!
   * Service not started - the specified region is not supported
   * or not activated with preprocessor definitions.
   */
  LORA_STATUS_REGION_NOT_SUPPORTED,


  LORA_STATUS_STATE_INVALID,
} LoRaStatus_t;

/*!
 * LoRa Tx Indication
 */
typedef struct
{
    /*!
     * Status of the operation
     */
    LoRaMacEventInfoStatus_t Status;
    /*!
     * The uplink frequency related to the frame
     */
    uint32_t UpLinkFrequency;
    /*!
     * Uplink datarate
     */
    uint8_t TxDatarate;
    /*!
     * Transmission power
     */
    int8_t TxPower;
    /*!
     * The uplink counter value related to the frame
     */
    uint32_t UpLinkCounter;
    /*!
     * Set if an acknowledgement was received
     */
    bool AckReceived;
    /*!
     * Rssi of the received packet
     */
    int16_t Rssi;
    /*!
     * Snr of the received packet
     */
    int8_t Snr;
    /*!
     * Rx indication, if valid data is received
     */
    bool RxIndication;
    /*!
     * Packet type
     */
    LoRaPacketType_t PacketType;
} LoRaTxConfirm_t;

/*!
 * LoRa Rx Indication
 */
typedef struct
{
    /*!
     * Status of the operation
     */
    LoRaMacEventInfoStatus_t Status;
    /*!
     * The uplink frequency related to the frame
     */
    uint32_t DownLinkFrequency;
    /*!
     * Downlink datarate
     */
    uint8_t RxDatarate;
    /*!
     * The downlink counter value for the received frame
     */
    uint32_t DownLinkCounter;
    /*!
     * Indicates, if data is available
     */
    bool RxData;
    /*!
     * Rssi of the received packet
     */
    int16_t Rssi;
    /*!
     * Snr of the received packet
     */
    int8_t Snr;
} LoRaRxIndication_t;

/* Lora Main callbacks*/
typedef struct sLoRaCallback
{
    /*!
     * @brief Get the current battery level
     *
     * @retval value  battery level ( 0: very low, 254: fully charged )
     */
    uint8_t ( *BoardGetBatteryLevel )( void );

    void ( *OnJoined ) (void);

    void ( *OnJoinFailed ) (void);

    /*!
     * @brief Process Rx Data received from Lora network
     *
     * @param [IN] AppData is a buffer to process
     */
    void ( *OnRxData ) (LoRaAppData_t *appData, LoRaRxIndication_t *rxIndication);

    void ( *OnTxDone )(LoRaTxConfirm_t *txConfirm);

    void ( *OnCommandCallback)(void *type);

} LoRaCallback_t;



/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/**
 * @brief run Lora classA state Machine
 * @param [IN] none
 * @retval none
 */
void LoRaFSM( void );

//DEBUG

void LoRaRadioInit(void);

//DEBUGEND

/**
 * @brief Lora Initialisation
 * @param [IN] LoRaMainCallback_t
 * @param [IN] application parmaters
 * @retval none
 */
void LoRaInit(LoRaMacRegion_t region, LoRaCallback_t *callbacks);

/**
 * @brief Lora Join procedure
 * @param [IN] none
 * @retval status (ok or busy)
 */
LoRaStatus_t LoRaJoin(uint32_t trials);

/**
 * @brief
 * @param
 * @retval
 */
bool LoRaNetworkJoinStatusGet(void);

/**
 * [LoRaNetworkJoinStatusSet description]
 *
 * @param     joinStatus    [description]
 *
 * @return                  [description]
 */
LoRaStatus_t LoRaNetworkJoinStatusSet(bool joinStatus);

/**
 * [LoRaReqServerTime]
 *
 * @param
 *
 * @return                  [description]
 */
LoRaStatus_t LoRaReqServerTime(void);

/**
 * @brief Lora Send

 * @param [IN]
 * @param [IN]
 * @param [IN]
 *
 * @retval LoRa status
 */
LoRaStatus_t LoRaUnconfirmedPacketSend(uint8_t port, uint8_t *pBuff, uint16_t buffSize);

/**
 * @brief Lora Send

 * @param [IN]
 * @param [IN]
 * @param [IN]
 * @param [IN]
 *
 * @retval LoRa status
 */
LoRaStatus_t LoRaConfirmedPacketSend(uint8_t port, uint8_t trials, uint8_t *pBuff, uint16_t buffSize);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaDevEUISet(uint8_t eui[8]);

/**
  * @brief  Get Device EUI
  * @param  None
  * @retval DevEUI
  */
uint8_t *LoRaDevEUIGet(void);

/**
  * @brief  Set Application EUI
  * @param  AppEUI
  * @retval Nonoe
  */
LoRaStatus_t LoRaAppEUISet(uint8_t eui[8]);

/**
  * @brief  Get Application EUI
  * @param  None
  * @retval AppEUI
  */
uint8_t *LoRaAppEUIGet(void);

/**
  * @brief  Set Application Key
  * @param  AppKey
  * @retval None
  */
LoRaStatus_t LoRaAppKeySet(uint8_t key[16]);

/**
  * @brief  Get Application Key
  * @param  None
  * @retval AppKey
  */
uint8_t *LoRaAppKeyGet(void);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaAppSKeySet(uint8_t key[16]);

/**
 * @brief
 * @param
 * @retval
 */
uint8_t *LoRaAppSKeyGet(void);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaNwkSKeySet(uint8_t key[16]);

/**
 * @brief
 * @param
 * @retval
 */
uint8_t *LoRaNwkSKeyGet(void);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaDevAddrSet(uint32_t addr);

/**
 * @brief
 * @param
 * @retval
 */
uint32_t LoRaDevAddrGet(void);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaDeviceClassSet(DeviceClass_t device_class);

/**
 * @brief
 * @param
 * @retval
 */
DeviceClass_t LoRaDeviceClassGet(void);

/**
  * @brief  Set join activation process: OTAA vs ABP
  * @param
  * @retval return @LoRaMacStatus_t
  */
LoRaStatus_t LoRaActivationSet(ActivationType_t activation);

/**
  * @brief  Get join activation process: OTAA vs ABP
  * @param  None
  * @retval true if OTAA is used, false if ABP is used
  */
ActivationType_t LoRaActivationGet(void);


/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaTxDatarateSet(uint32_t datarate);

/**
 * @brief
 * @param
 * @retval
 */
uint32_t LoRaTxDatarateGet(void);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaNbReqSet(uint32_t nbReq);

/**
 * @brief
 * @param
 * @retval
 */
uint32_t LoRaNbReqGet(void);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaRx1DatarateOffsetSet(uint32_t datarateOffset);
/**
 * @brief
 * @param
 * @retval
 */
uint32_t LoRaCurTxDatarateGet(void);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaTxPowerSet(uint8_t level);

/**
 * @brief
 * @param
 * @retval
 */
uint32_t LoRaTxPowerGet(void);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaAdrStatusSet(bool enable);

/**
 * @brief
 * @param
 * @retval
 */
bool LoRaAdrStatusGet(void);

/**
  * @brief  Set duty cycle: true or false
  * @param  Duty cycle to set: enable or disable
  * @retval None
  */
LoRaStatus_t LoRaDutyCycleStatusSet(bool duty_cycle);

/**
  * @brief  Get Duty cycle: OTAA vs ABP
  * @param  None
  * @retval true / false
  */
bool LoRaDutyCycleStatusGet(void);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaRx2FrequencySet(uint32_t freq);

/**
 * @brief
 * @param
 * @retval
 */
uint32_t LoRaRx2FrequencyGet(void);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaRx2DatarateSet(uint32_t datarate);

/**
 * @brief
 * @param
 * @retval
 */
uint32_t LoRaRx2DatarateGet(void);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaRx1DelaySet(uint32_t delay);

/**
 * @brief
 * @param
 * @retval
 */
uint32_t LoRaRx1DelayGet(void);

/**
 * @brief
 * @param
 * @retval
 */
uint32_t LoRaRx2DelayGet(void);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaJoinRx1DelaySet(uint32_t delay);

/**
 * @brief
 * @param
 * @retval
 */
uint32_t LoRaJoinRx1DelayGet(void);

/**
 * @brief
 * @param
 * @retval
 */
uint32_t LoRaJoinRx2DelayGet(void);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaUplinkCounterSet(uint32_t count);

/**
 * @brief
 * @param
 * @retval
 */
uint32_t LoRaUplinkCounterGet(void);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaDownlinkCounterSet(uint32_t count);

/**
 * @brief
 * @param
 * @retval
 */
uint32_t LoRaDownlinkCounterGet(void);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaChannelMaskSet(uint16_t *pMask);

/**
 * @brief
 * @param
 * @retval
 */
uint16_t * LoRaChannelMaskGet(void);

/**
 * @brief
 * @param
 * @retval
 */
uint32_t LoRaMaxNbChannelsGet(void);

/**
 * @brief
 * @param
 * @retval
 */
ChannelParams_t * LoRaChannelsGet(void);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaChannelAdd(uint32_t id, ChannelParams_t param);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaChannelRemove(uint32_t id);

/**
 * @brief
 * @param
 * @retval
 */
LoRaStatus_t LoRaMaxEripSet(uint32_t eirp);

/**
 * @brief
 * @param
 * @retval
 */
uint64_t LoRaLastTimeStampGet(void);

/**
 * @brief
 * @param
 * @retval
 */
uint32_t LoRaMaxEripGet(void);

/**
 * @brief  Get the SNR of the last received data
 * @param  None
 * @retval SNR
 */
int32_t LoRaSNRGet(void);

/**
 * @brief  Get the RSSI of the last received data
 * @param  None
 * @retval RSSI
 */
int32_t LoRaRSSIGet(void);

/**
 * [LoRaLbtStatusSet description]
 *
 * @param     enable    [description]
 *
 * @return              [description]
 */
LoRaStatus_t LoRaLbtStatusSet(bool enable);

/**
 * [LoRaLbtStatusGet description]
 *
 * @return        [description]
 */
bool LoRaLbtStatusGet(void);

/**
 * [LoRaLbtThresholdSet description]
 *
 * @param     threshold    [description]
 *
 * @return                 [description]
 */
LoRaStatus_t LoRaLbtThresholdSet(int32_t threshold);

/**
 * [LoRaLbtThresholdGet description]
 *
 * @return        [description]
 */
int32_t LoRaLbtThresholdGet(void);

/**
 * @brief  Get whether or not the last sent data were acknowledged
 * @param  None
 * @retval true if so, false otherwise
 */
bool LoRaIsAckReceived(void);

/**
 * @brief  Launch LoraWan certification tests
 * @param  None
 * @retval The application port
 */
void LoRaWANCertif(void);

/**
 * @brief
 * @param
 * @retval
 */
void LoRaWANCertifStop(void);

/**
 * @brief
 * @param [IN] none
 * @retval -1: invalid tx power; >0:tx power index
 */
int LoRaTxPowerLevelGet(int8_t power);

/**
 * @brief
 * @param
 * @retval
 */
bool LoRaTxDatarateVerify(uint8_t dr);

/**
 * @brief
 * @param
 * @retval
 */
bool LoRaRxDatarateVerify(uint8_t dr);

/**
 * @brief
 * @param
 * @retval
 */
bool LoRaRx1DatarateOffsetVerify(uint8_t dr1Offset);

/**
 * @brief
 * @param
 * @retval
 */
uint32_t LoRaRx1DatarateOffsetGet(void);

/**
 * @brief
 * @param
 * @retval
 */
bool LoRaTxPowerVerify(uint8_t level);


#ifdef __cplusplus
}
#endif

#endif /*__LORA_MAIN_H__*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
