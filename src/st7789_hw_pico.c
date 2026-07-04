/*
 * src/st7789_hw_pico.c
 *
 * Example HW implementation for Raspberry Pi Pico (Pico SDK).
 *
 * This file implements the hooks declared in src/st7789_hw.h.
 */

#include "st7789_hw.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

#ifndef SPI_PORT
#define SPI_PORT spi0
#endif

#ifndef SPI_SPEED
#define SPI_SPEED ST7789_SPI_SPEED_HZ
#endif

/* ---- Resolve pins from user_setup.h ---- */
#ifndef PIN_SCK
#error "PIN_SCK must be defined in user_setup.h"
#endif
#ifndef PIN_MOSI
#error "PIN_MOSI must be defined in user_setup.h"
#endif

#ifndef LCD_RST_PIN
#error "LCD_RST_PIN must be defined in user_setup.h"
#endif
#ifndef LCD_DC_PIN
#error "LCD_DC_PIN must be defined in user_setup.h"
#endif

#if defined(PIN_CS)
#define ST7789_HAS_CS 1
#else
#define ST7789_HAS_CS 0
#endif

/* Use the same chunk buffer size as core */
static uint8_t st7789_chunk_buf[256];

#if (ST7789_USE_DMA == 1)
static int dma_chan;
static dma_channel_config dma_cfg;
#endif

static inline void st7789_hw_write(const uint8_t *data, size_t len)
{
    spi_write_blocking(SPI_PORT, data, (uint32_t)len);
}

void st7789_hw_delay_ms(uint32_t ms)
{
    sleep_ms(ms);
}

void st7789_hw_reset_begin(void)
{
    gpio_put(LCD_RST_PIN, 0);
}

void st7789_hw_reset_end(void)
{
    gpio_put(LCD_RST_PIN, 1);
}

void st7789_hw_spi_init(void)
{
    /* SPI init */
    spi_init(SPI_PORT, (uint32_t)SPI_SPEED);

    spi_set_format(
        SPI_PORT,
        8,          /* bits per frame */
        SPI_CPOL_1, /* clock idle high */
        SPI_CPHA_1, /* sample on falling edge */
        SPI_MSB_FIRST
    );

    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(LCD_DC_PIN);
    gpio_set_dir(LCD_DC_PIN, GPIO_OUT);

    gpio_init(LCD_RST_PIN);
    gpio_set_dir(LCD_RST_PIN, GPIO_OUT);

#if ST7789_HAS_CS
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1); /* deselect */
#endif

#if (ST7789_USE_DMA == 1)
    dma_chan = dma_claim_unused_channel(true);
    dma_cfg = dma_channel_get_default_config(dma_chan);

    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_8);
    channel_config_set_dreq(&dma_cfg, spi_get_dreq(SPI_PORT, true));
#endif
}

void st7789_hw_write_cmd(uint8_t cmd)
{
#if ST7789_HAS_CS
    gpio_put(PIN_CS, 0);
#endif
    gpio_put(LCD_DC_PIN, 0);
    st7789_hw_write(&cmd, 1);
#if ST7789_HAS_CS
    gpio_put(PIN_CS, 1);
#endif
}

void st7789_hw_write_data(const uint8_t *data, size_t len)
{
#if ST7789_HAS_CS
    gpio_put(PIN_CS, 0);
#endif
    gpio_put(LCD_DC_PIN, 1);
    st7789_hw_write(data, len);
#if ST7789_HAS_CS
    gpio_put(PIN_CS, 1);
#endif
}

#if (ST7789_USE_DMA == 1)
void st7789_hw_write_data_dma(const uint8_t *data, size_t len)
{
#if ST7789_HAS_CS
    gpio_put(PIN_CS, 0);
#endif
    gpio_put(LCD_DC_PIN, 1);

    dma_channel_configure(
        dma_chan,
        &dma_cfg,
        &spi_get_hw(SPI_PORT)->dr,
        data,
        len,
        true
    );

    dma_channel_wait_for_finish_blocking(dma_chan);

#if ST7789_HAS_CS
    gpio_put(PIN_CS, 1);
#endif
}
#endif
