//==============================================================================
//    S E N S I R I O N   AG,  Laubisruetistr. 50, CH-8712 Staefa, Switzerland
//==============================================================================
// Project   :  SHT2x Sample Code (V1.2)
// File      :  SHT2x.h
// Author    :  MST
// Controller:  NEC V850/SG3 (uPD70F3740)
// Compiler  :  IAR compiler for V850 (3.50A)
// Brief     :  Sensor layer. Definitions of commands and registers,
//              functions for sensor access
//==============================================================================

/* Copyright (c) 2018 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#ifndef __SHT2X_H__
#define __SHT2X_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__CC_ARM) || defined(__GNUC__)
#define _PACKED                                      __attribute__( ( __packed__ ) )
#elif defined( __ICCARM__ )
#define _PACKED                                      __packed
#else
#warning Not supported compiler type
#endif

//---------- Defines -----------------------------------------------------------
//Processor endian system
//#define BIG ENDIAN   //e.g. Motorola (not tested at this time)
#define LITTLE_ENDIAN  //e.g. PIC, 8051, NEC V850

/**
 * Sht2x io transfer prototype
 */
typedef bool (*sht2x_io_transfer_t)(uint8_t address, uint8_t *p_buffer, uint8_t bytes, bool stop);

/**
 * Sht2x reset prototype.
 */
typedef void (*sht2x_reset_t)(void *handle);

/**
 * Sht2x delsy prototype
 */
typedef void (*sht2x_delay_ms)(uint32_t);


typedef union
{
    uint16_t u16;               // element specifier for accessing whole u16
    int16_t i16;                // element specifier for accessing whole i16
    struct
    {
#ifdef LITTLE_ENDIAN      // Byte-order is little endian
        uint8_t u8L;              // element specifier for accessing low u8
        uint8_t u8H;              // element specifier for accessing high u8
#else                     // Byte-order is big endian
        uint8_t u8H;              // element specifier for accessing low u8
        uint8_t u8L;              // element specifier for accessing high u8
#endif
    } s16;                      // element spec. for acc. struct with low or high u8
} nt16;

typedef union
{
    uint32_t u32;               // element specifier for accessing whole u32
    int32_t  i32;               // element specifier for accessing whole i32
    struct
    {
#ifdef LITTLE_ENDIAN      // Byte-order is little endian
        uint16_t u16L;            // element specifier for accessing low u16
        uint16_t u16H;            // element specifier for accessing high u16
#else                     // Byte-order is big endian
        uint16_t u16H;            // element specifier for accessing low u16
        uint16_t u16L;            // element specifier for accessing high u16
#endif
    } s32;                      // element spec. for acc. struct with low or high u16
} nt32;

// Error codes
typedef enum
{
    ACK_ERROR                = 0x01,
    TIME_OUT_ERROR           = 0x02,
    CHECKSUM_ERROR           = 0x04,
    UNIT_ERROR               = 0x08
} etError;

// sensor command
typedef enum
{
    TRIG_T_MEASUREMENT_HM    = 0xE3, // command trig. temp meas. hold master
    TRIG_RH_MEASUREMENT_HM   = 0xE5, // command trig. humidity meas. hold master
    TRIG_T_MEASUREMENT_POLL  = 0xF3, // command trig. temp meas. no hold master
    TRIG_RH_MEASUREMENT_POLL = 0xF5, // command trig. humidity meas. no hold master
    USER_REG_W               = 0xE6, // command writing user register
    USER_REG_R               = 0xE7, // command reading user register
    SOFT_RESET               = 0xFE  // command soft reset
} etSHT2xCommand;

typedef enum
{
    SHT2x_RES_12_14BIT       = 0x00, // RH=12bit, T=14bit
    SHT2x_RES_8_12BIT        = 0x01, // RH= 8bit, T=12bit
    SHT2x_RES_10_13BIT       = 0x80, // RH=10bit, T=13bit
    SHT2x_RES_11_11BIT       = 0x81, // RH=11bit, T=11bit
    SHT2x_RES_MASK           = 0x81  // Mask for res. bits (7,0) in user reg.
} etSHT2xResolution;

typedef enum
{
    SHT2x_EOB_ON             = 0x40, // end of battery
    SHT2x_EOB_MASK           = 0x40, // Mask for EOB bit(6) in user reg.
} etSHT2xEob;

typedef enum
{
    SHT2x_HEATER_ON          = 0x04, // heater on
    SHT2x_HEATER_OFF         = 0x00, // heater off
    SHT2x_HEATER_MASK        = 0x04, // Mask for Heater bit(2) in user reg.
} etSHT2xHeater;

// measurement signal selection
typedef enum
{
    HUMIDITY,
    TEMP
} etSHT2xMeasureType;

typedef enum
{
    I2C_ADR_W                = 128,   // sensor I2C address + write bit
    I2C_ADR_R                = 129    // sensor I2C address + read bit
} etI2cHeader;

typedef struct
{
    uint8_t address;
    uint32_t reset_pin;
    sht2x_reset_t reset;
    sht2x_io_transfer_t transfer;
    sht2x_delay_ms delay_ms;
} sht2x_t;


bool sht2x_init(void *handle);

//==============================================================================
uint8_t SHT2x_CheckCrc(uint8_t data[], uint8_t nbrOfBytes, uint8_t checksum);
//==============================================================================
// calculates checksum for n bytes of data and compares it with expected
// checksum
// input:  data[]       checksum is built based on this data
//         nbrOfBytes   checksum is built for n bytes of data
//         checksum     expected checksum
// return: error:       CHECKSUM_ERROR = checksum does not match
//                      0              = checksum matches

//==============================================================================
bool SHT2x_ReadUserRegister(void *handle, uint8_t* pRegisterValue);
//==============================================================================
// reads the SHT2x user register (8bit)
// input : -
// output: *pRegisterValue
// return: error

//==============================================================================
bool SHT2x_WriteUserRegister(void *handle, uint8_t* pRegisterValue);
//==============================================================================
// writes the SHT2x user register (8bit)
// input : *pRegisterValue
// output: -
// return: error

//==============================================================================
bool SHT2x_MeasurePoll(void *handle, etSHT2xMeasureType eSHT2xMeasureType, nt16* pMeasurand);
//==============================================================================
// measures humidity or temperature. This function polls every 10ms until
// measurement is ready.
// input:  eSHT2xMeasureType
// output: *pMeasurand:  humidity / temperature as raw value
// return: error
// note:   timing for timeout may be changed

//==============================================================================
bool SHT2x_MeasureHM(void *handle, etSHT2xMeasureType eSHT2xMeasureType, nt16* pMeasurand);
//==============================================================================
// measures humidity or temperature. This function waits for a hold master until
// measurement is ready or a timeout occurred.
// input:  eSHT2xMeasureType
// output: *pMeasurand:  humidity / temperature as raw value
// return: error
// note:   timing for timeout may be changed

//==============================================================================
bool SHT2x_SoftReset(void *handle);
//==============================================================================
// performs a reset
// input:  -
// output: -
// return: error

//==============================================================================
float SHT2x_CalcRH(uint16_t u16sRH);
//==============================================================================
// calculates the relative humidity
// input:  sRH: humidity raw value (16bit scaled)
// return: pHumidity relative humidity [%RH]

//==============================================================================
float SHT2x_CalcTemperatureC(uint16_t u16sT);
//==============================================================================
// calculates temperature
// input:  sT: temperature raw value (16bit scaled)
// return: temperature [?]

//==============================================================================
bool SHT2x_GetSerialNumber(void *handle, uint8_t u8SerialNumber[]);
//==============================================================================
// gets serial number of SHT2x according application note "How To
// Read-Out the Serial Number"
// note:   readout of this function is not CRC checked
//
// input:  -
// output: u8SerialNumber: Array of 8 bytes (64Bits)
//         MSB                                         LSB
//         u8SerialNumber[7]             u8SerialNumber[0]
//         SNA_1 SNA_0 SNB_3 SNB_2 SNB_1 SNB_0 SNC_1 SNC_0
// return: error
#endif
