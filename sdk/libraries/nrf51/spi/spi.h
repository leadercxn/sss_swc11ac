#ifndef SPI_H
#define SPI_H

#include <stdio.h>
#include <stdint.h>

void spi_init(uint32_t ss_pin, uint32_t miso_pin, uint32_t mosi_pin, uint32_t sck_pin);

void spi_deinit(void);

uint8_t spi_rw_byte(uint8_t byte);

void spi_write_buffer(uint8_t addr, uint8_t* buffer, size_t size);

void spi_read_buffer(uint8_t addr, uint8_t* buffer, size_t size);

#endif
