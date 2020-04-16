/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#ifndef __SD3078_H__
#define __SD3078_H__

#define SD3078_ADDR 					(0x32)

#define SD3078_REG_SECOND               (0x00)
#define SD3078_REG_MINUTE               (0x01)
#define SD3078_REG_HOUR                 (0x02)
#define SD3078_REG_WEEK                 (0x03)
#define SD3078_REG_DAY                  (0x04)
#define SD3078_REG_MONTH                (0x05)
#define SD3078_REG_YEAR                 (0x06)
#define SD3078_REG_SECOND_ALARM         (0x07)
#define SD3078_REG_MINUTE_ALARM         (0x08)
#define SD3078_REG_HOUR_ALARM           (0x09)
#define SD3078_REG_WEEK_ALARM           (0x0A)
#define SD3078_REG_DAY_ALARM            (0x0B)
#define SD3078_REG_MONTH_ALARM          (0x0C)
#define SD3078_REG_YEAR_ALARM           (0x0D)
#define SD3078_REG_ALARM_ALLOWED        (0x0E)
#define SD3078_REG_CTR1                 (0x0F)
#define SD3078_REG_CTR2                 (0x10)
#define SD3078_REG_CTR3                 (0x11)
#define SD3078_REG_25TTF                (0x12)
#define SD3078_REG_COUNTDOWN1           (0x13)
#define SD3078_REG_COUNTDOWN2           (0x14)
#define SD3078_REG_COUNTDOWN4           (0x15)
#define SD3078_REG_AGTC                 (0x17)
#define SD3078_REG_CHARGE               (0x18)
#define SD3078_REG_CTR4                 (0x19)
#define SD3078_REG_CTR5                 (0x1A)
#define SD3078_REG_BAT_VAL              (0x1B)
#define SD3078_REG_USER_RAM             (0x2C)
#define SD3078_REG_ID                   (0x73)

#define SD3078_CTR1_ENABLE_VALUE		(0x84)
#define SD3078_CTR2_ENABLE_VALUE		(0x80)
#define SD3078_CTR_DISABLE_VALUE		(0x00)
#define SD3078_CHARGE_DISABLE			(0x00)

#define NUM_TIME_REGS					(7)

typedef struct sd3078_s sd3078_t;

struct sd3078_s
{
    uint8_t address;
     bool(*write) (sd3078_t * p_sd3078, uint8_t addr, uint8_t * p_data, uint8_t sizes);
     bool(*read) (sd3078_t * p_sd3078, uint8_t addr, uint8_t * p_data, uint8_t sizes);
};

/**
 * [sd3078_rtc_init description]
 *
 * @param     p_sd3078    [description]
 *
 * @return                [description]
 */
bool sd3078_rtc_init(sd3078_t * p_sd3078);

/**
 * [sd3078_rtc_read_time description]
 *
 * @param     p_sd3078    [description]
 * @param     p_tm        [description]
 *
 * @return                [description]
 */
bool sd3078_rtc_get_time(sd3078_t * p_sd3078, struct tm *p_tm);

/**
 * [sd3078_rtc_get_timestamp description]
 *
 * @param     p_sd3078    [description]
 * @param     p_stamp     [description]
 *
 * @return                [description]
 */
bool sd3078_rtc_get_timestamp(sd3078_t * p_sd3078, time_t * p_stamp);

/**
 * [sd3078_rtc_set_time description]
 *
 * @param     p_sd3078    [description]
 * @param     tm          [description]
 *
 * @return                [description]
 */
bool sd3078_rtc_set_time(sd3078_t * p_sd3078, struct tm *p_tm);

/**
 * [sd3078_rtc_set_timestamp description]
 *
 * @param     p_sd3078     [description]
 * @param     timestamp    [description]
 *
 * @return                 [description]
 */
bool sd3078_rtc_set_timestamp(sd3078_t * p_sd3078, uint32_t timestamp);

#endif
