#ifndef USER_SETUP_H
#define USER_SETUP_H

/*
 * Unified user configuration for ST7789.
 * This header is intended to be edited by the user for their specific wiring/panel.
 *
 * The MCU-agnostic driver (src/st7789.c) will include this file.
 */

/* ----------- Panel selection ----------- */
#define ST7789_ROTATION       2  /* 0..3 */

/* Uncomment exactly one resolution. */
#define USING_240X240
/* #define USING_135X240 */
/* #define USING_170X320 */

/* ----------- SPI configuration ----------- */
/*
 * Keep in mind: many MCUs have different SPI port identifiers/types.
 * The driver treats the "SPI speed" as a numeric Hz, and leaves "SPI port"
 * to the HW implementation.
 */
#define ST7789_SPI_SPEED_HZ  62500000u

/* ----------- Pins ----------- */
/* The pico example uses these numeric GPIO IDs; other MCUs can reinterpret them.
 * If you have no CS pin, you may leave ST7789_USE_CS disabled in the HW code.
 */
#define PIN_SCK     18
#define PIN_MOSI    19
#define PIN_CS      17   /* optional; may be unused */
#define LCD_RST_PIN 20
#define LCD_DC_PIN  21

/* ----------- Feature toggles ----------- */
/* Enable if your HW layer supports DMA for bulk pixel writes. */
#define ST7789_USE_DMA 1

#endif /* USER_SETUP_H */
