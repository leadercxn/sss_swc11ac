/*!
 * \file      RegionAU915Hybrid.h
 *
 * \brief     Region definition for AU915
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013 Semtech
 *
 *               ___ _____ _   ___ _  _____ ___  ___  ___ ___
 *              / __|_   _/_\ / __| |/ / __/ _ \| _ \/ __| __|
 *              \__ \ | |/ _ \ (__| ' <| _| (_) |   / (__| _|
 *              |___/ |_/_/ \_\___|_|\_\_| \___/|_|_\\___|___|
 *              embedded.connectivity.solutions===============
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 *
 * \author    Daniel Jaeckle ( STACKFORCE )
 *
 * \defgroup  REGIONAU915 Region AU915
 *            Implementation according to LoRaWAN Specification v1.0.2.
 * \{
 */
#ifndef __REGION_AU915_HYBRID_HYBRID_H__
#define __REGION_AU915_HYBRID_HYBRID_H__

/*!
 * LoRaMac maximum number of channels
 */
#define AU915_HYBRID_MAX_NB_CHANNELS                       72

/*!
 * Minimal datarate that can be used by the node
 */
#define AU915_HYBRID_TX_MIN_DATARATE                       DR_0

/*!
 * Maximal datarate that can be used by the node
 */
#define AU915_HYBRID_TX_MAX_DATARATE                       DR_4

/*!
 * Minimal datarate that can be used by the node
 */
#define AU915_HYBRID_RX_MIN_DATARATE                       DR_8

/*!
 * Maximal datarate that can be used by the node
 */
#define AU915_HYBRID_RX_MAX_DATARATE                       DR_13

/*!
 * Default datarate used by the node
 */
#define AU915_HYBRID_DEFAULT_DATARATE                      DR_0

/*!
 * Minimal Rx1 receive datarate offset
 */
#define AU915_HYBRID_MIN_RX1_DR_OFFSET                     0

/*!
 * Maximal Rx1 receive datarate offset
 */
#define AU915_HYBRID_MAX_RX1_DR_OFFSET                     3

/*!
 * Default Rx1 receive datarate offset
 */
#define AU915_HYBRID_DEFAULT_RX1_DR_OFFSET                 0

/*!
 * Minimal Tx output power that can be used by the node
 */
#define AU915_HYBRID_MIN_TX_POWER                          TX_POWER_10

/*!
 * Maximal Tx output power that can be used by the node
 */
#define AU915_HYBRID_MAX_TX_POWER                          TX_POWER_0

/*!
 * Default Tx output power used by the node
 */
#define AU915_HYBRID_DEFAULT_TX_POWER                      TX_POWER_5

/*!
 * ADR Ack limit
 */
#define AU915_HYBRID_ADR_ACK_LIMIT                         64

/*!
 * ADR Ack delay
 */
#define AU915_HYBRID_ADR_ACK_DELAY                         32

/*!
 * Enabled or disabled the duty cycle
 */
#define AU915_HYBRID_DUTY_CYCLE_ENABLED                    0

/*!
 * Maximum RX window duration
 */
#define AU915_HYBRID_MAX_RX_WINDOW                         3000

/*!
 * Receive delay 1
 */
#define AU915_HYBRID_RECEIVE_DELAY1                        1000

/*!
 * Receive delay 2
 */
#define AU915_HYBRID_RECEIVE_DELAY2                        2000

/*!
 * Join accept delay 1
 */
#define AU915_HYBRID_JOIN_ACCEPT_DELAY1                    5000

/*!
 * Join accept delay 2
 */
#define AU915_HYBRID_JOIN_ACCEPT_DELAY2                    6000

/*!
 * Maximum frame counter gap
 */
#define AU915_HYBRID_MAX_FCNT_GAP                          16384

/*!
 * Ack timeout
 */
#define AU915_HYBRID_ACKTIMEOUT                            2000

/*!
 * Random ack timeout limits
 */
#define AU915_HYBRID_ACK_TIMEOUT_RND                       1000

/*!
 * Second reception window channel frequency definition.
 */
#define AU915_HYBRID_RX_WND_2_FREQ                         923300000

/*!
 * Second reception window channel datarate definition.
 */
#define AU915_HYBRID_RX_WND_2_DR                           DR_8

/*!
 * LoRaMac maximum number of bands
 */
#define AU915_HYBRID_MAX_NB_BANDS                           1

/*!
 * Band 0 definition
 * { DutyCycle, TxMaxPower, LastTxDoneTime, TimeOff }
 */
#define AU915_HYBRID_BAND0                                 { 1, AU915_HYBRID_DEFAULT_TX_POWER, 0,  0 } //  100.0 %

/*!
 * LoRaMac random offset for the first join request.
 */
#define AU915_HYBRID_BACKOFF_RND_OFFSET                    600000

/*!
 * Defines the first channel for RX window 1 for US band
 */
#define AU915_HYBRID_FIRST_RX1_CHANNEL                     ( (uint32_t) 923.3e6 )

/*!
 * Defines the last channel for RX window 1 for US band
 */
#define AU915_HYBRID_LAST_RX1_CHANNEL                      ( (uint32_t) 927.5e6 )

/*!
 * Defines the step width of the channels for RX window 1
 */
#define AU915_HYBRID_STEPWIDTH_RX1_CHANNEL                 ( (uint32_t) 600e3 )

/*!
 * RSSI threshold for a free channel [dBm]
 */
#define AU915_HYBRID_RSSI_FREE_TH                          -90

/*!
 * Specifies the time the node performs a carrier sense
 */
#define AU915_HYBRID_CARRIER_SENSE_TIME                    6

/*!
 * Lbt check retry times
 */
#define AU915_HYBRID_LBT_RETRY_TIMES                   3

/*!
 * Data rates table definition
 */
static const uint8_t DataratesAU915_Hybrid[]  = { 10, 9, 8,  7,  8,  0,  0, 0, 12, 11, 10, 9, 8, 7, 0, 0 };

/*!
 * Up/Down link data rates offset definition
 */
static const int8_t DatarateOffsetsAU915_Hybrid[5][4] =
{
    { DR_10, DR_9 , DR_8 , DR_8  }, // DR_0
    { DR_11, DR_10, DR_9 , DR_8  }, // DR_1
    { DR_12, DR_11, DR_10, DR_9  }, // DR_2
    { DR_13, DR_12, DR_11, DR_10 }, // DR_3
    { DR_13, DR_13, DR_12, DR_11 }, // DR_4
};

/*!
 * Maximum payload with respect to the datarate index. Cannot operate with repeater.
 */
static const uint8_t MaxPayloadOfDatarateAU915_Hybrid[] = { 11, 53, 126, 242, 242, 0, 0, 0, 53, 129, 242, 242, 242, 242, 0, 0 };

/*!
 * Maximum payload with respect to the datarate index. Can operate with repeater.
 */
static const uint8_t MaxPayloadOfDatarateRepeaterAU915_Hybrid[] = { 11, 53, 126, 242, 242, 0, 0, 0, 33, 109, 222, 222, 222, 222, 0, 0 };

/*!
 * Tx output powers table definition
 */
static const int8_t TxPowersAU915_Hybrid[] = { 30, 28, 26, 24, 22, 20, 18, 16, 14, 12, 10 };



/*!
 * \brief The function gets a value of a specific phy attribute.
 *
 * \param [IN] getPhy Pointer to the function parameters.
 */
void RegionAU915HybridGetPhyParam( GetPhyParams_t* getPhy );

/*!
 * \brief Updates the last TX done parameters of the current channel.
 *
 * \param [IN] txDone Pointer to the function parameters.
 */
void RegionAU915HybridSetBandTxDone( SetBandTxDoneParams_t* txDone );

/*!
 * \brief Initializes the channels masks and the channels.
 *
 * \param [IN] type Sets the initialization type.
 */
void RegionAU915HybridInitDefaults( InitType_t type );

/*!
 * \brief Verifies a parameter.
 *
 * \param [IN] verify Pointer to the function parameters.
 *
 * \param [IN] type Sets the initialization type.
 *
 * \retval Returns true, if the parameter is valid.
 */
bool RegionAU915HybridVerify( VerifyParams_t* verify, PhyAttribute_t phyAttribute );

/*!
 * \brief The function parses the input buffer and sets up the channels of the
 *        CF list.
 *
 * \param [IN] applyCFList Pointer to the function parameters.
 */
void RegionAU915HybridApplyCFList( ApplyCFListParams_t* applyCFList );

/*!
 * \brief Sets a channels mask.
 *
 * \param [IN] chanMaskSet Pointer to the function parameters.
 *
 * \retval Returns true, if the channels mask could be set.
 */
bool RegionAU915HybridChanMaskSet( ChanMaskSetParams_t* chanMaskSet );

/*!
 * \brief Calculates the next datarate to set, when ADR is on or off.
 *
 * \param [IN] adrNext Pointer to the function parameters.
 *
 * \param [OUT] drOut The calculated datarate for the next TX.
 *
 * \param [OUT] txPowOut The TX power for the next TX.
 *
 * \param [OUT] adrAckCounter The calculated ADR acknowledgement counter.
 *
 * \retval Returns true, if an ADR request should be performed.
 */
bool RegionAU915HybridAdrNext( AdrNextParams_t* adrNext, int8_t* drOut, int8_t* txPowOut, uint32_t* adrAckCounter );

/*!
 * \brief Configuration of the RX windows.
 *
 * \param [IN] rxConfig Pointer to the function parameters.
 *
 * \param [OUT] datarate The datarate index which was set.
 *
 * \retval Returns true, if the configuration was applied successfully.
 */
bool RegionAU915HybridRxConfig( RxConfigParams_t* rxConfig, int8_t* datarate );

/*!
 * \brief TX configuration.
 *
 * \param [IN] txConfig Pointer to the function parameters.
 *
 * \param [OUT] txPower The tx power index which was set.
 *
 * \param [OUT] txTimeOnAir The time-on-air of the frame.
 *
 * \retval Returns true, if the configuration was applied successfully.
 */
bool RegionAU915HybridTxConfig( TxConfigParams_t* txConfig, int8_t* txPower, uint32_t* txTimeOnAir );

/*!
 * \brief The function processes a Link ADR Request.
 *
 * \param [IN] linkAdrReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 */
uint8_t RegionAU915HybridLinkAdrReq( LinkAdrReqParams_t* linkAdrReq, int8_t* drOut, int8_t* txPowOut, uint8_t* nbRepOut, uint8_t* nbBytesParsed );

/*!
 * \brief The function processes a RX Parameter Setup Request.
 *
 * \param [IN] rxParamSetupReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 */
uint8_t RegionAU915HybridRxParamSetupReq( RxParamSetupReqParams_t* rxParamSetupReq );

/*!
 * \brief The function processes a Channel Request.
 *
 * \param [IN] newChannelReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 */
uint8_t RegionAU915HybridNewChannelReq( NewChannelReqParams_t* newChannelReq );

/*!
 * \brief The function processes a TX ParamSetup Request.
 *
 * \param [IN] txParamSetupReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 *         Returns -1, if the functionality is not implemented. In this case, the end node
 *         shall not process the command.
 */
int8_t RegionAU915HybridTxParamSetupReq( TxParamSetupReqParams_t* txParamSetupReq );

/*!
 * \brief The function processes a DlChannel Request.
 *
 * \param [IN] dlChannelReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 */
uint8_t RegionAU915HybridDlChannelReq( DlChannelReqParams_t* dlChannelReq );

/*
 * \brief Alternates the datarate of the channel for the join request.
 *
 * \param [IN] alternateDr Pointer to the function parameters.
 *
 * \retval Datarate to apply.
 */
int8_t RegionAU915HybridAlternateDr( AlternateDrParams_t* alternateDr );

/*!
 * \brief Calculates the back-off time.
 *
 * \param [IN] calcBackOff Pointer to the function parameters.
 */
void RegionAU915HybridCalcBackOff( CalcBackOffParams_t* calcBackOff );

/*!
 * \brief Searches and set the next random available channel
 *
 * \param [OUT] channel Next channel to use for TX.
 *
 * \param [OUT] time Time to wait for the next transmission according to the duty
 *              cycle.
 *
 * \retval Function status [1: OK, 0: Unable to find a channel on the current datarate]
 */
bool RegionAU915HybridNextChannel( NextChanParams_t* nextChanParams, uint8_t* channel, uint32_t* time );

/*!
 * \brief Adds a channel.
 *
 * \param [IN] channelAdd Pointer to the function parameters.
 *
 * \retval Status of the operation.
 */
LoRaMacStatus_t RegionAU915HybridChannelAdd( ChannelAddParams_t* channelAdd );

/*!
 * \brief Removes a channel.
 *
 * \param [IN] channelRemove Pointer to the function parameters.
 *
 * \retval Returns true, if the channel was removed successfully.
 */
bool RegionAU915HybridChannelsRemove( ChannelRemoveParams_t* channelRemove  );

/*!
 * \brief Sets the radio into continuous wave mode.
 *
 * \param [IN] continuousWave Pointer to the function parameters.
 */
void RegionAU915HybridSetContinuousWave( ContinuousWaveParams_t* continuousWave );

/*! \} defgroup REGIONAU915 */

#endif // __REGION_AU915_HYBRID_H__
