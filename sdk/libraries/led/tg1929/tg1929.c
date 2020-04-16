/* Copyright (c) 2019 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "tg1929.h"

bool tg1929_reg_write(tg1929_t * p_tg1929, uint8_t addr, uint8_t value)
{
    return p_tg1929->write(p_tg1929, addr, &value, 1);
}

bool tg1929_init(tg1929_t * p_tg1929)
{
    if (p_tg1929 == NULL || p_tg1929->reset == NULL)
    {
        return false;
    }
    p_tg1929->reset(p_tg1929);
    return true;
}

bool tg1929_sw_reset(tg1929_t * p_tg1929)
{
    if (p_tg1929 == NULL || p_tg1929->write == NULL)
    {
        return false;
    }

    if (!tg1929_reg_write(p_tg1929, TG1929_REG_RSTCTR, TG1929_POWER_OFF))
    {
        return false;
    }

    if (!tg1929_reg_write(p_tg1929, TG1929_REG_RSTCTR, TG1929_POWER_ON))
    {
        return false;
    }
    return true;
}

bool tg1929_fixbrit_set(tg1929_t * p_tg1929, uint8_t led, uint8_t value)
{
    if (p_tg1929 == NULL || p_tg1929->write == NULL)
    {
        return false;
    }

    if ((led > TG1929_REG_FIXBRIT_LED17) || (led < TG1929_REG_FIXBRIT_LED0) || (value > TG1929_FIXBRIT_MAX))
    {
        return false;
    }

    if (!tg1929_reg_write(p_tg1929, led, value))
    {
        return false;
    }
    return true;
}

bool tg1929_rgb_set(tg1929_t * p_tg1929, uint8_t rgb_led, uint8_t r, uint8_t g, uint8_t b)
{
    if (p_tg1929 == NULL || p_tg1929->write == NULL)
    {
        return false;
    }

    if (rgb_led > TG1929_RGB_MAX)
    {
        return false;
    }

    if (!tg1929_reg_write(p_tg1929, TG1929_REG_FIXBRIT_LED0 + rgb_led * 3, g))  // g
    {
        return false;
    }

    if (!tg1929_reg_write(p_tg1929, TG1929_REG_FIXBRIT_LED1 + rgb_led * 3, b))  // b
    {
        return false;
    }

    if (!tg1929_reg_write(p_tg1929, TG1929_REG_FIXBRIT_LED2 + rgb_led * 3, r))  // r
    {
        return false;
    }

    return true;
}

bool tg1929_rgb_oe_set(tg1929_t * p_tg1929, uint8_t value)
{
    if (p_tg1929 == NULL || p_tg1929->write == NULL)
    {
        return false;
    }

    if (!tg1929_reg_write(p_tg1929, TG1929_REG_RGB_OE, value))
    {
        return false;
    }

    return true;
}
