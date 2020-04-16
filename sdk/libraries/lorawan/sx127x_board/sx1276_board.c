/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: SX1276 driver specific target board functions implementation

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
 /*******************************************************************************
  * @file    sx1276mb1mas.c
  * @author  MCD Application Team
  * @version V1.0.2
  * @date    27-February-2017
  * @brief   driver sx1276mb1mas board
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
  
/* Includes ------------------------------------------------------------------*/

#include "radio.h"
#include "sx1276.h"
#include "sx1276_board.h"
#include "nrf_drv_gpiote.h" 
#include "app_error.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "spi.h"

static DioIrqHandler *mp_irq_handler;

/*!
 * Flag used to set the RF switch control pins in low power mode when the radio is not active.
 */
static bool RadioIsActive = false;

static void SX1276AntSwInit( void );
  
static void SX1276AntSwDeInit( void );

/*!
 * Radio driver structure initialization
 */
const struct Radio_s Radio =
{
//  SX1276IoInit,
//  SX1276IoDeInit,
  SX1276Init,
  SX1276GetStatus,
  SX1276SetModem,
  SX1276SetChannel,
  SX1276IsChannelFree,
  SX1276Random,
  SX1276SetRxConfig,
  SX1276SetTxConfig,
  SX1276CheckRfFrequency,
  SX1276GetTimeOnAir,
  SX1276Send,
  SX1276SetSleep,
  SX1276SetStby, 
  SX1276SetRx,
  SX1276StartCad,
  SX1276SetTxContinuousWave,
  SX1276ReadRssi,
  SX1276Write,
  SX1276Read,
  SX1276WriteBuffer,
  SX1276ReadBuffer,
  SX1276SetSyncWord,
  SX1276SetMaxPayloadLength,
  SX1276OnTimeoutIrq
};


void SX1276Reset( void )
{
    nrf_gpio_pin_clear(RADIO_RESET);
    DelayMs( 1 );
    nrf_gpio_cfg_input(RADIO_RESET, NRF_GPIO_PIN_NOPULL);
    DelayMs( 6 );
}

void SX1276IoInit( void )
{
    spi_init(_SPIM_SS_PIN, _SPIM_MISO_PIN, _SPIM_MOSI_PIN, _SPIM_SCK_PIN);
    
    nrf_gpio_cfg_output(RADIO_RESET);
    nrf_gpio_cfg_input(RADIO_DIO_0,NRF_GPIO_PIN_PULLDOWN);
    nrf_gpio_cfg_input(RADIO_DIO_1,NRF_GPIO_PIN_PULLDOWN);

#if defined(RADIO_DIO_2)
    nrf_gpio_cfg_input(RADIO_DIO_2,NRF_GPIO_PIN_PULLDOWN);
#endif

#if defined(RADIO_DIO_3)
    nrf_gpio_cfg_input(RADIO_DIO_3,NRF_GPIO_PIN_PULLDOWN);
#endif

#if defined(RADIO_DIO_4)
    nrf_gpio_cfg_input(RADIO_DIO_4,NRF_GPIO_PIN_PULLDOWN);
#endif
    
#if defined(RADIO_DIO_5)
    nrf_gpio_cfg_input(RADIO_DIO_5,NRF_GPIO_PIN_PULLDOWN);
#endif
}

static void dio_gpiote_handler( nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    if(!nrf_gpio_pin_read(pin))
    {
        return;
    }

    if(pin == RADIO_DIO_0)
    {
        mp_irq_handler[0]();
    }
    else if(pin == RADIO_DIO_1)
    {
        mp_irq_handler[1]();
    }
    
#if defined(RADIO_DIO_2)
    else if(pin == RADIO_DIO_2)
    {
        mp_irq_handler[2]();
    }
#endif
    
#if defined(RADIO_DIO_3)    
    else if(pin == RADIO_DIO_3)
    {
        mp_irq_handler[3]();
    }
#endif
    
#if defined(RADIO_DIO_4)    
    else if(pin == RADIO_DIO_4)
    {
        mp_irq_handler[4]();
    }
#endif
    
#if defined(RADIO_DIO_5)    
    else if(pin == RADIO_DIO_5)
    {
        mp_irq_handler[5]();
    }
#endif
    else
    {
        
    }
}

void SX1276IoIrqInit( DioIrqHandler *irqHandlers )
{
    uint32_t err_code;
        
    mp_irq_handler = irqHandlers;

    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
    
    err_code = nrf_drv_gpiote_in_init(RADIO_DIO_0, &in_config, dio_gpiote_handler);
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_gpiote_in_event_enable(RADIO_DIO_0, true);
    
    err_code = nrf_drv_gpiote_in_init(RADIO_DIO_1, &in_config, dio_gpiote_handler);
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_gpiote_in_event_enable(RADIO_DIO_1, true);
    
#if defined(RADIO_DIO_2)
    err_code = nrf_drv_gpiote_in_init(RADIO_DIO_2, &in_config, dio_gpiote_handler);
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_gpiote_in_event_enable(RADIO_DIO_2, true);
#endif
    
#if defined(RADIO_DIO_3)
    err_code = nrf_drv_gpiote_in_init(RADIO_DIO_3, &in_config, dio_gpiote_handler);
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_gpiote_in_event_enable(RADIO_DIO_3, true);
#endif

#if defined(RADIO_DIO_4)
    err_code = nrf_drv_gpiote_in_init(RADIO_DIO_4, &in_config, dio_gpiote_handler);
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_gpiote_in_event_enable(RADIO_DIO_4, true);
#endif

#if defined(RADIO_DIO_5)
    err_code = nrf_drv_gpiote_in_init(RADIO_DIO_5, &in_config, dio_gpiote_handler);
    APP_ERROR_CHECK(err_code);
    
    nrf_drv_gpiote_in_event_enable(RADIO_DIO_5, true);
#endif
}


void SX1276IoDeInit( void )
{
    nrf_gpio_cfg_default(RADIO_RESET);
    nrf_gpio_cfg_default(RADIO_DIO_0);
    nrf_gpio_cfg_default(RADIO_DIO_1);
    
#if defined(RADIO_DIO_2)
    nrf_gpio_cfg_default(RADIO_DIO_2);
#endif
    
#if defined(RADIO_DIO_3)
    nrf_gpio_cfg_default(RADIO_DIO_3);
#endif
    
#if defined(RADIO_DIO_4)
    nrf_gpio_cfg_default(RADIO_DIO_4);
#endif
    
#if defined(RADIO_DIO_5)    
    nrf_gpio_cfg_default(RADIO_DIO_5);
#endif    
}

void SX1276SetRfTxPower( int8_t power )
{
    uint8_t paConfig = 0;
    uint8_t paDac = 0;

    paConfig = SX1276Read( REG_PACONFIG );
    paDac = SX1276Read( REG_PADAC );

    paConfig = ( paConfig & RF_PACONFIG_PASELECT_MASK ) | SX1276GetPaSelect( SX1276.Settings.Channel );
    paConfig = ( paConfig & RF_PACONFIG_MAX_POWER_MASK ) | 0x70;

    if( ( paConfig & RF_PACONFIG_PASELECT_PABOOST ) == RF_PACONFIG_PASELECT_PABOOST )
    {
        if( power > 17 )
        {
            paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_ON;
        }
        else
        {
            paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_OFF;
        }
        if( ( paDac & RF_PADAC_20DBM_ON ) == RF_PADAC_20DBM_ON )
        {
            if( power < 5 )
            {
                power = 5;
            }
            if( power > 20 )
            {
                power = 20;
            }
            paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 5 ) & 0x0F );
        }
        else
        {
            if( power < 2 )
            {
                power = 2;
            }
            if( power > 17 )
            {
                power = 17;
            }
            paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 2 ) & 0x0F );
        }
    }
    else
    {
        if( power < -1 )
        {
            power = -1;
        }
        if( power > 14 )
        {
            power = 14;
        }
        paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power + 1 ) & 0x0F );
    }
    SX1276Write( REG_PACONFIG, paConfig );
    SX1276Write( REG_PADAC, paDac );
}

uint8_t SX1276GetPaSelect( uint32_t channel )
{
//    return RF_PACONFIG_PASELECT_RFO;
    return RF_PACONFIG_PASELECT_PABOOST;
}

void SX1276SetAntSwLowPower( bool status )
{
  if( RadioIsActive != status )
  {
    RadioIsActive = status;
    
    if( status == false )
    {
      SX1276AntSwInit( );
    }
    else
    {
      SX1276AntSwDeInit( );
    }
  }
}

static void SX1276AntSwInit( void )
{
    nrf_gpio_cfg_output(RADIO_ANT_TX);
    nrf_gpio_cfg_output(RADIO_ANT_RX);
    
    nrf_gpio_pin_set(RADIO_ANT_TX);
    nrf_gpio_pin_clear(RADIO_ANT_RX); 
}

static void SX1276AntSwDeInit( void )
{
    nrf_gpio_cfg_default(RADIO_ANT_TX);
    nrf_gpio_cfg_default(RADIO_ANT_RX);  
}

void SX1276SetAntSw( uint8_t opMode )
{
    switch( opMode )
    {
    case RFLR_OPMODE_TRANSMITTER:
        nrf_gpio_pin_clear(RADIO_ANT_TX);
        nrf_gpio_pin_set(RADIO_ANT_RX); 
    break;
    case RFLR_OPMODE_RECEIVER:
    case RFLR_OPMODE_RECEIVER_SINGLE:
    case RFLR_OPMODE_CAD:
    default:
        nrf_gpio_pin_clear(RADIO_ANT_RX);
        nrf_gpio_pin_set(RADIO_ANT_TX); 
        break;
    }
}

bool SX1276CheckRfFrequency( uint32_t frequency )
{
    // Implement check. Currently all frequencies are supported
    return true;
}

void SX1276WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    spi_write_buffer(addr, buffer, size);
}

void SX1276ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    spi_read_buffer(addr, buffer, size);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
