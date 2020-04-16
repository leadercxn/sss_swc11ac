/* Copyright (c) 2018 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#ifndef __OPT3001_H__
#define __OPT3001_H__

/* Register addresses */
#define OPT3001_REG_RESULT                      0x00
#define OPT3001_REG_CONFIG                      0x01
#define OPT3001_REG_LOW_LIMIT                   0x02
#define OPT3001_REG_HIGH_LIMIT                  0x03
#define OPT3001_REG_MANF_ID                     0x7E
#define OPT3001_REG_DEVICE_ID                   0x7F

/* Register values */
#define OPT3001_VAL_MANF_ID                     0x5449  // TI
#define OPT3001_VAL_DEVICE_ID                   0x3001  // Opt 3001
#define OPT3001_VAL_CONFIG_RESET                0xC810
#define OPT3001_VAL_CONFIG_TEST                 0xCC10
#define OPT3001_VAL_CONFIG_ENABLE               0xCC10 // 0xCC10
#define OPT3001_VAL_CONFIG_DISABLE              0xC810 // 0xC810

/* Bit values */
#define OPT3001_DATA_RDY_BIT                    0x0080  // Data ready

/* Register length */
#define OPT3001_REG_LENGTH                      2

/* Sensor data size */
#define OPT3001_DATA_LENGTH                     2

/**
 * Opt3001 io transfer prototype.
 */
typedef bool (*opt3001_io_transfer_t)(void *handle, uint8_t addr, uint8_t *p_buffer, uint16_t bytes);

/**
 * Opt3001 configuration.
 */
typedef union
{
    uint16_t value;
    struct
    {
        uint16_t FaultCount : 2;
        uint16_t MaskExponent : 1;
        uint16_t Polarity : 1;
        uint16_t Latch : 1;
        uint16_t FlagLow : 1;
        uint16_t FlagHigh : 1;
        uint16_t ConversionReady : 1;
        uint16_t OverflowFlag : 1;
        uint16_t ModeOfConversionOperation : 2;
        uint16_t ConvertionTime : 1;
        uint16_t RangeNumber : 4;
    };
} opt3001_config_t;

/**
 * Opt3001 conversion time.
 */
typedef enum
{
    OPT_CONV_TIME_100MS = 0,
    OPT_CONV_TIME_800MS = 1
} opt3001_ct_e;

/**
 * Opt3001 conversion mode.
 */
typedef enum
{
    OPT_CONV_MODE_SHUTDOWN   = 0,
    OPT_CONV_MODE_SIGLE      = 1,
    OPT_CONV_MODE_CONTINUOUS = 2
} opt3001_conv_mode_e;

/**
 * Opt3001 driver handle.
 */
typedef struct
{
    uint8_t address;
    opt3001_io_transfer_t read;
    opt3001_io_transfer_t write;
} opt3001_t;

/**
 * Function for initializing opt3001.
 *
 * @param     handle    Opt3001 handle
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool opt3001_init(void *handle);

/**
 * Function for enabling opt3001.
 *
 * @param     handle    Opt3001 handle.
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool opt3001_enable(void *handle);

/**
 * Function for disabling opt3001.
 *
 * @param     handle    Opt3001 handle.
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool opt3001_disable(void *handle);

/**
 * Function for reset opt3001.
 *
 * @param     handle    Opt3001 handle.
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool opt3001_reset(void *handle);

/**
 * Function for setting opt3001 conversion time.
 *
 * @param     handle    Opt3001 handle.
 * @param     ct        Conversion time.
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool opt3001_conversion_time_set(void *handle, opt3001_ct_e ct);

/**
 * Function for setting opt3001 conversion mode.
 *
 * @param     handle    Opt3001 handle.
 * @param     mode      Conversion mode.
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool opt3001_conversion_mode_set(void *handle, opt3001_conv_mode_e mode);

/**
 * Function for getting opt3001 conversion ready status.
 *
 * @param     handle    Opt3001 handle.
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool opt3001_conversion_ready_get(void *handle);

/**
 * Function for reading illuminance.
 *
 * @param     handle     Opt3001 handle.
 * @param     p_value    Illuminance readed.
 *
 * @return              true, operate successfully.
 *                      false, operate unsuccessfully.
 */
bool opt3001_lux_read(void *handle, float* p_value);

#endif
