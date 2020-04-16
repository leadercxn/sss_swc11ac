/* Copyright (c) 2018 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>

#include "opt3001.h"

/**
 * Function for writing opt3001 register.
 *
 * @param     handle    Opt3001 handle.
 * @param     reg       Register to write.
 * @param     value     Value to write.
 *
 * @return              true, write successfully.
 *                      false, write unsuccessfully.
 */
static bool opt3001_reg_write(void *handle, uint8_t reg, uint16_t value)
{
    opt3001_t *p_opt = (opt3001_t *)handle;
    uint8_t buff[OPT3001_REG_LENGTH];

    buff[0] = value >> 8;
    buff[1] = value & 0xff;

    return p_opt->write(handle, reg, buff, OPT3001_REG_LENGTH);
}

/**
 * Function for reading opt3001 register.
 *
 * @param     handle    Opt3001 handle.
 * @param     reg       Register to read.
 * @param     p_value   Value read from reg.
 *
 * @return              true, read successfully.
 *                      false, read unsuccessfully.
 */
static bool opt3001_reg_read(void *handle, uint8_t reg, uint16_t *p_value)
{
    opt3001_t *p_opt = (opt3001_t *)handle;
    uint8_t buff[OPT3001_REG_LENGTH];

    if(!p_opt->read(handle, reg, buff, OPT3001_REG_LENGTH))
    {
        return false;
    }
    *p_value = (buff[0] << 8) | buff[1];
    return true;
}

bool opt3001_init(void *handle)
{
    return true;
}

bool opt3001_config_write(void *handle, opt3001_config_t config)
{
    return opt3001_reg_write(handle, OPT3001_REG_CONFIG, config.value);
}

bool opt3001_config_read(void *handle, opt3001_config_t *config)
{
    return opt3001_reg_read(handle, OPT3001_REG_CONFIG, &config->value);
}

bool opt3001_conversion_time_set(void *handle, opt3001_ct_e ct)
{
    opt3001_config_t config;
    if(!opt3001_reg_read(handle, OPT3001_REG_CONFIG, &config.value))
    {
        return false;
    }
    config.ConvertionTime = ct;
    if(!opt3001_reg_write(handle, OPT3001_REG_CONFIG, config.value))
    {
        return false;
    }
    return true;
}

bool opt3001_conversion_mode_set(void *handle, opt3001_conv_mode_e mode)
{
    opt3001_config_t config;
    if(!opt3001_reg_read(handle, OPT3001_REG_CONFIG, &config.value))
    {
        return false;
    }
    config.ModeOfConversionOperation = mode;
    if(!opt3001_reg_write(handle, OPT3001_REG_CONFIG, config.value))
    {
        return false;
    }
    return true;
}

bool opt3001_enable(void *handle)
{
    return opt3001_reg_write(handle, OPT3001_REG_CONFIG, OPT3001_VAL_CONFIG_ENABLE);
}

bool opt3001_disable(void *handle)
{
    return opt3001_reg_write(handle, OPT3001_REG_CONFIG, OPT3001_VAL_CONFIG_DISABLE);
}

bool opt3001_reset(void *handle)
{
    return opt3001_reg_write(handle, OPT3001_REG_CONFIG, OPT3001_VAL_CONFIG_RESET);
}

bool opt3001_conversion_ready_get(void *handle)
{
    opt3001_config_t config;
    if(!opt3001_reg_read(handle, OPT3001_REG_CONFIG, &config.value))
    {
        return false;
    }
    if(config.ConversionReady != 1)
    {
        return false;
    }

    return true;
}

bool opt3001_raw_read(void *handle, uint16_t *raw_data)
{
    if(!opt3001_reg_read(handle, OPT3001_REG_RESULT, raw_data))
    {
        return false;
    }
    return true;
}

#if 0
#include <math.h>
float opt3001_convert(uint16_t raw_data)
{
    uint16_t e, m;

    m = raw_data & 0x0FFF;
    e = (raw_data & 0xF000) >> 12;

    return m * (0.01 * exp2(e));
}
#endif

static float opt3001_convert(uint16_t raw_data)
{
    uint16_t e, m;
    uint32_t exp2_e = 1;

    m = raw_data & 0x0FFF;
    e = (raw_data & 0xF000) >> 12;

    for(uint16_t i = 0; i < e; i++)
    {
        exp2_e = exp2_e * 2;
    }

    return m * (0.01 * exp2_e);
}

bool opt3001_lux_read(void *handle, float *p_value)
{
    uint16_t raw = 0;

    if(!opt3001_raw_read(handle, &raw))
    {
        return false;
    }

    *p_value = opt3001_convert(raw);

    return true;
}
