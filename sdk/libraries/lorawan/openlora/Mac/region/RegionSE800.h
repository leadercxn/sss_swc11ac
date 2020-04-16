/*!
 * \file      RegionSE800.h
 *
 * \brief     Region definition for SE800
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
 * \defgroup  REGIONSE800 Region SE800
 *            Implementation according to LoRaWAN Specification v1.0.2.
 * \{
 */
#ifndef __REGION_SE800_H__
#define __REGION_SE800_H__

/*!
 * LoRaMac maximum number of channels
 */
#define SE800_MAX_NB_CHANNELS                       16

/*!
 * Number of default channels
 */
#define SE800_NUMB_DEFAULT_CHANNELS                 3

/*!
 * Minimal datarate that can be used by the node
 */
#define SE800_TX_MIN_DATARATE                       DR_0

/*!
 * Maximal datarate that can be used by the node
 */
#define SE800_TX_MAX_DATARATE                       DR_5

/*!
 * Minimal datarate that can be used by the node
 */
#define SE800_RX_MIN_DATARATE                       DR_0

/*!
 * Maximal datarate that can be used by the node
 */
#define SE800_RX_MAX_DATARATE                       DR_5

/*!
 * Default datarate used by the node
 */
#define SE800_DEFAULT_DATARATE                      DR_3

/*!
 * Minimal Rx1 receive datarate offset
 */
#define SE800_MIN_RX1_DR_OFFSET                     0

/*!
 * Maximal Rx1 receive datarate offset
 */
#define SE800_MAX_RX1_DR_OFFSET                     5

/*!
 * Default Rx1 receive datarate offset
 */
#define SE800_DEFAULT_RX1_DR_OFFSET                 0

/*!
 * Minimal Tx output power that can be used by the node
 */
#define SE800_MIN_TX_POWER                          TX_POWER_7

/*!
 * Maximal Tx output power that can be used by the node
 */
#define SE800_MAX_TX_POWER                          TX_POWER_0

/*!
 * Default Tx output power used by the node
 */
#define SE800_DEFAULT_TX_POWER                      TX_POWER_1

/*!
 * Default Max EIRP
 */
#define SE800_DEFAULT_MAX_EIRP                      20

/*!
 * ADR Ack limit
 */
#define SE800_ADR_ACK_LIMIT                         64

/*!
 * ADR Ack delay
 */
#define SE800_ADR_ACK_DELAY                         32

/*!
 * Enabled or disabled the duty cycle
 */
#define SE800_DUTY_CYCLE_ENABLED                    0

/*!
 * Maximum RX window duration
 */
#define SE800_MAX_RX_WINDOW                         3000

/*!
 * Receive delay 1
 */
#define SE800_RECEIVE_DELAY1                        1000

/*!
 * Receive delay 2
 */
#define SE800_RECEIVE_DELAY2                        2000

/*!
 * Join accept delay 1
 */
#define SE800_JOIN_ACCEPT_DELAY1                    5000

/*!
 * Join accept delay 2
 */
#define SE800_JOIN_ACCEPT_DELAY2                    6000

/*!
 * Maximum frame counter gap
 */
#define SE800_MAX_FCNT_GAP                          16384

/*!
 * Ack timeout
 */
#define SE800_ACKTIMEOUT                            2000

/*!
 * Random ack timeout limits
 */
#define SE800_ACK_TIMEOUT_RND                       1000

/*!
 * Verification of default datarate
 */
#if ( SE800_DEFAULT_DATARATE > DR_5 )
#error "A default DR higher than DR_5 may lead to connectivity loss."
#endif

/*!
 * Second reception window channel frequency definition.
 */
#define SE800_RX_WND_2_FREQ                         867700000

/*!
 * Second reception window channel datarate definition.
 */
#define SE800_RX_WND_2_DR                           DR_0

/*!
 * LoRaMac maximum number of bands
 */
#define SE800_MAX_NB_BANDS                          1

/*!
 * Band 0 definition
 * { DutyCycle, TxMaxPower, LastTxDoneTime, TimeOff }
 */
#define SE800_BAND0                                 { 1, SE800_DEFAULT_TX_POWER, 0,  0 } //  1.0 %

/*!
 * LoRaMac random offset for the first join request.
 */
#define SE800_BACKOFF_RND_OFFSET      600000

/*!
 * Defines the step width of the channels for RX window 1
 */
#define SE800_STEPWIDTH_RX1_CHANNEL                 ( (uint32_t) 200e3 )

/*! 
 * RSSI threshold for a free channel [dBm]
 */
#define SE800_RSSI_FREE_TH                          -65

/*!
 * Specifies the time the node performs a carrier sense
 */
#define SE800_CARRIER_SENSE_TIME                    6

/*!
 * Lbt check retry times
 */
#define SE800_LBT_RETRY_TIMES                   3

/*!
 * Data rates table definition
 */
static const uint8_t DataratesSE800[] = { 12, 11, 10,  9,  8,  7 };

/*!
 * Maximum payload with respect to the datarate index. Cannot operate with repeater.
 */
static const uint8_t MaxPayloadOfDatarateSE800[] = { 51, 51, 51, 115, 242, 242 };

/*!
 * Maximum payload with respect to the datarate index. Can operate with repeater.
 */
static const uint8_t MaxPayloadOfDatarateRepeaterSE800[] = { 51, 51, 51, 115, 222, 222 };



/*!
 * \brief The function gets a value of a specific phy attribute.
 *
 * \param [IN] getPhy Pointer to the function parameters.
 */
void RegionSE800GetPhyParam( GetPhyParams_t* getPhy );

/*!
 * \brief Updates the last TX done parameters of the current channel.
 *
 * \param [IN] txDone Pointer to the function parameters.
 */
void RegionSE800SetBandTxDone( SetBandTxDoneParams_t* txDone );

/*!
 * \brief Initializes the channels masks and the channels.
 *
 * \param [IN] type Sets the initialization type.
 */
void RegionSE800InitDefaults( InitType_t type );

/*!
 * \brief Verifies a parameter.
 *
 * \param [IN] verify Pointer to the function parameters.
 *
 * \param [IN] type Sets the initialization type.
 *
 * \retval Returns true, if the parameter is valid.
 */
bool RegionSE800Verify( VerifyParams_t* verify, PhyAttribute_t phyAttribute );

/*!
 * \brief The function parses the input buffer and sets up the channels of the
 *        CF list.
 *
 * \param [IN] applyCFList Pointer to the function parameters.
 */
void RegionSE800ApplyCFList( ApplyCFListParams_t* applyCFList );

/*!
 * \brief Sets a channels mask.
 *
 * \param [IN] chanMaskSet Pointer to the function parameters.
 *
 * \retval Returns true, if the channels mask could be set.
 */
bool RegionSE800ChanMaskSet( ChanMaskSetParams_t* chanMaskSet );

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
bool RegionSE800AdrNext( AdrNextParams_t* adrNext, int8_t* drOut, int8_t* txPowOut, uint32_t* adrAckCounter );

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
bool RegionSE800RxConfig( RxConfigParams_t* rxConfig, int8_t* datarate );

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
bool RegionSE800TxConfig( TxConfigParams_t* txConfig, int8_t* txPower, uint32_t* txTimeOnAir );

/*!
 * \brief The function processes a Link ADR Request.
 *
 * \param [IN] linkAdrReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 */
uint8_t RegionSE800LinkAdrReq( LinkAdrReqParams_t* linkAdrReq, int8_t* drOut, int8_t* txPowOut, uint8_t* nbRepOut, uint8_t* nbBytesParsed );

/*!
 * \brief The function processes a RX Parameter Setup Request.
 *
 * \param [IN] rxParamSetupReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 */
uint8_t RegionSE800RxParamSetupReq( RxParamSetupReqParams_t* rxParamSetupReq );

/*!
 * \brief The function processes a Channel Request.
 *
 * \param [IN] newChannelReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 */
uint8_t RegionSE800NewChannelReq( NewChannelReqParams_t* newChannelReq );

/*!
 * \brief The function processes a TX ParamSetup Request.
 *
 * \param [IN] txParamSetupReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 *         Returns -1, if the functionality is not implemented. In this case, the end node
 *         shall not process the command.
 */
int8_t RegionSE800TxParamSetupReq( TxParamSetupReqParams_t* txParamSetupReq );

/*!
 * \brief The function processes a DlChannel Request.
 *
 * \param [IN] dlChannelReq Pointer to the function parameters.
 *
 * \retval Returns the status of the operation, according to the LoRaMAC specification.
 */
uint8_t RegionSE800DlChannelReq( DlChannelReqParams_t* dlChannelReq );

/*
 * \brief Alternates the datarate of the channel for the join request.
 *
 * \param [IN] alternateDr Pointer to the function parameters.
 *
 * \retval Datarate to apply.
 */
int8_t RegionSE800AlternateDr( AlternateDrParams_t* alternateDr );

/*!
 * \brief Calculates the back-off time.
 *
 * \param [IN] calcBackOff Pointer to the function parameters.
 */
void RegionSE800CalcBackOff( CalcBackOffParams_t* calcBackOff );

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
bool RegionSE800NextChannel( NextChanParams_t* nextChanParams, uint8_t* channel, uint32_t* time );

/*!
 * \brief Adds a channel.
 *
 * \param [IN] channelAdd Pointer to the function parameters.
 *
 * \retval Status of the operation.
 */
LoRaMacStatus_t RegionSE800ChannelAdd( ChannelAddParams_t* channelAdd );

/*!
 * \brief Removes a channel.
 *
 * \param [IN] channelRemove Pointer to the function parameters.
 *
 * \retval Returns true, if the channel was removed successfully.
 */
bool RegionSE800ChannelsRemove( ChannelRemoveParams_t* channelRemove  );

/*!
 * \brief Sets the radio into continuous wave mode.
 *
 * \param [IN] continuousWave Pointer to the function parameters.
 */
void RegionSE800SetContinuousWave( ContinuousWaveParams_t* continuousWave );

/*! \} defgroup REGIONSE800 */

#endif // __REGION_SE800_H__
