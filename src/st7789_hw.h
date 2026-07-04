#ifndef ST7789_HW_H
#define ST7789_HW_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "../user_setup.h"

/*
 * Hardware abstraction hooks.
 * The MCU-specific implementation must provide these functions.
 *
 * Notes:
 * - The core driver uses 16-bit color (RGB565).
 * - "DC" distinction: command vs data. The easiest HW layer may provide
 *   st7789_hw_write_cmd() and st7789_hw_write_data().
 */

/* ----------- Core-required hooks ----------- */

void st7789_hw_delay_ms(uint32_t ms);
void st7789_hw_reset_begin(void);
void st7789_hw_reset_end(void);

/* Optional CS support is expected to be implemented by HW layer if used. */
void st7789_hw_spi_init(void);
void st7789_hw_write_cmd(uint8_t cmd);
void st7789_hw_write_data(const uint8_t *data, size_t len);

/* Optional DMA bulk write: if ST7789_USE_DMA==1, implement it. */
#if (ST7789_USE_DMA == 1)
void st7789_hw_write_data_dma(const uint8_t *data, size_t len);
#endif

/* Optional: implement if you want core to call a centralized rotation mapping
 * from the HW layer. For now core handles MADCTL values directly, so not required.
 */

#endif /* ST7789_HW_H */
