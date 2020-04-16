/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "sd3078.h"

#define BCD2BIN(x)  (((x) >> 4) * 10 + ((x) & 0x0f))
#define BIN2BCD(x)	((((x) / 10) << 4) + ((x) % 10))

bool sd3078_rtc_init(sd3078_t * p_sd3078)
{
    uint8_t value = SD3078_CHARGE_DISABLE;

    if (!p_sd3078->write(p_sd3078, SD3078_REG_CHARGE, &value, 1))
    {
        return false;
    }

    value = SD3078_CTR2_ENABLE_VALUE;

    if (!p_sd3078->write(p_sd3078, SD3078_REG_CTR2, &value, 1))
    {
        return false;
    }

    value = SD3078_CTR1_ENABLE_VALUE;

    if (!p_sd3078->write(p_sd3078, SD3078_REG_CTR1, &value, 1))
    {
        return false;
    }

    return true;
}

bool sd3078_rtc_get_time(sd3078_t * p_sd3078, struct tm * p_tm)
{
    uint8_t hour;
    uint8_t rtc_data[NUM_TIME_REGS] = { 0 };

    if (!p_sd3078->read(p_sd3078, SD3078_REG_SECOND, rtc_data, NUM_TIME_REGS))
    {
        return false;
    }

    p_tm->tm_sec = BCD2BIN(rtc_data[SD3078_REG_SECOND] & 0x7F);
    p_tm->tm_min = BCD2BIN(rtc_data[SD3078_REG_MINUTE] & 0x7F);

    /*
     * The sd3078 supports 12/24 hour mode.
     * When getting time,
     * we need to convert the 12 hour mode to the 24 hour mode.
     */
    hour = rtc_data[SD3078_REG_HOUR];
    if (hour & 0x80)            /* 24H MODE */
        p_tm->tm_hour = BCD2BIN(rtc_data[SD3078_REG_HOUR] & 0x3F);
    else if (hour & 0x20)       /* 12H MODE PM */
        p_tm->tm_hour = BCD2BIN(rtc_data[SD3078_REG_HOUR] & 0x1F) + 12;
    else                        /* 12H MODE AM */
        p_tm->tm_hour = BCD2BIN(rtc_data[SD3078_REG_HOUR] & 0x1F);

    p_tm->tm_wday = rtc_data[SD3078_REG_WEEK] & 0x07;
    p_tm->tm_mday = BCD2BIN(rtc_data[SD3078_REG_DAY] & 0x3F);
    p_tm->tm_mon = BCD2BIN(rtc_data[SD3078_REG_MONTH] & 0x1F) - 1;
    p_tm->tm_year = BCD2BIN(rtc_data[SD3078_REG_YEAR]) + 100;

    return true;
}

bool sd3078_rtc_get_timestamp(sd3078_t * p_sd3078, time_t * p_stamp)
{
    struct tm tm_time;

    if (!sd3078_rtc_get_time(p_sd3078, &tm_time))
    {
        return false;
    }

    *p_stamp = mktime(&tm_time);

    return true;
}

bool sd3078_rtc_set_time(sd3078_t * p_sd3078, struct tm * p_tm)
{
    uint8_t rtc_data[NUM_TIME_REGS];

    rtc_data[SD3078_REG_SECOND] = BIN2BCD(p_tm->tm_sec);
    rtc_data[SD3078_REG_MINUTE] = BIN2BCD(p_tm->tm_min);
    rtc_data[SD3078_REG_HOUR] = BIN2BCD(p_tm->tm_hour) | 0x80;
    rtc_data[SD3078_REG_DAY] = BIN2BCD(p_tm->tm_mday);
    rtc_data[SD3078_REG_WEEK] = p_tm->tm_wday & 0x07;
    rtc_data[SD3078_REG_MONTH] = BIN2BCD(p_tm->tm_mon) + 1;
    rtc_data[SD3078_REG_YEAR] = BIN2BCD(p_tm->tm_year - 100);

    if (!p_sd3078->write(p_sd3078, SD3078_REG_SECOND, rtc_data, NUM_TIME_REGS))
    {
        return false;
    }

    return true;
}

bool sd3078_rtc_set_timestamp(sd3078_t * p_sd3078, time_t timestamp)
{
    struct tm *p_tm = localtime(&timestamp);
    return sd3078_rtc_set_time(p_sd3078, p_tm);
}
