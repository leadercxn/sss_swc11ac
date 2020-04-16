/* Copyright (c) 2018 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#ifndef __SHT3XD_H__
#define __SHT3XD_H__

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

/**
 * Sht3xd io transfer prototype
 */
typedef bool (*sht3xd_io_transfer_t)(void *handle, uint16_t command, uint8_t *p_buffer, uint16_t bytes);

/**
 * Sht3xd reset prototype.
 */
typedef void (*sht3xd_reset_t)(void *handle);

/**
 * Sht3xd commands
 */
typedef enum
{
    CMD_READ_SERIAL_NUMBER = 0x3780,

    CMD_READ_STATUS = 0xF32D,
    CMD_CLEAR_STATUS = 0x3041,

    CMD_HEATER_ENABLE = 0x306D,
    CMD_HEATER_DISABLE = 0x3066,

    CMD_SOFT_RESET = 0x30A2,
    CMD_CALL_RESET = 0x0006,

    CMD_CLOCK_STRETCH_H = 0x2C06,
    CMD_CLOCK_STRETCH_M = 0x2C0D,
    CMD_CLOCK_STRETCH_L = 0x2C10,

    CMD_POLLING_H = 0x2400,
    CMD_POLLING_M = 0x240B,
    CMD_POLLING_L = 0x2416,

    CMD_ART = 0x2B32,

    CMD_PERIODIC_HALF_H = 0x2032,
    CMD_PERIODIC_HALF_M = 0x2024,
    CMD_PERIODIC_HALF_L = 0x202F,
    CMD_PERIODIC_1_H = 0x2130,
    CMD_PERIODIC_1_M = 0x2126,
    CMD_PERIODIC_1_L = 0x212D,
    CMD_PERIODIC_2_H = 0x2236,
    CMD_PERIODIC_2_M = 0x2220,
    CMD_PERIODIC_2_L = 0x222B,
    CMD_PERIODIC_4_H = 0x2334,
    CMD_PERIODIC_4_M = 0x2322,
    CMD_PERIODIC_4_L = 0x2329,
    CMD_PERIODIC_10_H = 0x2737,
    CMD_PERIODIC_10_M = 0x2721,
    CMD_PERIODIC_10_L = 0x272A,

    CMD_FETCH_DATA = 0xE000,
    CMD_STOP_PERIODIC = 0x3093,

    CMD_READ_ALR_LIMIT_LS = 0xE102,
    CMD_READ_ALR_LIMIT_LC = 0xE109,
    CMD_READ_ALR_LIMIT_HS = 0xE11F,
    CMD_READ_ALR_LIMIT_HC = 0xE114,

    CMD_WRITE_ALR_LIMIT_HS = 0x611D,
    CMD_WRITE_ALR_LIMIT_HC = 0x6116,
    CMD_WRITE_ALR_LIMIT_LC = 0x610B,
    CMD_WRITE_ALR_LIMIT_LS = 0x6100,

    CMD_NO_SLEEP = 0x303E,
} sht3xd_command_e;

/**
 * Sht3xd repeatability
 */
typedef enum
{
    REPEATABILITY_HIGH,
    REPEATABILITY_MEDIUM,
    REPEATABILITY_LOW,
} sht3xd_repeatability_e;

/**
 * Sht3xd mode.
 */
typedef enum
{
    MODE_CLOCK_STRETCH,
    MODE_POLLING,
} sht3xd_mode_e;

typedef enum
{
    FREQUENCY_HZ5,
    FREQUENCY_1HZ,
    FREQUENCY_2HZ,
    FREQUENCY_4HZ,
    FREQUENCY_10HZ
} sht3xd_frequency_e;

/**
 * Sht3xd error code.
 */
typedef enum
{
    SHT3XD_ERR_NONE = 0,
    SHT3XD_ERR_BUS,
    SHT3XD_ERR_CRC,
    SHT3XD_ERR_INVALID_MODE,
    SHT3XD_ERR_INVALID_REPEATABILITY,
    SHT3XD_ERR_INVALID_FREQUENCY,
    SHT3XD_ERR_INVALID_ALERT,
} sht3xd_errorCode;

/**
 * Sht3xd register status.
 */
typedef union
{
    uint16_t rawData;
    struct
    {
        uint16_t WriteDataChecksumStatus : 1;
        uint16_t CommandStatus : 1;
        uint16_t Reserved0 : 2;
        uint16_t SystemResetDetected : 1;
        uint16_t Reserved1 : 5;
        uint16_t T_TrackingAlert : 1;
        uint16_t RH_TrackingAlert : 1;
        uint16_t Reserved2 : 1;
        uint16_t HeaterStatus : 1;
        uint16_t Reserved3 : 1;
        uint16_t AlertPending : 1;
    };
} sht3xd_reg_status_t;

/**
 * Sht3xd handle.
 */
typedef struct
{
    uint8_t address;
    uint32_t reset_pin;
    sht3xd_reset_t reset;
    sht3xd_io_transfer_t write;
    sht3xd_io_transfer_t read;
} sht3xd_t;

/**
 * Function for initializing sht3xd.
 *
 * @param     handle    Sht3xd handle.
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool sht3xd_init(void *handle);

/**
 * [sht3xd_softReset description]
 *
 * @param     handle    Sht3xd handle.
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool sht3xd_softReset(void *handle);

/**
 * Function for clear all status.
 *
 * @param     handle    Sht3xd handle.
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool sht3xd_clearAll(void *handle);

/**
 * [sht3xd_readTempHumiClockStretch description]
 *
 * @param     handle           Sht3xd handle.
 * @param     repeatability    [description]
 * @param     p_temp           [description]
 * @param     p_rh             [description]
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool sht3xd_readTempHumiClockStretch(void *handle, sht3xd_repeatability_e repeatability, float* p_temp, float* p_rh);

/**
 * [sht3xd_periodicStart description]
 *
 * @param     handle           Sht3xd handle.
 * @param     repeatability    [description]
 * @param     frequency        [description]
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool sht3xd_periodicStart(void *handle, sht3xd_repeatability_e repeatability, sht3xd_frequency_e frequency);

/**
 * [sht3xd_periodicStop description]
 *
 * @param     handle    Sht3xd handle.
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool sht3xd_periodicStop(void *handle);

/**
 * [sht3xd_periodicFetchData description]
 *
 * @param     handle    Sht3xd handle.
 * @param     p_temp    [description]
 * @param     p_rh      [description]
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool sht3xd_periodicFetchData(void *handle, float* p_temp, float* p_rh);

/**
 * Function for enabling heater.
 *
 * @param     handle    Sht3xd handle.
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool sht3xd_heaterEnable(void *handle);

/**
 * Function for disabling heater.
 *
 * @param     handle    Sht3xd handle.
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool sht3xd_heaterDisable(void *handle);

/**
 * Function for reading status register
 *
 * @param     handle    Sht3xd handle.
 * @param     p_status  Sht3xd register status.
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool sht3xd_readStatusRegister(void *handle, sht3xd_reg_status_t* p_status);


#ifdef _cplusplus
}
#endif

#endif
