/* Copyright (c) 2018 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "sht3xd.h"

static uint8_t calculateCrc(uint8_t data[])
{
    uint8_t bit;
    uint8_t crc = 0xFF;
    uint8_t dataCounter = 0;
    for(; dataCounter < 2; dataCounter++)
    {
        crc ^= (data[dataCounter]);
        for(bit = 8; bit > 0; --bit)
        {
            if(crc & 0x80)
            {
                crc = (crc << 1) ^ 0x131;
            }
            else
            {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

static uint8_t checkCrc(uint8_t data[], uint8_t checksum)
{
    return calculateCrc(data) != checksum;
}

static float sht3xd_calTemp(uint16_t rawValue)
{
    return 175.0f * (float)rawValue / 65535.0f - 45.0f;
}

static float sht3xd_calHumi(uint16_t rawValue)
{
    return 100.0f * rawValue / 65535.0f;
}

static uint16_t sht3xd_calRawTemp(float value)
{
    return (value + 45.0f) / 175.0f * 65535.0f;
}

static uint16_t sht3xd_calRawHumi(float value)
{
    return value / 100.0f * 65535.0f;
}

static bool writeCommand(void *handle, sht3xd_command_e command)
{
    sht3xd_t *p_sht = (sht3xd_t *)handle;

    return p_sht->write(handle, command, NULL, 0);
}

static bool sht3xd_readTempHumi(void *handle, sht3xd_command_e command, float *p_temp, float *p_rh)
{
    sht3xd_t *p_sht = (sht3xd_t *)handle;
    uint8_t rx_buff[6];
    uint16_t temp_raw = 0;
    uint16_t humi_raw = 0;

    if(!p_sht->read(handle, command, rx_buff, sizeof(rx_buff)))
    {
        return false;
    }

    if((checkCrc(rx_buff, rx_buff[2])     != 0) ||
       (checkCrc(&rx_buff[3], rx_buff[5]) != 0))
    {
        return false;
    }

    temp_raw = (rx_buff[0] << 8) | rx_buff[1];
    humi_raw = (rx_buff[3] << 8) | rx_buff[4];
    *p_temp = sht3xd_calTemp(temp_raw);
    *p_rh   = sht3xd_calHumi(humi_raw);

    return true;
}

bool sht3xd_init(void *handle)
{
    sht3xd_t *p_sht = (sht3xd_t *)handle;

    p_sht->reset(handle);

    return sht3xd_clearAll(handle);
}

bool sht3xd_periodicFetchData(void *handle, float *p_temp, float *p_rh)
{
    return sht3xd_readTempHumi(handle, CMD_FETCH_DATA, p_temp, p_rh);
}

bool sht3xd_periodicStop(void *handle)
{
    return writeCommand(handle, CMD_STOP_PERIODIC);
}

bool sht3xd_periodicStart(void *handle, sht3xd_repeatability_e repeatability, sht3xd_frequency_e frequency)
{
    bool status = true;
    switch(repeatability)
    {
        case REPEATABILITY_LOW:
            switch(frequency)
            {
                case FREQUENCY_HZ5:
                    status = writeCommand(handle, CMD_PERIODIC_HALF_L);
                    break;
                case FREQUENCY_1HZ:
                    status = writeCommand(handle, CMD_PERIODIC_1_L);
                    break;
                case FREQUENCY_2HZ:
                    status = writeCommand(handle, CMD_PERIODIC_2_L);
                    break;
                case FREQUENCY_4HZ:
                    status = writeCommand(handle, CMD_PERIODIC_4_L);
                    break;
                case FREQUENCY_10HZ:
                    status = writeCommand(handle, CMD_PERIODIC_10_L);
                    break;
                default:
                    status = false;
                    break;
            }
            break;
        case REPEATABILITY_MEDIUM:
            switch(frequency)
            {
                case FREQUENCY_HZ5:
                    status = writeCommand(handle, CMD_PERIODIC_HALF_M);
                    break;
                case FREQUENCY_1HZ:
                    status = writeCommand(handle, CMD_PERIODIC_1_M);
                    break;
                case FREQUENCY_2HZ:
                    status = writeCommand(handle, CMD_PERIODIC_2_M);
                    break;
                case FREQUENCY_4HZ:
                    status = writeCommand(handle, CMD_PERIODIC_4_M);
                    break;
                case FREQUENCY_10HZ:
                    status = writeCommand(handle, CMD_PERIODIC_10_M);
                    break;
                default:
                    status = false;
                    break;
            }
            break;
        case REPEATABILITY_HIGH:
            switch(frequency)
            {
                case FREQUENCY_HZ5:
                    status = writeCommand(handle, CMD_PERIODIC_HALF_H);
                    break;
                case FREQUENCY_1HZ:
                    status = writeCommand(handle, CMD_PERIODIC_1_H);
                    break;
                case FREQUENCY_2HZ:
                    status = writeCommand(handle, CMD_PERIODIC_2_H);
                    break;
                case FREQUENCY_4HZ:
                    status = writeCommand(handle, CMD_PERIODIC_4_H);
                    break;
                case FREQUENCY_10HZ:
                    status = writeCommand(handle, CMD_PERIODIC_10_H);
                    break;
                default:
                    status = SHT3XD_ERR_INVALID_FREQUENCY;
                    break;
            }
            break;
        default:
            status = false;
            break;
    }
    return status;
}

bool sht3xd_readTempHumiClockStretch(void *handle, sht3xd_repeatability_e repeatability, float *p_temp, float *p_rh)
{
    sht3xd_command_e command;
    switch(repeatability)
    {
        case REPEATABILITY_LOW:
            command = CMD_CLOCK_STRETCH_L;
            break;
        case REPEATABILITY_MEDIUM:
            command = CMD_CLOCK_STRETCH_M;
            break;
        case REPEATABILITY_HIGH:
            command = CMD_CLOCK_STRETCH_H;
            break;
        default:
            return false;
    }
    return sht3xd_readTempHumi(handle, command, p_temp, p_rh);
}

/*Note timeout is not used*/
bool sht3xd_readTempHumiPolling(void *handle, sht3xd_repeatability_e repeatability, uint8_t timeout, float *p_temp, float *p_rh)
{
    sht3xd_command_e command;
    switch(repeatability)
    {
        case REPEATABILITY_LOW:
            command = CMD_POLLING_L;
            break;
        case REPEATABILITY_MEDIUM:
            command = CMD_POLLING_M;
            break;
        case REPEATABILITY_HIGH:
            command = CMD_POLLING_H;
            break;
        default:
            return false;
    }
    return sht3xd_readTempHumi(handle, command, p_temp, p_rh);
}

bool sht3xd_softReset(void *handle)
{
    return writeCommand(handle, CMD_SOFT_RESET);
}

bool sht3xd_generalCallReset(void *handle)
{
    return writeCommand(handle, CMD_CALL_RESET);
}

bool sht3xd_heaterEnable(void *handle)
{
    return writeCommand(handle, CMD_HEATER_ENABLE);
}

bool sht3xd_heaterDisable(void *handle)
{
    return writeCommand(handle, CMD_HEATER_DISABLE);
}

bool sht3xd_artEnable(void *handle)
{
    return writeCommand(handle, CMD_ART);
}

bool sht3xd_readSerialNumber(void *handle, uint32_t *p_serial)
{
    sht3xd_t *p_sht = (sht3xd_t *)handle;
    uint8_t rx_buff[6];

    if(!p_sht->read(handle, CMD_READ_SERIAL_NUMBER, rx_buff, sizeof(rx_buff)))
    {
        return false;
    }

    if((checkCrc(rx_buff, rx_buff[2])     != 0) ||
       (checkCrc(&rx_buff[3], rx_buff[5]) != 0))
    {
        return false;
    }

    *p_serial = (rx_buff[0] << 24) | (rx_buff[1] << 16) | (rx_buff[3] << 8) | rx_buff[4];

    return true;
}

bool sht3xd_readStatusRegister(void *handle, sht3xd_reg_status_t *p_status)
{
    sht3xd_t *p_sht = (sht3xd_t *)handle;
    uint8_t rx_buff[3];

    if(!p_sht->read(handle, CMD_READ_STATUS, rx_buff, sizeof(rx_buff)))
    {
        return false;
    }

    if(checkCrc(rx_buff, rx_buff[2]) != 0)
    {
        return false;
    }

    p_status->rawData = (rx_buff[0] << 8) | rx_buff[1];

    return true;
}

bool sht3xd_clearAll(void *handle)
{
    return writeCommand(handle, CMD_CLEAR_STATUS);
}

static bool sht3xd_readAlertData(void *handle, sht3xd_command_e command, float *p_temp, float *p_rh)
{
    sht3xd_t *p_sht = (sht3xd_t *)handle;
    uint8_t rx_buff[3];
    uint16_t raw = 0;

    if(!p_sht->read(handle, command, rx_buff, sizeof(rx_buff)))
    {
        return false;
    }

    if(checkCrc(rx_buff, rx_buff[2]) != 0)
    {
        return false;
    }

    raw = (rx_buff[0] << 8) | rx_buff[1];
    *p_rh = sht3xd_calHumi(raw << 7);
    *p_temp = sht3xd_calTemp(raw & 0xFE00);

    return true;
}

bool sht3xd_readAlertHighSet(void *handle, float *p_temp, float *p_rh)
{
    return sht3xd_readAlertData(handle, CMD_READ_ALR_LIMIT_HS, p_temp, p_rh);
}

bool sht3xd_readAlertHighClear(void *handle, float *p_temp, float *p_rh)
{
    return sht3xd_readAlertData(handle, CMD_READ_ALR_LIMIT_HC, p_temp, p_rh);
}

bool sht3xd_readAlertLowSet(void *handle, float *p_temp, float *p_rh)
{
    return sht3xd_readAlertData(handle, CMD_READ_ALR_LIMIT_LS, p_temp, p_rh);
}

bool sht3xd_readAlertLowClear(void *handle, float *p_temp, float *p_rh)
{
    return sht3xd_readAlertData(handle, CMD_READ_ALR_LIMIT_LC, p_temp, p_rh);
}

static bool sht3xd_writeAlertData(void *handle, sht3xd_command_e command, float temperature, float humidity)
{
    sht3xd_t *p_sht = (sht3xd_t *)handle;

    if((humidity < 0.0) || (humidity > 100.0) || (temperature < -40.0) || (temperature > 125.0))
    {
        return false;
    }
    else
    {
        uint16_t rawTemperature = sht3xd_calRawTemp(temperature);
        uint16_t rawHumidity = sht3xd_calRawHumi(humidity);
        uint16_t data = (rawHumidity & 0xFE00) | ((rawTemperature >> 7) & 0x001FF);
        uint8_t buff[3];

        buff[0] = data >> 8;
        buff[1] = data & 0xFF;
        buff[2] = calculateCrc(buff);

        return p_sht->write(handle, command, buff, sizeof(buff));
    }
}

bool sht3xd_writeAlertHigh(void *handle, float temperatureSet, float temperatureClear, float humiditySet, float humidityClear)
{
    bool status = true;
    status = sht3xd_writeAlertData(handle, CMD_WRITE_ALR_LIMIT_HS, temperatureSet, humiditySet);
    if(true == status)
    {
        status = sht3xd_writeAlertData(handle, CMD_WRITE_ALR_LIMIT_HC, temperatureClear, humidityClear);
    }
    return status;
}

bool sht3xd_writeAlertLow(void *handle, float temperatureClear, float temperatureSet, float humidityClear, float humiditySet)
{
    bool status = true;
    status = sht3xd_writeAlertData(handle, CMD_WRITE_ALR_LIMIT_LS, temperatureSet, humiditySet);
    if(true == status)
    {
        status = sht3xd_writeAlertData(handle, CMD_WRITE_ALR_LIMIT_LC, temperatureClear, humidityClear);
    }
    return status;
}
