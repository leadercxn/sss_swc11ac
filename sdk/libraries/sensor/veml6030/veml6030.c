/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>

#include "veml6030.h"

static uint16_t m_conf = 0;

bool veml6030_conf_set(veml6030_t * p_veml6030, uint16_t value)
{
    if (!p_veml6030->reg_write(p_veml6030, VEML6030_REG_ALS_CONF, value))
    {
        return false;
    }

    m_conf = value;

    return true;
}

bool veml6030_psm_set(veml6030_t * p_veml6030, uint16_t value)
{
    if (!p_veml6030->reg_write(p_veml6030, VEML6030_REG_POWER_SAVING, value))
    {
        return false;
    }

    return true;
}

bool veml6030_power_on(veml6030_t * p_veml6030)
{
    m_conf &= ~VEML6030_ALS_SD_MASK;

    if (!p_veml6030->reg_write(p_veml6030, VEML6030_REG_ALS_CONF, m_conf))
    {
        return false;
    }

    return true;
}

bool veml6030_power_off(veml6030_t * p_veml6030)
{
    m_conf |= (VEML6030_ALS_SD_POWER_OFF << VEML6030_ALS_SD_POS);

    if (!p_veml6030->reg_write(p_veml6030, VEML6030_REG_ALS_CONF, m_conf))
    {
        return false;
    }

    return true;
}

bool veml6030_als_data_read(veml6030_t * p_veml6030, uint16_t * p_value)
{
    if (!p_veml6030->reg_read(p_veml6030, VEML6030_REG_ALS, p_value))
    {
        return false;
    }
    return true;
}
