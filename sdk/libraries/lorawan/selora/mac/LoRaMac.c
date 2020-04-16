/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: LoRa MAC layer implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "radio.h"
#include "LoRaMacCrypto.h"
#include "LoRaMac.h"
#include "app_timer.h"
#include "app_error.h"
#include "nordic_common.h"
#include "app_mailbox.h"
#include "app_util.h"
#include "sx1276Regs-LoRa.h"
#include "app_scheduler.h"
#include "run_time.h"
#include "pstorage.h"
#include "LoRaMacFacCfg.h"

#include "debug_config.h"
#ifdef LORAMAC_DEBUG
    #include "trace.h"
#else
    #include "trace_disable.h"
#endif

#define RANDR(min, max) ((int32_t)rand() % (max - min + 1) + min)

#define TIMER_CREATE(p_id, mode, handler)                                           \
{                                                                                   \
    uint32_t err_code;                                                              \
    err_code = app_timer_create(p_id, mode, handler);                                   \
    APP_ERROR_CHECK(err_code);                                                      \
}                                                                                   \

#define TIMER_START(id, ms)                                                         \
{                                                                                   \
    uint32_t err_code;                                                              \
    err_code = app_timer_start(id, APP_TIMER_TICKS(ms, 0), NULL);                   \
    APP_ERROR_CHECK(err_code);                                                      \
}                                                                                   \

#define TIMER_STOP(id)                                                              \
{                                                                                   \
    uint32_t err_code;                                                              \
    err_code = app_timer_stop(id);                                                  \
    APP_ERROR_CHECK(err_code);                                                      \
}                                                                                   \


#define LORAMAC_VERSION                     0X14
#define LORAMAC_STORAGE_MAGIC_BYTE_05       0X05
#define LORAMAC_STORAGE_MAGIC_BYTE_06       0X06
#define LORAMAC_STORAGE_MAGIC_BYTE_V140     LORAMAC_STORAGE_MAGIC_BYTE_06

#define LORA_MAX_NB_CHANNELS        16
#define LORAMAC_MIN_DATARATE        DR_0
#define LORAMAC_MAX_DATARATE        DR_5
#define LORAMAC_DEFAULT_DATARATE    DR_1
#define LORAMAC_MIN_TX_POWER        TX_POWER_02_DBM
#define LORAMAC_MAX_TX_POWER        TX_POWER_20_DBM
#define LORAMAC_DEFAULT_TX_POWER    TX_POWER_14_DBM

#define LC( channelIndex )          ( uint16_t )( 1 << ( channelIndex - 1 ) )

#define TX_POWER_20_DBM             0
#define TX_POWER_17_DBM             1
#define TX_POWER_16_DBM             2
#define TX_POWER_15_DBM             3
#define TX_POWER_14_DBM             4
#define TX_POWER_13_DBM             5
#define TX_POWER_12_DBM             6
#define TX_POWER_11_DBM             7
#define TX_POWER_10_DBM             8
#define TX_POWER_09_DBM             9
#define TX_POWER_08_DBM             10
#define TX_POWER_07_DBM             11
#define TX_POWER_06_DBM             12
#define TX_POWER_05_DBM             13
#define TX_POWER_04_DBM             14
#define TX_POWER_02_DBM             15

#define DR_0                        0  // SF12 - BW125
#define DR_1                        1  // SF11 - BW125
#define DR_2                        2  // SF10 - BW125
#define DR_3                        3  // SF9  - BW125
#define DR_4                        4  // SF8  - BW125
#define DR_5                        5  // SF7  - BW125
#define DR_6                        6  // SF7  - BW250
#define DR_7                        7  // FSK

#define BEACON_PAYLOAD_LEN          17
#define LORA_MAX_NB_BANDS           1

//*****************************************

#define RECEIVE_DELAY1                              1000
#define RECEIVE_DELAY2                              2000
#define JOIN_ACCEPT_DELAY1                          5000
#define JOIN_ACCEPT_DELAY2                          6000
#define MAX_RX_WINDOW                               3000
#define MAX_FCNT_GAP                                16384
#define ADR_ACK_LIMIT                               64
#define ADR_ACK_DELAY                               32
#define ACK_TIMEOUT                                 2000
#define ACK_TIMEOUT_RND                             1000
#define MAC_STATE_CHECK_TIMEOUT                     1000
#define MAX_ACK_RETRIES                             8
#define RSSI_FREE_TH                                (int8_t)( -90 ) // [dBm]
#define UP_LINK                                     0
#define DOWN_LINK                                   1
#define LORAMAC_MFR_LEN                             4
#define LORA_MAC_PRIVATE_SYNCWORD                   0x12
#define LORA_MAC_PUBLIC_SYNCWORD                    0x34


#define RAND_SEED                                   (*(uint32_t *)0x100000a4) & 0x00ffffff
#define LORAMAC_PHY_MAXPAYLOAD                      64

#define RADIO_WAKEUP_TIME                           1 // [ms]

#define BEACON_RESERVED                             2120
#define BEACON_PERIOD                               128000
#define BEACON_GUARD                                3000
#define BEACON_WINDOW                               122880

#define BEACON_TIME_ON_AIR                          165

typedef struct{
  uint32_t FreqUp;
  uint32_t FreqDown;
  uint8_t DRMax;
  uint8_t DRMin;
  uint8_t DutyCycle;
}ChannelMagic05_t;

typedef struct
{
  uint8_t magic_byte;
  uint8_t DevEUI[8];
  uint8_t AppEUI[8];
  uint8_t AppKey[16];
  uint8_t AppSKey[16];
  uint8_t NwkSKey[16];
  uint32_t DevAddr;
  uint32_t NetID;
  uint32_t AppNonce;
  uint16_t DevNonce;
  ChannelMagic05_t AlohaChannel[16];
  ChannelMagic05_t PingChannel;
  uint8_t Adr;
  int8_t MaxTXPower;
  int8_t TXPower;
  uint8_t DataRate;
  uint16_t ChMask;
  uint8_t ChMaskCntl;
  uint8_t NbTrans;
  uint8_t MaxDCycle;
  uint8_t RX1DROffset;
  uint32_t RX2Freq;
  uint8_t RX2DateRate;
  uint8_t Del;
  uint8_t ClassBEnable;
  uint8_t PingPeriodicity;
  uint8_t PingDataRate;
  uint32_t BeaconFreq;
  uint8_t BeaconDataRate;
}_PACKED LoRaParamMagic05_t;


typedef struct{
  uint32_t FreqUp;
  uint32_t FreqDown;
  uint8_t DRMax;
  uint8_t DRMin;
  uint8_t Band;
}Channel_t;

typedef struct{
    uint16_t    DCycle;
    int8_t      MaxTxPower;
}RegBand_t;

typedef struct
{
  uint8_t magic_byte;
  uint8_t DevEUI[8];
  uint8_t AppEUI[8];
  uint8_t AppKey[16];
  uint8_t AppSKey[16];
  uint8_t NwkSKey[16];
  uint32_t DevAddr;
  uint32_t NetID;
  uint32_t AppNonce;
  uint16_t DevNonce;
    
  uint8_t Activation;
  uint8_t ClassType;
    
  uint8_t   BandLen;
  RegBand_t Band[10];
  Channel_t AlohaChannel[16];
  
  uint8_t Adr;
  int8_t MaxTXPower;
  int8_t TXPower;
  uint8_t DataRate;
  
  uint16_t ChMasks[7];
  
  uint8_t NbTrans;
  uint8_t MaxDCycle;
  uint8_t RX1DROffset;
  uint32_t RX2Freq;
  uint8_t RX2DateRate;
  uint8_t Del;
  uint8_t ClassBEnable;
  
  Channel_t PingChannel;
  uint8_t PingPeriodicity;
  uint8_t PingDataRate;
  uint32_t BeaconFreq;
  uint8_t BeaconDataRate;
  
  Channel_t ClassGChannel;
  uint8_t ClassGEnabled;
  uint8_t ClassGPeriod;
  uint8_t ClassGDataRate;
  
}LoRaParamLayout_t;

typedef union
{
    LoRaParamLayout_t db;
    uint32_t padding[CEIL_DIV(sizeof(LoRaParamLayout_t), 4)];
}LoRaParam_t;
    
typedef struct
{
    uint32_t lsb;
    uint16_t msb;
}LoRaFcntLayout_t;

typedef union
{
    LoRaFcntLayout_t db;
    uint32_t padding[CEIL_DIV(sizeof(LoRaFcntLayout_t), 4)];
}LoRaFcnt_t;





typedef struct
{
    uint32_t Frequency; // Hz
    uint8_t  Datarate;  // [0: SF12, 1: SF11, 2: SF10, 3: SF9, 4: SF8, 5: SF7, 6: SF7, 7: FSK]
} RxChannelParams_t;

typedef struct MulticastParams_s
{
    uint32_t Address;
    uint8_t NwkSKey[16];
    uint8_t AppSKey[16];
    uint32_t DownLinkCounter;
    struct MulticastParams_s *Next;
} MulticastParams_t;


/*!
 * LoRaMAC frame types
 */
typedef enum
{
    FRAME_TYPE_JOIN_REQ              = 0x00,
    FRAME_TYPE_JOIN_ACCEPT           = 0x01,
    FRAME_TYPE_DATA_UNCONFIRMED_UP   = 0x02,
    FRAME_TYPE_DATA_UNCONFIRMED_DOWN = 0x03,
    FRAME_TYPE_DATA_CONFIRMED_UP     = 0x04,
    FRAME_TYPE_DATA_CONFIRMED_DOWN   = 0x05,
    FRAME_TYPE_RFU                   = 0x06,
    FRAME_TYPE_PROPRIETARY           = 0x07,
} LoRaMacFrameType_t;

/*!
 * LoRaMAC mote MAC commands
 */
typedef enum
{
    MOTE_MAC_LINK_CHECK_REQ          = 0x02,
    MOTE_MAC_LINK_ADR_ANS            = 0x03,
    MOTE_MAC_DUTY_CYCLE_ANS          = 0x04,
    MOTE_MAC_RX_PARAM_SETUP_ANS      = 0x05,
    MOTE_MAC_DEV_STATUS_ANS          = 0x06,
    MOTE_MAC_NEW_CHANNEL_ANS         = 0x07,
    MOTE_MAC_RX_TIMING_SETUP_ANS     = 0x08,
    MOTE_MAC_TX_PARAM_SETUP_ANS      = 0x09,
    MOTE_MAC_DI_CHANNEL_ANS          = 0x0A,
    MOTE_MAC_PING_SLOT_INFO_REQ      = 0x10,
    MOTE_MAC_PING_SLOT_FREQ_ANS      = 0x11,
    MOTE_MAC_BEACON_TIMING_REQ       = 0x12,
    MOTE_MAC_BEACON_FREQ_ANS         = 0x13,
    MOTE_MAC_INTERVAL_LIMIT_ANS      = 0xFE,
    MOTE_MAC_CUS_STATUS              = 0xFD,
    MOTE_MAC_DR_CHECK_REQ            = 0xFC,
    MOTE_MAC_CUS_PING_SLOT_INFO_ANS  = 0xFB,
    MOTE_MAC_VERSION_REQ             = 0xFA,
    MOTE_MAC_CLASSG_PARAM_ANS        = 0xF9,
    MOTE_MAC_CLASSG_CHANNEL_ANS      = 0xF8,
} LoRaMacMoteCmd_t;

/*!
 * LoRaMAC server MAC commands
 */
typedef enum
{
    SRV_MAC_LINK_CHECK_ANS           = 0x02,
    SRV_MAC_LINK_ADR_REQ             = 0x03,
    SRV_MAC_DUTY_CYCLE_REQ           = 0x04,
    SRV_MAC_RX_PARAM_SETUP_REQ       = 0x05,
    SRV_MAC_DEV_STATUS_REQ           = 0x06,
    SRV_MAC_NEW_CHANNEL_REQ          = 0x07,
    SRV_MAC_RX_TIMING_SETUP_REQ      = 0x08,
    SRV_MAC_TX_PARAM_SETUP_REQ       = 0x09,
    SRV_MAC_DI_CHANNEL_REQ           = 0x0A,
    SRV_MAC_PING_SLOT_INFO_ANS       = 0x10,
    SRV_MAC_PING_SLOT_CHANNEL_REQ    = 0x11,
    SRV_MAC_BEACON_TIMING_ANS        = 0x12,
    SRV_MAC_BEACON_FREQ_REQ          = 0x13,
    SRV_MAC_CMD_ANS                  = 0xff,
    SRV_MAC_INTERVAL_LIMIT_REQ       = 0xfe,
    SRV_MAC_DR_CHECK_ANS             = 0xfc,
    SRV_MAC_CUS_PING_SLOT_INFO_REQ   = 0xFB,
    SRV_MAC_VERSION_ANS              = 0xFA,
    SRV_MAC_CLASSG_PARAM_REQ         = 0xF9,
    SRV_MAC_CLASSG_CHANNEL_REQ       = 0xF8,
} LoRaMacSrvCmd_t;

/*!
 * LoRaMAC Battery level indicator
 */
typedef enum
{
    BAT_LEVEL_EXT_SRC                = 0x00,
    BAT_LEVEL_EMPTY                  = 0x01,
    BAT_LEVEL_FULL                   = 0xFE,
    BAT_LEVEL_NO_MEASURE             = 0xFF,
} LoRaMacBatteryLevel_t;

/*!
 * LoRaMAC header field definition
 */
typedef union
{
    uint8_t Value;
    struct
    {
        uint8_t Major           : 2;
        uint8_t RFU             : 3; 
        uint8_t MType           : 3;
    }_PACKED Bits;
} LoRaMacHeader_t;

/*!
 * LoRaMAC frame header field definition
 */
typedef union
{
    uint8_t Value;
    struct
    {
        uint8_t FOptsLen        : 4;
        uint8_t FPending        : 1;
        uint8_t Ack             : 1;
        uint8_t AdrAckReq       : 1;
        uint8_t Adr             : 1;
    }_PACKED Bits;
} LoRaMacFrameCtrl_t;

typedef struct
{
    uint8_t evt;
    uint8_t data[64];
    uint8_t data_len;
    int8_t rssi;
    int8_t snr;
}LoRaMacRadioEvent_t;

enum
{
    LORAMAC_RADIO_EVT_NONE = 0,
    LORAMAC_RADIO_EVT_ALOHA_TX_DONE,
    LORAMAC_RADIO_EVT_ALOHA_TX_TIMEOUT,
    LORAMAC_RADIO_EVT_ALOHA_RX_DONE,
    LORAMAC_RADIO_EVT_ALOHA_RX_TIMEOUT,
    LORAMAC_RADIO_EVT_ALOHA_RX_ERROR,
    LORAMAC_RADIO_EVT_BEACON_RX_DONE,
    LORAMAC_RADIO_EVT_BEACON_RX_TIMEOUT,
    LORAMAC_RADIO_EVT_BEACON_RX_ERROR,
    LORAMAC_RADIO_EVT_PING_RX_DONE,
    LORAMAC_RADIO_EVT_PING_RX_TIMEOUT,
    LORAMAC_RADIO_EVT_PING_RX_ERROR,
    LORAMAC_RADIO_EVT_CLASSC_RX_DONE,
    LORAMAC_RADIO_EVT_CLASSC_RX_ERROR,
    LORAMAC_RADIO_EVT_CLASSG_RX_DONE,
    LORAMAC_RADIO_EVT_CLASSG_RX_TIMEOUT,
    LORAMAC_RADIO_EVT_CLASSG_RX_ERROR,
};

enum LoRaMacState_t
{
    MAC_STATE_IDLE          = 0,
    MAC_STATE_ALOHA         = 0x0001,
    MAC_STATE_BEACON_RX     = 0x0002,  
    MAC_STATE_BEACON_GUARD  = 0x0004,
    MAC_STATE_PING_RX       = 0x0008,
    MAC_STATE_CLASSC_RX     = 0x0010,
    MAC_STATE_CLASSG_RX     = 0x0020,
};

enum LoRaMacAlohaState_t
{
    MAC_ALOHA_IDLE          = 0x00000000,
    MAC_ALOHA_TX_RUNNING    = 0x00000001,
    MAC_ALOHA_RX            = 0x00000002,
    MAC_ALOHA_ACK_REQ       = 0x00000004,
    MAC_ALOHA_ACK_RETRY     = 0x00000008,
    MAC_ALOHA_CHANNEL_CHECK = 0x00000010,
};


APP_MAILBOX_DEF(Mailbox, 5, sizeof(LoRaMacRadioEvent_t));

static uint8_t                  PingCount = 0;

static uint8_t                  BeaconSymbTimeout = 25;
static uint32_t                 BeaconGPSTime = 0;
static uint16_t                 BeaconStartOffset = 0;
static uint8_t                  BeaconLostCount = 0;
static bool                     IsBeaconSearching = false;
static uint16_t                 LoRaMacState = MAC_STATE_IDLE;
static bool                     LoRaMacAlohaPending = false;
static bool                     PingSlotInfoAnsed = false;
static bool                     VersionAnsed = false;

static uint32_t                 LastTicks = 0;
static uint32_t                 FirstPingDelay = 0;
static uint32_t                 RxTimeOnAir = 0;

static LoRaMacEventRxInfo_t     EventRxInfo;
static bool                     RxDone = false;

//*********************************************************
static Band_t               Bands[LORA_MAX_NB_BANDS];
static ChannelParams_t      Channels[LORA_MAX_NB_CHANNELS];

static MulticastParams_t*   MulticastChannels = NULL;
static DeviceClass_t        DeviceClass;
static uint8_t              LoRaMacBuffer[LORAMAC_PHY_MAXPAYLOAD];
static uint16_t             LoRaMacBufferPktLen = 0;
static uint8_t              LoRaMacRxPayload[LORAMAC_PHY_MAXPAYLOAD];
static uint16_t             UpLinkCounter = 1;
static uint16_t             DownLinkCounter = 0;
static bool                 IsLoRaMacNetworkJoined = false;

static uint32_t             AdrAckCounter = 0;
static bool                 NodeAckRequested = false;
static bool                 SrvAckRequested = false;

static bool                 MacCmdsInNextTx = false;
static uint8_t              MacCmdsBufferIdx = 0;
static uint8_t              MacCmdsBuffer[10];

static bool                 MacCmdsAnsInNextTx = false;
static uint8_t              MacCmdsDelayAnsBufferIdx = 0;
static uint8_t              MacCmdsDelayAnsBuffer[10];

static bool                 MacCmdsImmAnsInNextTx = false;
static uint8_t              MacCmdsImmAnsBufferIdx = 0;
static uint8_t              MacCmdsImmAnsBuffer[10];

static bool                 MacCmdsDrCheck = false;
static bool                 MacCmdsDrCheckAnsWait = false;
static uint8_t              MacCmdsDrCheckAnsWaitCounter = 0;

const uint8_t               Datarates[] = { 12, 11, 10,  9,  8,  7,  7, 50 };
const uint16_t              SymbTime[] = {32768, 16384, 8192, 4096, 2048, 1024, 1024};
const int8_t                TxPowers[16] = { 20, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 2 };

static int8_t               ChannleDatarate = LORAMAC_DEFAULT_DATARATE;
static uint8_t              ChannelsNbRepCounter = 0;

static uint16_t             AggregatedDCycle;
static uint32_t             AggregatedLastTxDoneTime;
static uint32_t             AggregatedTimeOff;

static bool                 DutyCycleOn = false;
static uint8_t              Channel;

static uint32_t                 LoRaMacAlohaState = MAC_ALOHA_IDLE;
static LoRaMacEvent_t*          LoRaMacEvents;
static LoRaMacEventFlags_t      LoRaMacEventFlags;
static LoRaMacEventAlohaInfo_t  LoRaMacEventAlohaInfo;
static RadioEvents_t            RadioEvents;

static LoRaParam_t          LoRaParam;

static uint32_t             JoinAcceptDelay1;
static uint32_t             RxWindow1Delay;
static uint32_t             MaxRxWindow;

static uint8_t              AckTimeoutRetries = 1;
static uint8_t              AckTimeoutRetriesCounter = 1;
static bool                 AckTimeoutRetry = false;

static uint32_t             TxTimeOnAir = 0;

static uint8_t              Rx1DatarateNew = UINT8_MAX;
static uint8_t              Rx2DatarateNew = UINT8_MAX;
static int8_t               TxPowerNew = INT8_MAX;
static uint32_t             ChMaskNew = UINT32_MAX;
static uint8_t              NbTransNew = UINT8_MAX;
static uint8_t              Rx1DROffsetNew = UINT8_MAX;
static uint32_t             Rx2FreqNew = UINT32_MAX;

static uint8_t              PingDatarateNew = UINT8_MAX;
static uint8_t              ClassBEnableNew = UINT8_MAX;
static uint8_t              PingPrdNew = UINT8_MAX;

static uint8_t              ClassGEnabledNew = UINT8_MAX;
static uint8_t              ClassGPrdNew = UINT8_MAX;
static uint8_t              ClassGDatarateNew = UINT8_MAX;

static uint32_t             LastTxTime = 0;
static uint32_t             SilenceTime = 0;

static bool                 ClassGScheduling = false;

static pstorage_handle_t    FcntDbBlockId;
static pstorage_handle_t    LoRaParamBlockId;



APP_TIMER_DEF(AckTimeoutTimer);
APP_TIMER_DEF(ChannelCheckTimer);
//APP_TIMER_DEF(TxDelayedTimer;
APP_TIMER_DEF(RxWindowTimer1);
APP_TIMER_DEF(RxWindowTimer2);
APP_TIMER_DEF(BeaconTimer);
APP_TIMER_DEF(PingTimer);
APP_TIMER_DEF(RadioTimeoutTimer);
APP_TIMER_DEF(ClassGTimer);

static void LoRaMacVersionChange(void);

static void OnBeaconTimerEvent(void* p_context);
static void OnPingTimerEvent(void* p_context);
static void OnRadioTimeoutEvent(void *p_context);

static void OnRadioTxDone(void);
static void OnRadioRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr);
static void OnRadioTxTimeout(void);
static void OnRadioRxError(void);
static void OnRadioRxTimeout(void);
static void OnMacStateCheckTimerEvent(void* p_context);

//static void OnTxDelayedTimerEvent( void *p_context );
static void OnChannelCheckTimerEvent(void* p_context);
static void OnRxWindow1TimerEvent(void* p_context);
static void OnRxWindow2TimerEvent(void* p_context);
static void OnAckTimeoutTimerEvent(void* p_context);
static void OnClassGTimerEvent(void * p_context);

static uint8_t LoRaMacSendFrame(ChannelParams_t channel);
static uint8_t LoRaMacSendPacket(LoRaMacHeader_t* macHdr, 
                          uint8_t* fOpts, 
                          uint8_t fPort, 
                          void* fBuffer, 
                          uint16_t fBufferSize);
static uint8_t LoRaMacPrepareFrame(LoRaMacHeader_t* macHdr, 
                                   LoRaMacFrameCtrl_t* fCtrl, 
                                   uint8_t* fOpts, 
                                   uint8_t fPort, 
                                   void* fBuffer, 
                                   uint16_t fBufferSize);
static void LoRaMacRxWindowSetup(uint8_t rxSlot, bool rxContinuous);
static void LoRaMacClassGStart(void);

static void pstorage_ntf_cb(pstorage_handle_t*   p_handle,
                            uint8_t              op_code,
                            uint32_t             result,
                            uint8_t*             p_data,
                            uint32_t             data_len)
{
    APP_ERROR_CHECK(result);
}

/**@brief 将fcnt存到flash中
 *
 *@detail 由于fcnt每次加1都要记录,为了防止flash多次擦写失效,使用LoRaFcnt_t结构来存, 
 *         msb存放fcnt/32, lsb存放fcnt%32,fcnt%32为多少,则lsb存多少个0,如fcnt%32=2,
 *         lsb = 0b......1111 1100, fcnt%32=8,lsb = 0b......1111 0000 0000. 
 *         当fcnt%32 = 0时,擦一次flash.
 *         用这样的方法实现fcnt每增加32存一次
 */
static void LoRaMacStorageFcntUpdate(uint16_t fcnt)
{
    uint32_t err_code;
    static LoRaFcnt_t fcnt_db;
    uint8_t lsb_bits = 0;

    memset(&fcnt_db, 0, sizeof(LoRaFcnt_t));

    if(fcnt >= 32)
    {
        fcnt_db.db.msb = fcnt >> 5;
    }
    lsb_bits = fcnt & 0x1f;

    trace_verbose("msb = %d\r\n", fcnt_db.db.msb);
    trace_verbose("lsb_bits = %d\r\n", lsb_bits);

    if(0 != lsb_bits)
    {
        for(uint8_t i = 0; i < lsb_bits; i++)
        {
            fcnt_db.db.lsb |= (uint32_t)(1 << i);
        }
    }
    else
    {
        err_code = pstorage_clear(&FcntDbBlockId, sizeof(LoRaFcnt_t));
        APP_ERROR_CHECK(err_code);
    }
    fcnt_db.db.lsb = ~fcnt_db.db.lsb;
    trace_verbose("fcnt_db.lsb = %lx\r\n", fcnt_db.db.lsb);

    err_code = pstorage_store(&FcntDbBlockId, (uint8_t*)&fcnt_db, sizeof(LoRaFcnt_t), 0);
    APP_ERROR_CHECK(err_code);
}

static uint16_t LoRaMacStorageFcntGet(void)
{
    uint16_t lsb_bits = 0;
    uint32_t lsb = 0;
    LoRaFcnt_t* p_fcnt_db = (LoRaFcnt_t*)FcntDbBlockId.block_id;

    trace_verbose("lsb = %lx\r\n", p_fcnt_db->db.lsb);
    if(p_fcnt_db->db.lsb == 0xffffffff && p_fcnt_db->db.msb == 0xffff)
    {
        return 0;
    }

    lsb = ~p_fcnt_db->db.lsb;

    for(uint8_t i = 0; i < 32; i++)
    {
        if((lsb & (1 << i)) != 0)
        {
            lsb_bits++;
        }
        else
        {
            break;
        }
    }
    trace_verbose("msb = %d\r\n", p_fcnt_db->db.msb);
    trace_verbose("lsb_bits = %d\r\n", lsb_bits);

    return ((p_fcnt_db->db.msb << 5) + lsb_bits);
}

void LoRaMacUpdateConfig(void)
{
    uint32_t err_code;
    err_code = pstorage_clear(&LoRaParamBlockId, sizeof(LoRaParam_t));
    APP_ERROR_CHECK(err_code);

    err_code = pstorage_store(&LoRaParamBlockId, (uint8_t*)&LoRaParam, sizeof(LoRaParam_t), 0);
    APP_ERROR_CHECK(err_code);
}

void LoRaMacResetConfig(void)
{
    LoRaParam_t *p_fac = (LoRaParam_t *)STORAGE_ADDR_FAC_LORA_PARAM;
    memcpy(&LoRaParam, p_fac, sizeof(LoRaParam_t));
        
    if(LoRaParam.db.MaxDCycle > 15)
    {
        LoRaParam.db.MaxDCycle = 0;
    }
        
    ChannleDatarate = LoRaParam.db.DataRate;
    ChannelsNbRepCounter = 0;

    AggregatedDCycle = (1 << LoRaParam.db.MaxDCycle);
    AggregatedLastTxDoneTime = 0;
    AggregatedTimeOff = 0;

    DutyCycleOn = false;
    MaxRxWindow = MAX_RX_WINDOW;
    JoinAcceptDelay1 = JOIN_ACCEPT_DELAY1;
    
    Channel = LORA_MAX_NB_CHANNELS;
    
    DeviceClass = (DeviceClass_t)LoRaParam.db.ClassType;
}

static uint8_t crc8_ccitt(const uint8_t *data, unsigned size)
{
    uint8_t crc_poly = 0x87; /* CCITT */
    uint8_t init_val = 0xFF; /* CCITT */
    uint8_t x = init_val;
    unsigned i, j;
    if (data == NULL)  {
        return 0;
    }
    for (i = 0; i < size; ++i) {
        x ^= data[i];
        for (j = 0; j < 8; ++j) {
            x = (x & 0x80) ? (x << 1) ^ crc_poly : (x << 1);
        }
    }
    return x;
}

/*!
 * Searches and set the next random available channel
 *
 * \retval status  Function status [0: OK, 1: Unable to find a free channel]
 */
static uint8_t LoRaMacSetNextChannel(void)
{
    trace_verbose("LoRaMacSetNextChannel\r\n");
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t channelNext = Channel;
    uint8_t nbEnabledChannels = 0;
    uint8_t enabledChannels[LORA_MAX_NB_CHANNELS];
    uint32_t curTime = 0;

    if((LoRaMacState & (MAC_STATE_BEACON_RX|MAC_STATE_BEACON_GUARD)) != 0 )
    {
        trace_info("enter beacon state, stop set channel~~~~~~~~~\r\n");
        LoRaMacAlohaState &= ~MAC_ALOHA_CHANNEL_CHECK;
        return 1;
    }

    curTime = run_time_get();

    memset(enabledChannels, 0, LORA_MAX_NB_CHANNELS);

    // Update Aggregated duty cycle
    if(AggregatedTimeOff <= (curTime - AggregatedLastTxDoneTime))
    {
        AggregatedTimeOff = 0;
    }
    else
    {
        trace_info("AggregatedTimeOff >>>>>>\r\n");
        if((LoRaMacAlohaState & MAC_ALOHA_CHANNEL_CHECK) == 0)
        {
            uint32_t delay_time = AggregatedTimeOff - (curTime - AggregatedLastTxDoneTime);
            delay_time = (delay_time < 1000) ? 1000 : delay_time;
            TIMER_START(ChannelCheckTimer, delay_time);

            LoRaMacAlohaState |= MAC_ALOHA_CHANNEL_CHECK;
        }
        return 1;
    }
    // Update bands Time OFF
    uint32_t minTime = 0XFFFFFFFF;
    for(i = 0; i < LORA_MAX_NB_BANDS; i++)
    {
        if(DutyCycleOn == true)
        {
            if(Bands[i].TimeOff < (curTime - Bands[i].LastTxDoneTime))
            {
                Bands[i].TimeOff = 0;
            }
            if(Bands[i].TimeOff != 0)
            {
                minTime = MIN(Bands[i].TimeOff, minTime);
            }
        }
        else
        {
            minTime = 0;
            Bands[i].TimeOff = 0;
        }
    }
    // Search how many channels are enabled
    for(i = 0; i < LORA_MAX_NB_CHANNELS; i++)
    {
        if(((1 << i) & LoRaParam.db.ChMasks[0]) != 0)
        {
            trace_debug("ch = %d\r\n", i);
            if(Channels[i].FreqUp == 0)
            {
                // Check if the channel is enabled
                continue;
            }
            if((ChannleDatarate < Channels[i].DrRange.Min) ||
               (ChannleDatarate > Channels[i].DrRange.Max))
            {
                // Check if the current channel selection supports the given datarate
                continue;
            }
            if(Bands[Channels[i].Band].TimeOff > 0)
            {
                // Check if the band is available for transmission
                continue;
            }
            //            if( AggregatedTimeOff > 0 )
            //            { // Check if there is time available for transmission
            //                continue;
            //            }
            enabledChannels[nbEnabledChannels++] = i;
        }
    }

    if(nbEnabledChannels > 0)
    {
        for(i = 0, j = RANDR(0, nbEnabledChannels - 1); i < LORA_MAX_NB_CHANNELS; i++)
        {
            channelNext = enabledChannels[j];
            j = (j + 1) % nbEnabledChannels;

            if(Radio.IsChannelFree(MODEM_LORA, Channels[channelNext].FreqUp, RSSI_FREE_TH) == true)
            {
                // Free channel found
                Channel = channelNext;
                LoRaMacAlohaState &= ~MAC_ALOHA_CHANNEL_CHECK;
                return 0;
            }
        }
    }

    // No free channel found.
    // Check again
    if((LoRaMacAlohaState & MAC_ALOHA_CHANNEL_CHECK) == 0)
    {
        uint32_t delay_time = (minTime < 1000) ? 1000 : minTime;
        TIMER_START(ChannelCheckTimer, delay_time);
        LoRaMacAlohaState |= MAC_ALOHA_CHANNEL_CHECK;
    }
    return 1;
}

/*
 * TODO: Add documentation
 */
static void OnChannelCheckTimerEvent(void* p_context)
{
    trace_info("OnChCheck\r\n");
    LoRaMacAlohaState &= ~MAC_ALOHA_CHANNEL_CHECK;
    if(LoRaMacSetNextChannel() == 0)
    {
        trace_info("***\r\n");
        if((LoRaMacAlohaState & MAC_ALOHA_TX_RUNNING) == MAC_ALOHA_TX_RUNNING)
        {
            LoRaMacSendFrame(Channels[Channel]);
        }
    }
}

static uint8_t AddMacCommand(uint8_t *p_buff, uint8_t *p_index, uint8_t cmd, uint8_t p1, uint8_t p2)
{
    trace_verbose("AddMacCommand\r\n");
    uint8_t index = *p_index;
    p_buff[index++] = cmd;
    switch(cmd)
    {
        case MOTE_MAC_DEV_STATUS_ANS:
            // 1st byte Battery
            // 2nd byte Margin
            p_buff[index++] = p1;
            p_buff[index++] = p2;
            break;
        
        case MOTE_MAC_LINK_CHECK_REQ:
            // No payload for this command
        case MOTE_MAC_DUTY_CYCLE_ANS:
            // No payload for this answer
        case MOTE_MAC_RX_TIMING_SETUP_ANS:
            // No payload for this answer
        case MOTE_MAC_TX_PARAM_SETUP_ANS:
        case MOTE_MAC_BEACON_TIMING_REQ:
            break;
        
        case MOTE_MAC_LINK_ADR_ANS:
            // Margin
        case MOTE_MAC_RX_PARAM_SETUP_ANS:
            // Status: Datarate ACK, Channel ACK
        case MOTE_MAC_NEW_CHANNEL_ANS:
            // Status: Datarate range OK, Channel frequency OK
        case MOTE_MAC_DI_CHANNEL_ANS:
        case MOTE_MAC_CUS_STATUS:
        case MOTE_MAC_PING_SLOT_INFO_REQ:
        case MOTE_MAC_PING_SLOT_FREQ_ANS:
        case MOTE_MAC_BEACON_FREQ_ANS:
        case MOTE_MAC_DR_CHECK_REQ:
        case MOTE_MAC_INTERVAL_LIMIT_ANS:
        case MOTE_MAC_CUS_PING_SLOT_INFO_ANS:
        case MOTE_MAC_VERSION_REQ:
        case MOTE_MAC_CLASSG_PARAM_ANS:
        case MOTE_MAC_CLASSG_CHANNEL_ANS:
            p_buff[index++] = p1;
            break;
        
        default:
            return 1;
    }
    *p_index = index;    
    return 0;
}

static void LoRaMacPackMacCommand(uint8_t *p_cmd, uint8_t fopt_len, uint8_t *p_in, uint8_t len)
{
    uint8_t idx = 0;
    uint8_t cmd = 0;
    uint8_t cusBuff[15] = {0};
    uint8_t cusNb = 0;
    uint8_t stdBuff[15] = {0};
    uint8_t stdNb = 0;
    
    uint8_t macCmdCache[15] = {0};
    
    memcpy(macCmdCache, p_cmd, fopt_len);
    
    while(idx < len)
    {
        cmd = p_in[idx++];
        switch(cmd)
        {
            // standard mac cmd with no payload
            case MOTE_MAC_LINK_CHECK_REQ:
            case MOTE_MAC_DUTY_CYCLE_ANS:
            case MOTE_MAC_RX_TIMING_SETUP_ANS:
            case MOTE_MAC_TX_PARAM_SETUP_ANS:
            case MOTE_MAC_BEACON_TIMING_REQ:
                stdBuff[stdNb++] = cmd;
                break;
            
            // standard mac cmd with 1 byte payload
            case MOTE_MAC_LINK_ADR_ANS:
            case MOTE_MAC_RX_PARAM_SETUP_ANS:
            case MOTE_MAC_NEW_CHANNEL_ANS:
            case MOTE_MAC_DI_CHANNEL_ANS:
            case MOTE_MAC_PING_SLOT_INFO_REQ:
            case MOTE_MAC_PING_SLOT_FREQ_ANS:
            case MOTE_MAC_BEACON_FREQ_ANS:
                stdBuff[stdNb++] = cmd;
                stdBuff[stdNb++] = p_in[idx++];
                break;
            
            // standard mac cmd with 2 bytes payload
            case MOTE_MAC_DEV_STATUS_ANS:
                stdBuff[stdNb++] = cmd;
                stdBuff[stdNb++] = p_in[idx++];
                stdBuff[stdNb++] = p_in[idx++];
                break;
            
            // cus mac cmd with 1 byte payload
            case MOTE_MAC_CUS_STATUS:
            case MOTE_MAC_DR_CHECK_REQ:
            case MOTE_MAC_INTERVAL_LIMIT_ANS:
            case MOTE_MAC_CUS_PING_SLOT_INFO_ANS:
            case MOTE_MAC_VERSION_REQ:
            case MOTE_MAC_CLASSG_PARAM_ANS:
            case MOTE_MAC_CLASSG_CHANNEL_ANS:
                cusBuff[cusNb++] = cmd;
                cusBuff[cusNb++] = p_in[idx++];
                break;
        }
    } 
    memcpy(p_cmd, stdBuff, stdNb);
    memcpy(&p_cmd[stdNb], macCmdCache, fopt_len);
    memcpy(&p_cmd[stdNb + fopt_len], cusBuff, cusNb);
}

static uint8_t LoRaMacVersionReq(void)
{
    uint8_t version = 0;
    
    version = LORAMAC_VERSION;
    
    AddMacCommand(MacCmdsBuffer, &MacCmdsBufferIdx, MOTE_MAC_VERSION_REQ, version, 0);
    if(MacCmdsBufferIdx < 10)
    {
        MacCmdsInNextTx = true;
    }
    return 0;
}

static uint8_t LoRaMacCusStatusInfo(void)
{
    uint8_t cus_statue;
    uint8_t txPowerIdx = LoRaMacCheckTxPower(LoRaParam.db.TXPower);
    cus_statue = (ChannleDatarate << 4) | txPowerIdx;
    
    AddMacCommand(MacCmdsBuffer, &MacCmdsBufferIdx, MOTE_MAC_CUS_STATUS, cus_statue, 0);
    if(MacCmdsBufferIdx < 10)
    {
        MacCmdsInNextTx = true;
    }
    return 0;
}

static uint8_t LoRaMacPingSlotInfoReq(void)
{
    trace_info("PingSlotInfoReq~\r\n");
    uint8_t status = (LoRaParam.db.PingDataRate | (LoRaParam.db.PingPeriodicity << 4));
    AddMacCommand(MacCmdsBuffer, &MacCmdsBufferIdx, MOTE_MAC_PING_SLOT_INFO_REQ, status, 0);
    if(MacCmdsBufferIdx < 10)
    {
        MacCmdsInNextTx = true;
    }
    return 0;
}

static uint8_t LoRaMacBeaconTimingReq(void)
{
    trace_info("BeaconTimingReq~\r\n");
    AddMacCommand(MacCmdsBuffer, &MacCmdsBufferIdx, MOTE_MAC_BEACON_TIMING_REQ, 0, 0);
    if(MacCmdsBufferIdx < 10)
    {
        MacCmdsInNextTx = true;
    }
    return 0;
}

uint8_t LoRaMacLinkCheckReq(void)
{
    AddMacCommand(MacCmdsBuffer, &MacCmdsBufferIdx, MOTE_MAC_LINK_CHECK_REQ, 0, 0);
    if(MacCmdsBufferIdx < 10)
    {
        MacCmdsInNextTx = true;
    }
    return 0;
}

static uint8_t LoRaMacDrCheckReq(void)
{
    uint8_t dr = 0xff;
    
    if(Rx1DatarateNew != UINT8_MAX)
    {
        dr = Rx1DatarateNew;
    }
    else if(Rx2DatarateNew != UINT8_MAX)
    {
        dr = Rx2DatarateNew;
    }
    else if(PingDatarateNew != UINT8_MAX)
    {
        dr = PingDatarateNew;
    }
    else if(ClassGDatarateNew != UINT8_MAX)
    {
        dr = ClassGDatarateNew;
    }
    else
    {
        dr = 0xff;
    }
    
    AddMacCommand(MacCmdsBuffer, &MacCmdsBufferIdx, MOTE_MAC_DR_CHECK_REQ, dr, 0);
    if(MacCmdsBufferIdx < 10)
    {
        MacCmdsInNextTx = true;
    }
    return 0;
}

static void LoRaMacAlohaNotify(LoRaMacEventFlags_t* flags, LoRaMacEventAlohaInfo_t* info)
{
    if((LoRaMacEvents != NULL) && (LoRaMacEvents->AlohaEvent != NULL))
    {
        LoRaMacEvents->AlohaEvent(flags, info);
    }
    flags->Value = 0;
}

static void LoRaMacRxNotify(LoRaMacEventRxInfo_t* info)
{
    if((LoRaMacEvents != NULL) && (LoRaMacEvents->RxEvent != NULL))
    {
        LoRaMacEvents->RxEvent(info);
    }
}

static void LoRaMacChannelsInit(void)
{
    
    for(uint8_t i = 0; i< LoRaParam.db.BandLen; i++)
    {
        Bands[i].DCycle = LoRaParam.db.Band[i].DCycle;
        Bands[i].TxMaxPower = LoRaParam.db.Band[i].MaxTxPower;
    }
    
    for(uint8_t i = 0; i < 16; i++)
    {
        Channels[i].Band = LoRaParam.db.AlohaChannel[i].Band;
        
        Channels[i].FreqUp = LoRaParam.db.AlohaChannel[i].FreqUp;
        Channels[i].FreqDown = LoRaParam.db.AlohaChannel[i].FreqDown;
        Channels[i].DrRange.Value = ( LoRaParam.db.AlohaChannel[i].DRMax << 4 ) | 
                                    LoRaParam.db.AlohaChannel[i].DRMin ;
    }
}

void LoRaMacInitParam(void)
{
    memcpy(&LoRaParam, (uint8_t *)STORAGE_ADDR_LORA_PARAM, sizeof(LoRaParam_t));
    trace_notice("LoRa DevEUI = ");
    trace_dump_n((uint8_t *)LoRaParam.db.DevEUI, sizeof(LoRaParam.db.DevEUI));
    
    trace_notice("LoRa AppEUI = ");
    trace_dump_n((uint8_t *)LoRaParam.db.AppEUI, sizeof(LoRaParam.db.AppEUI));
    
    trace_notice("LoRa AppKey = ");
    trace_dump_n((uint8_t *)LoRaParam.db.AppKey, sizeof(LoRaParam.db.AppKey));
    
    trace_notice("LoRa AppSKey = ");
    trace_dump_n((uint8_t *)LoRaParam.db.AppSKey, sizeof(LoRaParam.db.AppSKey));
    
    trace_notice("LoRa NwkSKey = ");
    trace_dump_n((uint8_t *)LoRaParam.db.NwkSKey, sizeof(LoRaParam.db.NwkSKey));
    
    trace_notice("LoRa DevAddr = %x\r\n", LoRaParam.db.DevAddr);
    trace_notice("LoRa NetId = %x\r\n", LoRaParam.db.NetID);
    trace_notice("LoRa AppNonce = %x\r\n", LoRaParam.db.AppNonce);
    trace_notice("LoRa DevNonce = %x\r\n", LoRaParam.db.DevNonce);
    trace_notice("LoRa MaxTXPower = %d\r\n", LoRaParam.db.MaxTXPower);
    trace_notice("LoRa TXPower = %d\r\n", LoRaParam.db.TXPower);
    trace_notice("LoRa DataRate = %d\r\n", LoRaParam.db.DataRate);
    trace_notice("LoRa ChMask = %x\r\n", LoRaParam.db.ChMasks[0]);
    trace_notice("LoRa NbTrans = %d\r\n", LoRaParam.db.NbTrans);
    trace_notice("LoRa Adr = %d\r\n", LoRaParam.db.Adr);
    trace_notice("LoRa MaxDcycle = %d\r\n", LoRaParam.db.MaxDCycle);
    trace_notice("LoRa RX1DrOffset = %d\r\n", LoRaParam.db.RX1DROffset);
    trace_notice("LoRa RX2Freq = %d\r\n", LoRaParam.db.RX2Freq);
    trace_notice("LoRa RX2DateRate = %d\r\n", LoRaParam.db.RX2DateRate);
    trace_notice("LoRa Del = %d\r\n", LoRaParam.db.Del);
    trace_notice("LoRa LoRaParam.db.ClassBEnable = %d\r\n", LoRaParam.db.ClassBEnable);
    trace_notice("LoRa PingPeriodicity = %d\r\n", LoRaParam.db.PingPeriodicity);
    trace_notice("LoRa PingDataRate = %d\r\n", LoRaParam.db.PingDataRate);
    trace_notice("LoRa BeaconFreq = %d\r\n", LoRaParam.db.BeaconFreq);
    trace_notice("LoRa BeaconDataRate = %d\r\n", LoRaParam.db.BeaconDataRate);
 
#ifdef DEBUG 
    LoRaParam.db.ClassGEnabled = false;
    LoRaParam.db.ClassGChannel.FreqDown = 505300000;
//    LoRaParam.db.ClassGDataRate = 3;
//    LoRaParam.db.ClassGPeriod = 1;
    trace_notice("LoRa ClassGEnabled = %d\r\n", LoRaParam.db.ClassGEnabled);
    trace_notice("LoRa ClassGFreq = %d\r\n", LoRaParam.db.ClassGChannel.FreqDown);
    trace_notice("LoRa ClassGDataRate = %d\r\n", LoRaParam.db.ClassGDataRate);
    trace_notice("LoRa ClassGPeriod = %d\r\n", LoRaParam.db.ClassGPeriod);
#endif    
    
    trace_notice("LoRa Activation = %d\r\n", LoRaParam.db.Activation);
    trace_notice("LoRa ClassType = %d\r\n", LoRaParam.db.ClassType);
    
    for(uint8_t i = 0; i<LoRaParam.db.BandLen; i++)
    {
        trace_notice("LoRa Band[%d].DCycle = %d\r\n", i, LoRaParam.db.Band[i].DCycle);
        trace_notice("LoRa Band[%d].MaxTxPower = %d\r\n", i, LoRaParam.db.Band[i].MaxTxPower);
    }
    for(uint8_t i = 0; i<8; i++)
    {
        trace_notice("LoRa Channels[%d].FreqUp = %d\r\n", i, LoRaParam.db.AlohaChannel[i].FreqUp);
        trace_notice("LoRa Channels[%d].FreqDown = %d\r\n", i, LoRaParam.db.AlohaChannel[i].FreqDown);
    }
    trace_notice("LoRa PingChannel.FreqUp = %d\r\n", LoRaParam.db.PingChannel.FreqUp);
    trace_notice("LoRa PingChannel.FreqDown = %d\r\n\r\n", LoRaParam.db.PingChannel.FreqDown);
    
    if(LoRaParam.db.NbTrans == 0)
    {
        LoRaParam.db.NbTrans = 1;
    }
    
    IsLoRaMacNetworkJoined = true;
#ifdef DEBUG
//    LoRaParam.db.PingPeriodicity = 2;
//    LoRaParam.db.MaxDCycle = 5;
#endif    
        
    if(LoRaParam.db.MaxDCycle > 15)
    {
        LoRaParam.db.MaxDCycle = 0;
    }
    
    UpLinkCounter = LoRaMacStorageFcntGet();
    trace_notice("LoRa fcnt = %d\r\n", UpLinkCounter);
    
    DownLinkCounter = 0;
    ChannleDatarate = LoRaParam.db.DataRate;
    ChannelsNbRepCounter = 0;

    AggregatedDCycle = (1 << LoRaParam.db.MaxDCycle);
    AggregatedLastTxDoneTime = 0;
    AggregatedTimeOff = 0;

    DutyCycleOn = false;
    MaxRxWindow = MAX_RX_WINDOW;
    JoinAcceptDelay1 = JOIN_ACCEPT_DELAY1;
    
    Channel = LORA_MAX_NB_CHANNELS;
    
    DeviceClass = (DeviceClass_t)LoRaParam.db.ClassType;
    
#ifdef DEBUG
//    uint8_t nwkskey[16]={0xd3,0x68,0xc2,0xa6,0xb7,0xee,0x74,0xff,0xcb,0x60,0x25,0x06,0x3c,0x58,0x45,0x32};
//    uint8_t appskey[16]={0xfd,0x77,0x0f,0x2c,0xce,0x3f,0x49,0xe1,0xa4,0x65,0x10,0xb2,0xab,0xc8,0x8e,0xdd};
// 
//    memcpy(LoRaParam.db.AppSKey, appskey,16);
//    memcpy(LoRaParam.db.NwkSKey, nwkskey,16);
//    LoRaParam.db.DevAddr = 0x02001F05;
//    DeviceClass = CLASS_C;
#endif

}

void LoRaMacEventHandlerRegister(LoRaMacEvent_t* events)
{
    LoRaMacEvents = events;
}

void LoRaMacInit(void)
{
    trace_info("LoRaMacInit\r\n");
    uint32_t err_code;
    
    LoRaMacVersionChange();
        
    pstorage_module_param_t pstorage_param;
    
    pstorage_param.cb = pstorage_ntf_cb;
    FcntDbBlockId.block_id = STORAGE_ADDR_LORA_FCNT;
    err_code = pstorage_register(&pstorage_param, &FcntDbBlockId);
    APP_ERROR_CHECK(err_code);
    
    pstorage_param.cb = pstorage_ntf_cb;
    LoRaParamBlockId.block_id = STORAGE_ADDR_LORA_PARAM;
    err_code = pstorage_register(&pstorage_param, &LoRaParamBlockId);
    APP_ERROR_CHECK(err_code);
    
    LoRaMacEventFlags.Value = 0;
    memset(&LoRaMacEventAlohaInfo, 0, sizeof(LoRaMacEventAlohaInfo_t));
    LoRaMacEventAlohaInfo.Status = LORAMAC_EVENT_INFO_STATUS_OK;
    memset(&EventRxInfo, 0, sizeof(LoRaMacEventRxInfo_t));
    
    LoRaMacInitParam();
    LoRaMacChannelsInit();

    LoRaMacAlohaState = MAC_ALOHA_IDLE;
    
    // Initialize Radio driver
    RadioEvents.TxDone = OnRadioTxDone;
    RadioEvents.RxDone = OnRadioRxDone;
    RadioEvents.RxError = OnRadioRxError;
    RadioEvents.TxTimeout = OnRadioTxTimeout;
    RadioEvents.RxTimeout = OnRadioRxTimeout;
    Radio.Init(&RadioEvents);

    // Random seed initialization
    srand(RAND_SEED);

    LoRaMacSetPublicNetwork(true);
    Radio.Sleep();
    
    err_code = app_mailbox_create(&Mailbox);
    APP_ERROR_CHECK(err_code);
    
    TIMER_CREATE(&RadioTimeoutTimer, APP_TIMER_MODE_SINGLE_SHOT, OnRadioTimeoutEvent);
    TIMER_CREATE(&ChannelCheckTimer, APP_TIMER_MODE_SINGLE_SHOT, OnChannelCheckTimerEvent);
//  TIMER_CREATE(&TxDelayedTimer, APP_TIMER_MODE_SINGLE_SHOT, OnTxDelayedTimerEvent);
    TIMER_CREATE(&RxWindowTimer1, APP_TIMER_MODE_SINGLE_SHOT, OnRxWindow1TimerEvent);
    TIMER_CREATE(&RxWindowTimer2, APP_TIMER_MODE_SINGLE_SHOT, OnRxWindow2TimerEvent);
    TIMER_CREATE(&AckTimeoutTimer, APP_TIMER_MODE_SINGLE_SHOT, OnAckTimeoutTimerEvent);
    TIMER_CREATE(&BeaconTimer, APP_TIMER_MODE_SINGLE_SHOT, OnBeaconTimerEvent);
    TIMER_CREATE(&PingTimer, APP_TIMER_MODE_SINGLE_SHOT, OnPingTimerEvent);
    TIMER_CREATE(&ClassGTimer, APP_TIMER_MODE_SINGLE_SHOT, OnClassGTimerEvent);
    
    if(DeviceClass == CLASS_C)
    {
        LoRaMacState |= MAC_STATE_CLASSC_RX;
        LoRaMacRxWindowSetup(1, true);
    } 
    else if(DeviceClass == CLASS_G)
    {
        LoRaMacClassGStart();
    }
}



void LoRaMacMulticastChannelAdd(MulticastParams_t* channelParam)
{
    // Reset downlink counter
    channelParam->DownLinkCounter = 0;

    if(MulticastChannels == NULL)
    {
        MulticastChannels = channelParam;
    }
    else
    {
        MulticastParams_t* cur = MulticastChannels;
        while(cur->Next != NULL)
        {
            cur = cur->Next;
        }
        cur->Next = channelParam;
    }
}

void LoRaMacMulticastChannelRemove(MulticastParams_t* channelParam)
{
    MulticastParams_t* cur = NULL;

    // Remove the front element
    if(MulticastChannels == channelParam)
    {
        if(MulticastChannels != NULL)
        {
            cur = MulticastChannels;
            MulticastChannels = MulticastChannels->Next;
            cur->Next = NULL;
            // Last node in the list
            if(cur == MulticastChannels)
            {
                MulticastChannels = NULL;
            }
        }
        return;
    }

    // Remove last element
    if(channelParam->Next == NULL)
    {
        if(MulticastChannels != NULL)
        {
            cur = MulticastChannels;
            MulticastParams_t* last = NULL;
            while(cur->Next != NULL)
            {
                last = cur;
                cur = cur->Next;
            }
            if(last != NULL)
            {
                last->Next = NULL;
            }
            // Last node in the list
            if(cur == last)
            {
                MulticastChannels = NULL;
            }
        }
        return;
    }

    // Remove a middle element
    cur = MulticastChannels;
    while(cur != NULL)
    {
        if(cur->Next == channelParam)
        {
            break;
        }
        cur = cur->Next;
    }
    if(cur != NULL)
    {
        MulticastParams_t* tmp = cur ->Next;
        cur->Next = tmp->Next;
        tmp->Next = NULL;
    }
}

uint8_t LoRaMacJoinReq(uint8_t* devEui, uint8_t* appEui, uint8_t* appKey)
{
    LoRaMacHeader_t macHdr;

//    LoRaParam.db.DevEUI = devEui;
//    LoRaParam.db.AppEUI = appEui;
//    LoRaParam.db.AppKey = appKey;

    macHdr.Value = 0;
    macHdr.Bits.MType        = FRAME_TYPE_JOIN_REQ;

    IsLoRaMacNetworkJoined = false;
    return LoRaMacSendPacket(&macHdr, NULL, 0, NULL, 0);
}

uint8_t LoRaMacSendUnconfirmedPacket(uint8_t fPort, void* fBuffer, uint16_t fBufferSize)
{
    trace_verbose("LoRaMacSendUnconfirmedPacket\r\n");
    LoRaMacHeader_t macHdr;
    
    if((SilenceTime > 0) && (LastTxTime != 0))
    {
        uint32_t curTime = run_time_get();
        if((curTime - LastTxTime) < SilenceTime*1000)
        {
            return LORAMAC_ERROR_TX_LIMITED;
        }
        else
        {
            SilenceTime = 0;
        }
    }
    
    if( (LoRaMacState & (MAC_STATE_BEACON_GUARD | MAC_STATE_BEACON_RX | MAC_STATE_PING_RX | MAC_STATE_CLASSG_RX)) != 0 )
    {
        trace_warn("busy, aloha not permmite\r\n");
        return LORAMAC_ERROR_BUSY; 
    }
    
    if((LoRaMacAlohaState & MAC_ALOHA_TX_RUNNING) == MAC_ALOHA_TX_RUNNING)
    {
        trace_warn("busy\r\n");
        return LORAMAC_ERROR_BUSY; 
    }

    LoRaMacAlohaState |= MAC_ALOHA_TX_RUNNING;
    LoRaMacState |= MAC_STATE_ALOHA;
    
    //modify by wanghuayuan
    if(LoRaParam.db.Adr == false)
    {
        ChannleDatarate = LoRaParam.db.DataRate;
    }

    macHdr.Value = 0;

    macHdr.Bits.MType = FRAME_TYPE_DATA_UNCONFIRMED_UP;
    return LoRaMacSendPacket(&macHdr, NULL, fPort, fBuffer, fBufferSize);
}

uint8_t LoRaMacSendConfirmedPacket(uint8_t fPort, void* fBuffer, uint16_t fBufferSize, uint8_t retries)
{
    LoRaMacHeader_t macHdr;
    
    if(SilenceTime > 0 && LastTxTime != 0)
    {
        uint32_t curTime = run_time_get();
        if((curTime - LastTxTime) < SilenceTime*1000)
        {
            return LORAMAC_ERROR_TX_LIMITED;
        }
        else
        {
            SilenceTime = 0;
        }
    }
    
    if( (LoRaMacState & (MAC_STATE_BEACON_GUARD | MAC_STATE_BEACON_RX | MAC_STATE_PING_RX | MAC_STATE_CLASSG_RX)) != 0 )
    {
        trace_warn("busy, aloha not permmite\r\n");
        return LORAMAC_ERROR_BUSY; 
    }
    
    if((LoRaMacAlohaState & MAC_ALOHA_TX_RUNNING) == MAC_ALOHA_TX_RUNNING)
    {
        trace_warn("busy\r\n");
        return LORAMAC_ERROR_BUSY; 
    }

    LoRaMacAlohaState |= MAC_ALOHA_TX_RUNNING;
    LoRaMacState |= MAC_STATE_ALOHA;

    if(LoRaParam.db.Adr == false)
    {
        ChannleDatarate = LoRaParam.db.DataRate;
    }
    AckTimeoutRetries = retries;
    AckTimeoutRetriesCounter = 1;

    macHdr.Value = 0;
    macHdr.Bits.MType = FRAME_TYPE_DATA_CONFIRMED_UP;
    return LoRaMacSendPacket(&macHdr, NULL, fPort, fBuffer, fBufferSize);
}

static uint8_t LoRaMacSendPacket(LoRaMacHeader_t* macHdr, uint8_t* fOpts, uint8_t fPort, void* fBuffer, uint16_t fBufferSize)
{
    trace_verbose("LoRaMacSend\r\n");
    LoRaMacFrameCtrl_t fCtrl;
    uint8_t status = 0;

    fCtrl.Value = 0;
    
    fCtrl.Bits.FOptsLen      = 0;
    fCtrl.Bits.FPending      = 0;
    fCtrl.Bits.Ack           = false;
    fCtrl.Bits.AdrAckReq     = false;
    fCtrl.Bits.Adr           = LoRaParam.db.Adr;
    if(CLASS_B == DeviceClass)
    {
        trace_info("classb bit\r\n");
        fCtrl.Bits.FPending  = 1;
    }
    
    UpLinkCounter++;
    LoRaMacStorageFcntUpdate(UpLinkCounter);
    
    if(!VersionAnsed)
    {
        LoRaMacVersionReq();
    }
        
    if(MacCmdsDrCheck)
    {
        MacCmdsDrCheckAnsWait = true;
        LoRaMacDrCheckReq();
    }

    LoRaMacCusStatusInfo();

    if((CLASS_A == DeviceClass) && LoRaParam.db.ClassBEnable)
    {
        if(!PingSlotInfoAnsed)
        {
            LoRaMacPingSlotInfoReq();
        }
        
        if(!IsBeaconSearching)
        {
            LoRaMacBeaconTimingReq();
        }
    }

    status = LoRaMacPrepareFrame(macHdr, &fCtrl, fOpts, fPort, fBuffer, fBufferSize);
    if(status != 0)
    {
        trace_info("return %d\r\n", status);
        return status;
    }
    LoRaMacEventAlohaInfo.TxPort = fPort;
    LoRaMacEventAlohaInfo.TxNbRetries = 0;
    LoRaMacEventAlohaInfo.TxAckReceived = false;
    trace_notice("ufcnt = %d\r\n", UpLinkCounter);

    if(LoRaMacSetNextChannel() == 0)
    {
        return LoRaMacSendFrame(Channels[Channel]);
    }
    return LORAMAC_ERROR_NONE;
}

static uint8_t LoRaMacPrepareFrame(LoRaMacHeader_t* macHdr, LoRaMacFrameCtrl_t* fCtrl, uint8_t* fOpts, uint8_t fPort, void* fBuffer, uint16_t fBufferSize)
{
    trace_verbose("LoRaMacPrepareFrame\r\n");
    uint16_t i;
    uint8_t pktHeaderLen = 0;
    uint32_t mic = 0;

    LoRaMacBufferPktLen = 0;

    NodeAckRequested = false;

    if(fBuffer == NULL)
    {
        fBufferSize = 0;
    }

    LoRaMacBuffer[pktHeaderLen++] = macHdr->Value;

    switch(macHdr->Bits.MType)
    {
        case FRAME_TYPE_JOIN_REQ:
            RxWindow1Delay = JoinAcceptDelay1 - RADIO_WAKEUP_TIME;
        
            LoRaMacBufferPktLen = pktHeaderLen;

            memcpy(LoRaMacBuffer + LoRaMacBufferPktLen, LoRaParam.db.AppEUI, 8);
            LoRaMacBufferPktLen += 8;
            memcpy(LoRaMacBuffer + LoRaMacBufferPktLen, LoRaParam.db.DevEUI, 8);
            LoRaMacBufferPktLen += 8;

            LoRaParam.db.DevNonce = Radio.Random();

            LoRaMacBuffer[LoRaMacBufferPktLen++] = LoRaParam.db.DevNonce & 0xFF;
            LoRaMacBuffer[LoRaMacBufferPktLen++] = (LoRaParam.db.DevNonce >> 8) & 0xFF;

            LoRaMacJoinComputeMic(LoRaMacBuffer, LoRaMacBufferPktLen & 0xFF, (uint8_t *)LoRaParam.db.AppKey, &mic);

            LoRaMacBuffer[LoRaMacBufferPktLen++] = mic & 0xFF;
            LoRaMacBuffer[LoRaMacBufferPktLen++] = (mic >> 8) & 0xFF;
            LoRaMacBuffer[LoRaMacBufferPktLen++] = (mic >> 16) & 0xFF;
            LoRaMacBuffer[LoRaMacBufferPktLen++] = (mic >> 24) & 0xFF;

            break;
        case FRAME_TYPE_DATA_CONFIRMED_UP:
            NodeAckRequested = true;

        //Intentional falltrough
        case FRAME_TYPE_DATA_UNCONFIRMED_UP:

            if(IsLoRaMacNetworkJoined == false)
            {
                trace_info("retrun 2\r\n");
                return LORAMAC_ERROR_NO_NETWORK_JOINED; // No network has been joined yet
            }

            RxWindow1Delay = LoRaParam.db.Del * 1000 - RADIO_WAKEUP_TIME;

            if(fOpts == NULL)
            {
                fCtrl->Bits.FOptsLen = 0;
            }

            if(SrvAckRequested == true)
            {
                trace_info("SrvAckRequested = %d\r\n", SrvAckRequested);
                trace_info("uplink ack===========\r\n");
                SrvAckRequested = false;
                fCtrl->Bits.Ack = 1;
            }
            if(fCtrl->Bits.Adr == true)
            {
                if(ChannleDatarate == LORAMAC_MIN_DATARATE)
                {
                    AdrAckCounter = 0;
                    fCtrl->Bits.AdrAckReq = false;
                }
                else
                {
                    if(AdrAckCounter > ADR_ACK_LIMIT)
                    {
                        fCtrl->Bits.AdrAckReq = true;
                        trace_info("adr req ~~~~~~~~~~~~~\r\n");
                    }
                    else
                    {
                        fCtrl->Bits.AdrAckReq = false;
                    }
                    if(AdrAckCounter > (ADR_ACK_LIMIT + ADR_ACK_DELAY))
                    {
                        AdrAckCounter = 0;
                        if(ChannleDatarate > LORAMAC_MIN_DATARATE)
                        {
                            ChannleDatarate--;
                        }
                        else
                        {
                            // Re-enable default channels LC1, LC2, LC3
                            // LoRaParam.db.ChMask = LoRaParam.db.ChMask | ( LC( 1 )  + LC( 2 ) + LC( 3 ) );//!!!
                        }
                    }
                }
            }

            LoRaMacBuffer[pktHeaderLen++] = (LoRaParam.db.DevAddr) & 0xFF;
            LoRaMacBuffer[pktHeaderLen++] = (LoRaParam.db.DevAddr >> 8) & 0xFF;
            LoRaMacBuffer[pktHeaderLen++] = (LoRaParam.db.DevAddr >> 16) & 0xFF;
            LoRaMacBuffer[pktHeaderLen++] = (LoRaParam.db.DevAddr >> 24) & 0xFF;

            LoRaMacBuffer[pktHeaderLen++] = fCtrl->Value;

            LoRaMacBuffer[pktHeaderLen++] = UpLinkCounter & 0xFF;
            LoRaMacBuffer[pktHeaderLen++] = (UpLinkCounter >> 8) & 0xFF;

            if(fOpts != NULL)
            {
                for(i = 0; i < fCtrl->Bits.FOptsLen; i++)
                {
                    LoRaMacBuffer[pktHeaderLen++] = fOpts[i];
                }
            }
            
            if((MacCmdsDelayAnsBufferIdx + fCtrl->Bits.FOptsLen) <= 15)
            {
                if(MacCmdsAnsInNextTx)
                {
                    trace_info("MacCmdsAns = ");
                    trace_dump_i(MacCmdsDelayAnsBuffer, MacCmdsDelayAnsBufferIdx);
                    
                    LoRaMacPackMacCommand(&LoRaMacBuffer[pktHeaderLen], fCtrl->Bits.FOptsLen, 
                                            MacCmdsDelayAnsBuffer, MacCmdsDelayAnsBufferIdx);
                    fCtrl->Bits.FOptsLen += MacCmdsDelayAnsBufferIdx;

//                    for(i = 0; i < MacCmdsDelayAnsBufferIdx; i++)
//                    {
//                        LoRaMacBuffer[pktHeaderLen++] = MacCmdsDelayAnsBuffer[i];
//                    }
                }
            }
            
            if((MacCmdsImmAnsBufferIdx + fCtrl->Bits.FOptsLen) <= 15)
            {
                if(MacCmdsImmAnsInNextTx)
                {
                    trace_info("MacCmdsImmAns = ");
                    trace_dump_i(MacCmdsImmAnsBuffer, MacCmdsImmAnsBufferIdx);
                    
                    LoRaMacPackMacCommand(&LoRaMacBuffer[pktHeaderLen], fCtrl->Bits.FOptsLen, 
                                            MacCmdsImmAnsBuffer, MacCmdsImmAnsBufferIdx);
                    fCtrl->Bits.FOptsLen += MacCmdsImmAnsBufferIdx;

//                    for(i = 0; i < MacCmdsImmAnsBufferIdx; i++)
//                    {
//                        LoRaMacBuffer[pktHeaderLen++] = MacCmdsImmAnsBuffer[i];
//                    }
                }
            }
            
            if((MacCmdsBufferIdx + fCtrl->Bits.FOptsLen) <= 15)
            {
                if(MacCmdsInNextTx)
                {
                    trace_info("MacCmdsOne = ");
                    trace_dump_i(MacCmdsBuffer, MacCmdsBufferIdx);
                    
                    LoRaMacPackMacCommand(&LoRaMacBuffer[pktHeaderLen], fCtrl->Bits.FOptsLen, 
                                            MacCmdsBuffer, MacCmdsBufferIdx);
                    fCtrl->Bits.FOptsLen += MacCmdsBufferIdx;
                    
//                    for(i = 0; i < MacCmdsBufferIdx; i++)
//                    {
//                        LoRaMacBuffer[pktHeaderLen++] = MacCmdsBuffer[i];
//                    }
                }
                MacCmdsInNextTx = false;
                MacCmdsBufferIdx = 0;
            }
            
            pktHeaderLen += fCtrl->Bits.FOptsLen;
            LoRaMacBuffer[0x05] = fCtrl->Value;
            trace_debug("FOptsLen = %d\r\n", LoRaMacBuffer[0x05]);

            if((pktHeaderLen + fBufferSize) > LORAMAC_PHY_MAXPAYLOAD)
            {
                trace_info("retrun 3\r\n");
                return LORAMAC_ERROR_MAXPAYLOAD_EXCEEDED;
            }
            
            if(fBuffer != NULL)
            {
                uint8_t LoRaMacPayload[LORAMAC_PHY_MAXPAYLOAD];
                LoRaMacBuffer[pktHeaderLen] = fPort;
                if(fPort == 0)
                {
                    LoRaMacPayloadEncrypt(fBuffer, fBufferSize, (uint8_t *)LoRaParam.db.NwkSKey, LoRaParam.db.DevAddr, UP_LINK, UpLinkCounter, LoRaMacPayload);
                }
                else
                {
                    LoRaMacPayloadEncrypt(fBuffer, fBufferSize, (uint8_t *)LoRaParam.db.AppSKey, LoRaParam.db.DevAddr, UP_LINK, UpLinkCounter, LoRaMacPayload);
                }
                memcpy(LoRaMacBuffer + pktHeaderLen + 1, LoRaMacPayload, fBufferSize);
            }
            LoRaMacBufferPktLen = pktHeaderLen + 1 + fBufferSize;

            LoRaMacComputeMic(LoRaMacBuffer, LoRaMacBufferPktLen, (uint8_t *)LoRaParam.db.NwkSKey, LoRaParam.db.DevAddr, UP_LINK, UpLinkCounter, &mic);

            if((LoRaMacBufferPktLen + LORAMAC_MFR_LEN) > LORAMAC_PHY_MAXPAYLOAD)
            {
                trace_info("retrun 3\r\n");
                return LORAMAC_ERROR_MAXPAYLOAD_EXCEEDED;
            }
            LoRaMacBuffer[LoRaMacBufferPktLen + 0] = mic & 0xFF;
            LoRaMacBuffer[LoRaMacBufferPktLen + 1] = (mic >> 8) & 0xFF;
            LoRaMacBuffer[LoRaMacBufferPktLen + 2] = (mic >> 16) & 0xFF;
            LoRaMacBuffer[LoRaMacBufferPktLen + 3] = (mic >> 24) & 0xFF;

            LoRaMacBufferPktLen += LORAMAC_MFR_LEN;
            break;
        default:
            trace_info("retrun 4\r\n");
            return LORAMAC_ERROR_INVALID_FRAME_TYPE;
    }

    return LORAMAC_ERROR_NONE;
}


static uint8_t LoRaMacSendFrame(ChannelParams_t channel)
{
    trace_verbose("LoRaMacSendFrame\r\n");
    uint8_t txPower = 0;
    uint8_t txDatarate = 0;
    
    if(TxPowerNew != INT8_MAX)
    {
       txPower = TxPowerNew;
    }
    else
    {
        txPower = LoRaParam.db.TXPower;
    }
    
    if(Rx1DatarateNew != UINT8_MAX)
    {
        txDatarate = Rx1DatarateNew;
    }
    else
    {
        txDatarate = ChannleDatarate;
    }
    
    LoRaMacEventAlohaInfo.Status = LORAMAC_EVENT_INFO_STATUS_ERROR;
    LoRaMacEventAlohaInfo.TxDatarate = txDatarate;
    LoRaMacEventAlohaInfo.TxFreq = channel.FreqUp;
    LoRaMacEventAlohaInfo.TxPower = txPower;

    Radio.SetChannel(channel.FreqUp);
    
    if(ChannleDatarate == DR_7)
    {
        // High Speed FSK channel
        Radio.SetTxConfig(MODEM_FSK, 
                          txPower, 
                          25e3, 
                          0, 
                          Datarates[txDatarate] * 1000, 
                          0, 
                          5, 
                          false, 
                          true, 
                          0, 
                          0, 
                          false);
        TxTimeOnAir = Radio.TimeOnAir(MODEM_FSK, LoRaMacBufferPktLen);
        TxTimeOnAir = TxTimeOnAir / 1000;
    }
    else if(ChannleDatarate == DR_6)
    {
        // High speed LoRa channel
        Radio.SetTxConfig(MODEM_LORA, 
                          txPower, 
                          0, 
                          1, 
                          Datarates[txDatarate], 
                          1, 
                          8, 
                          false, 
                          true, 
                          0, 
                          0, 
                          false);
        TxTimeOnAir = Radio.TimeOnAir(MODEM_LORA, LoRaMacBufferPktLen);
        TxTimeOnAir = TxTimeOnAir / 1000;
    }
    else
    {
        // Normal LoRa channel
        Radio.SetTxConfig(MODEM_LORA, 
                          txPower, 
                          0, 
                          0, 
                          Datarates[txDatarate], 
                          1, 
                          8, 
                          false, 
                          true, 
                          0, 
                          0, 
                          false);
        TxTimeOnAir = Radio.TimeOnAir(MODEM_LORA, LoRaMacBufferPktLen);
        TxTimeOnAir = TxTimeOnAir / 1000;
        
    }

    if(LoRaParam.db.MaxDCycle == 0)
    {
        AggregatedTimeOff = 0;
    }


    //    if( MAX( Bands[channel.Band].TimeOff, AggregatedTimeOff ) > ( run_time_get( ) ) )
    //    {
    //        // Schedule transmission
    ////        TimerSetValue( &TxDelayedTimer, MAX( Bands[channel.Band].TimeOff, AggregatedTimeOff ) );
    ////        TimerStart( &TxDelayedTimer );
    //    }
    //    else
    {
        trace_notice("fre=%d, sf=%d, txp=%d\r\n", channel.FreqUp, Datarates[txDatarate], txPower);
        trace_notice("Send: ");
        trace_dump_n(LoRaMacBuffer, 15);
        trace_debug("TOA = %u\r\n", TxTimeOnAir);

        Radio.Send(LoRaMacBuffer, LoRaMacBufferPktLen);
        TIMER_START(RadioTimeoutTimer, 3000);
    }
    return LORAMAC_ERROR_NONE;
}

static void LoRaMacProcessMacCommands(uint8_t* payload, uint8_t macIndex, uint8_t commandsSize)
{
    trace_info("cmd %d: ", commandsSize - macIndex);
    trace_dump_i(&payload[macIndex], commandsSize - macIndex);
    bool paramUpdate = false;
    
    while(macIndex < commandsSize)
    {
        switch(payload[macIndex++])
        {
            case SRV_MAC_LINK_CHECK_ANS:
                trace_debug("[cmd] LINK_CHECK_ANS\r\n");
                LoRaMacEventFlags.Bits.LinkCheck = 1;
                LoRaMacEventAlohaInfo.DemodMargin = payload[macIndex++];
                LoRaMacEventAlohaInfo.NbGateways = payload[macIndex++];
                break;
            
            case SRV_MAC_DR_CHECK_ANS:
                trace_debug("[cmd] DR_CHECK_ANS\r\n");
                if(MacCmdsDrCheckAnsWait)
                {
                    trace_debug("modify confirmed~~~~\r\n");
                    MacCmdsDrCheckAnsWait = false;
                    MacCmdsDrCheck = false;
                    MacCmdsDrCheckAnsWaitCounter = 0;
                    MacCmdsAnsInNextTx = true;
                    
                    if(Rx1DatarateNew != UINT8_MAX)
                    {
                        LoRaParam.db.DataRate = ChannleDatarate = Rx1DatarateNew;
                        Rx1DatarateNew = UINT8_MAX;
                        paramUpdate = true;
                    }
                    
                    if(Rx2DatarateNew != UINT8_MAX)
                    {
                        LoRaParam.db.RX2DateRate = Rx2DatarateNew;
                        Rx2DatarateNew = UINT8_MAX;
                        paramUpdate = true;
                    }
                    
                    if(TxPowerNew != INT8_MAX)
                    {
                        LoRaParam.db.TXPower = TxPowerNew;
                        TxPowerNew = INT8_MAX;
                        paramUpdate = true;
                    }
                    
                    if(ChMaskNew != UINT32_MAX)
                    {
                        LoRaParam.db.ChMasks[0] = ChMaskNew;
                        paramUpdate = true;
                        ChMaskNew = UINT32_MAX;
                    }
                    
                    if(NbTransNew != UINT8_MAX)
                    {
                        LoRaParam.db.NbTrans = NbTransNew;
                        paramUpdate = true;
                        NbTransNew = UINT8_MAX;
                    }
                    
                    if(Rx2FreqNew != UINT32_MAX)
                    {
                        LoRaParam.db.RX2Freq = Rx2FreqNew;
                        paramUpdate = true;
                        Rx2FreqNew = UINT32_MAX;
                    }
                    
                    if(Rx1DROffsetNew != UINT8_MAX)
                    {
                        LoRaParam.db.RX1DROffset = Rx1DROffsetNew;
                        paramUpdate = true;
                        Rx1DROffsetNew = UINT8_MAX;
                    }
                    
                    if(PingDatarateNew != UINT8_MAX)
                    {
                        LoRaMacSetPingDatarate(PingDatarateNew);
                        paramUpdate = true;
                        PingDatarateNew = UINT8_MAX;
                    }
                    
                    if(ClassBEnableNew != UINT8_MAX)
                    {
//                        LoRaParam.db.ClassBEnable = ClassBEnableNew;
                        LoRaMacSetClassBEnable(ClassBEnableNew);
                        paramUpdate = true;
                        ClassBEnableNew = UINT8_MAX;
//                        DeviceClass = CLASS_A;
                    }
                    
                    if(PingPrdNew != UINT8_MAX)
                    {
                        LoRaMacSetPeriodity(PingPrdNew);
                        paramUpdate = true;
                        PingPrdNew = UINT8_MAX;
                    }
                    
                    if(ClassGDatarateNew != UINT8_MAX)
                    {
                        LoRaParam.db.ClassGDataRate = ClassGDatarateNew;
                        paramUpdate = true;
                        ClassGDatarateNew = UINT8_MAX;
                    }
                    if(ClassGEnabledNew != UINT8_MAX)
                    {
                        LoRaParam.db.ClassGEnabled = ClassGEnabledNew;
                        paramUpdate = true;
                        ClassGEnabledNew = UINT8_MAX;
                        if(LoRaParam.db.ClassGEnabled)
                        {
                            DeviceClass = CLASS_G;
                        }
                    }
                    
                    if(ClassGPrdNew != UINT8_MAX)
                    {
                        LoRaParam.db.ClassGPeriod = ClassGPrdNew;
                        paramUpdate = true;
                        ClassGPrdNew = UINT8_MAX;
                    }
                }
                break;
                
            case SRV_MAC_LINK_ADR_REQ:
            {
                trace_debug("[cmd] LINK_ADR_REQ\r\n");
                uint8_t i;
                uint8_t status = 0x07;
                uint16_t chMask = 0;
                uint8_t txPowerIdx = 0;
                int8_t txPower;
                int8_t datarate = 0;
                uint8_t nbTrans = 0;
                uint8_t chMaskCntl = 0;
                
                if(MacCmdsImmAnsInNextTx)
                {
                    MacCmdsImmAnsInNextTx = false;
                    MacCmdsImmAnsBufferIdx = 0;
                }
                
                if(MacCmdsAnsInNextTx)
                {
                    MacCmdsAnsInNextTx = false;
                    MacCmdsDelayAnsBufferIdx = 0;
                }

                datarate = payload[macIndex++];
                txPowerIdx = datarate & 0x0F;
                datarate = (datarate >> 4) & 0x0F;

                
                chMask = payload[macIndex++];
                chMask |= payload[macIndex++] << 8;

                nbTrans = payload[macIndex++];
                chMaskCntl = (nbTrans >> 4) & 0x07;
                nbTrans &= 0x0F;
                
                trace_info("datarate = %d\r\n", datarate);
                trace_info("txPowerIdx = %d\r\n", txPowerIdx);
                trace_info("chMask = %x\r\n", chMask);
                trace_info("chMaskCntl = %d\r\n", chMaskCntl);
                trace_info("nbTrans = %d\r\n", nbTrans);
                
//                if(!LoRaParam.db.Adr)
//                {
//                    AddMacCommand(MacCmdsImmAnsBuffer,&MacCmdsImmAnsBufferIdx, MOTE_MAC_LINK_ADR_ANS, 0, 0);
//                    MacCmdsImmAnsInNextTx = true;
//                    break;
//                }

                if(nbTrans == 0)
                {
                    nbTrans = 1;
                }
                if((chMaskCntl == 0) && ((chMask&0x07) == 0))
                {
                    status &= 0xFE; // Channel mask KO
                }
                else
                {
                    for(i = 0; i < LORA_MAX_NB_CHANNELS; i++)
                    {
                        if(chMaskCntl == 6)
                        {
                            if(Channels[i].FreqUp != 0)
                            {
                                chMask |= 1 << i;
                            }
                        }
                        else
                        {
                            if(((chMask & (1 << i)) != 0) &&
                               (Channels[i].FreqUp == 0))
                            {
                                // Trying to enable an undefined channel
                                status &= 0xFE; // Channel mask KO
                            }
                        }
                    }
                }
                
                if((datarate < LORAMAC_MIN_DATARATE) || (datarate > LORAMAC_MAX_DATARATE))
                {
                    status &= 0xFD; // Datarate KO
                }

                if( txPowerIdx > LORAMAC_MIN_TX_POWER )
                {
                    status &= 0xFB; // TxPower KO
                }
                if((status & 0x07) == 0x07)
                {
                    bool drcheck = false;
                    
                    if(ChannleDatarate != datarate)
                    {
                        if(datarate > ChannleDatarate)
                        {
                            Rx1DatarateNew = datarate;
                            drcheck = true;
                        }
                        else
                        {
                            ChannleDatarate = LoRaParam.db.DataRate = datarate;
                            paramUpdate = true;
                        }
                    }
                    
                    txPower = TxPowers[txPowerIdx];
                    if(LoRaParam.db.TXPower != txPower)
                    {
                        if(txPower < LoRaParam.db.TXPower)
                        {
                            TxPowerNew = txPower;
                            drcheck = true;
                        }
                        else
                        {
                            LoRaParam.db.TXPower = txPower;
                            paramUpdate = true;
                        }
                    }
                    
                    if(LoRaParam.db.ChMasks[0] != chMask)
                    {
                        if(drcheck)
                        {
                            ChMaskNew = chMask;
                        }
                        else
                        {
                            LoRaParam.db.ChMasks[0] = chMask;
                            paramUpdate = true;
                        }
                    }
                    
                    if(LoRaParam.db.NbTrans != nbTrans)
                    {
                        if(drcheck)
                        {
                            NbTransNew = nbTrans;
                        }
                        else
                        {
                            LoRaParam.db.NbTrans = nbTrans;
                            paramUpdate = true;
                        }
                    }
                    trace_info("MacCmdsDrCheck ~\r\n");
                    if(drcheck)
                    {
                        AddMacCommand(MacCmdsDelayAnsBuffer, &MacCmdsDelayAnsBufferIdx, MOTE_MAC_LINK_ADR_ANS, status, 0);
                        MacCmdsDrCheck = true;
                        break;
                    }
                }
                AddMacCommand(MacCmdsImmAnsBuffer,&MacCmdsImmAnsBufferIdx, MOTE_MAC_LINK_ADR_ANS, status, 0);
                MacCmdsImmAnsInNextTx = true;
            }
            break;
            
            case SRV_MAC_DUTY_CYCLE_REQ:
            {
                uint8_t maxDCycle = 0;
                trace_debug("[cmd] DUTY_CYCLE_REQ\r\n");
                if(MacCmdsImmAnsInNextTx)
                {
                    MacCmdsImmAnsInNextTx = false;
                    MacCmdsImmAnsBufferIdx = 0;
                }
                maxDCycle = payload[macIndex++];
                if(LoRaParam.db.MaxDCycle != maxDCycle)
                {
                    LoRaParam.db.MaxDCycle = maxDCycle;
                    paramUpdate = true;
                }
                AggregatedDCycle = 1 << LoRaParam.db.MaxDCycle;
                AddMacCommand(MacCmdsImmAnsBuffer,&MacCmdsImmAnsBufferIdx, MOTE_MAC_DUTY_CYCLE_ANS, 0, 0);
                MacCmdsImmAnsInNextTx = true;
            }
                break;
            
            case SRV_MAC_RX_PARAM_SETUP_REQ:
                trace_debug("[cmd] RX_PARAM_SETUP_REQ\r\n");
                {
                    uint8_t status = 0x07;
                    int8_t datarate = 0;
                    int8_t drOffset = 0;
                    uint32_t freq = 0;
                    
                    if(MacCmdsImmAnsInNextTx)
                    {
                        MacCmdsImmAnsInNextTx = false;
                        MacCmdsImmAnsBufferIdx = 0;
                    }
                
                    if(MacCmdsAnsInNextTx)
                    {
                        MacCmdsAnsInNextTx = false;
                        MacCmdsDelayAnsBufferIdx = 0;
                    }

                    drOffset = payload[macIndex++];
                    datarate = drOffset & 0x0F;
                    drOffset = (drOffset >> 4) & 0x0F;

                    freq = payload[macIndex++];
                    freq |= payload[macIndex++] << 8;
                    freq |= payload[macIndex++] << 16;
                    freq *= 100;
                    
                    trace_notice("freq=%d, DR=%x, drOffset=%x\r\n", freq, datarate, drOffset);

                    if(!Radio.CheckRfFrequency(freq))
                    {
                        status &= 0xFE; // Channel frequency KO
                    }

                    if((datarate < LORAMAC_MIN_DATARATE) || (datarate > LORAMAC_MAX_DATARATE))
                    {
                        status &= 0xFD; // Datarate KO
                    }

                    if((drOffset < 0) || (drOffset > 5))
                    {
                        status &= 0xFB; // LoRaParam.db.RX1DROffset range KO
                    }

                    if((status & 0x07) == 0x07)
                    {
                        bool drcheck = false;
                        
                        if(LoRaParam.db.RX2DateRate != datarate)
                        {
                            if(datarate > LoRaParam.db.RX2DateRate)
                            {
                                Rx2DatarateNew = datarate;
                                drcheck = true;
                            }
                            else
                            {
                                LoRaParam.db.RX2DateRate = datarate;
                                paramUpdate = true;
                            }
                        }
                        
                        if(LoRaParam.db.RX2Freq != freq)
                        {
                            if(drcheck)
                            {
                                Rx2FreqNew = freq;
                            }
                            else
                            {
                                LoRaParam.db.RX2Freq = freq;
                                paramUpdate = true;
                            }
                        }
                        if(LoRaParam.db.RX1DROffset != drOffset)
                        {
                            if(drcheck)
                            {
                                Rx1DROffsetNew = drOffset;
                            }
                            else
                            {
                                LoRaParam.db.RX1DROffset = drOffset;
                                paramUpdate = true;
                            }
                        }
                        
                        if(drcheck)
                        {                       
                            AddMacCommand(MacCmdsDelayAnsBuffer, &MacCmdsDelayAnsBufferIdx, MOTE_MAC_RX_PARAM_SETUP_ANS, status, 0);
                            MacCmdsDrCheck = true;
                            break;
                        }
                    }
                    AddMacCommand(MacCmdsImmAnsBuffer,&MacCmdsImmAnsBufferIdx, MOTE_MAC_RX_PARAM_SETUP_ANS, status, 0);
                    MacCmdsImmAnsInNextTx = true;
                }
                break;
                
            case SRV_MAC_DEV_STATUS_REQ:
                trace_debug("[cmd] DEV_STATUS_REQ\r\n");
                if(MacCmdsImmAnsInNextTx)
                {
                    MacCmdsImmAnsInNextTx = false;
                    MacCmdsImmAnsBufferIdx = 0;
                }
                AddMacCommand( MacCmdsImmAnsBuffer, &MacCmdsImmAnsBufferIdx,MOTE_MAC_DEV_STATUS_ANS,  LoRaMacMeasureBatterieLevel(), LoRaMacEventAlohaInfo.RxSnr );
                MacCmdsImmAnsInNextTx = true;
                break;
            
            case SRV_MAC_NEW_CHANNEL_REQ:
                trace_debug("[cmd] NEW_CHANNEL_REQ\r\n");
                {
                    uint8_t status = 0x03;
                    int8_t channelIndex = 0;
                    ChannelParams_t chParam;
                    
                    if(MacCmdsImmAnsInNextTx)
                    {
                        MacCmdsImmAnsInNextTx = false;
                        MacCmdsImmAnsBufferIdx = 0;
                    }
                
                    if(MacCmdsAnsInNextTx)
                    {
                        MacCmdsAnsInNextTx = false;
                        MacCmdsDelayAnsBufferIdx = 0;
                    }

                    channelIndex = payload[macIndex++];
                    chParam.FreqUp = payload[macIndex++];
                    chParam.FreqUp |= payload[macIndex++] << 8;
                    chParam.FreqUp |= payload[macIndex++] << 16;
                    chParam.FreqUp *= 100;
                    chParam.FreqDown = chParam.FreqUp;
                    chParam.DrRange.Value = payload[macIndex++];
                    
                    trace_notice("new ch, freq=%d, chIdx=%d, DrRg=%02X\r\n", chParam.FreqUp, 
                                 channelIndex, chParam.DrRange.Value);

                    if((channelIndex < 3) || (channelIndex > LORA_MAX_NB_CHANNELS))
                    {
                        status &= 0xFE; // Channel frequency KO
                    }

                    if(!Radio.CheckRfFrequency(chParam.FreqUp))
                    {
                        status &= 0xFE; // Channel frequency KO
                    }

                    if((chParam.DrRange.Min > chParam.DrRange.Max) ||
                       (chParam.DrRange.Min < LORAMAC_MIN_DATARATE ) ||
                       (chParam.DrRange.Max > LORAMAC_MAX_DATARATE))
                    {
                        status &= 0xFD; // Datarate range KO
                    }
                    
                    if(status == 0x03)
                    {
                        LoRaMacSetChannel(channelIndex, chParam);
                       
                        LoRaParam.db.AlohaChannel[channelIndex].FreqUp = chParam.FreqUp;
                        LoRaParam.db.AlohaChannel[channelIndex].FreqDown = chParam.FreqUp;
                        LoRaParam.db.AlohaChannel[channelIndex].DRMax = chParam.DrRange.Max;
                        LoRaParam.db.AlohaChannel[channelIndex].DRMin = chParam.DrRange.Min;
                        LoRaParam.db.AlohaChannel[channelIndex].Band = Channels[channelIndex].Band;
                        paramUpdate = true;
                    }
                    
                    AddMacCommand(MacCmdsImmAnsBuffer,&MacCmdsImmAnsBufferIdx, MOTE_MAC_NEW_CHANNEL_ANS, status, 0);
                    MacCmdsImmAnsInNextTx = true;
                }
                break;
                
            case SRV_MAC_RX_TIMING_SETUP_REQ:
                trace_debug("[cmd] RX_TIMING_SETUP_REQ\r\n");
                {
                    uint8_t delay = payload[macIndex++] & 0x0F;
                    
                    if(MacCmdsImmAnsInNextTx)
                    {
                        MacCmdsImmAnsInNextTx = false;
                        MacCmdsImmAnsBufferIdx = 0;
                    }
                    
                    trace_notice("delay = %d\r\n", delay);
                    if(delay == 0)
                    {
                        delay++;
                    }
                    if( LoRaParam.db.Del != delay)
                    {
                        LoRaParam.db.Del = delay;
                        paramUpdate = true;
                    }
                    AddMacCommand(MacCmdsImmAnsBuffer,&MacCmdsImmAnsBufferIdx, MOTE_MAC_RX_TIMING_SETUP_ANS, 0, 0);
                    MacCmdsImmAnsInNextTx = true;
                }
                break;
                
            case SRV_MAC_TX_PARAM_SETUP_REQ:
            {
                trace_debug("[cmd] TX_PARAM_SETUP_REQ\r\n");
                uint8_t eirp_dwelltime = payload[macIndex++];
                trace_notice("eirp_dwelltime = %x\r\n", eirp_dwelltime);
                AddMacCommand(MacCmdsImmAnsBuffer,&MacCmdsImmAnsBufferIdx, MOTE_MAC_TX_PARAM_SETUP_ANS, 0, 0);
                MacCmdsImmAnsInNextTx = true;
            }
                break;
            
            case SRV_MAC_DI_CHANNEL_REQ:
            {
                trace_debug("[cmd] DI_CHANNEL_REQ\r\n");
                uint8_t status = 0x03;
                uint8_t channelIndex = 0;
                uint32_t freq = 0;
                
                channelIndex = payload[macIndex++];
                freq = payload[macIndex++];
                freq |= payload[macIndex++] << 8;
                freq |= payload[macIndex++] << 16;
                freq *= 100;
                
                trace_notice("channelIndex = %d, freq = %d\r\n", channelIndex, freq);
                
                if(!Radio.CheckRfFrequency(freq))
                {
                    status &= 0xFE; // Channel frequency KO
                }
                
                if(LoRaParam.db.AlohaChannel[channelIndex].FreqUp == 0)
                {
                    status &= 0xFd;
                }
                
                if(status == 0x03)
                {
                    Channels[channelIndex].FreqDown = LoRaParam.db.AlohaChannel[channelIndex].FreqDown = freq;
                    paramUpdate = true;
                }
                
                AddMacCommand(MacCmdsImmAnsBuffer,&MacCmdsImmAnsBufferIdx, MOTE_MAC_DI_CHANNEL_ANS, status, 0);
                MacCmdsImmAnsInNextTx = true;
                UNUSED_VARIABLE(channelIndex);
            }
                break;

            case SRV_MAC_PING_SLOT_INFO_ANS:
                // no payload
                PingSlotInfoAnsed = true;
                trace_debug("[cmd] PING_SLOT_INFO_ANS\r\n");
                break;
            
            case SRV_MAC_PING_SLOT_CHANNEL_REQ:
            {
                trace_debug("SRV_MAC_PING_SLOT_CHANNEL_REQ\r\n");
                uint8_t status = 0x03;
                ChannelParams_t chParam;
                
                if(MacCmdsImmAnsInNextTx)
                {
                    MacCmdsImmAnsInNextTx = false;
                    MacCmdsImmAnsBufferIdx = 0;
                }
                
                chParam.FreqDown = payload[macIndex++];
                chParam.FreqDown |= payload[macIndex++] << 8;
                chParam.FreqDown |= payload[macIndex++] << 16;
                chParam.FreqDown *= 100;
                chParam.DrRange.Value = payload[macIndex++];
                
                trace_notice("ping freq=%d, DrRange = %02x\r\n", chParam.FreqDown, chParam.DrRange.Value);
                
                if(!Radio.CheckRfFrequency(chParam.FreqDown))
                {
                    status &= 0xFE; // Channel frequency KO
                }

                if((chParam.DrRange.Min > chParam.DrRange.Max) ||
                   (chParam.DrRange.Min < LORAMAC_MIN_DATARATE ) ||
                   (chParam.DrRange.Max > LORAMAC_MAX_DATARATE))
                {
                    status &= 0xFD; // Datarate range KO
                }
                if((status & 0x03) == 0x03)
                {
                    if(LoRaParam.db.PingChannel.FreqDown != chParam.FreqDown)
                    {
                        LoRaParam.db.PingChannel.FreqDown = chParam.FreqDown;
                        LoRaParam.db.PingChannel.DRMax = chParam.DrRange.Max;
                        LoRaParam.db.PingChannel.DRMin = chParam.DrRange.Min;
                        paramUpdate = true;
                    }
                }
                
                AddMacCommand(MacCmdsImmAnsBuffer,&MacCmdsImmAnsBufferIdx, MOTE_MAC_PING_SLOT_FREQ_ANS, status, 0);
                MacCmdsImmAnsInNextTx = true;
            }
                break;
            
            case SRV_MAC_CUS_PING_SLOT_INFO_REQ:
            {
                uint8_t daterate = 0;
                uint8_t prd = 0;
                uint8_t enable = 0;
                uint8_t status = 0x07;
                bool drcheck = false;
                
                uint8_t cmd = payload[macIndex++];
                
                daterate = cmd & 0x0f;
                prd = (cmd & 0x70) >> 4;
                enable = (cmd & 0x80) >> 7;
                
                trace_notice("ping dr=%d, prd=%d, enable=%d\r\n", daterate, prd, enable);
                
                if(MacCmdsImmAnsInNextTx)
                {
                    MacCmdsImmAnsInNextTx = false;
                    MacCmdsImmAnsBufferIdx = 0;
                }
            
                if(MacCmdsAnsInNextTx)
                {
                    MacCmdsAnsInNextTx = false;
                    MacCmdsDelayAnsBufferIdx = 0;
                }
                
                if(daterate > LORAMAC_MAX_DATARATE)
                {
                    status &= 0XFE;
                }
                
                if(prd > 7)
                {
                    status &= 0xFD;
                }
                
                if(status == 0X07)
                {
                    if(LoRaParam.db.PingDataRate != daterate)
                    {
                        if(daterate > LoRaParam.db.PingDataRate)
                        {
                            PingDatarateNew = daterate;
                            drcheck = true;
                        }
                        else
                        {
                            LoRaMacSetPingDatarate(daterate);
                            paramUpdate = true;
                        }
                    }
                    
                    if(LoRaParam.db.PingPeriodicity != prd)
                    {
                        if(drcheck)
                        {
                            PingPrdNew = prd;
                        }
                        else
                        {
                            LoRaMacSetPeriodity(prd);
                            paramUpdate = true;
                        }
                    }
                    
                    if(LoRaParam.db.ClassBEnable != enable)
                    {
                        if(drcheck)
                        {
                            ClassBEnableNew = enable;
                        }
                        else
                        {
                            LoRaMacSetClassBEnable(enable);
                            paramUpdate = true;
                        }
                    }
                    if(drcheck)
                    {                       
                        AddMacCommand(MacCmdsDelayAnsBuffer, &MacCmdsDelayAnsBufferIdx, MOTE_MAC_CUS_PING_SLOT_INFO_ANS, status, 0);
                        MacCmdsDrCheck = true;
                        break;
                    }                 
                }
                AddMacCommand(MacCmdsImmAnsBuffer,&MacCmdsImmAnsBufferIdx, MOTE_MAC_CUS_PING_SLOT_INFO_ANS, status, 0);
                MacCmdsImmAnsInNextTx = true;
            }
                break;
            
            case SRV_MAC_BEACON_TIMING_ANS:
                if((CLASS_A == DeviceClass) && LoRaParam.db.ClassBEnable)
                {
                    if(!IsBeaconSearching)
                    {
                        uint16_t delay = 0;
                        uint8_t channleIndex = 0;
                        
                        delay = payload[macIndex++];
                        delay |= payload[macIndex++] << 8;
                        channleIndex = payload[macIndex++];
                        UNUSED_VARIABLE(channleIndex);
                        
                        if(delay> 0)
                        {
                            BeaconSymbTimeout = 16;
                            uint32_t nextBeaconDelay =  delay*30 - RxTimeOnAir - (2 * SymbTime[LoRaParam.db.BeaconDataRate]/1000);
                            
                            if(nextBeaconDelay > 500 && nextBeaconDelay < 128000)
                            {
                                IsBeaconSearching = true;
                                TIMER_START(BeaconTimer, nextBeaconDelay);
                            }
                            trace_notice("beacon delay = %d, %d, %d\r\n", delay, delay*30, nextBeaconDelay);
                        }
                    }
                }
                break;
            
            case SRV_MAC_BEACON_FREQ_REQ:
            {
                uint8_t status = 0x01;
                uint32_t freq = 0;
                
                freq = payload[macIndex++];
                freq |= payload[macIndex++] << 8;
                freq |= payload[macIndex++] << 16;
                freq *= 100;
                
                trace_info("beacon freq = %d\r\n", freq);
                
                if(!Radio.CheckRfFrequency(freq))
                {
                    status &= 0xFE; // Channel frequency KO
                }
                if(0x01 == status)
                {
                    if(LoRaParam.db.BeaconFreq != freq)
                    {
                        LoRaParam.db.BeaconFreq = freq;
                        paramUpdate = true;
                    }
                }
                AddMacCommand(MacCmdsImmAnsBuffer,&MacCmdsImmAnsBufferIdx, MOTE_MAC_BEACON_FREQ_ANS, status, 0);
                MacCmdsImmAnsInNextTx = true;
            }
                break;
            
            case SRV_MAC_INTERVAL_LIMIT_REQ:
            {
                trace_debug("[cmd] INTV_LIMIT_REQ\r\n");
                uint32_t limit = 0;
                uint8_t status = 0x01;
                
                if(MacCmdsImmAnsInNextTx)
                {
                    MacCmdsImmAnsInNextTx = false;
                    MacCmdsImmAnsBufferIdx = 0;
                }
                
                limit = uint24_decode(&payload[macIndex]);
                macIndex += 3;
                trace_info("limit = %u\r\n", limit);
                
                if(limit <= 7200)
                {
                    SilenceTime = limit;
                }
                else
                {
                    status = 0x00;
                }
                
                AddMacCommand(MacCmdsImmAnsBuffer,&MacCmdsImmAnsBufferIdx, MOTE_MAC_INTERVAL_LIMIT_ANS, status, 0);
                MacCmdsImmAnsInNextTx = true;
            }
                break;
            
            case SRV_MAC_CMD_ANS:
                trace_debug("[cmd] CMD_ANS\r\n");
                MacCmdsAnsInNextTx = false;
                MacCmdsDelayAnsBufferIdx = 0;
            
                MacCmdsImmAnsInNextTx = false;
                MacCmdsImmAnsBufferIdx = 0;
                break;
            
            case SRV_MAC_VERSION_ANS:
                trace_debug("[cmd] VERSION_ANS\r\n");
                VersionAnsed = true;
                break;
            
            case SRV_MAC_CLASSG_PARAM_REQ:
            {
                trace_debug("[cmd] CLASSG_PARAM_REQ\r\n");
                
                uint8_t daterate = 0;
                uint8_t prd = 0;
                uint8_t enable = 0;
                uint8_t status = 0x07;
                bool drcheck = false;
                
                uint8_t cmd = payload[macIndex++];
                
                daterate = cmd & 0x0f;
                enable = (cmd & 0x80) >> 7;
                
                prd = payload[macIndex++];
                
                
                trace_notice("classg dr = %d, prd = %d, enable = %d\r\n", daterate, prd, enable);
                
                if(MacCmdsImmAnsInNextTx)
                {
                    MacCmdsImmAnsInNextTx = false;
                    MacCmdsImmAnsBufferIdx = 0;
                }
            
                if(MacCmdsAnsInNextTx)
                {
                    MacCmdsAnsInNextTx = false;
                    MacCmdsDelayAnsBufferIdx = 0;
                }
                
                if(daterate > LORAMAC_MAX_DATARATE)
                {
                    status &= 0XFD;
                }
                
                if(prd == 0)
                {
                    prd = 1;
                }
                
                if(status == 0X07)
                {
                    if(LoRaParam.db.ClassGDataRate != daterate)
                    {
                        if(daterate > LoRaParam.db.ClassGDataRate)
                        {
                            trace_debug("drcheck\r\n");
                            ClassGDatarateNew = daterate;
                            drcheck = true;
                        }
                        else
                        {
                            LoRaParam.db.ClassGDataRate = daterate;
                            paramUpdate = true;
                        }
                    }
                    
                    if(LoRaParam.db.ClassGPeriod != prd)
                    {
                        if(drcheck)
                        {
                            ClassGPrdNew = prd;
                        }
                        else
                        {
                            LoRaParam.db.ClassGPeriod = prd;
                            paramUpdate = true;
                        }
                    }
                    
                    if(LoRaParam.db.ClassGEnabled != enable)
                    {
                        if(drcheck)
                        {
                            ClassGEnabledNew = enable;
                        }
                        else
                        {
                            LoRaParam.db.ClassGEnabled = enable;
                            paramUpdate = true;
                            DeviceClass = CLASS_G;
                        }
                    }
                    if(drcheck)
                    {                       
                        AddMacCommand(MacCmdsDelayAnsBuffer, &MacCmdsDelayAnsBufferIdx, MOTE_MAC_CLASSG_PARAM_ANS, status, 0);
                        MacCmdsDrCheck = true;
                        break;
                    }                 
                }
                AddMacCommand(MacCmdsImmAnsBuffer,&MacCmdsImmAnsBufferIdx, MOTE_MAC_CLASSG_PARAM_ANS, status, 0);
                MacCmdsImmAnsInNextTx = true;
            }
                break;
            
            case SRV_MAC_CLASSG_CHANNEL_REQ:
            {
                trace_debug("[cmd] CLASSG_CHANNEL_REQ\r\n");
                uint8_t status = 0x03;
                ChannelParams_t chParam;
                
                if(MacCmdsImmAnsInNextTx)
                {
                    MacCmdsImmAnsInNextTx = false;
                    MacCmdsImmAnsBufferIdx = 0;
                }
                
                chParam.FreqDown = payload[macIndex++];
                chParam.FreqDown |= payload[macIndex++] << 8;
                chParam.FreqDown |= payload[macIndex++] << 16;
                chParam.FreqDown *= 100;
                chParam.DrRange.Value = payload[macIndex++];
                
                trace_notice("classg freq = %d, drRg = %02X\r\n", chParam.FreqDown, chParam.DrRange.Value);
                
                if(!Radio.CheckRfFrequency(chParam.FreqDown))
                {
                    status &= 0xFE; // Channel frequency KO
                }

                if((chParam.DrRange.Min > chParam.DrRange.Max) ||
                   (chParam.DrRange.Min < LORAMAC_MIN_DATARATE ) ||
                   (chParam.DrRange.Max > LORAMAC_MAX_DATARATE))
                {
                    status &= 0xFD; // Datarate range KO
                }
                if((status & 0x03) == 0x03)
                {
                    if(LoRaParam.db.ClassGChannel.FreqDown != chParam.FreqDown)
                    {
                        LoRaParam.db.ClassGChannel.FreqDown = chParam.FreqDown;
                        LoRaParam.db.ClassGChannel.DRMax = chParam.DrRange.Max;
                        LoRaParam.db.ClassGChannel.DRMin = chParam.DrRange.Min;
                        paramUpdate = true;
                    }
                }
                
                AddMacCommand(MacCmdsImmAnsBuffer,&MacCmdsImmAnsBufferIdx, MOTE_MAC_CLASSG_CHANNEL_ANS, status, 0);
                MacCmdsImmAnsInNextTx = true;
            }
                break;

            default:
                trace_warn("Unknown command~~~\r\n");
                // Unknown command. ABORT MAC commands processing
                return;
        }
    }
    
    if(paramUpdate)
    {
        paramUpdate = false;
        LoRaMacUpdateConfig();
    }
}



static void OnRadioTimeoutEvent(void *p_context)
{
    Radio.OnTimeout();
}

/*!
 * Function to be executed on Rx Done event
 */
static void LoRaMacAlohaDataDecode(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr)
{
    LoRaMacHeader_t macHdr;
    LoRaMacFrameCtrl_t fCtrl;

    uint8_t pktHeaderLen = 0;
    uint32_t address = 0;
    uint16_t sequenceCounter = 0;
    int32_t sequence = 0;
    uint8_t appPayloadStartIndex = 0;
    uint8_t port = 0xFF;
    uint8_t frameLen = 0;
    uint32_t mic = 0;
    uint32_t micRx = 0;

    MulticastParams_t* curMulticastParams = NULL;
    uint8_t* nwkSKey = (uint8_t *)LoRaParam.db.NwkSKey;
    uint8_t* appSKey = (uint8_t *)LoRaParam.db.AppSKey;
    uint32_t downLinkCounter = 0;

    bool isMicOk = false;
    
    macHdr.Value = payload[pktHeaderLen++];

    switch(macHdr.Bits.MType)
    {
        case FRAME_TYPE_JOIN_ACCEPT:
            trace_info("FRAME_TYPE_JOIN_ACCEPT\r\n");
            if(IsLoRaMacNetworkJoined == true)
            {
                trace_warn("already joined\r\n");
                LoRaMacEventFlags.Bits.Tx = 1;
                LoRaMacEventAlohaInfo.Status = LORAMAC_EVENT_INFO_STATUS_MIC_FAIL;
                break;
            }
            LoRaMacJoinDecrypt(payload + 1, size - 1, (uint8_t *)LoRaParam.db.AppKey, LoRaMacRxPayload + 1);

            LoRaMacRxPayload[0] = macHdr.Value;

            LoRaMacJoinComputeMic(LoRaMacRxPayload, size - LORAMAC_MFR_LEN, (uint8_t *)LoRaParam.db.AppKey, &mic);

            micRx |= LoRaMacRxPayload[size - LORAMAC_MFR_LEN];
            micRx |= (LoRaMacRxPayload[size - LORAMAC_MFR_LEN + 1] << 8);
            micRx |= (LoRaMacRxPayload[size - LORAMAC_MFR_LEN + 2] << 16);
            micRx |= (LoRaMacRxPayload[size - LORAMAC_MFR_LEN + 3] << 24);

            if(micRx == mic)
            {
                LoRaMacEventFlags.Bits.Rx = 1;
                LoRaMacEventAlohaInfo.RxSnr = snr;
                LoRaMacEventAlohaInfo.RxRssi = rssi;

                LoRaMacJoinComputeSKeys((uint8_t *)LoRaParam.db.AppKey, LoRaMacRxPayload + 1, LoRaParam.db.DevNonce, (uint8_t *)LoRaParam.db.NwkSKey, (uint8_t *)LoRaParam.db.AppSKey);

                LoRaParam.db.NetID = LoRaMacRxPayload[4];
                LoRaParam.db.NetID |= (LoRaMacRxPayload[5] << 8);
                LoRaParam.db.NetID |= (LoRaMacRxPayload[6] << 16);

                LoRaParam.db.DevAddr = LoRaMacRxPayload[7];
                LoRaParam.db.DevAddr |= (LoRaMacRxPayload[8] << 8);
                LoRaParam.db.DevAddr |= (LoRaMacRxPayload[9] << 16);
                LoRaParam.db.DevAddr |= (LoRaMacRxPayload[10] << 24);

                // DLSettings
                LoRaParam.db.RX1DROffset = (LoRaMacRxPayload[11] >> 4) & 0x07;
                LoRaParam.db.RX2DateRate = LoRaMacRxPayload[11] & 0x0F;

                // RxDelay
                LoRaParam.db.Del = (LoRaMacRxPayload[12] & 0x0F);
                if(LoRaParam.db.Del == 0)
                {
                    LoRaParam.db.Del = 1;
                }

                //CFList
                if((size - 1) > 16)
                {
                    ChannelParams_t param;
                    param.DrRange.Value = (DR_5 << 4) | DR_0;

                    for(uint8_t i = 3, j = 0; i < (5 + 3); i++, j += 3)
                    {
                        param.FreqUp = (LoRaMacRxPayload[13 + j] | (LoRaMacRxPayload[14 + j] << 8) | (LoRaMacRxPayload[15 + j] << 16)) * 100;
                        LoRaMacSetChannel(i, param);
                    }
                }

                LoRaMacEventFlags.Bits.JoinAccept = 1;
                IsLoRaMacNetworkJoined = true;
                ChannleDatarate = LoRaParam.db.DataRate;
                LoRaMacEventAlohaInfo.Status = LORAMAC_EVENT_INFO_STATUS_OK;
            }
            else
            {
                trace_warn("mic error\r\n");
                LoRaMacEventAlohaInfo.Status = LORAMAC_EVENT_INFO_STATUS_JOIN_FAIL;
            }

            LoRaMacEventFlags.Bits.Tx = 1;
            break;
        case FRAME_TYPE_DATA_UNCONFIRMED_DOWN:
        case FRAME_TYPE_DATA_CONFIRMED_DOWN:
        {
            address = payload[pktHeaderLen++];
            address |= (payload[pktHeaderLen++] << 8);
            address |= (payload[pktHeaderLen++] << 16);
            address |= (payload[pktHeaderLen++] << 24);
            
            
            if(address != LoRaParam.db.DevAddr)
            {
                trace_warn("DevAddr error %08X!=%08X\r\n", address, LoRaParam.db.DevAddr);
                curMulticastParams = MulticastChannels;
                while(curMulticastParams != NULL)
                {
                    if(address == curMulticastParams->Address)
                    {
                        LoRaMacEventFlags.Bits.Multicast = 1;
                        nwkSKey = curMulticastParams->NwkSKey;
                        appSKey = curMulticastParams->AppSKey;
                        downLinkCounter = curMulticastParams->DownLinkCounter;
                        break;
                    }
                    curMulticastParams = curMulticastParams->Next;
                }
                if(LoRaMacEventFlags.Bits.Multicast == 0)
                {
                    // We are not the destination of this frame.
                    LoRaMacEventFlags.Bits.Tx = 1;
                    LoRaMacEventAlohaInfo.Status = LORAMAC_EVENT_INFO_STATUS_ADDRESS_FAIL;
//                    LoRaMacAlohaState &= ~MAC_ALOHA_TX_RUNNING;
//                    if(LoRaMacEventFlags.Bits.RxSlot == 0)
//                    {
//                        if(NodeAckRequested == true)
//                        {
//                            trace_info("ts AckTimeoutTimer\n");
//                            if(AckTimeoutRetriesCounter < AckTimeoutRetries)
//                            {
//                                AckTimeoutRetry = true;
//                                TIMER_START(AckTimeoutTimer, ACK_TIMEOUT + RANDR(-ACK_TIMEOUT_RND, ACK_TIMEOUT_RND));
//                            }
//                        }
//                    }
                    
                    return;
                }
            }
            else
            {
                LoRaMacEventFlags.Bits.Multicast = 0;
                nwkSKey = (uint8_t *)LoRaParam.db.NwkSKey;
                appSKey = (uint8_t *)LoRaParam.db.AppSKey;
                downLinkCounter = DownLinkCounter;
            }

            fCtrl.Value = payload[pktHeaderLen++];
            trace_verbose("fCtrl=%02X\r\n", fCtrl.Value);

            sequenceCounter |= payload[pktHeaderLen++];
            sequenceCounter |= payload[pktHeaderLen++] << 8;

            trace_notice("dfcnt=%d\r\n", sequenceCounter);

            appPayloadStartIndex = 8 + fCtrl.Bits.FOptsLen;

            micRx |= payload[size - LORAMAC_MFR_LEN];
            micRx |= (payload[size - LORAMAC_MFR_LEN + 1] << 8);
            micRx |= (payload[size - LORAMAC_MFR_LEN + 2] << 16);
            micRx |= (payload[size - LORAMAC_MFR_LEN + 3] << 24);

            sequence = (int32_t)sequenceCounter - (int32_t)(downLinkCounter & 0xFFFF);

            if(sequence < 0)
            {
                trace_warn("sequence < 0 \r\n");
                // sequence reset or roll over happened
                downLinkCounter = (downLinkCounter & 0xFFFF0000) | (sequenceCounter + (uint32_t)0x10000);
                LoRaMacComputeMic(payload, size - LORAMAC_MFR_LEN, nwkSKey, address, DOWN_LINK, downLinkCounter, &mic);
                if(micRx == mic)
                {
                    isMicOk = true;
                }
                else
                {
                    trace_warn("mic error\r\n");
                    isMicOk = false;
                    // sequence reset
                    if(LoRaMacEventFlags.Bits.Multicast == 1)
                    {
                        curMulticastParams->DownLinkCounter = downLinkCounter = sequenceCounter;
                    }
                    else
                    {
                        DownLinkCounter = downLinkCounter = sequenceCounter;
                    }
                    LoRaMacComputeMic(payload, size - LORAMAC_MFR_LEN, nwkSKey, address, DOWN_LINK, downLinkCounter, &mic);
                }
            }
            else
            {
                downLinkCounter = (downLinkCounter & 0xFFFF0000) | sequenceCounter;
                LoRaMacComputeMic(payload, size - LORAMAC_MFR_LEN, nwkSKey, address, DOWN_LINK, downLinkCounter, &mic);
            }

            if((isMicOk == true) ||
               (micRx == mic))      
            {
                LoRaMacEventFlags.Bits.Rx = 1;
                LoRaMacEventAlohaInfo.RxSnr = snr;
                LoRaMacEventAlohaInfo.RxRssi = rssi;
                LoRaMacEventAlohaInfo.RxBufferSize = 0;
                AdrAckCounter = 0;

                if(LoRaMacEventFlags.Bits.Multicast == 1)
                {
                    curMulticastParams->DownLinkCounter = downLinkCounter;
                }
                else
                {
                    DownLinkCounter = downLinkCounter;
                }

                if(macHdr.Bits.MType == FRAME_TYPE_DATA_CONFIRMED_DOWN)
                {
                    trace_info("confirm down\r\n");
                    SrvAckRequested = true;
                }
                else
                {
                    SrvAckRequested = false;
                }
                // Check if the frame is an acknowledgement
                if(fCtrl.Bits.Ack == 1)
                {
                    LoRaMacEventAlohaInfo.TxAckReceived = true;

                    trace_info("fCtrl.Bits.Ack == 1 -------------------\r\n");

                    // Stop the AckTimeout timer as no more retransmissions are needed.
                    trace_info("stop timer\n");
                    AckTimeoutRetry = false;
                    TIMER_STOP(AckTimeoutTimer);
                }
                else
                {
                    LoRaMacEventAlohaInfo.TxAckReceived = false;

//                    if(LoRaMacEventFlags.Bits.RxSlot == 0)
//                    {
//                        if(NodeAckRequested == true)
//                        {
//                            trace_info("ts AckTimeoutTimer\n");
//                            if(AckTimeoutRetriesCounter < AckTimeoutRetries)
//                            {
//                                AckTimeoutRetry = true;
//                                TIMER_START(AckTimeoutTimer, ACK_TIMEOUT + RANDR(-ACK_TIMEOUT_RND, ACK_TIMEOUT_RND));
//                            }
//                        }
//                    }
                }

                if(fCtrl.Bits.FOptsLen > 0)
                {
                    // Decode Options field MAC commands
                    LoRaMacProcessMacCommands(payload, 8, appPayloadStartIndex);
                }

                if(((size - 4) - appPayloadStartIndex) > 0)
                {
                    port = payload[appPayloadStartIndex++];
                    frameLen = (size - 4) - appPayloadStartIndex;

                    if(port == 0)
                    {
                        LoRaMacPayloadDecrypt(payload + appPayloadStartIndex,
                                              frameLen,
                                              nwkSKey,
                                              address,
                                              DOWN_LINK,
                                              downLinkCounter,
                                              LoRaMacRxPayload);
                        // Decode frame payload MAC commands
                        trace_info("port = %d, MacCommands: ", port);
                        trace_dump_i(LoRaMacRxPayload, frameLen);
                        LoRaMacProcessMacCommands(LoRaMacRxPayload, 0, frameLen);
                    }
                    else if(port == 224)
                    {
                        memcpy(LoRaMacRxPayload, payload + appPayloadStartIndex, frameLen);
                        LoRaMacEventFlags.Bits.RxData = 1;
                        LoRaMacEventAlohaInfo.RxPort = port;
                        LoRaMacEventAlohaInfo.RxBuffer = LoRaMacRxPayload;
                        LoRaMacEventAlohaInfo.RxBufferSize = frameLen;
                    }
                    else
                    {
                        LoRaMacPayloadDecrypt(payload + appPayloadStartIndex,
                                              frameLen,
                                              appSKey,
                                              address,
                                              DOWN_LINK,
                                              downLinkCounter,
                                              LoRaMacRxPayload);

                        trace_debug("port = %d, RxPayload: ", port);
                        trace_dump_d(LoRaMacRxPayload, frameLen);
                        
                        LoRaMacEventFlags.Bits.RxData = 1;
                        LoRaMacEventAlohaInfo.RxPort = port;
                        LoRaMacEventAlohaInfo.RxBuffer = LoRaMacRxPayload;
                        LoRaMacEventAlohaInfo.RxBufferSize = frameLen;
                    }
                    // wanghuayuan
                    if(fCtrl.Bits.FPending == 1)
                    {
                        trace_info("FPending = %d\r\n", fCtrl.Bits.FPending);
                        LoRaMacEventAlohaInfo.Pendding = 1;
                    }
                }
                LoRaMacEventFlags.Bits.Tx = 1;
                LoRaMacEventAlohaInfo.Status = LORAMAC_EVENT_INFO_STATUS_OK;
            }
            else
            {
                trace_warn("mic error\r\n");
                LoRaMacEventAlohaInfo.TxAckReceived = false;
                LoRaMacEventFlags.Bits.Tx = 1;
                LoRaMacEventAlohaInfo.Status = LORAMAC_EVENT_INFO_STATUS_MIC_FAIL;
//                LoRaMacAlohaState &= ~MAC_ALOHA_TX_RUNNING;
            }
        }
        break;
        case FRAME_TYPE_PROPRIETARY:
        //Intentional falltrough
        default:
            LoRaMacEventFlags.Bits.Tx = 1;
            LoRaMacEventAlohaInfo.Status = LORAMAC_EVENT_INFO_STATUS_ERROR;
            break;
    }
}

static void LoRaMacRxDecode(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr)
{
    LoRaMacHeader_t macHdr;
    LoRaMacFrameCtrl_t fCtrl;

    uint8_t pktHeaderLen = 0;
    uint32_t address = 0;
    uint16_t sequenceCounter = 0;
    int32_t sequence = 0;
    uint8_t appPayloadStartIndex = 0;
    uint8_t port = 0xFF;
    uint8_t frameLen = 0;
    uint32_t mic = 0;
    uint32_t micRx = 0;

    MulticastParams_t* curMulticastParams = NULL;
    uint8_t* nwkSKey = (uint8_t *)LoRaParam.db.NwkSKey;
    uint8_t* appSKey = (uint8_t *)LoRaParam.db.AppSKey;
    uint32_t downLinkCounter = 0;

    bool isMicOk = false;
    
    trace_notice("BC RD: ");
    trace_dump_n(payload, size);

    macHdr.Value = payload[pktHeaderLen++];

    switch(macHdr.Bits.MType)
    {
        case FRAME_TYPE_DATA_UNCONFIRMED_DOWN:
        case FRAME_TYPE_DATA_CONFIRMED_DOWN:
        {
            address = payload[pktHeaderLen++];
            address |= (payload[pktHeaderLen++] << 8);
            address |= (payload[pktHeaderLen++] << 16);
            address |= (payload[pktHeaderLen++] << 24);

            if(address != LoRaParam.db.DevAddr)
            {
                trace_warn("DevAddr error %08X!=%08X\r\n", address, LoRaParam.db.DevAddr);
                curMulticastParams = MulticastChannels;
                while(curMulticastParams != NULL)
                {
                    if(address == curMulticastParams->Address)
                    {
                        nwkSKey = curMulticastParams->NwkSKey;
                        appSKey = curMulticastParams->AppSKey;
                        downLinkCounter = curMulticastParams->DownLinkCounter;
                        break;
                    }
                    curMulticastParams = curMulticastParams->Next;
                }
            }
            else
            {
                nwkSKey = (uint8_t *)LoRaParam.db.NwkSKey;
                appSKey = (uint8_t *)LoRaParam.db.AppSKey;
                downLinkCounter = DownLinkCounter;
            }

            fCtrl.Value = payload[pktHeaderLen++];
            trace_debug("fCtrl=%02X\r\n", fCtrl.Value);

            sequenceCounter |= payload[pktHeaderLen++];
            sequenceCounter |= payload[pktHeaderLen++] << 8;

            trace_notice("dfcnt = %d\r\n", sequenceCounter);

            appPayloadStartIndex = 8 + fCtrl.Bits.FOptsLen;

            micRx |= payload[size - LORAMAC_MFR_LEN];
            micRx |= (payload[size - LORAMAC_MFR_LEN + 1] << 8);
            micRx |= (payload[size - LORAMAC_MFR_LEN + 2] << 16);
            micRx |= (payload[size - LORAMAC_MFR_LEN + 3] << 24);

            sequence = (int32_t)sequenceCounter - (int32_t)(downLinkCounter & 0xFFFF);

            if(sequence < 0)
            {
                // sequence reset or roll over happened
                downLinkCounter = (downLinkCounter & 0xFFFF0000) | (sequenceCounter + (uint32_t)0x10000);
                LoRaMacComputeMic(payload, size - LORAMAC_MFR_LEN, nwkSKey, address, DOWN_LINK, downLinkCounter, &mic);
                if(micRx == mic)
                {
                    isMicOk = true;
                }
                else
                {
                    trace_warn("mic error\r\n");
                    isMicOk = false;
                    // sequence reset
                    if(LoRaMacEventFlags.Bits.Multicast == 1)
                    {
                        curMulticastParams->DownLinkCounter = downLinkCounter = sequenceCounter;
                    }
                    else
                    {
                        DownLinkCounter = downLinkCounter = sequenceCounter;
                    }
                    LoRaMacComputeMic(payload, size - LORAMAC_MFR_LEN, nwkSKey, address, DOWN_LINK, downLinkCounter, &mic);
                }
            }
            else
            {
                downLinkCounter = (downLinkCounter & 0xFFFF0000) | sequenceCounter;
                LoRaMacComputeMic(payload, size - LORAMAC_MFR_LEN, nwkSKey, address, DOWN_LINK, downLinkCounter, &mic);
            }

            if((isMicOk == true) ||
               (micRx == mic))
            {
                EventRxInfo.RxSnr = snr;
                EventRxInfo.RxRssi = rssi;
                EventRxInfo.RxBufferSize = 0;
                AdrAckCounter = 0;

                if(LoRaMacEventFlags.Bits.Multicast == 1)
                {
                    curMulticastParams->DownLinkCounter = downLinkCounter;
                }
                else
                {
                    DownLinkCounter = downLinkCounter;
                }

                if(macHdr.Bits.MType == FRAME_TYPE_DATA_CONFIRMED_DOWN)
                {
                    trace_info("confirm down\r\n");
                    SrvAckRequested = true;
                }
                else
                {
                    SrvAckRequested = false;
                }
               
                if(fCtrl.Bits.FOptsLen > 0)
                {
                    // Decode Options field MAC commands
                    LoRaMacProcessMacCommands(payload, 8, appPayloadStartIndex);
                }

                if(((size - 4) - appPayloadStartIndex) > 0)
                {
                    port = payload[appPayloadStartIndex++];
                    frameLen = (size - 4) - appPayloadStartIndex;

                    if(port == 0)
                    {
                        LoRaMacPayloadDecrypt(payload + appPayloadStartIndex,
                                              frameLen,
                                              nwkSKey,
                                              address,
                                              DOWN_LINK,
                                              downLinkCounter,
                                              LoRaMacRxPayload);
                        // Decode frame payload MAC commands
                        trace_info("port = %d, MacCommands: ", port);
                        trace_dump_i(LoRaMacRxPayload, frameLen);
                        
                        LoRaMacProcessMacCommands(LoRaMacRxPayload, 0, frameLen);
                    }
                    else if(port == 224)
                    {
                        memcpy(LoRaMacRxPayload, payload + appPayloadStartIndex, frameLen);
                        EventRxInfo.RxData = 1;
                        EventRxInfo.RxPort = port;
                        EventRxInfo.RxBuffer = LoRaMacRxPayload;
                        EventRxInfo.RxBufferSize = frameLen;
                    }
                    else
                    {
                        LoRaMacPayloadDecrypt(payload + appPayloadStartIndex,
                                              frameLen,
                                              appSKey,
                                              address,
                                              DOWN_LINK,
                                              downLinkCounter,
                                              LoRaMacRxPayload);

                        trace_info("port = %d, Payload: ", port);
                        trace_dump_i(LoRaMacRxPayload, frameLen);
                        
                        EventRxInfo.RxData = 1;
                        EventRxInfo.RxPort = port;
                        EventRxInfo.RxBuffer = LoRaMacRxPayload;
                        EventRxInfo.RxBufferSize = frameLen;
                    }
                    // wanghuayuan
                    if(fCtrl.Bits.FPending == 1)
                    {
                        trace_info("FPending = %d\r\n", fCtrl.Bits.FPending);
                        EventRxInfo.Pendding = 1;
                    }
                }
            }
            else
            {
                trace_info("mic error\r\n");
            }
        }
        break;
        case FRAME_TYPE_PROPRIETARY:
        //Intentional falltrough
        default:
            break;
    }
}

static bool LoRaMacBeaconDecode(uint8_t* payload, uint16_t size)
{
    trace_info("Beacon: ");
    trace_dump_i(payload, size);
  
    uint32_t netid = 0;
    uint32_t beacon_time = 0;
    uint16_t field1_crc = 0;
    uint16_t field1_cal_crc = 0;
    
    netid = uint24_decode(payload);
    beacon_time = uint32_decode(&payload[3]);
    field1_crc = payload[7];
    field1_cal_crc = crc8_ccitt(payload, 7);
    
    UNUSED_VARIABLE(netid);

    trace_debug("netid            = %06x\r\n", netid);
    trace_debug("beacon_time      = %u\r\n", beacon_time);
    trace_debug("field1_crc       = %02x\r\n", field1_crc);
    trace_debug("field1_cal_crc   = %02x\r\n", field1_cal_crc);

    if(field1_crc == field1_cal_crc)
    {
        BeaconGPSTime = beacon_time;
        return true;
    }
    return false;
}

/*!
 * Initializes and opens the reception window
 *
 * \param [IN] freq window channel frequency
 * \param [IN] datarate window channel datarate
 * \param [IN] bandwidth window channel bandwidth
 * \param [IN] timeout window channel timeout
 */
static bool LoRaMacRxSetup(uint32_t freq, int8_t datarate, uint32_t bandwidth, uint16_t timeout, bool rxContinuous)
{
    trace_debug("rx freq = %d, dr = %d, %d\r\n", freq, datarate, rxContinuous);
    if(Radio.Status() == RF_IDLE)
    {
        LoRaMacEventAlohaInfo.RxFreq = freq;
        LoRaMacEventAlohaInfo.RxDatarate = datarate;
        Radio.SetChannel(freq);
        
        if(datarate == DR_7)
        {
            Radio.SetRxConfig(MODEM_FSK,                // modem
                              50e3,                     // bandwidth
                              Datarates[datarate] * 1000,// datarate
                              0,                        // coderate
                              83.333e3,                 // bandwidthAfc
                              5,                        // preambleLen
                              0,                        // symbTimeout
                              false,                    // fixLen
                              0,                        // payloadLen
                              true,                     // crcOn
                              0,                        // FreqHopOn
                              0,                        // HopPeriod
                              false,                    // iqInverted
                              rxContinuous);            // rxContinuous
        }
        else
        {
            Radio.SetRxConfig(MODEM_LORA,           // modem
                              bandwidth,            // bandwidth
                              Datarates[datarate],  // datarate
                              1,                    // coderate
                              0,                    // bandwidthAfc
                              8,                    // preambleLen
                              timeout,              // symbTimeout
                              false,                // fixLen
                              0,                    // payloadLen
                              false,                // crcOn
                              0,                    // FreqHopOn
                              0,                    // HopPeriod
                              true,                 // iqInverted
                              rxContinuous);        // rxContinuous
        }
        if(rxContinuous == false)
        {
            trace_verbose("RX\r\n");
            Radio.Rx(MaxRxWindow);
            TIMER_START(RadioTimeoutTimer, MaxRxWindow);
        }
        else
        {
            trace_verbose("Continuous mode\r\n");
            Radio.Rx(0); // Continuous mode
        }
        return true;
    }
#ifdef DEBUG
    else
    {
        trace_info("RF not IDLE\r\n");
    }
#endif
    return false;
}

static bool LoRaMacBeaconRxWindowSetup(uint32_t freq, int8_t datarate, uint8_t payloadLen ,uint16_t symbTimeout, bool rxContinuous)
{
    trace_info("BeaconRxSetup\r\n");
    if(Radio.Status() == RF_IDLE)
    {
        Radio.SetChannel(freq);
        Radio.SetRxConfig(MODEM_LORA,           // modem
                          0,                    // bandwidth
                          Datarates[datarate],  // datarate
                          1,                    // coderate
                          0,                    // bandwidthAfc
                          10,                   // preambleLen
                          symbTimeout,          // symbTimeout
                          true,                 // fixLen
                          payloadLen,           // payloadLen
                          false,                // crcOn
                          0,                    // FreqHopOn
                          0,                    // HopPeriod
                          false,                // iqInverted
                          rxContinuous);        // rxContinuous
                          
        if(rxContinuous == false)
        {
            Radio.Rx(MaxRxWindow);
            TIMER_START(RadioTimeoutTimer, MaxRxWindow);
        }
        else
        {
            Radio.Rx(0); // Continuous mode
        }
        return true;
    }
#ifdef DEBUG
    else
    {
        trace_info("RF not IDLE\r\n");
    }
#endif
    return false;
}

static void LoRaMacRxWindowSetup(uint8_t rxSlot, bool rxContinuous)
{
    uint16_t symbTimeout = 8; // DR_2, DR_1, DR_0
    int8_t datarate = 0;
    uint32_t bandwidth = 0; // LoRa 125 kHz
    uint32_t freq = 0;
    
    if(rxSlot == 0)
    {
        datarate = ChannleDatarate - LoRaParam.db.RX1DROffset;
        freq = Channels[Channel].FreqDown;
    }
    else
    {
        datarate = LoRaParam.db.RX2DateRate;  
        freq = LoRaParam.db.RX2Freq;
    }
    
    if(datarate < 0)
    {
        datarate = DR_0;
    }
    
    if(Rx1DatarateNew != UINT8_MAX)
    {
        datarate = Rx1DatarateNew;
    }
    else if(Rx2DatarateNew != UINT8_MAX)
    {
        datarate = Rx2DatarateNew;
    }
    else if(PingDatarateNew != UINT8_MAX)
    {
        datarate = PingDatarateNew;
    }
    else if(ClassGDatarateNew != UINT8_MAX)
    {
        datarate = ClassGDatarateNew;
    }
    else
    {
        //nothing
    }

    switch(datarate)
    {
        case DR_0:
        case DR_1:
            symbTimeout = 9;
            break;
        
        case DR_2:
            symbTimeout = 10;
            break;
        
        case DR_3:
            symbTimeout = 12;
            break;
        
        case DR_4:
            symbTimeout = 16;
        
        case DR_5:
            symbTimeout = 20;
            break;
        
        case DR_6:
            symbTimeout = 20;
            bandwidth  = 1;
            break;
        
        default:
            symbTimeout = 8;
            break;
    }
     
    LoRaMacRxSetup(freq, datarate, bandwidth, symbTimeout, rxContinuous);
}

/*!
 * Function to be executed on Tx Done event
 */
static void OnRadioTxDone(void)
{
    uint32_t curTime = 0;
    
    TIMER_STOP(RadioTimeoutTimer);
    
    RxDone = false;

    Radio.Sleep();

    TIMER_START(RxWindowTimer1, RxWindow1Delay - 10);
    TIMER_START(RxWindowTimer2, RxWindow1Delay + 1000 - 10);
    
    curTime = run_time_get();
    
    // Update Band Time OFF
    Bands[Channels[Channel].Band].LastTxDoneTime = curTime;
    if(DutyCycleOn == true)
    {
        Bands[Channels[Channel].Band].TimeOff = TxTimeOnAir * Bands[Channels[Channel].Band].DCycle - TxTimeOnAir;
    }
    else
    {
        Bands[Channels[Channel].Band].TimeOff = 0;
    }

    // Update Agregated Time OFF
    AggregatedLastTxDoneTime = curTime;
    AggregatedTimeOff = AggregatedTimeOff + (TxTimeOnAir * AggregatedDCycle - TxTimeOnAir);

    LastTxTime = curTime;
    
    if(NodeAckRequested == false)
    {
        ChannelsNbRepCounter++;
    }
    trace_notice("TD\r\n");
}

static void OnRadioRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr)
{
    uint32_t err_code;
    LoRaMacRadioEvent_t radioEvent;
    
    trace_verbose("RD: ");
    trace_dump_v(payload, size);
    
    TIMER_STOP(RadioTimeoutTimer);
    
    Radio.Sleep();
    
    if( (LoRaMacState & MAC_STATE_ALOHA ) != 0 )
    {
        RxDone = true;
        if(LoRaMacEventFlags.Bits.RxSlot == 0)
        {
            TIMER_STOP(RxWindowTimer2);
        }
        radioEvent.evt = LORAMAC_RADIO_EVT_ALOHA_RX_DONE;
        memcpy(radioEvent.data, payload, size);
        radioEvent.data_len = size;
        radioEvent.rssi = rssi;
        radioEvent.snr = snr;
    }
    else if((LoRaMacState & MAC_STATE_BEACON_RX) != 0)
    {
        radioEvent.evt = LORAMAC_RADIO_EVT_BEACON_RX_DONE;
        memcpy(radioEvent.data, payload, size);
        radioEvent.data_len = size;
        radioEvent.rssi = rssi;
        radioEvent.snr = snr;
    }
    else if((LoRaMacState & MAC_STATE_PING_RX) != 0)
    {
        radioEvent.evt = LORAMAC_RADIO_EVT_PING_RX_DONE;
        memcpy(radioEvent.data, payload, size);
        radioEvent.data_len = size;
        radioEvent.rssi = rssi;
        radioEvent.snr = snr;
    }
    else if((LoRaMacState & MAC_STATE_CLASSC_RX) != 0)
    {
        radioEvent.evt = LORAMAC_RADIO_EVT_CLASSC_RX_DONE;
        memcpy(radioEvent.data, payload, size);
        radioEvent.data_len = size;
        radioEvent.rssi = rssi;
        radioEvent.snr = snr;
    }
    else if((LoRaMacState & MAC_STATE_CLASSG_RX) != 0)
    {
        radioEvent.evt = LORAMAC_RADIO_EVT_CLASSG_RX_DONE;
        memcpy(radioEvent.data, payload, size);
        radioEvent.data_len = size;
        radioEvent.rssi = rssi;
        radioEvent.snr = snr;
    }
    else
    {
        // should not be here!!
        return;
    }
    err_code = app_mailbox_put(&Mailbox, &radioEvent);
    APP_ERROR_CHECK(err_code);

    trace_notice("onRxD~\r\n");
}


/*!
 * Function executed on Radio Tx Timeout event
 */
static void OnRadioTxTimeout(void)
{
    uint32_t err_code = 0;

    trace_warn("OnRadioTxTimeout\r\n");
    
    Radio.Sleep();
    
    if(NodeAckRequested == true)
    {
        trace_info("NodeAckRequested = true\r\n");
        if((AckTimeoutRetriesCounter < AckTimeoutRetries) && (AckTimeoutRetriesCounter <= MAX_ACK_RETRIES))
        {
            AckTimeoutRetry = true;
            TIMER_START(AckTimeoutTimer, ACK_TIMEOUT + RANDR(-ACK_TIMEOUT_RND, ACK_TIMEOUT_RND));
        }
    }

    LoRaMacEventFlags.Bits.Tx = 1;
    LoRaMacEventAlohaInfo.Status = LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT;
    
    LoRaMacRadioEvent_t radioEvent;
    radioEvent.evt = LORAMAC_RADIO_EVT_ALOHA_TX_TIMEOUT;
    err_code = app_mailbox_put(&Mailbox, &radioEvent);
    APP_ERROR_CHECK(err_code);
}

/*!
 * Function executed on Radio Rx Timeout event
 */
static void OnRadioRxTimeout(void)
{
    uint32_t err_code;
    LoRaMacRadioEvent_t radioEvent;
    
    trace_notice("RTO\r\n");

    TIMER_STOP(RadioTimeoutTimer);
    
    Radio.Sleep();
    
    if( (LoRaMacState & MAC_STATE_ALOHA ) != 0 )
    {
        if(LoRaMacEventFlags.Bits.RxSlot == 1)
        {
            LoRaMacEventFlags.Bits.Tx = 1;
            LoRaMacEventAlohaInfo.Status = LORAMAC_EVENT_INFO_STATUS_RX2_TIMEOUT;
            radioEvent.evt = LORAMAC_RADIO_EVT_ALOHA_RX_TIMEOUT;                      
        }
        else
        {
            return;
        }
    }
    else if( (LoRaMacState & MAC_STATE_BEACON_RX) != 0 )
    {
        radioEvent.evt = LORAMAC_RADIO_EVT_BEACON_RX_TIMEOUT;
    }
    else if((LoRaMacState & MAC_STATE_PING_RX) != 0)
    {
        radioEvent.evt = LORAMAC_RADIO_EVT_PING_RX_TIMEOUT;      
    }
    else if((LoRaMacState & MAC_STATE_CLASSG_RX) != 0)
    {
        radioEvent.evt = LORAMAC_RADIO_EVT_CLASSG_RX_TIMEOUT;
    }
    else
    {
        // should not be here!!
        return;
    }
    err_code = app_mailbox_put(&Mailbox, &radioEvent);
    APP_ERROR_CHECK(err_code);  
}

/*!
 * Function executed on Radio Rx Error event
 */
static void OnRadioRxError(void)
{
    trace_warn("OnRadioRxError\r\n");
    TIMER_STOP(RadioTimeoutTimer);
    Radio.Sleep();
    if(LoRaMacEventFlags.Bits.RxSlot == 1)
    {
        uint32_t err_code;
        LoRaMacEventFlags.Bits.Tx = 1;
        LoRaMacEventAlohaInfo.Status = LORAMAC_EVENT_INFO_STATUS_RX2_ERROR;
        
        LoRaMacRadioEvent_t radioEvent;
        radioEvent.evt = LORAMAC_RADIO_EVT_ALOHA_RX_TIMEOUT;
        err_code = app_mailbox_put(&Mailbox, &radioEvent);
        APP_ERROR_CHECK(err_code);
    }
}

/*!
 * Function executed on first Rx window timer event
 */
static void OnRxWindow1TimerEvent(void* p_context)
{
    trace_info("R1TO\r\n");
    LoRaMacEventFlags.Bits.RxSlot = 0;
    LoRaMacRxWindowSetup(0, false);
}

/**
  *
  */
static void OnRxWindow2TimerEvent(void* p_context)
{
    trace_info("R2TO\r\n");
    if(RxDone == true)
    {
        return;
    }
    if(NodeAckRequested == true)
    {
        trace_info("ts AckT\n");
        if((AckTimeoutRetriesCounter < AckTimeoutRetries) && (AckTimeoutRetriesCounter <= MAX_ACK_RETRIES))
        {
            AckTimeoutRetry = true;
            TIMER_START(AckTimeoutTimer, ACK_TIMEOUT + RANDR(-ACK_TIMEOUT_RND, ACK_TIMEOUT_RND));
        }
    }
    LoRaMacEventFlags.Bits.RxSlot = 1;
    LoRaMacRxWindowSetup(1, false);
}

static void LoRaMacResendConfirmFrame(void)
{
    if((AckTimeoutRetriesCounter < AckTimeoutRetries) && (AckTimeoutRetriesCounter <= MAX_ACK_RETRIES))
    {
        AckTimeoutRetriesCounter++;
        
        trace_info("retry send %d\r\n", AckTimeoutRetriesCounter);
        
        if((AckTimeoutRetriesCounter % 2) == 1)
        {
            ChannleDatarate = MAX(ChannleDatarate - 1, LORAMAC_MIN_DATARATE);
        }
        LoRaMacEventFlags.Bits.Tx = 0;
        // Sends the same frame again
        LoRaMacBuffer[5] &= 0xdf;
        if(LoRaMacSetNextChannel() == 0)
        {
            LoRaMacSendFrame(Channels[Channel]);
        }
    }
}

/*!
 * Function executed on MacStateCheck timer event
 */
static void OnMacStateCheckTimerEvent(void* p_context)
{
    trace_verbose("OnMacStateCheckTimerEvent\r\n");
    if((LoRaMacState & MAC_STATE_ALOHA) != 0)
    {
        if(LoRaMacEventFlags.Bits.Tx == 1)
        {
            if(NodeAckRequested == false)
            {
                if(LoRaMacEventFlags.Bits.JoinAccept == true)
                {
                    // Join messages aren't repeated automatically
                    ChannelsNbRepCounter = LoRaParam.db.NbTrans;
                    UpLinkCounter = 0;
                }
                
                if((ChannelsNbRepCounter == 0) || (ChannelsNbRepCounter >= LoRaParam.db.NbTrans))
                {
                    ChannelsNbRepCounter = 0;
                    LoRaMacEventAlohaInfo.Status = LORAMAC_EVENT_INFO_STATUS_OK;
                    AdrAckCounter++;
//                    UpLinkCounter++;
                    LoRaMacAlohaState &= ~MAC_ALOHA_TX_RUNNING;
                }
                else
                {
                    if(LoRaMacEventFlags.Bits.Rx != 1)
                    {
                        LoRaMacEventFlags.Bits.Tx = 0;
                        // Sends the same frame again
                        if(LoRaMacSetNextChannel() == 0)
                        {
                            LoRaMacSendFrame(Channels[Channel]);
                        }
                    }
                    else
                    {
                        LoRaMacAlohaState &= ~MAC_ALOHA_TX_RUNNING;
                    }
                }
            }
            else
            {
                if(LoRaMacEventFlags.Bits.Rx == 1)
                {
                    if((LoRaMacEventAlohaInfo.TxAckReceived == true) || (AckTimeoutRetriesCounter > AckTimeoutRetries))
                    {
                        trace_info("receive ack or retry count to max\r\n");
                        AckTimeoutRetry = false;
//                        UpLinkCounter++;
                        LoRaMacEventAlohaInfo.TxNbRetries = AckTimeoutRetriesCounter;
                        LoRaMacAlohaState &= ~MAC_ALOHA_TX_RUNNING;
                    }
                }
                else
                {
                    if((AckTimeoutRetriesCounter >= AckTimeoutRetries) || (AckTimeoutRetriesCounter > MAX_ACK_RETRIES))
                    {
                        trace_info("retry count to max\r\n");
                        // Re-enable default channels LC1, LC2, LC3
                        // LoRaParam.db.ChMask = LoRaParam.db.ChMask | ( LC( 1 )  + LC( 2 ) + LC( 3 ) );//!!!
                        LoRaMacAlohaState &= ~MAC_ALOHA_TX_RUNNING;

                        LoRaMacEventAlohaInfo.TxAckReceived = false;
                        LoRaMacEventAlohaInfo.TxNbRetries = AckTimeoutRetriesCounter;
    //                    UpLinkCounter++;
                        LoRaMacEventAlohaInfo.Status = LORAMAC_EVENT_INFO_STATUS_OK;
                    }
                    else
                    {
                        if(LoRaMacEventFlags.Bits.RxSlot == 0)
                        {
                            if(NodeAckRequested == true)
                            {
                                trace_info("ts AckTimeoutTimer\n");
                                if(AckTimeoutRetriesCounter < AckTimeoutRetries)
                                {
                                    AckTimeoutRetry = true;
                                    TIMER_START(AckTimeoutTimer, ACK_TIMEOUT + RANDR(-ACK_TIMEOUT_RND, ACK_TIMEOUT_RND));
                                }
                            }
                        }
                    }
                }
            }
            
            
        }
        // Handle reception for Class B and Class C
        if((LoRaMacAlohaState & MAC_ALOHA_RX) == MAC_ALOHA_RX)
        {
            LoRaMacAlohaState &= ~MAC_ALOHA_RX;
        }
        if(LoRaMacAlohaState == MAC_ALOHA_IDLE)
        {
            if(MacCmdsDrCheckAnsWait)
            {
                MacCmdsDrCheckAnsWaitCounter++;
                if(MacCmdsDrCheckAnsWaitCounter > 2)
                {
                    trace_info("rollback~~~~\r\n");
                    MacCmdsDrCheckAnsWait = false;
                    MacCmdsDrCheck = false;
                    MacCmdsDrCheckAnsWaitCounter = 0;
                    MacCmdsDelayAnsBufferIdx = 0;
                    
                    Rx1DatarateNew = UINT8_MAX;
                    Rx2DatarateNew = UINT8_MAX;
                    TxPowerNew = INT8_MAX;
                    
                    ChMaskNew = UINT32_MAX;
                    NbTransNew = UINT8_MAX;
                    Rx1DROffsetNew = UINT8_MAX;
                    Rx2FreqNew = UINT32_MAX;
                    
                    PingDatarateNew = UINT8_MAX;
                    ClassBEnableNew = UINT8_MAX;
                    PingPrdNew = UINT8_MAX;
                    
                    ClassGDatarateNew = UINT8_MAX;
                    ClassGEnabledNew = UINT8_MAX;
                    ClassGPrdNew = UINT8_MAX;
                }
            }
            
            
            LoRaMacAlohaNotify(&LoRaMacEventFlags, &LoRaMacEventAlohaInfo);
            memset(&LoRaMacEventAlohaInfo, 0, sizeof(LoRaMacEventAlohaInfo_t));
            
            LoRaMacState &= ~MAC_STATE_ALOHA;
        }
    }
    else if((LoRaMacState & MAC_STATE_PING_RX) != 0 || ((LoRaMacState & MAC_STATE_CLASSG_RX) != 0))
    {
        trace_debug("MAC_STATE_PING_RX\r\n");
        if(EventRxInfo.RxData == 1)
        {
            trace_debug("LoRaMacRxNotify\r\n");
            LoRaMacRxNotify(&EventRxInfo);
        }
        memset(&EventRxInfo, 0, sizeof(LoRaMacEventRxInfo_t));
    }
    else if((LoRaMacState & MAC_STATE_CLASSC_RX) != 0)
    {
        trace_debug("MAC_STATE_CLASSC_RX\r\n");
        if(EventRxInfo.RxData == 1)
        {
            trace_debug("LoRaMacRxNotify\r\n");
            LoRaMacRxNotify(&EventRxInfo);
        }
        memset(&EventRxInfo, 0, sizeof(LoRaMacEventRxInfo_t));
    }
}

static void OnAckTimeoutTimerEvent(void* p_context)
{
    trace_info("AckTimeoutRetry\n");
    if(AckTimeoutRetry)
    {
        AckTimeoutRetry = false;
        LoRaMacResendConfirmFrame();
    }
}

void LoRaMacBeaconScan(void)
{
    bool rc = false;
    trace_info("LoRaMacBeaconScan\r\n");    
    Radio.Sleep();
    if((LoRaMacState & MAC_STATE_ALOHA) != 0)
    {
        trace_info("error: on aloha!!!!!!!!!!!\r\n");
        TIMER_STOP(RxWindowTimer1);
        TIMER_STOP(RxWindowTimer2);
        
        LoRaMacState &= ~MAC_STATE_ALOHA;
        
        LoRaMacAlohaPending = true;
        LoRaMacEventFlags.Value = 0;        
    }
    
    rc = LoRaMacBeaconRxWindowSetup(LoRaParam.db.BeaconFreq,
                               LoRaParam.db.BeaconDataRate,
                               BEACON_PAYLOAD_LEN,
                               BeaconSymbTimeout,
                               true);
    if(rc)
    {
        LoRaMacState |= MAC_STATE_BEACON_RX;
    }
}


static void LoRaMacBeaconRx(void)
{
    bool rc = false;
    Radio.Sleep();
    if((LoRaMacState & MAC_STATE_ALOHA) != 0)
    {
        trace_info("error: on aloha!!!!!!!!!!!\r\n");
        TIMER_STOP(RxWindowTimer1);
        TIMER_STOP(RxWindowTimer2);
        
        LoRaMacState &= ~MAC_STATE_ALOHA;
        
        LoRaMacAlohaPending = true;
        LoRaMacEventFlags.Value = 0;        
    }
    
    rc = LoRaMacBeaconRxWindowSetup(LoRaParam.db.BeaconFreq,
                               LoRaParam.db.BeaconDataRate,
                               BEACON_PAYLOAD_LEN,
                               BeaconSymbTimeout,
                               false);
    if(rc)
    {
        LoRaMacState |= MAC_STATE_BEACON_RX;
    }
    trace_notice("BeaconRx\r\n");    
}

static void LoRaMacPingRx(void)
{
    bool rc = false;
    uint32_t bandwidth = 0; // LoRa 125 kHz
    uint8_t symbTimeout = 16;
    
    switch(LoRaParam.db.PingDataRate)
    {
        case DR_0:
            symbTimeout = 8;
            break;
        case DR_1:
            symbTimeout = 10;
            break;
        case DR_2:
            symbTimeout = 12;
            break;
        case DR_3:
        case DR_4:
        case DR_5:
            symbTimeout = 16;
            break;
        default:
            break;
    }
        
    trace_verbose("ping freq = %d, dr = %d\r\n", LoRaParam.db.PingChannel.FreqDown, LoRaParam.db.PingDataRate);
    rc = LoRaMacRxSetup(LoRaParam.db.PingChannel.FreqDown, 
                         LoRaParam.db.PingDataRate, 
                         bandwidth, 
                         symbTimeout, 
                         false);
    if(rc)
    {
        LoRaMacState |= MAC_STATE_PING_RX;
    }
}

void LoRaMacClassGRx(void)
{
    bool rc = false;
    uint32_t bandwidth = 0; // LoRa 125 kHz
    uint8_t symbTimeout = 16;
    
    switch(LoRaParam.db.ClassGDataRate)
    {
        case DR_0:
            symbTimeout = 9;
            break;
        case DR_1:
            symbTimeout = 10;
            break;
        case DR_2:
            symbTimeout = 11;
            break;
        case DR_3:
            symbTimeout = 18;
        case DR_4:
            symbTimeout = 28;
            break;
        case DR_5:
            symbTimeout = 48;
            break;
        default:
            break;
    }
    
    if((LoRaMacState & MAC_STATE_ALOHA) == 0)
    {
        rc = LoRaMacRxSetup(LoRaParam.db.ClassGChannel.FreqDown, 
                         LoRaParam.db.ClassGDataRate, 
                         bandwidth, 
                         symbTimeout, 
                         false);
        if(rc)
        {
            LoRaMacState |= MAC_STATE_CLASSG_RX;
        }
    }
    else
    {
        trace_verbose("on aloha, cancel rx~\r\n");
    }
}


static void LoRaMacClassGStart(void)
{    
    LoRaMacClassGTimeSpec_t time_spec;
    uint16_t pingOffset = 0;
    uint32_t pingDelay = 0;
    uint16_t classgPeriod = 0;
    uint8_t classgNbK = 0;
    uint32_t currentMs = 0;
    int8_t ping_deviation = 2;
          
    if(!LoRaParam.db.ClassGEnabled)
    {
        return;
    }
    
    if(ClassGScheduling)
    {
        return;
    }

    if(LoRaMacGetClassGTimeSpec(&time_spec))
    {
        switch(LoRaParam.db.PingDataRate)
        {
            case DR_0:
                ping_deviation = -2;
                break;
            case DR_1:
                ping_deviation = 0; // 没有按照算法来, 人为设置为0
                break;
            case DR_2:
                ping_deviation = 0;
                break;
            case DR_3:
            case DR_4:
            case DR_5:
                ping_deviation = 4;
                break;
            default:
                break;
        }
        
        classgNbK = 7 - LoRaParam.db.ClassGPeriod;
        if(0 == classgNbK)
        {
            classgNbK = 1;
        }
        classgPeriod = 1 << (12 - classgNbK);
        
        
        uint8_t beaconOffsetS = time_spec.time_s%128;
        uint32_t beaconOffsetMs = beaconOffsetS*1000 + time_spec.time_ms;
        uint32_t beaconTime = time_spec.time_s - beaconOffsetS;
        
        uint32_t nextBeaconDelay = (128 - beaconOffsetS)*1000 - time_spec.time_ms;
        
        pingOffset =  LoRaMacPingOffsetGet(beaconTime, LoRaParam.db.DevAddr, classgPeriod);
 
        if(beaconOffsetMs <= (BEACON_RESERVED + pingOffset * 30))
        {
            trace_notice("22\r\n");
            pingDelay = BEACON_RESERVED +  pingOffset * 30  - beaconOffsetMs;
            PingCount = 0;            
        }
        else
        {
            trace_notice("33\r\n");
            currentMs = beaconOffsetMs - (BEACON_RESERVED + pingOffset * 30);
            pingDelay = classgPeriod*30 - currentMs%(classgPeriod*30);
            PingCount = currentMs/(classgPeriod*30) + 1;
        }
        pingDelay = pingDelay - (ping_deviation * SymbTime[LoRaParam.db.ClassGDataRate]/1000);
        
        TIMER_START(PingTimer, pingDelay);
        TIMER_START(BeaconTimer, nextBeaconDelay);   

        ClassGScheduling = true;;

        trace_debug("beaconOffsetS  = %u\r\n", beaconOffsetS);
        trace_debug("beaconOffsetMs = %u\r\n", beaconOffsetMs);
        trace_debug("beaconTime     = %u\r\n", beaconTime);
        trace_debug("nextBeaconDelay  = %u\r\n", nextBeaconDelay);
        
        trace_debug("pingOffset  = %u\r\n", pingOffset);
        trace_debug("pingDelay  = %u\r\n", pingDelay);
        trace_debug("PingCount  = %u\r\n", PingCount);
    }
    else
    {
        trace_warn("time spec get error\r\n");
//        TIMER_START(ClassGTimer, LoRaParam.db.ClassGPeriod * 1000);
        TIMER_START(ClassGTimer, 3000);
    }    
}

static void OnClassGTimerEvent(void * p_context)
{
    LoRaMacClassGStart();
}

static void OnBeaconTimerEvent(void* p_context)
{
    trace_verbose("OnBeaconTimerEvent\r\n");
    
    switch(DeviceClass)
    {
        case CLASS_A:
            if(IsBeaconSearching)
            {
                // 有可能会出现已经启动BeaconSearching timer, 但是中途classB被关闭, 所以先判断一下
                // ClassBEnable是否使能
                if(LoRaParam.db.ClassBEnable)
                {
                    LoRaMacBeaconRx();
                }
                IsBeaconSearching = false;
            }
            break;
            
        case CLASS_B:
            if((LoRaMacState & (MAC_STATE_BEACON_GUARD | MAC_STATE_BEACON_RX)) == 0)
            {
                trace_info("enter beacon guard region~~~~~~~~~~~~~~\r\n");
                LoRaMacState |= MAC_STATE_BEACON_GUARD;
                TIMER_START(BeaconTimer, BEACON_GUARD);
            }
            else if((LoRaMacState & MAC_STATE_BEACON_GUARD)!=0)
            {
                LoRaMacState &= ~MAC_STATE_BEACON_GUARD;
                LoRaMacBeaconRx();
            }
            else
            {
                trace_info("invalid state\r\n");
            }
            break;
            
        case CLASS_G:
            ClassGScheduling = false;
            LoRaMacClassGStart();
            break;
    }
}

static void OnPingTimerEvent(void* p_context)
{
    int32_t offset = 0;
    uint32_t ticks;
    uint32_t diff;
    
    uint32_t nextPingTime = 0;
    
    uint8_t pingNbK = 0;
    uint8_t pingNb = 0;
    uint16_t pingPeriod = 0;
    
    if(CLASS_B == DeviceClass)
    {
        app_timer_cnt_get(&ticks);
        diff = (ticks - LastTicks)&0xffffff;

        pingNbK = 7 - LoRaParam.db.PingPeriodicity;
        if(0 == pingNbK)
        {
            pingNbK = 1;
        }
        pingNb = (1 << pingNbK);
        pingPeriod = 1 << (12 - pingNbK);
        
        offset = diff*1000/32768 - FirstPingDelay - pingPeriod*30*PingCount;
        
        trace_notice("~p %d\r\n", PingCount);
        
        PingCount++;
        if(PingCount < pingNb)
        {
            nextPingTime = pingPeriod*30 - offset;
            TIMER_START(PingTimer, nextPingTime);
        }
        
        if((LoRaMacState & MAC_STATE_ALOHA) == 0)
        {
            LoRaMacPingRx();
        }
        else
        {
            trace_info("on aloha, cancel pingrx~~~\r\n");
        }
        trace_verbose("offset=%d ms, NPT=%d\r\n", offset, nextPingTime);
    }
    else if(CLASS_G == DeviceClass)
    {
        pingNbK = 7 - LoRaParam.db.ClassGPeriod;
        if(0 == pingNbK)
        {
            pingNbK = 1;
        }
        pingNb = (1 << pingNbK);
        pingPeriod = 1 << (12 - pingNbK);
                
        PingCount++;
        if(PingCount < pingNb)
        {
            nextPingTime = pingPeriod*30;
            TIMER_START(PingTimer, nextPingTime);
        }
        
        if((LoRaMacState & MAC_STATE_ALOHA) == 0)
        {
            LoRaMacPingRx();
        }
        else
        {
            trace_info("on aloha, cancel pingrx~~~\r\n");
        }
        trace_notice("~p %d\r\n", PingCount);
        trace_verbose("offset=%d ms, NPT=%d\r\n", offset, nextPingTime);
    }
}

static void LoRaMacClassBStart(uint32_t timeOnAir)
{
    uint32_t nextBeaconDelay = 0;  
    int8_t ping_deviation = 2;
    uint8_t pingNbK = 0;
    uint16_t pingPeriod = 0;
    uint16_t pingOffset = 0;

    
    switch(LoRaParam.db.PingDataRate)
    {
        case DR_0:
            ping_deviation = -2;
            break;
        case DR_1:
            ping_deviation = 0; // 没有按照算法来, 人为设置为0
            break;
        case DR_2:
            ping_deviation = 0;
            break;
        case DR_3:
        case DR_4:
        case DR_5:
            ping_deviation = 2;
            break;
        default:
            break;
    }
    
    pingNbK = 7 - LoRaParam.db.PingPeriodicity;
    if(0 == pingNbK)
    {
        pingNbK = 1;
    }
    pingPeriod = 1 << (12 - pingNbK);
    
    PingCount = 0;
    
    pingOffset =  LoRaMacPingOffsetGet(BeaconGPSTime, LoRaParam.db.DevAddr, pingPeriod);
    nextBeaconDelay = BEACON_WINDOW + BEACON_RESERVED - timeOnAir - (2 * SymbTime[LoRaParam.db.BeaconDataRate]/1000);
    FirstPingDelay = BEACON_RESERVED + pingOffset * 30  - timeOnAir - (ping_deviation * SymbTime[LoRaParam.db.PingDataRate]/1000);
    
    TIMER_START(PingTimer, FirstPingDelay);
    TIMER_START(BeaconTimer, nextBeaconDelay - BeaconStartOffset);

    app_timer_cnt_get(&LastTicks);
    trace_debug("pingOffset = %d, FirstPingDelay = %d\r\n", pingOffset, FirstPingDelay);
}

void LoRaMacScheduler(void)
{
    uint32_t err_code;
    LoRaMacRadioEvent_t radioEvent;
    err_code = app_mailbox_get(&Mailbox, &radioEvent);
    if(NRF_SUCCESS == err_code)
    {
        switch(radioEvent.evt)
        {
            case LORAMAC_RADIO_EVT_ALOHA_RX_DONE:
                if(DeviceClass == CLASS_C)
                {
                    LoRaMacState |= MAC_STATE_CLASSC_RX;
                    LoRaMacRxWindowSetup(1, true);
                }
                else if(DeviceClass == CLASS_G)
                {
                    LoRaMacClassGStart();
                }
                RxTimeOnAir = Radio.TimeOnAir(MODEM_LORA, radioEvent.data_len)/1000;
                trace_debug("rcv = %d, ToA = %u\r\n", radioEvent.data_len, RxTimeOnAir);
                LoRaMacAlohaDataDecode(radioEvent.data, radioEvent.data_len, radioEvent.rssi, radioEvent.snr);
                OnMacStateCheckTimerEvent(NULL);
                
                break;
            
            case LORAMAC_RADIO_EVT_ALOHA_TX_TIMEOUT:
                trace_warn("ALOHA_TX_TIMEOUT\r\n");
            case LORAMAC_RADIO_EVT_ALOHA_RX_ERROR:
                trace_warn("ALOHA_RX_ERROR\r\n");
            case LORAMAC_RADIO_EVT_ALOHA_RX_TIMEOUT:
                if(DeviceClass == CLASS_C)
                {
                    LoRaMacState |= MAC_STATE_CLASSC_RX;
                    LoRaMacRxWindowSetup(1, true);
                }
                else if(DeviceClass == CLASS_G)
                {
                    LoRaMacClassGStart();
                }
                OnMacStateCheckTimerEvent(NULL);
                break;
            
            case LORAMAC_RADIO_EVT_BEACON_RX_DONE:
            {
                bool isValidBeacon = false;
                
                isValidBeacon = LoRaMacBeaconDecode(radioEvent.data, radioEvent.data_len);
                if(isValidBeacon)
                {
                    BeaconLostCount = 0;
                    BeaconStartOffset = 0;
                    BeaconSymbTimeout = 16;
                    LoRaMacClassBStart(BEACON_TIME_ON_AIR);
                    
                    if(CLASS_A == DeviceClass)
                    {
                        trace_info("turn to class B\r\n");
                        DeviceClass = CLASS_B;
                        LoRaMacEvents->BeaconEvent(LORAMAC_BEACON_LOCKED);
                    }
                    trace_notice("valid beacon\r\n");
                }
                else
                {
                    if(CLASS_B == DeviceClass)
                    {
                        BeaconGPSTime += 128;
                        RxTimeOnAir = Radio.TimeOnAir(MODEM_LORA, radioEvent.data_len)/1000;
                        BeaconStartOffset += SymbTime[LoRaParam.db.BeaconDataRate]/1000;
                        BeaconSymbTimeout += 1;
                        LoRaMacClassBStart(RxTimeOnAir);
                    }
                    trace_notice("invalid beacon\r\n");
                }
                LoRaMacState &= ~MAC_STATE_BEACON_RX;
            } 
                break;
            
            case LORAMAC_RADIO_EVT_BEACON_RX_TIMEOUT:
            {
                if(CLASS_B == DeviceClass)
                {
                    BeaconLostCount++;      
                    if(BeaconLostCount > 56)             
                    {
                        trace_debug("back to class A #####\r\n");
                        DeviceClass = CLASS_A;
                        LoRaMacEvents->BeaconEvent(LORAMAC_BEACON_UNLOCKED);
                    }
                    else
                    {
                        uint32_t beaconSymbTime = 0;
                        BeaconGPSTime += 128;
                        beaconSymbTime = (SymbTime[LoRaParam.db.BeaconDataRate] * (BeaconSymbTimeout + 4) + SymbTime[LoRaParam.db.BeaconDataRate]/4)/1000; // add 4.25 symbols
                        BeaconStartOffset += SymbTime[LoRaParam.db.BeaconDataRate]/1000;
                        BeaconSymbTimeout += 1;
                        LoRaMacClassBStart(beaconSymbTime);
                    }
                }
                LoRaMacState &= ~MAC_STATE_BEACON_RX;
            }
                break;
                
            case LORAMAC_RADIO_EVT_PING_RX_DONE:
                LoRaMacRxDecode(radioEvent.data, radioEvent.data_len, radioEvent.rssi, radioEvent.snr);
                OnMacStateCheckTimerEvent(NULL);
                LoRaMacState &= ~MAC_STATE_PING_RX;
                break;
            
            case LORAMAC_RADIO_EVT_PING_RX_ERROR:
            case LORAMAC_RADIO_EVT_PING_RX_TIMEOUT:
                LoRaMacState &= ~MAC_STATE_PING_RX;  
                break;
            
            case LORAMAC_RADIO_EVT_CLASSC_RX_DONE:
                trace_debug("CLASSC_RX_DONE\r\n");
                LoRaMacState |= MAC_STATE_CLASSC_RX;
                LoRaMacRxWindowSetup(1, true);
                LoRaMacRxDecode(radioEvent.data, radioEvent.data_len, radioEvent.rssi, radioEvent.snr);
                OnMacStateCheckTimerEvent(NULL);
                
                break;
            
            case LORAMAC_RADIO_EVT_CLASSG_RX_DONE:
                LoRaMacState &= ~MAC_STATE_CLASSG_RX; 
//                LoRaMacClassGStart();
                LoRaMacRxDecode(radioEvent.data, radioEvent.data_len, radioEvent.rssi, radioEvent.snr);
                OnMacStateCheckTimerEvent(NULL);
                break;
            
            case LORAMAC_RADIO_EVT_CLASSG_RX_ERROR:
            case LORAMAC_RADIO_EVT_CLASSG_RX_TIMEOUT:
                LoRaMacState &= ~MAC_STATE_CLASSG_RX; 
//                LoRaMacClassGStart();
                break;
        }
        
        switch(radioEvent.evt)
        {
            case LORAMAC_RADIO_EVT_BEACON_RX_DONE:
            case LORAMAC_RADIO_EVT_BEACON_RX_TIMEOUT:
            case LORAMAC_RADIO_EVT_PING_RX_DONE:
            case LORAMAC_RADIO_EVT_PING_RX_TIMEOUT:
            case LORAMAC_RADIO_EVT_PING_RX_ERROR:
                if(LoRaMacAlohaPending)
                {
                    LoRaMacAlohaPending = false;
                    LoRaMacState |= MAC_STATE_ALOHA;
                    trace_notice("continue session\r\n");
                    if(LoRaMacSetNextChannel() == 0)
                    {
                        LoRaMacSendFrame(Channels[Channel]);
                    }
                }
                break;
            
            default:
                break;
        }
    }
}


void LoRaMacSetDeviceClass(DeviceClass_t deviceClass)
{
    DeviceClass = deviceClass;
    if(DeviceClass == CLASS_C)
    {
        LoRaMacState |= MAC_STATE_CLASSC_RX;
        LoRaMacRxWindowSetup(1, true);
    }
}

void LoRaMacSetPublicNetwork(bool enable)
{
    Radio.SetModem(MODEM_LORA);
    if(enable)
    {
        // Change LoRa modem SyncWord
        Radio.Write(REG_LR_SYNCWORD, LORA_MAC_PUBLIC_SYNCWORD);
    }
    else
    {
        // Change LoRa modem SyncWord
        Radio.Write(REG_LR_SYNCWORD, LORA_MAC_PRIVATE_SYNCWORD);
    }
}

void LoRaMacSetDutyCycleOn(bool enable)
{
    DutyCycleOn = enable;
}

void LoRaMacSetChannel(uint8_t id, ChannelParams_t params)
{
    params.Band = 0;
    Channels[id] = params;
    // Activate the newly created channel
    LoRaParam.db.ChMasks[0] |= 1 << id;

//#if defined( USE_BAND_868 )
//    if((Channels[id].Frequency >= 865000000) && (Channels[id].Frequency <= 868000000))
//    {
//        if(Channels[id].Band != BAND_G1_0)
//        {
//            Channels[id].Band = BAND_G1_0;
//        }
//    }
//    else if((Channels[id].Frequency > 868000000) && (Channels[id].Frequency <= 868600000))
//    {
//        if(Channels[id].Band != BAND_G1_1)
//        {
//            Channels[id].Band = BAND_G1_1;
//        }
//    }
//    else if((Channels[id].Frequency >= 868700000) && (Channels[id].Frequency <= 869200000))
//    {
//        if(Channels[id].Band != BAND_G1_2)
//        {
//            Channels[id].Band = BAND_G1_2;
//        }
//    }
//    else if((Channels[id].Frequency >= 869400000) && (Channels[id].Frequency <= 869650000))
//    {
//        if(Channels[id].Band != BAND_G1_3)
//        {
//            Channels[id].Band = BAND_G1_3;
//        }
//    }
//    else if((Channels[id].Frequency >= 869700000) && (Channels[id].Frequency <= 870000000))
//    {
//        if(Channels[id].Band != BAND_G1_4)
//        {
//            Channels[id].Band = BAND_G1_4;
//        }
//    }
//    else
//    {
//        Channels[id].Frequency = 0;
//        Channels[id].DrRange.Value = 0;
//    }
//#endif
    // Check if it is a valid channel
    if(Channels[id].FreqUp == 0)
    {
        LoRaParam.db.ChMasks[0] &= ~(1 << id);
    }
}

int LoRaMacIsValidFreq(uint32_t freq)
{
    for(uint8_t i = 0; i < LORA_MAX_NB_CHANNELS; i++)
    {
        if(freq == Channels[i].FreqUp)
        {
            return i;
        }
    }
    return -1;
}



void LoRaMacSetTxPower(int8_t txPower)
{
    for(uint8_t i = 0; i < sizeof(TxPowers); i++)
    {
        if(txPower == TxPowers[i])
        {
            LoRaParam.db.TXPower = txPower;
            return;
        }
    }
    LoRaParam.db.TXPower = 14;
}

int8_t LoRaMacCheckTxPower(int8_t txPower)
{
    for(uint8_t i = 0; i < sizeof(TxPowers); i++)
    {
        if(txPower == TxPowers[i])
        {
            return i;
        }
    }
    return -1;
}

int8_t LoRaMacGetTxPower(void)
{
    return LoRaParam.db.TXPower;
}

void LoRaMacSetChannelsDatarate(uint8_t datarate)
{
    if(LoRaParam.db.DataRate != datarate)
    {
        LoRaParam.db.DataRate = ChannleDatarate = datarate;
    }
}

bool LoRaMacCheckChannelsDatarate(uint8_t datarate)
{
    if(datarate > 5)
    {
        return false;
    }
    return true;
}

uint8_t LoRaMacGetChannelsDatarate(void)
{
    return LoRaParam.db.DataRate;
}

void LoRaMacSetChannelsMask(uint16_t mask)
{
    if(LoRaParam.db.ChMasks[0] != mask)
    {
        LoRaParam.db.ChMasks[0] = mask;
    }
}

uint16_t LoRaMacGetChannelsMask(void)
{
    return LoRaParam.db.ChMasks[0];
}

void LoRaMacSetChannelsNbRep(uint8_t nbTrans)
{
    if(nbTrans < 1)
    {
        nbTrans = 1;
    }
    if(nbTrans > 15)
    {
        nbTrans = 15;
    }
    if(LoRaParam.db.NbTrans != nbTrans)
    {
        LoRaParam.db.NbTrans = nbTrans;
    }
}

void LoRaMacSetMaxRxWindow(uint32_t delay)
{
    MaxRxWindow = delay;
}

void LoRaMacSetReceiveDelay1(uint32_t delay)
{
    if(LoRaParam.db.Del != delay)
    {
        LoRaParam.db.Del = delay;
    }
}

uint8_t LoRaMacGetReceiveDelay1(void)
{
    return LoRaParam.db.Del;
}

void LoRaMacSetJoinAcceptDelay1(uint32_t delay)
{
    JoinAcceptDelay1 = delay;
}

uint16_t LoRaMacGetUpLinkCounter(void)
{
    return UpLinkCounter;
}

void LoRaMacSetUpLinkCounter(uint16_t count)
{
    UpLinkCounter = count;
}

uint16_t LoRaMacGetDownLinkCounter(void)
{
    return DownLinkCounter;
}

uint16_t LoRaMacGetState(void)
{
    uint16_t state = LoRaMacState;
    state &= ~MAC_STATE_CLASSC_RX;
    return state;
}

void LoRaMacSetDevAddr(uint32_t devAddr)
{
    if(LoRaParam.db.DevAddr != devAddr)
    {
       LoRaParam.db.DevAddr = devAddr;
    }
}

uint32_t LoRaMacGetDevAddr(void)
{
    return LoRaParam.db.DevAddr;
}

void LoRaMacSetNwkSKey(uint8_t* nwkSKey)
{
    memcpy(LoRaParam.db.NwkSKey, nwkSKey, 16);
}

void LoRaMacGetNwkSKey(uint8_t* nwkSKey)
{
    memcpy(nwkSKey, LoRaParam.db.NwkSKey, 16);
}

void LoRaMacSetAppSKey(uint8_t* appSKey)
{
    memcpy(LoRaParam.db.AppSKey, appSKey, 16);
}

void LoRaMacGetAppSKey(uint8_t* appSKey)
{
    memcpy(appSKey, LoRaParam.db.AppSKey, 16);
}

void LoRaMacSetAppKey(uint8_t* appKey)
{
    memcpy(LoRaParam.db.AppKey, appKey, 16);
}

void LoRaMacGetAppKey(uint8_t* appKey)
{
    memcpy(appKey, LoRaParam.db.AppKey, 16);
}

void LoRaMacSetAppEui(uint8_t *appEui)
{
    memcpy(LoRaParam.db.AppEUI, appEui, 8); 
}

void LoRaMacGetAppEui(uint8_t *appEui)
{
    memcpy(appEui, LoRaParam.db.AppEUI, 8); 
}

void LoRaMacSetDevEui(uint8_t *devEui)
{
    memcpy(LoRaParam.db.DevEUI, devEui, 8); 
}

void LoRaMacGetDevEui(uint8_t *devEui)
{
    memcpy(devEui, LoRaParam.db.DevEUI, 8); 
}
   
void LoRaMacSetAdr(bool enable)
{
    if(LoRaParam.db.Adr != enable)
    {
        LoRaParam.db.Adr = enable;
    }
}

bool LoRaMacGetAdr(void)
{
    return LoRaParam.db.Adr;
}

void LoRaMacSetClassBEnable(bool enable)
{
    if(LoRaParam.db.ClassBEnable != enable)
    {
        LoRaParam.db.ClassBEnable = enable;
        if(!enable)
        {
            DeviceClass = CLASS_A;
            TIMER_STOP(BeaconTimer);
            TIMER_STOP(PingTimer);
            IsBeaconSearching = false;
        }
    }
}

void LoRaMacSetClassGEnable(bool enable)
{
    if(LoRaParam.db.ClassGEnabled != enable)
    {
        LoRaParam.db.ClassGEnabled = enable;
        if(enable)
        {
            DeviceClass = CLASS_G;
            ClassGScheduling = false;
            LoRaMacClassGStart();
        }
    }
}

bool LoRaMacGetClassBEnable(void)
{
    return LoRaParam.db.ClassBEnable;
}

void LoRaMacSetPingDatarate(uint8_t dr)
{
    if(LoRaParam.db.PingDataRate != dr)
    {
        LoRaParam.db.PingDataRate = dr;
        DeviceClass = CLASS_A;
        TIMER_STOP(PingTimer);
        TIMER_STOP(BeaconTimer);
        IsBeaconSearching = false;
    }
}

uint8_t LoRaMacGetPingDatarate(void)
{
    return LoRaParam.db.PingDataRate;
}

void LoRaMacSetPeriodity(uint8_t periodity)
{
    if( LoRaParam.db.PingPeriodicity != periodity )
    {
        LoRaParam.db.PingPeriodicity = periodity;
        DeviceClass = CLASS_A;
        TIMER_STOP(PingTimer);
        TIMER_STOP(BeaconTimer);
        IsBeaconSearching = false;
    }
}

uint8_t LoRaMacGetPeriodity(void)
{
    return LoRaParam.db.PingPeriodicity;
}

uint32_t LoRaMacGetTxLimitedRemain(void)
{
    if((SilenceTime > 0) && (LastTxTime != 0))
    {
        uint32_t curTime = run_time_get();
        if((curTime - LastTxTime) >= SilenceTime*1000)
        {
            SilenceTime = 0;
            return 0;
        }
        else
        {
            return (SilenceTime*1000 - (curTime - LastTxTime));
        }
    }
    else
    {
        return 0;
    }
}

uint8_t LoRaMacGetVersion(void)
{
    return LORAMAC_VERSION;
}

void LoRaMacSetNbTrans(uint8_t value)
{
    LoRaParam.db.NbTrans = value;
}

__WEAK uint8_t LoRaMacMeasureBatterieLevel(void)
{
    return 0xff;
}

__WEAK bool LoRaMacGetClassGTimeSpec(LoRaMacClassGTimeSpec_t * timeSpec)
{
    return false;
}

static void storage_access_wait(void)
{
    uint32_t count = 1;
    do
    {
        app_sched_execute();
        pstorage_access_status_get(&count);
    }
    while(count != 0);
}

bool LoRaMacFacConfig(uint8_t *p_data, uint16_t len)
{
    struct FacLoRaParam    protoConfig;
    uint8_t buff[512] = {0};
    LoRaParam_t         loraParam;
            
    memset(&loraParam, 0, sizeof(LoRaParam_t)); 
   
    memcpy(buff, p_data, len);
    
    trace_info("LoRaMacFacConfig\r\n");
    FacLoRaParam_read_delimited_from(buff, &protoConfig, len, 0);
    trace_info("LoRaMacFacConfig -\r\n");
    
    trace_info("param checking\r\n");
        
    if(protoConfig._EUI_len != 8){
        trace_info("invalid _EUI_len %d\r\n", protoConfig._EUI_len);
        return false;
    }
    
    if(protoConfig._appEUI_len != 8){
        trace_info("invalid _appEUI_len\r\n");
        return false;
    }
    
    if(protoConfig._appKey_len != 16){
        trace_info("invalid _appKey_len\r\n");
        return false;
    }
    
    if(protoConfig._appSKey_len != 16){
        trace_info("invalid _appSKey_len\r\n");
        return false;
    }
    
    if(protoConfig._nwkSKey_len != 16){
        trace_info("invalid _nwkSKey_len\r\n");
        return false;
    }
    
    if(protoConfig._devAddr_len != 4){
        trace_info("invalid _devAddr_len\r\n");
        return false;
    }
    
    if(protoConfig._devNonce_len != 2){
        trace_info("invalid _devNonce_len\r\n");
        return false;
    }
    
    if(protoConfig._appNonce_len != 3){
        trace_info("invalid _appNonce_len\r\n");
        return false;
    }
    
    if(protoConfig._netId_len != 3){
        trace_info("invalid _netId_len\r\n");
        return false;
    }
 
    if(protoConfig._channels_repeated_len == 0){
        trace_info("invalid _channels_repeated_len\r\n");
        return false;
    }
    trace_info("check ok\r\n");
   
    loraParam.db.magic_byte = LORAMAC_STORAGE_MAGIC_BYTE_V140;
    
    memcpy(loraParam.db.DevEUI, protoConfig._EUI, protoConfig._EUI_len);
    memcpy(loraParam.db.AppEUI, protoConfig._appEUI, protoConfig._appEUI_len);
    memcpy(loraParam.db.AppKey, protoConfig._appKey, protoConfig._appKey_len);
    memcpy(loraParam.db.AppSKey, protoConfig._appSKey, protoConfig._appSKey_len);
    memcpy(loraParam.db.NwkSKey, protoConfig._nwkSKey, protoConfig._nwkSKey_len);
    loraParam.db.DevAddr         = uint32_big_decode((const uint8_t * )protoConfig._devAddr);
    loraParam.db.DevNonce        = uint16_big_decode((const uint8_t * )protoConfig._devNonce);
    loraParam.db.AppNonce        = uint24_big_decode((const uint8_t * )protoConfig._appNonce);
    loraParam.db.NetID           = uint24_big_decode((const uint8_t * )protoConfig._netId);

    loraParam.db.Adr             = protoConfig._adr;
    loraParam.db.TXPower         = protoConfig._txPower;
    loraParam.db.DataRate        = protoConfig._dataRate;
    
    for(uint8_t i = 0; i < 7; i++)
    {
        loraParam.db.ChMasks[i]  = 0x0;
    }
    for(uint8_t i = 0; i < protoConfig._channelMask_repeated_len; i++)
    {
        loraParam.db.ChMasks[i]      = protoConfig._channelMask[i];
    }
    
    loraParam.db.NbTrans         = protoConfig._nbTrans;
    loraParam.db.MaxDCycle       = protoConfig._maxDutyCycle;
    loraParam.db.RX1DROffset     = protoConfig._rx1DRoffset;
    loraParam.db.RX2Freq         = protoConfig._rx2Frequency;
    loraParam.db.RX2DateRate     = protoConfig._rx2DataRate;
    loraParam.db.Del             = protoConfig._delay;
    
    loraParam.db.ClassBEnable    = protoConfig._classBEnabled;
    loraParam.db.PingPeriodicity = protoConfig._pingPeriodicity;
    loraParam.db.PingDataRate    = protoConfig._pingDataRate;
    loraParam.db.BeaconFreq      = protoConfig._beaconFrequency;
    loraParam.db.BeaconDataRate  = protoConfig._beaconDataRate;
    
    loraParam.db.PingChannel.FreqDown = protoConfig._pingSlotChannel._rx1Frequency;
    loraParam.db.PingChannel.FreqUp = protoConfig._pingSlotChannel._frequency;
    loraParam.db.PingChannel.DRMax = protoConfig._pingSlotChannel._drRange._drMax;
    loraParam.db.PingChannel.DRMin = protoConfig._pingSlotChannel._drRange._drMin;
    
    loraParam.db.ClassBEnable = protoConfig._classGEnabled;
    loraParam.db.ClassGDataRate = protoConfig._classGDataRate;
    loraParam.db.ClassGPeriod = protoConfig._classGPeriod;
    loraParam.db.ClassGChannel.FreqDown = protoConfig._pingSlotChannel._rx1Frequency;
    loraParam.db.ClassGChannel.FreqUp = protoConfig._pingSlotChannel._frequency;
    loraParam.db.ClassGChannel.DRMax = protoConfig._pingSlotChannel._drRange._drMax;
    loraParam.db.ClassGChannel.DRMin = protoConfig._pingSlotChannel._drRange._drMin;
    
    loraParam.db.Activation = protoConfig._activation;
    loraParam.db.ClassType = protoConfig._classType;
    
    loraParam.db.BandLen = protoConfig._bands_repeated_len;
    for(uint8_t i = 0; i < protoConfig._bands_repeated_len; i++)
    {
        loraParam.db.Band[i].DCycle = protoConfig._bands[i]._dutyCycle;
        loraParam.db.Band[i].MaxTxPower = protoConfig._bands[i]._maxTxPower;
    }
    for(uint8_t i = 0; i < protoConfig._channels_repeated_len; i++)
    {
        loraParam.db.AlohaChannel[i].FreqDown = protoConfig._channels[i]._rx1Frequency;
        loraParam.db.AlohaChannel[i].FreqUp = protoConfig._channels[i]._frequency;
        loraParam.db.AlohaChannel[i].DRMax = protoConfig._channels[i]._drRange._drMax;
        loraParam.db.AlohaChannel[i].DRMin = protoConfig._channels[i]._drRange._drMin;
        loraParam.db.AlohaChannel[i].Band = protoConfig._channels[i]._bandIndex;
    }
    
    trace_info("store param\r\n");
    
    pstorage_module_param_t pstorage_param;
    pstorage_handle_t raw_block_id;
    pstorage_param.cb = pstorage_ntf_cb;
    pstorage_register(&pstorage_param, &raw_block_id);
    
    memset(buff, 0, sizeof(buff));        
    memcpy(buff, &loraParam, sizeof(LoRaParam_t));

    raw_block_id.block_id = STORAGE_ADDR_LORA_PARAM;
    pstorage_clear(&raw_block_id, sizeof(LoRaParam_t));
    pstorage_store(&raw_block_id, buff, sizeof(LoRaParam_t), 0);
    storage_access_wait();
       
    raw_block_id.block_id = STORAGE_ADDR_FAC_LORA_PARAM;
    pstorage_clear(&raw_block_id, sizeof(LoRaParam_t));
    pstorage_store(&raw_block_id, buff, sizeof(LoRaParam_t), 0);
    storage_access_wait();
    
    trace_info("configure ok\r\n");
    
    return true;
}

static void LoRaMacVersionChange(void)
{
    uint8_t *p_data = (uint8_t *)STORAGE_ADDR_FAC_LORA_PARAM;
    uint8_t magic_byte = *p_data;
    
    if(LORAMAC_STORAGE_MAGIC_BYTE_05 == magic_byte)
    {
        trace_info("LoRaMacVersionChange\r\n");
        uint8_t buff[512] = {0};
        LoRaParamMagic05_t loraParam05;
        LoRaParam_t loraParam;
        
        memcpy(&loraParam05, p_data, sizeof(LoRaParamMagic05_t));
        
        loraParam.db.magic_byte = LORAMAC_STORAGE_MAGIC_BYTE_V140;
        
        memcpy(loraParam.db.DevEUI, loraParam05.DevEUI, 8);
        memcpy(loraParam.db.AppEUI, loraParam05.AppEUI, 8);
        memcpy(loraParam.db.AppKey, loraParam05.AppKey, 16);
        memcpy(loraParam.db.AppSKey, loraParam05.AppSKey, 16);
        memcpy(loraParam.db.NwkSKey, loraParam05.NwkSKey, 16);
        loraParam.db.DevAddr    = loraParam05.DevAddr;
        loraParam.db.NetID      = loraParam05.NetID;
        loraParam.db.AppNonce   = loraParam05.AppNonce;
        loraParam.db.DevNonce   = loraParam05.DevNonce;
        
        loraParam.db.Activation = 0;
        loraParam.db.ClassType  = 0;
        
        loraParam.db.BandLen            = 1;
        loraParam.db.Band[0].DCycle     = 1;
        loraParam.db.Band[0].MaxTxPower = 20;
        
        for(uint8_t i = 0; i < 16; i++)
        {
            loraParam.db.AlohaChannel[i].FreqUp = loraParam05.AlohaChannel[i].FreqUp;
            loraParam.db.AlohaChannel[i].FreqDown = loraParam05.AlohaChannel[i].FreqDown;
            loraParam.db.AlohaChannel[i].DRMax = loraParam05.AlohaChannel[i].DRMax;
            loraParam.db.AlohaChannel[i].DRMin = loraParam05.AlohaChannel[i].DRMin;
            loraParam.db.AlohaChannel[i].Band = 0;
        }
        
        loraParam.db.Adr = loraParam05.Adr;
        loraParam.db.MaxTXPower = loraParam05.MaxTXPower;
        loraParam.db.TXPower = loraParam05.TXPower;
        loraParam.db.DataRate = loraParam05.DataRate;
        
        for(uint8_t i = 0; i < 7; i++)
        {
            loraParam.db.ChMasks[i]  = 0x0;
        }
        loraParam.db.ChMasks[0]     = loraParam05.ChMask;
        
        loraParam.db.NbTrans        = loraParam05.NbTrans;
        loraParam.db.MaxDCycle      = loraParam05.MaxDCycle;
        loraParam.db.RX1DROffset    = loraParam05.RX1DROffset;
        loraParam.db.RX2Freq        = loraParam05.RX2Freq;
        loraParam.db.RX2DateRate    = loraParam05.RX2DateRate;
        loraParam.db.Del            = loraParam05.Del;
        
        loraParam.db.ClassBEnable   = loraParam05.ClassBEnable;
        loraParam.db.PingChannel.FreqUp     = loraParam05.PingChannel.FreqUp;
        loraParam.db.PingChannel.FreqDown   = loraParam05.PingChannel.FreqDown;
        loraParam.db.PingChannel.DRMax      = loraParam05.PingChannel.DRMax;
        loraParam.db.PingChannel.DRMin      = loraParam05.PingChannel.DRMin;
        loraParam.db.PingChannel.Band       = 0;
        loraParam.db.PingPeriodicity    = loraParam05.PingPeriodicity;
        loraParam.db.PingDataRate       = loraParam05.PingDataRate;
        loraParam.db.BeaconFreq         = loraParam05.BeaconFreq;
        loraParam.db.BeaconDataRate     = loraParam05.BeaconDataRate;
        
        pstorage_module_param_t pstorage_param;
        pstorage_handle_t raw_block_id;
        pstorage_param.cb = pstorage_ntf_cb;
        pstorage_register(&pstorage_param, &raw_block_id);
        
        memset(buff, 0, sizeof(buff));        
        memcpy(buff, &loraParam, sizeof(LoRaParam_t));

        raw_block_id.block_id = STORAGE_ADDR_LORA_PARAM;
        pstorage_clear(&raw_block_id, sizeof(LoRaParam_t));
        pstorage_store(&raw_block_id, buff, sizeof(LoRaParam_t), 0);
        storage_access_wait();
           
        raw_block_id.block_id = STORAGE_ADDR_FAC_LORA_PARAM;
        pstorage_clear(&raw_block_id, sizeof(LoRaParam_t));
        pstorage_store(&raw_block_id, buff, sizeof(LoRaParam_t), 0);
        storage_access_wait();
        
        NVIC_SystemReset();
    }
}
