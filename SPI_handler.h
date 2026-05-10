#ifndef SPI_HANDLER_H
#define SPI_HANDLER_H

#include <xc.h>
#include <stdint.h>

void spi_init_pins(void);
void spi_init_module(void);

uint8_t spi_write(uint8_t data);
uint8_t spi_read_register(uint8_t addr);
void    spi_write_register(uint8_t addr, uint8_t value);

#endif
