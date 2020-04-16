#include "spi.h"
#include "nrf51.h"
#include "nrf51_bitfields.h"
#include "nrf_assert.h"
#include "nrf_gpio.h"

static uint32_t m_spi_ss_pin = 0;

void spi_init(uint32_t ss_pin, uint32_t miso_pin, uint32_t mosi_pin, uint32_t sck_pin)
{
    m_spi_ss_pin = ss_pin;
    
    nrf_gpio_cfg_output(m_spi_ss_pin);
    nrf_gpio_cfg_input(miso_pin, NRF_GPIO_PIN_NOPULL);
    nrf_gpio_cfg_output(mosi_pin);
    nrf_gpio_cfg_output(sck_pin);

    NRF_SPI0->PSELMISO = miso_pin;
    NRF_SPI0->PSELMOSI = mosi_pin;
    NRF_SPI0->PSELSCK  = sck_pin;

    NRF_SPI0->FREQUENCY = SPI_FREQUENCY_FREQUENCY_M8 << SPI_FREQUENCY_FREQUENCY_Pos;
    NRF_SPI0->CONFIG = (uint32_t)(SPI_CONFIG_CPHA_Leading << SPI_CONFIG_CPHA_Pos) |
                       (SPI_CONFIG_CPOL_ActiveHigh << SPI_CONFIG_CPOL_Pos) |
                       (SPI_CONFIG_ORDER_MsbFirst << SPI_CONFIG_ORDER_Pos);
    NRF_SPI0->EVENTS_READY = 0;

    NRF_SPI0->INTENCLR = 1;
    NRF_SPI0->ENABLE = 1;
}

void spi_deinit(void)
{
    NRF_SPI0->ENABLE = 0;
}


uint8_t spi_rw_byte(uint8_t byte)
{
    NRF_SPI0->TXD = byte;
    while(!NRF_SPI0->EVENTS_READY);
    NRF_SPI0->EVENTS_READY = 0;
    return (uint8_t)NRF_SPI0->RXD;
}

void spi_write_buffer(uint8_t addr, uint8_t* buffer, size_t size)
{
    ASSERT(NULL != buffer);

    nrf_gpio_pin_clear(m_spi_ss_pin);
    spi_rw_byte(addr | 0x80);
    for(size_t i = 0; i < size; i++)
    {
        spi_rw_byte(buffer[i]);
    }
    nrf_gpio_pin_set(m_spi_ss_pin);
}

void spi_read_buffer(uint8_t addr, uint8_t* buffer, size_t size)
{
    ASSERT(NULL != buffer);

    nrf_gpio_pin_clear(m_spi_ss_pin);
    spi_rw_byte(addr & 0x7F);
    for(size_t i = 0; i < size; i++)
    {
        buffer[i] = spi_rw_byte(0);
    }
    nrf_gpio_pin_set(m_spi_ss_pin);
}



