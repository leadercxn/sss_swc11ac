//==============================================================================
//    S E N S I R I O N   AG,  Laubisruetistr. 50, CH-8712 Staefa, Switzerland
//==============================================================================
// Project   :  SHT2x Sample Code (V1.2)
// File      :  SHT2x.c
// Author    :  MST
// Controller:  NEC V850/SG3 (uPD70F3740)
// Compiler  :  IAR compiler for V850 (3.50A)
// Brief     :  Sensor layer. Functions for sensor access
//==============================================================================

/* Copyright (c) 2018 SENSORO Co.,Ltd. All Rights Reserved.
 *
 */

//---------- Includes ----------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "sht2x.h"

const uint16_t POLYNOMIAL = 0x131;  //P(x)=x^8+x^5+x^4+1 = 100110001

bool sht2x_init(void *handle)
{
    sht2x_t *p_sht = (sht2x_t *)handle;

    if(p_sht->reset != NULL)
    {
        p_sht->reset(handle);
    }

    // --- Reset sensor by command ---
    if(!SHT2x_SoftReset(handle))
    {
        return false;
    }

    p_sht->delay_ms(15); // wait till sensor has restarted

#if 0
    uint8_t userRegister = 0;

    // --- Set Resolution e.g. RH 10bit, Temp 13bit ---
    if(!SHT2x_ReadUserRegister(handle, &userRegister))   //get actual user reg
    {
        return false;
    }
    userRegister = ((userRegister & ~SHT2x_RES_MASK) | SHT2x_RES_12_14BIT) | 0x02;
    if(!SHT2x_WriteUserRegister(handle, &userRegister))   //write changed user reg
    {
        return false;
    }
#endif
    return true;
}

uint8_t SHT2x_CheckCrc(uint8_t data[], uint8_t nbrOfBytes, uint8_t checksum)
{
    uint8_t crc = 0;
    uint8_t byteCtr;

    //calculates 8-Bit checksum with given polynomial
    for(byteCtr = 0; byteCtr < nbrOfBytes; ++byteCtr)
    {
        crc ^= (data[byteCtr]);
        for(uint8_t bit = 8; bit > 0; --bit)
        {
            if(crc & 0x80)
            {
                crc = (crc << 1) ^ POLYNOMIAL;
            }
            else
            {
                crc = (crc << 1);
            }
        }
    }
    if(crc != checksum)
    {
        return CHECKSUM_ERROR;
    }
    else
    {
        return 0;
    }
}


bool SHT2x_ReadUserRegister(void *handle, uint8_t *pRegisterValue)
{
    sht2x_t *p_sht = (sht2x_t *)handle;

    uint8_t checksum;   //variable for checksum byte
    uint8_t data[2];
    uint8_t cmd = USER_REG_R;

    if(!p_sht->transfer(p_sht->address, &cmd, 1, true))
    {
        return false;
    }

    if(!p_sht->transfer(p_sht->address | 0x01, data, sizeof(data), true))
    {
        return false;
    }

    *pRegisterValue = data[1];
    checksum = data[0];

    if(0 != SHT2x_CheckCrc(pRegisterValue, 1, checksum))
    {
        return false;
    }

    return true;
}


bool SHT2x_WriteUserRegister(void *handle, uint8_t *pRegisterValue)
{
    sht2x_t *p_sht = (sht2x_t *)handle;

    uint8_t cmd[2] = {0};

    cmd[0] = USER_REG_W;
    cmd[1] = *pRegisterValue;

    return p_sht->transfer(p_sht->address, cmd, sizeof(cmd), true);
}

#if 0
uint8_t SHT2x_MeasureHM(etSHT2xMeasureType eSHT2xMeasureType, nt16 *pMeasurand)
{
    uint8_t  checksum;   //checksum
    uint8_t  data[2];    //data array for checksum verification
    uint8_t  error = 0;  //error variable
    uint16_t i;          //counting variable
    uint8_t  cmd[2];

    //-- write I2C sensor address and command --

    I2c_StartCondition();
    if(!I2c_WriteByte(I2C_ADR_W));   // I2C Adr
    switch(eSHT2xMeasureType)
    {
        case HUMIDITY:
            cmd[0] = TRIG_RH_MEASUREMENT_HM;
            if(!twi_master_transfer(m_sht2x_addr, cmd, 1, TWI_ISSUE_STOP));
            break;
        case TEMP    :
            cmd[0] = TRIG_T_MEASUREMENT_HM;
            if(!twi_master_transfer(m_sht2x_addr, cmd, 1, TWI_ISSUE_STOP));
            break;
        default:
            assert(0);
    }
    -- wait until hold master is released --
    if(!twi_master_transfer(m_sht2x_addr, cmd, 1, TWI_DONT_ISSUE_STOP));
    // set SCL I/O port as input
    for(i = 0; i < 1000; i++)     // wait until master hold is released or
    {
        DelayMicroSeconds(1000);    // a timeout (~1s) is reached
        if(SCL_CONF == 1)
        {
            break;
        }
    }
    //-- check for timeout --
    if(SCL_CONF == 0);
    if(!TIME_OUT_ERROR);

    //-- read two data bytes and one checksum byte --
    pMeasurand->s16.u8H = data[0] = I2c_ReadByte(1);
    pMeasurand->s16.u8L = data[1] = I2c_ReadByte(1);
    checksum = I2c_ReadByte(0);

    //-- verify checksum --
    if(!SHT2x_CheckCrc(data, 2, checksum));
    I2c_StopCondition();
    return error;
}
#endif

bool SHT2x_MeasurePoll(void *handle, etSHT2xMeasureType eSHT2xMeasureType, nt16 *pMeasurand)
{
    sht2x_t *p_sht = (sht2x_t *)handle;
    uint8_t  checksum;   //checksum
    uint8_t  data[3];    //data array for checksum verification
    uint16_t i = 0;      //counting variable
    uint8_t  cmd;

    //-- write I2C sensor address and command --
    switch(eSHT2xMeasureType)
    {
        case HUMIDITY:
            cmd = TRIG_RH_MEASUREMENT_POLL;
            if(!p_sht->transfer(p_sht->address, &cmd, 1, true))
            {
                return false;
            }
            break;
        case TEMP:
            cmd = TRIG_T_MEASUREMENT_POLL;
            if(!p_sht->transfer(p_sht->address, &cmd, 1, true))
            {
                return false;
            }
            break;
        default:
            return false;
    }

    // -- poll every 10ms for measurement ready. Timeout afloater 20 retries (200ms)--
    do
    {
        p_sht->delay_ms(10);  //delay 10ms
        if(i++ >= 20)
        {
            break;
        }
    } while(!p_sht->transfer(p_sht->address | 0x01, data, sizeof(data), true));

    if(i >= 20)
    {
        return false;
    }

    //-- read two data bytes and one checksum byte --
    pMeasurand->s16.u8H = data[0] ;
    pMeasurand->s16.u8L = data[1] ;
    checksum = data[2] ;

    //-- verify checksum --
    if(0 != SHT2x_CheckCrc(data, 2, checksum))
    {
        return false;
    }

    return true;
}

bool SHT2x_SoftReset(void *handle)
{
    sht2x_t *p_sht = (sht2x_t *)handle;
    uint8_t cmd = SOFT_RESET;

    if(!p_sht->transfer(p_sht->address, &cmd, 1, true))
    {
        return false;
    }
    return true;
}

float SHT2x_CalcRH(uint16_t u16sRH)
{
    float humidityRH;              // variable for result
    u16sRH &= ~0x0003;          // clear bits [1..0] (status bits)

    //-- calculate relative humidity [%RH] --
    humidityRH = -6.0 + 125.0 / 65536 * (float)u16sRH; // RH= -6 + 125 * SRH/2^16
    return humidityRH;
}

float SHT2x_CalcTemperatureC(uint16_t u16sT)
{
    float temperatureC;            // variable for result
    u16sT &= ~0x0003;           // clear bits [1..0] (status bits)

    //-- calculate temperature [?] --
    temperatureC = -46.85 + 175.72 / 65536 * (float)u16sT; //T= -46.85 + 175.72 * ST/2^16
    return temperatureC;
}

#if 0
bool SHT2x_GetSerialNumber(void *handle, uint8_t u8SerialNumber[])
{
    uint8_t  cmd[2] = {0xFA, 0x0F};
    uint8_t  data[8];

    //Read from memory location 1
    if(!twi_master_transfer(m_sht2x_addr, cmd, 2, TWI_ISSUE_STOP))
    {
        return false;
    }
    if(!twi_master_transfer(m_sht2x_addr, data, 8, TWI_ISSUE_STOP))
    {
        return false;
    }
    u8SerialNumber[5] = data[0]; //Read SNB_3
    //data[1]                    //Read CRC SNB_3 (CRC is not analyzed)

    u8SerialNumber[4] = data[2]; //Read SNB_2
    //data[3]                   //Read CRC SNB_2 (CRC is not analyzed)

    u8SerialNumber[3] = data[4]; //Read SNB_1
    //data[5]                    //Read CRC SNB_1 (CRC is not analyzed)

    u8SerialNumber[2] = data[6]; //Read SNB_0
    //data[7]                  //Read CRC SNB_0 (CRC is not analyzed)

    //Read from memory location 2
    cmd[0] = 0xFC;
    cmd[1] = 0xC9;
    if(!twi_master_transfer(m_sht2x_addr, data, 7, TWI_ISSUE_STOP))
    {
        return false;
    }
    u8SerialNumber[1] = data[1]; //Read SNC_1
    u8SerialNumber[0] = data[2]; //Read SNC_0
    //data[3]                    //Read CRC SNC0/1 (CRC is not analyzed)

    u8SerialNumber[7] = data[4]; //Read SNA_1
    u8SerialNumber[6] = data[5]; //Read SNA_0
    //data[6]                 //Read CRC SNA0/1 (CRC is not analyzed)

    return true;
}
#endif
