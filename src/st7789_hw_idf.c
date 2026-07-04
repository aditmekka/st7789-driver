/*
 * src/st7789_hw_idf.c
 *
 * Example HW implementation for ESP-IDF (ESP32-C3).
 *
 * Implements the hooks declared in src/st7789_hw.h:
 * - st7789_hw_delay_ms
 * - st7789_hw_reset_begin / st7789_hw_reset_end
 * - st7789_hw_spi_init
 * - st7789_hw_write_cmd / st7789_hw_write_data
 * - optional st7789_hw_write_data_dma (NOT implemented by default; you can extend)
 */

#include "st7789_hw.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../user_setup.h"

/* ---- user_setup.h required pins ---- */
#ifndef PIN_SCK
#error "PIN_SCK must be defined in user_setup.h"
#endif
#ifndef PIN_MOSI
#error "PIN_MOSI must be defined in user_setup.h"
#endif
#ifndef LCD_DC_PIN
#error "LCD_DC_PIN must be defined in user_setup.h"
#endif
#ifndef LCD_RST_PIN
#error "LCD_RST_PIN must be defined in user_setup.h"
#endif

/* CS is optional; if PIN_CS is provided it will be used. */
#ifndef PIN_CS
#define PIN_CS (-1)
#endif

#define ST7789_TAG "st7789_idf"

/* spi host + dma channel are ESP-IDF specific; use default host */
#ifndef ST7789_SPI_HOST
#define ST7789_SPI_HOST SPI2_HOST
#endif

#ifndef ST7789_SPI_DEVICE_ID
#define ST7789_SPI_DEVICE_ID 0
#endif

#ifndef ST7789_SPI_USE_DUAL
#define ST7789_SPI_USE_DUAL 0
#endif

#if (ST7789_USE_DMA == 1)
/* Hook exists but we can fall back to non-DMA for now. */
#endif

static spi_device_handle_t st7789_spi = NULL;

static inline void st7789_spi_tx(const uint8_t *data, size_t len)
{
    spi_transaction_t t = {0};
    t.length = (uint32_t)(len * 8);
    t.tx_buffer = data;

    esp_err_t err = spi_device_transmit(st7789_spi, &t);
    if (err != ESP_OK) {
        ESP_LOGE(ST7789_TAG, "spi_device_transmit failed: %d", (int)err);
    }
}

void st7789_hw_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void st7789_hw_reset_begin(void)
{
    gpio_set_level(LCD_RST_PIN, 0);
}

void st7789_hw_reset_end(void)
{
    gpio_set_level(LCD_RST_PIN, 1);
}

void st7789_hw_spi_init(void)
{
    /* GPIO init */
    gpio_config_t io = {0};

    /* SCK */
    io.pin_bit_mask = (1ULL << PIN_SCK);
    io.mode = GPIO_MODE_AF;
    io.pull_up_en = GPIO_PULLUP_DISABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io);

    /* MOSI */
    io.pin_bit_mask = (1ULL << PIN_MOSI);
    gpio_config(&io);

    /* DC */
    gpio_config_t dc = {0};
    dc.pin_bit_mask = (1ULL << LCD_DC_PIN);
    dc.mode = GPIO_MODE_OUTPUT;
    dc.pull_up_en = GPIO_PULLUP_DISABLE;
    dc.pull_down_en = GPIO_PULLDOWN_DISABLE;
    dc.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&dc);

    /* RST */
    gpio_config_t rst = {0};
    rst.pin_bit_mask = (1ULL << LCD_RST_PIN);
    rst.mode = GPIO_MODE_OUTPUT;
    rst.pull_up_en = GPIO_PULLUP_DISABLE;
    rst.pull_down_en = GPIO_PULLDOWN_DISABLE;
    rst.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&rst);

    /* CS (optional) */
#if defined(PIN_CS) && (PIN_CS >= 0)
    gpio_config_t cs = {0};
    cs.pin_bit_mask = (1ULL << PIN_CS);
    cs.mode = GPIO_MODE_OUTPUT;
    cs.pull_up_en = GPIO_PULLUP_DISABLE;
    cs.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cs.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&cs);
    gpio_set_level(PIN_CS, 1); /* deselect */
#endif

    /* SPI bus init */
    spi_bus_config_t buscfg = {0};
    buscfg.sclk_io_num = PIN_SCK;
    buscfg.mosi_io_num = PIN_MOSI;
    buscfg.miso_io_num = -1;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 4096;

    esp_err_t err = spi_bus_initialize(ST7789_SPI_HOST, &buscfg, SPI_DMA_DISABLED);
    if (err != ESP_OK) {
        ESP_LOGE(ST7789_TAG, "spi_bus_initialize failed: %d", (int)err);
    }

    spi_device_interface_config_t devcfg = {0};
    devcfg.command_bits = 0;
    devcfg.address_bits = 0;
    devcfg.dummy_bits = 0;

    devcfg.spics_io_num =
#if defined(PIN_CS) && (PIN_CS >= 0)
        PIN_CS
#else
        -1
#endif
        ;

    devcfg.mode = 1; /* CPOL=1, CPHA=1 (matches pico: CPOL_1, CPHA_1) */
    devcfg.clock_speed_hz = ST7789_SPI_SPEED_HZ;
    devcfg.queue_size = 1;
    devcfg.flags = 0;

    err = spi_bus_add_device(ST7789_SPI_HOST, &devcfg, &st7789_spi);
    if (err != ESP_OK) {
        ESP_LOGE(ST7789_TAG, "spi_bus_add_device failed: %d", (int)err);
    }
}

void st7789_hw_write_cmd(uint8_t cmd)
{
    gpio_set_level(LCD_DC_PIN, 0);
    st7789_spi_tx(&cmd, 1);
}

void st7789_hw_write_data(const uint8_t *data, size_t len)
{
    gpio_set_level(LCD_DC_PIN, 1);
    st7789_spi_tx(data, len);
}

#if (ST7789_USE_DMA == 1)
/* Simple default: use non-DMA transmit (still works) */
void st7789_hw_write_data_dma(const uint8_t *data, size_t len)
{
    st7789_hw_write_data(data, len);
}
#endif
