/*
 * st7789_hw.c
 *
 * Template/empty hardware layer.
 * The MCU-specific project should replace/override these functions.
 *
 * Required by: src/st7789.c
 *
 * HOW TO USE:
 * - For a new MCU: copy this file to a new one (e.g. st7789_hw_myplatform.c)
 * - Implement the hooks declared in src/st7789_hw.h
 * - Ensure your build links your platform file (not this template) OR replace
 *   the symbols at link time.
 */

#include "st7789_hw.h"

void st7789_hw_delay_ms(uint32_t ms)
{
    (void)ms;
    /* TODO: implement delay */
}

void st7789_hw_reset_begin(void)
{
    /* TODO: implement reset assert (or begin) */
}

void st7789_hw_reset_end(void)
{
    /* TODO: implement reset deassert (or end) */
}

void st7789_hw_spi_init(void)
{
    /* TODO: implement SPI init + pin mux */
}

void st7789_hw_write_cmd(uint8_t cmd)
{
    (void)cmd;
    /* TODO: set DC=CMD and write 1 byte */
}

void st7789_hw_write_data(const uint8_t *data, size_t len)
{
    (void)data;
    (void)len;
    /* TODO: set DC=DATA and write len bytes */
}

#if (ST7789_USE_DMA == 1)
void st7789_hw_write_data_dma(const uint8_t *data, size_t len)
{
    (void)data;
    (void)len;
    /* TODO: optional DMA bulk transfer; must be blocking */
}
#endif
