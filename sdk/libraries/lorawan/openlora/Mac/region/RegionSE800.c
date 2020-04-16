/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech
 ___ _____ _   ___ _  _____ ___  ___  ___ ___
/ __|_   _/_\ / __| |/ / __/ _ \| _ \/ __| __|
\__ \ | |/ _ \ (__| ' <| _| (_) |   / (__| _|
|___/ |_/_/ \_\___|_|\_\_| \___/|_|_\\___|___|
embedded.connectivity.solutions===============

Description: LoRa MAC region SE800 implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis ( Semtech ), Gregory Cristian ( Semtech ) and Daniel Jaeckle ( STACKFORCE )
*/
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "radio.h"
#include "LoRaMac.h"

#include "timer_platform.h"
#include "util.h"

#include "Region.h"
#include "RegionCommon.h"
#include "RegionSE800.h"



// Definitions
#define CHANNELS_MASK_SIZE              1

/*!
 * lbtRetryTimes
 */
static uint8_t lbtRetryTimes = 0;

// Global attributes
/*!
 * LoRaMAC channels
 */
static ChannelParams_t Channels[SE800_MAX_NB_CHANNELS];

/*!
 * LoRaMac bands
 */
static Band_t Bands[SE800_MAX_NB_BANDS] =
{
    SE800_BAND0
};

/*!
 * LoRaMac channels mask
 */
static uint16_t ChannelsMask[CHANNELS_MASK_SIZE];

/*!
 * LoRaMac channels default mask
 */
static uint16_t ChannelsDefaultMask[CHANNELS_MASK_SIZE];



static uint8_t ApplyDrOffset( int8_t dr, int8_t drOffset )
{
    int8_t datarate = dr - drOffset;
    if ( datarate < 0 )
    {
        datarate = DR_0;
    }
    return datarate;
}

static uint16_t GetSymbTimeout( int8_t dr )
{
    uint16_t symbolTimeout = 5;

    if ( ( dr == DR_3 ) || ( dr == DR_4 ) )
    {
        symbolTimeout = 8;
    }
    else if ( dr == DR_5 )
    {
        symbolTimeout = 10;
    }

    return symbolTimeout;
}

static bool VerifyTxFreq( uint32_t freq )
{
    // Check radio driver support
    if ( Radio.CheckRfFrequency( freq ) == false )
    {
        return false;
    }

    //1276
    if ( ( freq < 137000000 ) || ( freq > 1020000000 ) )
    {
        return false;
    }
    return true;
}

static uint8_t CountNbOfEnabledChannels( bool joined, uint8_t datarate, uint16_t* channelsMask, ChannelParams_t* channels, Band_t* bands, uint8_t* enabledChannels, uint8_t* delayTx )
{
    uint8_t nbEnabledChannels = 0;
    uint8_t delayTransmission = 0;

    for ( uint8_t i = 0, k = 0; i < SE800_MAX_NB_CHANNELS; i += 16, k++ )
    {
        for ( uint8_t j = 0; j < 16; j++ )
        {
            if ( ( channelsMask[k] & ( 1 << j ) ) != 0 )
            {
                if ( channels[i + j].Frequency == 0 )
                {   // Check if the channel is enabled
                    continue;
                }
                if ( RegionCommonValueInRange( datarate, channels[i + j].DrRange.Fields.Min,
                                               channels[i + j].DrRange.Fields.Max ) == false )
                {   // Check if the current channel selection supports the given datarate
                    continue;
                }
                if ( bands[channels[i + j].Band].TimeOff > 0 )
                {   // Check if the band is available for transmission
                    delayTransmission++;
                    continue;
                }
                enabledChannels[nbEnabledChannels++] = i + j;
            }
        }
    }

    *delayTx = delayTransmission;
    return nbEnabledChannels;
}



void RegionSE800GetPhyParam( GetPhyParams_t* getPhy )
{
    switch ( getPhy->Attribute )
    {
        case PHY_MIN_DR:
        {
            getPhy->Param.Value = SE800_TX_MIN_DATARATE;
            break;
        }
        case PHY_DEF_TX_DR:
        {
            getPhy->Param.Value = SE800_DEFAULT_DATARATE;
            break;
        }
        case PHY_DEF_TX_POWER:
        {
            getPhy->Param.Value = SE800_DEFAULT_TX_POWER;
            break;
        }
        case PHY_MAX_PAYLOAD:
        {
            getPhy->Param.Value = MaxPayloadOfDatarateSE800[getPhy->Datarate];
            break;
        }
        case PHY_MAX_PAYLOAD_REPEATER:
        {
            getPhy->Param.Value = MaxPayloadOfDatarateRepeaterSE800[getPhy->Datarate];
            break;
        }
        case PHY_DUTY_CYCLE:
        {
            getPhy->Param.Value = SE800_DUTY_CYCLE_ENABLED;
            break;
        }
        case PHY_MAX_RX_WINDOW:
        {
            getPhy->Param.Value = SE800_MAX_RX_WINDOW;
            break;
        }
        case PHY_RECEIVE_DELAY1:
        {
            getPhy->Param.Value = SE800_RECEIVE_DELAY1;
            break;
        }
        case PHY_RECEIVE_DELAY2:
        {
            getPhy->Param.Value = SE800_RECEIVE_DELAY2;
            break;
        }
        case PHY_JOIN_ACCEPT_DELAY1:
        {
            getPhy->Param.Value = SE800_JOIN_ACCEPT_DELAY1;
            break;
        }
        case PHY_JOIN_ACCEPT_DELAY2:
        {
            getPhy->Param.Value = SE800_JOIN_ACCEPT_DELAY2;
            break;
        }
        case PHY_MAX_FCNT_GAP:
        {
            getPhy->Param.Value = SE800_MAX_FCNT_GAP;
            break;
        }
        case PHY_ACK_TIMEOUT:
        {
            getPhy->Param.Value = ( SE800_ACKTIMEOUT + randr( -SE800_ACK_TIMEOUT_RND, SE800_ACK_TIMEOUT_RND ) );
            break;
        }
        case PHY_DEF_DR1_OFFSET:
        {
            getPhy->Param.Value = SE800_DEFAULT_RX1_DR_OFFSET;
            break;
        }
        case PHY_DEF_RX2_FREQUENCY:
        {
            getPhy->Param.Value = SE800_RX_WND_2_FREQ;
            break;
        }
        case PHY_DEF_RX2_DR:
        {
            getPhy->Param.Value = SE800_RX_WND_2_DR;
            break;
        }
        case PHY_CHANNELS_MASK:
        {
            getPhy->Param.ChannelsMask = ChannelsMask;
            break;
        }
        case PHY_CHANNELS_DEFAULT_MASK:
        {
            getPhy->Param.ChannelsMask = ChannelsDefaultMask;
            break;
        }
        case PHY_MAX_NB_CHANNELS:
        {
            getPhy->Param.Value = SE800_MAX_NB_CHANNELS;
            break;
        }
        case PHY_CHANNELS:
        {
            getPhy->Param.Channels = Channels;
            break;
        }
        case PHY_DEF_UPLINK_DWELL_TIME:
        case PHY_DEF_DOWNLINK_DWELL_TIME:
        {
            getPhy->Param.Value = 0;
            break;
        }
        case PHY_DEF_MAX_EIRP:
        {
            getPhy->Param.Value = SE800_DEFAULT_MAX_EIRP;
            break;
        }
        case PHY_NB_JOIN_TRIALS:
        case PHY_DEF_NB_JOIN_TRIALS:
        {
            getPhy->Param.Value = 48;
            break;
        }
        case PHY_DEF_LBT_THRESHOLD:
        {
            getPhy->Param.Value = (uint32_t)SE800_RSSI_FREE_TH;
            break;
        }
        case PHY_DEF_LBT_RETRYTIMES:
        {
            getPhy->Param.Value = (uint8_t)SE800_LBT_RETRY_TIMES;
            break;
        }
        default:
        {
            return;
        }
    }
}

void RegionSE800SetBandTxDone( SetBandTxDoneParams_t* txDone )
{
    RegionCommonSetBandTxDone( &Bands[Channels[txDone->Channel].Band], txDone->LastTxDoneTime );
}

void RegionSE800InitDefaults( InitType_t type )
{
    switch ( type )
    {
        case INIT_TYPE_INIT:
        {
            // 125 kHz channels
            for ( uint8_t i = 0; i < SE800_MAX_NB_CHANNELS; i++ )
            {
                Channels[i].Frequency = 866100000 + i * 200000;
                Channels[i].DrRange.Value = ( DR_5 << 4 ) | DR_0;
                Channels[i].Band = 0;
            }

            // Initialize channels default mask
            ChannelsDefaultMask[0] = 0x00FF;

            // Update the channels mask
            RegionCommonChanMaskCopy( ChannelsMask, ChannelsDefaultMask, 1 );
            break;
        }
        case INIT_TYPE_RESTORE:
        {
            // Restore channels default mask
            RegionCommonChanMaskCopy( ChannelsMask, ChannelsDefaultMask, 1 );
            break;
        }
        default:
        {
            break;
        }
    }
}

bool RegionSE800Verify( VerifyParams_t* verify, PhyAttribute_t phyAttribute )
{
    switch ( phyAttribute )
    {
        case PHY_TX_DR:
        {
            return RegionCommonValueInRange( verify->Datarate, SE800_TX_MIN_DATARATE, SE800_TX_MAX_DATARATE );
        }
        case PHY_DEF_TX_DR:
        {
            return RegionCommonValueInRange( verify->Datarate, DR_0, DR_5 );
        }
        case PHY_DEF_TX_POWER:
        case PHY_TX_POWER:
        {
            // Remark: switched min and max!
            return RegionCommonValueInRange( verify->TxPower, SE800_MAX_TX_POWER, SE800_MIN_TX_POWER );
        }
        case PHY_DUTY_CYCLE:
        {
            return SE800_DUTY_CYCLE_ENABLED;
        }
        case PHY_NB_JOIN_TRIALS:
        {
            if ( verify->NbJoinTrials < 48 )
            {
                return false;
            }
            break;
        }
        default:
            return false;
    }
    return true;
}

void RegionSE800ApplyCFList( ApplyCFListParams_t* applyCFList )
{
    ChannelParams_t newChannel;
    ChannelAddParams_t channelAdd;
    ChannelRemoveParams_t channelRemove;

    // Setup default datarate range
    newChannel.DrRange.Value = ( DR_5 << 4 ) | DR_0;
    newChannel.Rx1Frequency = 0;

    // Size of the optional CF list
    if ( applyCFList->Size != 16 )
    {
        return;
    }

    // Last byte is RFU, don't take it into account
    for ( uint8_t i = 0, chanIdx = SE800_NUMB_DEFAULT_CHANNELS; i < 15; i += 3, chanIdx++ )
    {
        newChannel.Frequency = (uint32_t) applyCFList->Payload[i];
        newChannel.Frequency |= ( (uint32_t) applyCFList->Payload[i + 1] << 8 );
        newChannel.Frequency |= ( (uint32_t) applyCFList->Payload[i + 2] << 16 );
        newChannel.Frequency *= 100;

        if ( newChannel.Frequency != 0 )
        {
            channelAdd.NewChannel = &newChannel;
            channelAdd.ChannelId = chanIdx;

            // Try to add all channels
            RegionSE800ChannelAdd( &channelAdd );
        }
        else
        {
            channelRemove.ChannelId = chanIdx;

            RegionSE800ChannelsRemove( &channelRemove );
        }
    }
}

bool RegionSE800ChanMaskSet( ChanMaskSetParams_t* chanMaskSet )
{
    switch ( chanMaskSet->ChannelsMaskType )
    {
        case CHANNELS_MASK:
        {
            RegionCommonChanMaskCopy( ChannelsMask, chanMaskSet->ChannelsMaskIn, 1 );
            break;
        }
        case CHANNELS_DEFAULT_MASK:
        {
            RegionCommonChanMaskCopy( ChannelsDefaultMask, chanMaskSet->ChannelsMaskIn, 1 );
            break;
        }
        default:
            return false;
    }
    return true;
}

bool RegionSE800AdrNext( AdrNextParams_t* adrNext, int8_t* drOut, int8_t* txPowOut, uint32_t* adrAckCounter )
{
    bool adrAckReq = false;
    int8_t datarate = adrNext->Datarate;
    int8_t txPower = adrNext->TxPower;

    // Report back the adr ack counter
    *adrAckCounter = adrNext->AdrAckCounter;

    if ( adrNext->AdrEnabled == true )
    {
        if ( datarate == SE800_TX_MIN_DATARATE )
        {
            *adrAckCounter = 0;
            adrAckReq = false;
        }
        else
        {
            if ( adrNext->AdrAckCounter >= SE800_ADR_ACK_LIMIT )
            {
                adrAckReq = true;
            }
            else
            {
                adrAckReq = false;
            }
            if ( adrNext->AdrAckCounter >= ( SE800_ADR_ACK_LIMIT + SE800_ADR_ACK_DELAY ) )
            {
                if ( ( adrNext->AdrAckCounter % SE800_ADR_ACK_DELAY ) == 0 )
                {
                    if ( txPower == SE800_MAX_TX_POWER )
                    {
                        if ( datarate > SE800_TX_MIN_DATARATE )
                        {
                            datarate--;
                        }

                        if ( datarate == SE800_TX_MIN_DATARATE )
                        {
                            if ( adrNext->UpdateChanMask == true )
                            {
                                // Re-enable default channels
                                ChannelsMask[0] = 0x00FF;
                            }
                        }
                    }
                    else
                    {
                        txPower = SE800_MAX_TX_POWER;
                    }
                }
            }
        }
    }

    *drOut = datarate;
    *txPowOut = txPower;
    return adrAckReq;
}

// ToDo get phy datarate afterwards
bool RegionSE800RxConfig( RxConfigParams_t* rxConfig, int8_t* datarate )
{
    int8_t dr = rxConfig->Datarate;
    uint8_t maxPayload = 0;
    uint16_t symbTimeout = 0;
    int8_t phyDr = 0;
    uint32_t frequency = rxConfig->Frequency;

    if ( Radio.GetStatus( ) != RF_IDLE )
    {
        return false;
    }

    if ( rxConfig->Window == 0 )
    {
        // Apply the datarate offset for RX window 1
        dr = ApplyDrOffset( dr, rxConfig->DrOffset );
        // Apply window 1 frequency
        frequency = Channels[rxConfig->Channel].Frequency + 44800000;
        // Apply the alternative RX 1 window frequency, if it is available
        if ( Channels[rxConfig->Channel].Rx1Frequency != 0 )
        {
            frequency = Channels[rxConfig->Channel].Rx1Frequency;
        }
    }
    symbTimeout = GetSymbTimeout( dr );
    // Read the physical datarate from the datarates table
    phyDr = DataratesSE800[dr];

    Radio.SetChannel( frequency );

    Radio.SetRxConfig( MODEM_LORA, 0, phyDr, 1, 0, 8, symbTimeout, false, 0, false, 0, 0, true, rxConfig->RxContinuous );

    if ( rxConfig->RepeaterSupport == true )
    {
        maxPayload = MaxPayloadOfDatarateRepeaterSE800[dr];
    }
    else
    {
        maxPayload = MaxPayloadOfDatarateSE800[dr];
    }
    Radio.SetMaxPayloadLength( MODEM_LORA, maxPayload + LORA_MAC_FRMPAYLOAD_OVERHEAD );

    *datarate = (uint8_t) dr;
    rxConfig->Frequency = frequency;
    return true;
}

bool RegionSE800TxConfig( TxConfigParams_t* txConfig, int8_t* txPower, uint32_t* txTimeOnAir )
{
    RadioModems_t modem;
    int8_t phyDr = DataratesSE800[txConfig->Datarate];
    int8_t phyTxPower = txConfig->MaxEirp - ( txConfig->TxPower * 2U );

    // Setup the radio frequency
    Radio.SetChannel( Channels[txConfig->Channel].Frequency );

    Radio.SetTxConfig( MODEM_LORA, phyTxPower, 0, 0, phyDr, 1, 8, false, true, 0, 0, false, 3000 );
    // Setup maximum payload lenght of the radio driver
    Radio.SetMaxPayloadLength( MODEM_LORA, txConfig->PktLen );
    // Get the time-on-air of the next tx frame
    *txTimeOnAir = Radio.TimeOnAir( modem,  txConfig->PktLen );

    *txPower = txConfig->TxPower;
    txConfig->Frequency = Channels[txConfig->Channel].Frequency;

    trace_debug("TX freq %d ,DR %d, txp %d\n\r", Channels[txConfig->Channel].Frequency, txConfig->Datarate, phyTxPower);

    return true;
}

uint8_t RegionSE800LinkAdrReq( LinkAdrReqParams_t* linkAdrReq, int8_t* drOut, int8_t* txPowOut, uint8_t* nbRepOut, uint8_t* nbBytesParsed )
{
    uint8_t status = 0x07;
    LinkAdrParams_t linkAdrParams;
    uint8_t nextIndex = 0;
    uint8_t bytesProcessed = 0;
    uint16_t chMask = 0;

    while ( bytesProcessed < linkAdrReq->PayloadSize )
    {
        // Get ADR request parameters
        nextIndex = RegionCommonParseLinkAdrReq( &( linkAdrReq->Payload[bytesProcessed] ), &linkAdrParams );

        if ( nextIndex == 0 )
            break; // break loop, since no more request has been found

        // Update bytes processed
        bytesProcessed += nextIndex;

        // Revert status, as we only check the last ADR request for the channel mask KO
        status = 0x07;

        // Setup temporary channels mask
        chMask = linkAdrParams.ChMask;

        // Verify channels mask
        if ( ( linkAdrParams.ChMaskCtrl == 0 ) && ( chMask == 0 ) )
        {
            status &= 0xFE; // Channel mask KO
        }
        else if ( ( ( linkAdrParams.ChMaskCtrl >= 1 ) && ( linkAdrParams.ChMaskCtrl <= 5 )) ||
                  ( linkAdrParams.ChMaskCtrl >= 7 ) )
        {
            // RFU
            status &= 0xFE; // Channel mask KO
        }
        else
        {
            for ( uint8_t i = 0; i < SE800_MAX_NB_CHANNELS; i++ )
            {
                if ( linkAdrParams.ChMaskCtrl == 6 )
                {
                    if ( Channels[i].Frequency != 0 )
                    {
                        chMask |= 1 << i;
                    }
                }
                else
                {
                    if ( ( ( chMask & ( 1 << i ) ) != 0 ) &&
                            ( Channels[i].Frequency == 0 ) )
                    {   // Trying to enable an undefined channel
                        status &= 0xFE; // Channel mask KO
                    }
                }
            }
        }
    }

    // Verify datarate
    if ( RegionCommonChanVerifyDr( SE800_MAX_NB_CHANNELS, &chMask, linkAdrParams.Datarate, SE800_TX_MIN_DATARATE, SE800_TX_MAX_DATARATE, Channels  ) == false )
    {
        status &= 0xFD; // Datarate KO
    }

    // Verify tx power
    if ( RegionCommonValueInRange( linkAdrParams.TxPower, SE800_MAX_TX_POWER, SE800_MIN_TX_POWER ) == 0 )
    {
        // Verify if the maximum TX power is exceeded
        if ( SE800_MAX_TX_POWER > linkAdrParams.TxPower )
        {   // Apply maximum TX power. Accept TX power.
            linkAdrParams.TxPower = SE800_MAX_TX_POWER;
        }
        else
        {
            status &= 0xFB; // TxPower KO
        }
    }

    // Update channelsMask if everything is correct
    if ( status == 0x07 )
    {
        if ( linkAdrParams.NbRep == 0 )
        {   // Value of 0 is not allowed, revert to default.
            linkAdrParams.NbRep = 1;
        }

        // Set the channels mask to a default value
        memset( ChannelsMask, 0, sizeof( ChannelsMask ) );
        // Update the channels mask
        ChannelsMask[0] = chMask;
    }

    // Update status variables
    *drOut = linkAdrParams.Datarate;
    *txPowOut = linkAdrParams.TxPower;
    *nbRepOut = linkAdrParams.NbRep;
    *nbBytesParsed = bytesProcessed;

    return status;
}

uint8_t RegionSE800RxParamSetupReq( RxParamSetupReqParams_t* rxParamSetupReq )
{
    uint8_t status = 0x07;

    // Verify radio frequency
    if ( Radio.CheckRfFrequency( rxParamSetupReq->Frequency ) == false )
    {
        status &= 0xFE; // Channel frequency KO
    }

    // Verify datarate
    if ( RegionCommonValueInRange( rxParamSetupReq->Datarate, SE800_RX_MIN_DATARATE, SE800_RX_MAX_DATARATE ) == false )
    {
        status &= 0xFD; // Datarate KO
    }

    // Verify datarate offset
    if ( RegionCommonValueInRange( rxParamSetupReq->DrOffset, SE800_MIN_RX1_DR_OFFSET, SE800_MAX_RX1_DR_OFFSET ) == false )
    {
        status &= 0xFB; // Rx1DrOffset range KO
    }

    return status;
}

uint8_t RegionSE800NewChannelReq( NewChannelReqParams_t* newChannelReq )
{
    uint8_t status = 0x03;
    ChannelAddParams_t channelAdd;
    ChannelRemoveParams_t channelRemove;

    if ( newChannelReq->NewChannel->Frequency == 0 )
    {
        channelRemove.ChannelId = newChannelReq->ChannelId;

        // Remove
        if ( RegionSE800ChannelsRemove( &channelRemove ) == false )
        {
            status &= 0xFC;
        }
    }
    else
    {
        channelAdd.NewChannel = newChannelReq->NewChannel;
        channelAdd.ChannelId = newChannelReq->ChannelId;

        switch ( RegionSE800ChannelAdd( &channelAdd ) )
        {
            case LORAMAC_STATUS_OK:
            {
                break;
            }
            case LORAMAC_STATUS_FREQUENCY_INVALID:
            {
                status &= 0xFE;
                break;
            }
            case LORAMAC_STATUS_DATARATE_INVALID:
            {
                status &= 0xFD;
                break;
            }
            case LORAMAC_STATUS_FREQ_AND_DR_INVALID:
            {
                status &= 0xFC;
                break;
            }
            default:
            {
                status &= 0xFC;
                break;
            }
        }
    }

    return status;
}

int8_t RegionSE800TxParamSetupReq( TxParamSetupReqParams_t* txParamSetupReq )
{
    return 0;
}

uint8_t RegionSE800DlChannelReq( DlChannelReqParams_t* dlChannelReq )
{
    uint8_t status = 0x03;

    // Verify if the frequency is supported
    if ( VerifyTxFreq( dlChannelReq->Rx1Frequency ) == false )
    {
        status &= 0xFE;
    }

    // Verify if an uplink frequency exists
    if ( Channels[dlChannelReq->ChannelId].Frequency == 0 )
    {
        status &= 0xFD;
    }

    // Apply Rx1 frequency, if the status is OK
    if ( status == 0x03 )
    {
        Channels[dlChannelReq->ChannelId].Rx1Frequency = dlChannelReq->Rx1Frequency;
    }

    return status;
}

int8_t RegionSE800AlternateDr( AlternateDrParams_t* alternateDr )
{
    int8_t datarate = 0;

// wanghuayuan
#if 0
    if ( ( alternateDr->NbTrials % 48 ) == 0 )
    {
        datarate = DR_0;
    }
    else if ( ( alternateDr->NbTrials % 32 ) == 0 )
    {
        datarate = DR_1;
    }
    else if ( ( alternateDr->NbTrials % 24 ) == 0 )
    {
        datarate = DR_2;
    }
    else if ( ( alternateDr->NbTrials % 16 ) == 0 )
    {
        datarate = DR_3;
    }
    else if ( ( alternateDr->NbTrials % 8 ) == 0 )
    {
        datarate = DR_4;
    }
    else
    {
        datarate = DR_5;
    }
#endif

    if ( alternateDr->NbTrials <= 2 )
    {
        datarate = DR_3;
    }
    else if ( alternateDr->NbTrials <= 4 )
    {
        datarate = DR_2;
    }
    else if ( alternateDr->NbTrials <= 6 )
    {
        datarate = DR_1;
    }
    else
    {
        datarate = DR_0;
    }

    trace_debug("NbTrials = %d, AlternateDr = %d\r\n", alternateDr->NbTrials, datarate);
    return datarate;
}

void RegionSE800CalcBackOff( CalcBackOffParams_t* calcBackOff )
{
    uint8_t channel = calcBackOff->Channel;
    uint16_t joinDutyCycle = 0;

    // Reset time-off to initial value.
    Bands[Channels[channel].Band].TimeOff = 0;

    if ( calcBackOff->Joined == false )
    {
        // Get the join duty cycle
        joinDutyCycle = RegionCommonGetJoinDc( calcBackOff->ElapsedTime );
        // Apply band time-off.
        Bands[Channels[channel].Band].TimeOff = calcBackOff->TxTimeOnAir * joinDutyCycle - calcBackOff->TxTimeOnAir;
    }
    else
    {
        Bands[Channels[channel].Band].TimeOff = 0;
    }
}

bool RegionSE800NextChannel( NextChanParams_t* nextChanParams, uint8_t* channel, uint32_t* time )
{
    uint8_t channelNext = 0;
    uint8_t nbEnabledChannels = 0;
    uint8_t delayTx = 0;
    uint8_t enabledChannels[SE800_MAX_NB_CHANNELS] = { 0 };
    uint32_t nextTxDelay = 0;
    uint32_t carrierSenseTime = 0;
    bool channelFree = false;

    if ( RegionCommonCountChannels( ChannelsMask, 0, 1 ) == 0 )
    {   // Reactivate default channels
        ChannelsMask[0] = 0x00FF;
    }

    if ( nextChanParams->AggrTimeOff <= ELAPSED_TIME_GET( nextChanParams->LastAggrTx ) )
    {
        // Update bands Time OFF
        nextTxDelay = RegionCommonUpdateBandTimeOff( nextChanParams->DutyCycleEnabled, Bands, SE800_MAX_NB_BANDS );

        // Search how many channels are enabled
        nbEnabledChannels = CountNbOfEnabledChannels( nextChanParams->Joined, nextChanParams->Datarate,
                            ChannelsMask, Channels,
                            Bands, enabledChannels, &delayTx );
    }
    else
    {
        delayTx++;
        nextTxDelay = nextChanParams->AggrTimeOff - ELAPSED_TIME_GET( nextChanParams->LastAggrTx );
    }

    if ( nbEnabledChannels > 0 )
    {
        if (nextChanParams->LbtEnabled)
        {
            for ( uint8_t  i = 0, j = randr( 0, nbEnabledChannels - 1 ); i < nbEnabledChannels; i++ )
            {
                channelNext = enabledChannels[j];
                j = ( j + 1 ) % nbEnabledChannels;

                carrierSenseTime = RUN_TIME_GET( );
                channelFree = true;

                // Perform carrier sense for 6ms
                while ( ELAPSED_TIME_GET( carrierSenseTime ) < SE800_CARRIER_SENSE_TIME )
                {
                    if ( Radio.IsChannelFree( MODEM_LORA, Channels[channelNext].Frequency, nextChanParams->LbtThreshold ) == false )
                    {
                        channelFree = false;
                        break;
                    }
                }

                // If the channel is free, we can stop the LBT mechanism
                if ( channelFree == true )
                {
                    lbtRetryTimes = 0;
                    // Free channel found
                    *channel = channelNext;
                    *time = 0;
                    return true;
                }
            }

            lbtRetryTimes ++;
            trace_debug("retry times:%d\r\n", lbtRetryTimes);

            if (lbtRetryTimes == nextChanParams->LbtRetryTimes)
            {
                lbtRetryTimes = 0;

                *channel = enabledChannels[randr( 0, nbEnabledChannels - 1 )];

                *time = 0;
                return true;
            }
            else
            {
                // wanghuayuan
                *time = 1000;
                return true;

            }
        }
        else
        {
            // We found a valid channel
            *channel = enabledChannels[randr( 0, nbEnabledChannels - 1 )];

            *time = 0;
            return true;
        }
    }
    else
    {
        if ( delayTx > 0 )
        {
            // Delay transmission due to AggregatedTimeOff or to a band time off
            *time = nextTxDelay;
            return true;
        }
        // Datarate not supported by any channel, restore defaults
        ChannelsMask[0] = 0x00FF;
        *time = 0;
        return false;
    }
}

LoRaMacStatus_t RegionSE800ChannelAdd( ChannelAddParams_t* channelAdd )
{
    uint8_t band = 0;
    bool drInvalid = false;
    bool freqInvalid = false;
    uint8_t id = channelAdd->ChannelId;

    if ( id >= SE800_MAX_NB_CHANNELS )
    {
        return LORAMAC_STATUS_PARAMETER_INVALID;
    }

    // Validate the datarate range
    if ( RegionCommonValueInRange( channelAdd->NewChannel->DrRange.Fields.Min, SE800_TX_MIN_DATARATE, SE800_TX_MAX_DATARATE ) == false )
    {
        drInvalid = true;
    }
    if ( RegionCommonValueInRange( channelAdd->NewChannel->DrRange.Fields.Max, SE800_TX_MIN_DATARATE, SE800_TX_MAX_DATARATE ) == false )
    {
        drInvalid = true;
    }
    if ( channelAdd->NewChannel->DrRange.Fields.Min > channelAdd->NewChannel->DrRange.Fields.Max )
    {
        drInvalid = true;
    }

    // Check frequency
    if ( freqInvalid == false )
    {
        if ( VerifyTxFreq( channelAdd->NewChannel->Frequency ) == false )
        {
            freqInvalid = true;
        }
    }

    // Check status
    if ( ( drInvalid == true ) && ( freqInvalid == true ) )
    {
        return LORAMAC_STATUS_FREQ_AND_DR_INVALID;
    }
    if ( drInvalid == true )
    {
        return LORAMAC_STATUS_DATARATE_INVALID;
    }
    if ( freqInvalid == true )
    {
        return LORAMAC_STATUS_FREQUENCY_INVALID;
    }

    memcpy( &(Channels[id]), channelAdd->NewChannel, sizeof( Channels[id] ) );
    trace_verbose("freq = %d, rx1 freq = %d\r\n", Channels[id].Frequency, Channels[id].Rx1Frequency);
    Channels[id].Band = band;
    ChannelsMask[0] |= ( 1 << id );
    return LORAMAC_STATUS_OK;
}

bool RegionSE800ChannelsRemove( ChannelRemoveParams_t* channelRemove  )
{
    uint8_t id = channelRemove->ChannelId;

    // Remove the channel from the list of channels
    Channels[id] = ( ChannelParams_t ) { 0, 0, { 0 }, 0 };

    return RegionCommonChanDisable( ChannelsMask, id, SE800_MAX_NB_CHANNELS );
}

void RegionSE800SetContinuousWave( ContinuousWaveParams_t* continuousWave )
{
    int8_t phyTxPower = continuousWave->MaxEirp - ( continuousWave->TxPower * 2U );
    uint32_t frequency = Channels[continuousWave->Channel].Frequency;

    Radio.SetTxContinuousWave( frequency, phyTxPower, continuousWave->Timeout );
}
