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
#ifndef __LORAMAC_H__
#define __LORAMAC_H__

#include "app_util.h"

// Includes board dependent definitions such as channels frequencies

#if defined(__CC_ARM) || defined(__GNUC__)
#define _PACKED                                      __attribute__( ( __packed__ ) )
#elif defined( __ICCARM__ )
#define _PACKED                                      __packed
#else
    #warning Not supported compiler type
#endif

#define BAND_433    0
#define BAND_470    1
#define BAND_780    2
#define BAND_868    3
#define BAND_915    4

#define STORAGE_ADDR_LORA_FCNT          0X3A800
#define STORAGE_ADDR_LORA_PARAM         0X3AC00
#define STORAGE_ADDR_FAC_LORA_PARAM     0X3B000


/*!
 * LoRaWAN devices classes definition
 */
typedef enum
{
    CLASS_A,
    CLASS_B,
    CLASS_C,
    CLASS_G,
}DeviceClass_t;

enum LoRaMacError_t
{
    LORAMAC_ERROR_NONE                  = 0,
    LORAMAC_ERROR_BUSY                  = 1,
    LORAMAC_ERROR_NO_NETWORK_JOINED     = 2,
    LORAMAC_ERROR_MAXPAYLOAD_EXCEEDED   = 3,
    LORAMAC_ERROR_INVALID_FRAME_TYPE    = 4,
    LORAMAC_ERROR_TX_LIMITED,
};

/*!
 * LoRaMAC channels parameters definition
 */
typedef union
{
    int8_t Value;
    struct
    {
        int8_t Min : 4;
        int8_t Max : 4;
    };
} DrRange_t;

typedef struct
{
    uint16_t DCycle;
    int8_t TxMaxPower;
    uint32_t LastTxDoneTime;
    uint32_t TimeOff;
} Band_t;

typedef struct
{
    uint32_t FreqUp; // Hz
    uint32_t FreqDown;   
    DrRange_t DrRange;  // Max datarate [0: SF12, 1: SF11, 2: SF10, 3: SF9, 4: SF8, 5: SF7, 6: SF7, 7: FSK]
                        // Min datarate [0: SF12, 1: SF11, 2: SF10, 3: SF9, 4: SF8, 5: SF7, 6: SF7, 7: FSK]
    uint8_t Band;       // Band index
} ChannelParams_t;

/*!
 * LoRaMAC event flags
 */
typedef union
{
    uint8_t Value;
    struct
    {
        uint8_t Tx              : 1;
        uint8_t Rx              : 1;
        uint8_t RxData          : 1;
        uint8_t Multicast       : 1;
        uint8_t RxSlot          : 2;
        uint8_t LinkCheck       : 1;
        uint8_t JoinAccept      : 1;
    }_PACKED Bits;
}LoRaMacEventFlags_t;

typedef enum
{
    LORAMAC_EVENT_INFO_STATUS_OK = 0,
    LORAMAC_EVENT_INFO_STATUS_ERROR,
    LORAMAC_EVENT_INFO_STATUS_TX_TIMEOUT,
    LORAMAC_EVENT_INFO_STATUS_RX2_TIMEOUT,
    LORAMAC_EVENT_INFO_STATUS_RX2_ERROR,
    LORAMAC_EVENT_INFO_STATUS_JOIN_FAIL,
    LORAMAC_EVENT_INFO_STATUS_DOWNLINK_FAIL,
    LORAMAC_EVENT_INFO_STATUS_ADDRESS_FAIL,
    LORAMAC_EVENT_INFO_STATUS_MIC_FAIL,
}LoRaMacEventInfoStatus_t;

/*!
 * LoRaMAC event information
 */
typedef struct
{
    LoRaMacEventInfoStatus_t Status;
    bool TxAckReceived;
    bool Pendding;
    uint8_t TxNbRetries;
    uint8_t TxDatarate;
    uint8_t RxDatarate;
    uint8_t TxPort;
    uint8_t RxPort;
    uint32_t TxFreq;
    uint32_t RxFreq;
    int8_t   TxPower;
    uint8_t *RxBuffer;
    uint8_t RxBufferSize;
    int16_t RxRssi;
    uint8_t RxSnr;
    uint16_t Energy;
    uint8_t DemodMargin;
    uint8_t NbGateways;
}LoRaMacEventAlohaInfo_t;

typedef struct
{
    uint8_t RxData;
    bool Pendding;
    uint8_t RxDatarate;
    uint8_t RxPort;
    uint32_t RxFreq;
    uint8_t *RxBuffer;
    uint8_t RxBufferSize;
    int16_t RxRssi;
    uint8_t RxSnr;
    uint8_t CusAdrUpdStatus;
}LoRaMacEventRxInfo_t;

typedef enum
{
    LORAMAC_BEACON_LOCKED = 0,
    LORAMAC_BEACON_UNLOCKED
}LoRaMacEventBeaconInfo_t;

/*!
 * LoRaMAC events structure
 * Used to notify upper layers of MAC events
 */
typedef struct sLoRaMacEvent
{
    void ( *AlohaEvent )( LoRaMacEventFlags_t *flags, LoRaMacEventAlohaInfo_t *p_info );
    void ( *RxEvent )(LoRaMacEventRxInfo_t *p_info );
    void ( *BeaconEvent )(LoRaMacEventBeaconInfo_t info);
}LoRaMacEvent_t;

typedef struct sLoRaMacClassGTimeSpec
{
    uint32_t time_s;
    uint32_t time_ms;
}LoRaMacClassGTimeSpec_t;


void LoRaMacInit(void);
void LoRaMacInitParam(void);
void LoRaMacEventHandlerRegister(LoRaMacEvent_t* events);

uint8_t LoRaMacSendUnconfirmedPacket( uint8_t fPort, void *fBuffer, uint16_t fBufferSize );
uint8_t LoRaMacSendConfirmedPacket( uint8_t fPort, void *fBuffer, uint16_t fBufferSize, uint8_t nbRetries );

void LoRaMacScheduler(void);

void LoRaMacSetPublicNetwork( bool enable );
void LoRaMacSetDutyCycleOn( bool enable );

uint16_t LoRaMacGetUpLinkCounter(void);
void LoRaMacSetUpLinkCounter(uint16_t count);

uint16_t LoRaMacGetDownLinkCounter( void );

void LoRaMacSetChannelsCoderate( uint8_t coderate );
int LoRaMacIsValidFreq(uint32_t freq);

int8_t LoRaMacCheckTxPower(int8_t txPower);
void LoRaMacSetTxPower( int8_t txPower );
int8_t LoRaMacGetTxPower(void);

void LoRaMacSetChannelsDatarate(uint8_t datarate);
uint8_t LoRaMacGetChannelsDatarate(void);
bool LoRaMacCheckChannelsDatarate(uint8_t datarate);

void LoRaMacSetChannelsMask(uint16_t mask);
uint16_t LoRaMacGetChannelsMask(void);

void LoRaMacSetDevAddr(uint32_t devAddr);
uint32_t LoRaMacGetDevAddr(void);

void LoRaMacSetNwkSKey(uint8_t *nwkSKey);
void LoRaMacGetNwkSKey(uint8_t *nwkSKey);

void LoRaMacSetAppSKey(uint8_t *appSKey);
void LoRaMacGetAppSKey(uint8_t* appSKey);

void LoRaMacSetAppKey(uint8_t* appKey);
void LoRaMacGetAppKey(uint8_t* appKey);

void LoRaMacSetAppEui(uint8_t *appEui);
void LoRaMacGetAppEui(uint8_t *appEui);

void LoRaMacSetDevEui(uint8_t *devEui);
void LoRaMacGetDevEui(uint8_t *devEui);

void LoRaMacSetClassBEnable(bool enable);
bool LoRaMacGetClassBEnable(void);

void LoRaMacSetPingDatarate(uint8_t dr);
uint8_t LoRaMacGetPingDatarate(void);

void LoRaMacSetPeriodity(uint8_t periodity);
uint8_t LoRaMacGetPeriodity(void);

void LoRaMacSetAdr( bool enable );
bool LoRaMacGetAdr( void );

void LoRaMacSetNbTrans(uint8_t value);

uint16_t LoRaMacGetState(void);

uint32_t LoRaMacGetTxLimitedRemain(void);

void LoRaMacUpdateConfig(void);
void LoRaMacResetConfig(void);

uint8_t LoRaMacGetVersion(void);

bool LoRaMacFacConfig(uint8_t *p_data, uint16_t len);

void LoRaMacBeaconScan(void);

void LoRaMacSetDeviceClass(DeviceClass_t deviceClass);

void LoRaMacSetClassGEnable(bool enable);

__WEAK bool LoRaMacGetClassGTimeSpec(LoRaMacClassGTimeSpec_t * timeSpec);

__WEAK uint8_t LoRaMacMeasureBatterieLevel(void);

void LoRaMacSetReceiveDelay1(uint32_t delay);
uint8_t LoRaMacGetReceiveDelay1(void);

void LoRaMacSetChannel(uint8_t id, ChannelParams_t params);


#endif // __LORAMAC_H__
