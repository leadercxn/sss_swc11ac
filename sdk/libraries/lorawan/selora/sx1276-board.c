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
#include <stdint.h>
#include <string.h>
#include "boards.h"
#include "sx1276Regs-Fsk.h"
#include "sx1276Regs-LoRa.h"
#include "radio.h"
#include "sx1276.h"
#include "sx1276-board.h"
#include "nrf_gpio.h"
#include "app_error.h"
#include "spi.h"
#include "nrf_drv_gpiote.h" 
#include "nrf_delay.h"

/*!
 * Flag used to set the RF switch control pins in low power mode when the radio is not active.
 */
static bool RadioIsActive = false;

static DioIrqHandler *mp_irq_handler;

/*!
 * Radio driver structure initialization
 */
const struct Radio_s Radio =
{
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
    SX1276ReadRssi,
    SX1276OnTimeout,
    SX1276Write,
    SX1276Read,
};


void SX1276IoInit( void )
{
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

void SX1276Reset( void )
{
    nrf_gpio_pin_clear(RADIO_RESET);
    DelayMs( 1 );
    nrf_gpio_cfg_input(RADIO_RESET, NRF_GPIO_PIN_NOPULL);
    DelayMs( 6 );
}

static void dio_gpiote_handler( nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
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

    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(false);
    
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

uint8_t SX1276GetPaSelect( uint32_t channel )
{
    if( channel < RF_MID_BAND_THRESH )
    {
        return RF_PACONFIG_PASELECT_PABOOST;
    }
    else
    {
//        return RF_PACONFIG_PASELECT_RFO;
        return RF_PACONFIG_PASELECT_PABOOST;
    }
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

void SX1276AntSwInit( void )
{
    nrf_gpio_cfg_output(RADIO_ANT_TX);
    nrf_gpio_cfg_output(RADIO_ANT_RX);
    
    nrf_gpio_pin_clear(RADIO_ANT_RX);
    nrf_gpio_pin_set(RADIO_ANT_TX); 
        
}

void SX1276AntSwDeInit( void )
{
    nrf_gpio_cfg_default(RADIO_ANT_TX);
    nrf_gpio_cfg_default(RADIO_ANT_RX);     
}

void SX1276SetAntSw( uint8_t rxTx )
{
    if( SX1276.RxTx == rxTx )
    {
        return;
    }
    SX1276.RxTx = rxTx;
    if( rxTx != 0 ) // 1: TX, 0: RX
    {
        nrf_gpio_pin_clear(RADIO_ANT_TX);
        nrf_gpio_pin_set(RADIO_ANT_RX);    
    }
    else
    {
        nrf_gpio_pin_clear(RADIO_ANT_RX); 
        nrf_gpio_pin_set(RADIO_ANT_TX);
    }
}

bool SX1276CheckRfFrequency( uint32_t frequency )
{
    // Implement check. Currently all frequencies are supported
    return true;
}


