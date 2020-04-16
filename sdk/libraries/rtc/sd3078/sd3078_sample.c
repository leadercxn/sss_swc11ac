/* Copyright (c) 2018 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>

#include "twi_master.h"
#include "sd3078.h"

static sd3078_t m_sd3078;

APP_TIMER_DEF(m_rtc_test_timer);

static bool sd3078_write(sd3078_t * p_sd3078, uint8_t addr, uint8_t * p_data, uint8_t size)
{
    uint8_t buffer[32] = { 0 };

    buffer[0] = addr;
    memcpy(buffer + 1, p_data, size);

    if (!twi_master_transfer(p_sd3078->address, buffer, size + 1, TWI_ISSUE_STOP))
    {
        return false;
    }

    return true;
}

static bool sd3078_read(sd3078_t * p_sd3078, uint8_t addr, uint8_t * p_data, uint8_t size)
{
    uint8_t reg_addr = addr;

    if (!twi_master_transfer(p_sd3078->address, &reg_addr, 1, TWI_DONT_ISSUE_STOP))
    {
        return false;
    }

    if (!twi_master_transfer(p_sd3078->address | TWI_READ_BIT, p_data, size, TWI_ISSUE_STOP))
    {
        return false;
    }

    return true;
}

static void rtc_test_handler(void *p_context)
{
    struct tm tm_time;

    sd3078_rtc_get_time(&m_sd3078, &tm_time);

    trace_info("read time:%d/%d/%d,%d:%d:%d\r\n", tm_time.tm_year, tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
}

static void sd3078_test(void)
{
    twi_master_init(_I2C_SCL_PIN, _I2C_SDA_PIN);

    m_sd3078.address = (SD3078_ADDR << 1);
    m_sd3078.write = sd3078_write;
    m_sd3078.read = sd3078_read;

    sd3078_rtc_init(&m_sd3078);

    sd3078_rtc_set_timestamp(&m_sd3078, 1109635190);

    TIMER_CREATE(&m_rtc_test_timer, APP_TIMER_MODE_REPEATED, rtc_test_handler);
    TIMER_START(m_rtc_test_timer, 1000);
}
