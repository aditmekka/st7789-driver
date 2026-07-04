/*
 * st7789.c - MCU-agnostic ST7789 driver core
 *
 * Drawing primitives and command/init sequence live here.
 * All hardware-specific operations are delegated to src/st7789_hw.[ch].
 */

#include "st7789.h"
#include "st7789_hw.h"

#include <string.h>

#define ABS(x) ((x) > 0 ? (x) : -(x))

/* MADCTL/Rotation + panel mapping (matches your pico implementation) */
#ifdef USING_135X240
    #if ST7789_ROTATION == 0
        #define ST7789_WIDTH 135
        #define ST7789_HEIGHT 240
        #define X_SHIFT 53
        #define Y_SHIFT 40
    #endif
    #if ST7789_ROTATION == 1
        #define ST7789_WIDTH 240
        #define ST7789_HEIGHT 135
        #define X_SHIFT 40
        #define Y_SHIFT 52
    #endif
    #if ST7789_ROTATION == 2
        #define ST7789_WIDTH 135
        #define ST7789_HEIGHT 240
        #define X_SHIFT 52
        #define Y_SHIFT 40
    #endif
    #if ST7789_ROTATION == 3
        #define ST7789_WIDTH 240
        #define ST7789_HEIGHT 135
        #define X_SHIFT 40
        #define Y_SHIFT 53
    #endif
#endif

#ifdef USING_240X240
    #define ST7789_WIDTH 240
    #define ST7789_HEIGHT 240

    #if ST7789_ROTATION == 0
        #define X_SHIFT 0
        #define Y_SHIFT 80
    #elif ST7789_ROTATION == 1
        #define X_SHIFT 80
        #define Y_SHIFT 0
    #elif ST7789_ROTATION == 2
        #define X_SHIFT 0
        #define Y_SHIFT 0
    #elif ST7789_ROTATION == 3
        #define X_SHIFT 0
        #define Y_SHIFT 0
    #else
        #error "Invalid ST7789_ROTATION"
    #endif
#endif

#ifdef USING_170X320
    #if ST7789_ROTATION == 0
        #define ST7789_WIDTH 170
        #define ST7789_HEIGHT 320
        #define X_SHIFT 35
        #define Y_SHIFT 0
    #endif
    #if ST7789_ROTATION == 1
        #define ST7789_WIDTH 320
        #define ST7789_HEIGHT 170
        #define X_SHIFT 0
        #define Y_SHIFT 35
    #endif
    #if ST7789_ROTATION == 2
        #define ST7789_WIDTH 170
        #define ST7789_HEIGHT 320
        #define X_SHIFT 35
        #define Y_SHIFT 0
    #endif
    #if ST7789_ROTATION == 3
        #define ST7789_WIDTH 320
        #define ST7789_HEIGHT 170
        #define X_SHIFT 0
        #define Y_SHIFT 35
    #endif
#endif

#ifndef ST7789_WIDTH
#error "No panel resolution selected. Define USING_240X240 or USING_135X240 or USING_170X320 in user_setup.h"
#endif

/* Control Registers and constant codes */
#define ST7789_MADCTL 0x36
#define ST7789_CASET  0x2A
#define ST7789_RASET  0x2B
#define ST7789_RAMWR  0x2C

#define ST7789_SWRESET 0x01
#define ST7789_COLMOD  0x3A
#define ST7789_INVON   0x21
#define ST7789_SLPOUT  0x11
#define ST7789_NORON   0x13
#define ST7789_DISPON  0x29

/* MADCTL bits */
#define ST7789_MADCTL_MY  0x80
#define ST7789_MADCTL_MX  0x40
#define ST7789_MADCTL_MV  0x20
#define ST7789_MADCTL_RGB 0x00

/* Color mode */
#define ST7789_COLOR_MODE_16bit 0x55 /* RGB565 */

/* Optional DMA chunk buffer */
static uint8_t st7789_chunk_buf[256];

static void st7789_write_cmd(uint8_t cmd)
{
    st7789_hw_write_cmd(cmd);
}

static void st7789_write_data(uint8_t *buf, uint32_t size)
{
    st7789_hw_write_data(buf, (size_t)size);
}

static void st7789_write_small_data(uint8_t data)
{
    st7789_hw_write_data(&data, 1);
}

static void st7789_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint16_t x_start = (uint16_t)(x0 + X_SHIFT);
    uint16_t x_end   = (uint16_t)(x1 + X_SHIFT);
    uint16_t y_start = (uint16_t)(y0 + Y_SHIFT);
    uint16_t y_end   = (uint16_t)(y1 + Y_SHIFT);

    st7789_write_cmd(ST7789_CASET);
    {
        uint8_t data[] = { (uint8_t)(x_start >> 8), (uint8_t)(x_start & 0xFF),
                            (uint8_t)(x_end   >> 8), (uint8_t)(x_end   & 0xFF) };
        st7789_hw_write_data(data, sizeof(data));
    }

    st7789_write_cmd(ST7789_RASET);
    {
        uint8_t data[] = { (uint8_t)(y_start >> 8), (uint8_t)(y_start & 0xFF),
                            (uint8_t)(y_end   >> 8), (uint8_t)(y_end   & 0xFF) };
        st7789_hw_write_data(data, sizeof(data));
    }

    st7789_write_cmd(ST7789_RAMWR);
}

static void st7789_push_solid_color(uint16_t color, uint32_t pixel_count)
{
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color & 0xFF);

    for (size_t i = 0; i < sizeof(st7789_chunk_buf); i += 2) {
        st7789_chunk_buf[i]     = hi;
        st7789_chunk_buf[i + 1] = lo;
    }

    while (pixel_count) {
        uint32_t chunk_pixels = (uint32_t)(sizeof(st7789_chunk_buf) / 2);
        if (chunk_pixels > pixel_count) {
            chunk_pixels = pixel_count;
        }

#if (ST7789_USE_DMA == 1)
        st7789_hw_write_data_dma(st7789_chunk_buf, (size_t)(chunk_pixels * 2));
#else
        st7789_hw_write_data(st7789_chunk_buf, (size_t)(chunk_pixels * 2));
#endif
        pixel_count -= chunk_pixels;
    }
}

void st7789_set_rotation(uint8_t m)
{
    st7789_write_cmd(ST7789_MADCTL);

    switch (m) {
        case 0:
            st7789_write_small_data((uint8_t)(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB));
            break;
        case 1:
            st7789_write_small_data((uint8_t)(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB));
            break;
        case 2:
            st7789_write_small_data((uint8_t)(ST7789_MADCTL_RGB));
            break;
        case 3:
            st7789_write_small_data((uint8_t)(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB));
            break;
        default:
            break;
    }
}

void st7789_init(void)
{
    st7789_hw_spi_init();

    #if (ST7789_USE_DMA == 1)
    /* DMA init is MCU-specific; pico example can implement it inside st7789_hw_spi_init or other hooks. */
    #endif

    st7789_hw_delay_ms(25);

    st7789_hw_reset_begin();
    st7789_hw_delay_ms(50);
    st7789_hw_reset_end();
    st7789_hw_delay_ms(50);

    st7789_write_cmd(ST7789_SWRESET);
    st7789_hw_delay_ms(100);

    st7789_write_cmd(ST7789_COLMOD);
    st7789_write_small_data(ST7789_COLOR_MODE_16bit);

    st7789_write_cmd(0xB2);
    {
        uint8_t data[] = { 0x0C, 0x0C, 0x00, 0x33, 0x33 };
        st7789_hw_write_data(data, sizeof(data));
    }

    st7789_write_cmd(0xB7);
    st7789_write_small_data(0x35);

    st7789_write_cmd(0xBB);
    st7789_write_small_data(0x20);

    st7789_write_cmd(0xC0);
    st7789_write_small_data(0x2C);

    st7789_write_cmd(0xC2);
    st7789_write_small_data(0x01);

    st7789_write_cmd(0xC3);
    st7789_write_small_data(0x0b);

    st7789_write_cmd(0xC4);
    st7789_write_small_data(0x20);

    st7789_write_cmd(0xC6);
    st7789_write_small_data(0x0F);

    st7789_write_cmd(0xD0);
    st7789_write_small_data(0xA4);

    st7789_write_small_data(0xA1);

    st7789_write_cmd(0xE0);
    {
        uint8_t data[] = {0xD0, 0x04, 0x0D, 0x11, 0x13, 0x2B, 0x3F, 0x54, 0x4C, 0x18, 0x0D, 0x0B, 0x1F, 0x23};
        st7789_hw_write_data(data, sizeof(data));
    }

    st7789_write_cmd(0xE1);
    {
        uint8_t data[] = {0xD0, 0x04, 0x0C, 0x11, 0x13, 0x2C, 0x3F, 0x44, 0x51, 0x2F, 0x1F, 0x1F, 0x20, 0x23};
        st7789_hw_write_data(data, sizeof(data));
    }

    st7789_write_cmd(ST7789_INVON);
    st7789_write_cmd(0x11); /* SLPOUT */
    st7789_write_cmd(0x13); /* NORON */
    st7789_write_cmd(0x29); /* DISPON */

    st7789_hw_delay_ms(50);

    /* Apply rotation and fill black */
    st7789_set_rotation((uint8_t)ST7789_ROTATION);
    st7789_fill_color(BLACK);
}

void st7789_fill_color(uint16_t color)
{
    st7789_set_address_window(0, 0, (uint16_t)(ST7789_WIDTH - 1), (uint16_t)(ST7789_HEIGHT - 1));
    uint32_t total_pixels = (uint32_t)ST7789_WIDTH * (uint32_t)ST7789_HEIGHT;
    st7789_push_solid_color(color, total_pixels);
}

void st7789_fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
{
    if (xSta >= ST7789_WIDTH || ySta >= ST7789_HEIGHT) return;
    if (xEnd  >= ST7789_WIDTH)  xEnd  = (uint16_t)(ST7789_WIDTH  - 1);
    if (yEnd  >= ST7789_HEIGHT) yEnd  = (uint16_t)(ST7789_HEIGHT - 1);
    if (xEnd < xSta || yEnd < ySta) return;

    st7789_set_address_window(xSta, ySta, xEnd, yEnd);

    uint32_t w = (uint32_t)(xEnd - xSta + 1);
    uint32_t h = (uint32_t)(yEnd - ySta + 1);
    uint32_t total_pixels = w * h;

    st7789_push_solid_color(color, total_pixels);
}

void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT)) return;

    st7789_set_address_window(x, y, x, y);

    uint8_t data[] = { (uint8_t)(color >> 8), (uint8_t)(color & 0xFF) };
    st7789_hw_write_data(data, sizeof(data));
}

void st7789_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    if (y0 == y1) {
        if (x1 < x0) { uint16_t t = x0; x0 = x1; x1 = t; }
        st7789_fill(x0, y0, x1, y0, color);
        return;
    }

    if (x0 == x1) {
        if (y1 < y0) { uint16_t t = y0; y0 = y1; y1 = t; }
        st7789_fill(x0, y0, x0, y1, color);
        return;
    }

    uint16_t steep = (ABS((int16_t)y1 - (int16_t)y0) > ABS((int16_t)x1 - (int16_t)x0));

    if (steep) {
        uint16_t t = x0; x0 = y0; y0 = t;
        t = x1; x1 = y1; y1 = t;
    }

    if (x0 > x1) {
        uint16_t t = x0; x0 = x1; x1 = t;
        t = y0; y0 = y1; y1 = t;
    }

    int16_t dx = (int16_t)x1 - (int16_t)x0;
    int16_t dy = ABS((int16_t)y1 - (int16_t)y0);

    int16_t err = dx / 2;
    int16_t ystep = (y0 < y1) ? 1 : -1;

    for (; x0 <= x1; x0++) {
        if (steep) st7789_draw_pixel(y0, x0, color);
        else       st7789_draw_pixel(x0, y0, color);

        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void st7789_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    st7789_draw_line(x1, y1, x2, y1, color);
    st7789_draw_line(x1, y1, x1, y2, color);
    st7789_draw_line(x1, y2, x2, y2, color);
    st7789_draw_line(x2, y1, x2, y2, color);
}

void st7789_draw_filled_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT) return;

    uint16_t xEnd = (uint16_t)(x + w);
    uint16_t yEnd = (uint16_t)(y + h);

    if (xEnd >= ST7789_WIDTH)  xEnd = (uint16_t)(ST7789_WIDTH  - 1);
    if (yEnd >= ST7789_HEIGHT) yEnd = (uint16_t)(ST7789_HEIGHT - 1);

    st7789_fill(x, y, xEnd, yEnd, color);
}

void st7789_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    st7789_draw_pixel(x0, y0 + r, color);
    st7789_draw_pixel(x0, y0 - r, color);
    st7789_draw_pixel(x0 + r, y0, color);
    st7789_draw_pixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        st7789_draw_pixel(x0 + x, y0 + y, color);
        st7789_draw_pixel(x0 - x, y0 + y, color);
        st7789_draw_pixel(x0 + x, y0 - y, color);
        st7789_draw_pixel(x0 - x, y0 - y, color);

        st7789_draw_pixel(x0 + y, y0 + x, color);
        st7789_draw_pixel(x0 - y, y0 + x, color);
        st7789_draw_pixel(x0 + y, y0 - x, color);
        st7789_draw_pixel(x0 - y, y0 - x, color);
    }
}

void st7789_draw_filled_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    st7789_draw_pixel(x0, y0 + r, color);
    st7789_draw_pixel(x0, y0 - r, color);
    st7789_draw_pixel(x0 + r, y0, color);
    st7789_draw_pixel(x0 - r, y0, color);
    st7789_draw_line(x0 - r, y0, x0 + r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        st7789_draw_line(x0 - x, y0 + y, x0 + x, y0 + y, color);
        st7789_draw_line(x0 + x, y0 - y, x0 - x, y0 - y, color);

        st7789_draw_line(x0 + y, y0 + x, x0 - y, y0 + x, color);
        st7789_draw_line(x0 + y, y0 - x, x0 - y, y0 - x, color);
    }
}

void st7789_draw_triangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                           uint16_t x3, uint16_t y3, uint16_t color)
{
    st7789_draw_line(x1, y1, x2, y2, color);
    st7789_draw_line(x2, y2, x3, y3, color);
    st7789_draw_line(x3, y3, x1, y1, color);
}

void st7789_draw_filled_triangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                                  uint16_t x3, uint16_t y3, uint16_t color)
{
    int16_t deltax = 0, deltay = 0, x = 0, y = 0, xinc1 = 0, xinc2 = 0,
            yinc1 = 0, yinc2 = 0, den = 0, num = 0, numadd = 0, numpixels = 0,
            curpixel = 0;

    deltax = ABS(x2 - x1);
    deltay = ABS(y2 - y1);
    x = x1;
    y = y1;

    if (x2 >= x1) { xinc1 = 1; xinc2 = 1; } else { xinc1 = -1; xinc2 = -1; }
    if (y2 >= y1) { yinc1 = 1; yinc2 = 1; } else { yinc1 = -1; yinc2 = -1; }

    if (deltax >= deltay) {
        xinc1 = 0;
        yinc2 = 0;
        den = deltax;
        num = deltax / 2;
        numadd = deltay;
        numpixels = deltax;
    } else {
        xinc2 = 0;
        yinc1 = 0;
        den = deltay;
        num = deltay / 2;
        numadd = deltax;
        numpixels = deltay;
    }

    for (curpixel = 0; curpixel <= numpixels; curpixel++) {
        st7789_draw_line((uint16_t)x, (uint16_t)y, x3, y3, color);

        num += numadd;
        if (num >= den) {
            num -= den;
            x += xinc1;
            y += yinc1;
        }
        x += xinc2;
        y += yinc2;
    }
}

void st7789_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT)) return;
    if ((uint32_t)(x + w - 1) >= ST7789_WIDTH) return;
    if ((uint32_t)(y + h - 1) >= ST7789_HEIGHT) return;

    st7789_set_address_window(x, y, (uint16_t)(x + w - 1), (uint16_t)(y + h - 1));

#if (ST7789_USE_DMA == 1)
    st7789_hw_write_data_dma((const uint8_t *)data, sizeof(uint16_t) * (size_t)w * (size_t)h);
#else
    st7789_hw_write_data((const uint8_t *)data, sizeof(uint16_t) * (size_t)w * (size_t)h);
#endif
}

void st7789_invert_colors(uint8_t invert)
{
    st7789_write_cmd(invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);
}

void st7789_tear_effect(bool tear)
{
    st7789_write_cmd(tear ? 0x35 /* TEON */ : 0x34 /* TEOFF */);
}

void st7789_set_window_full(void)
{
    st7789_set_address_window(0, 0, (uint16_t)(ST7789_WIDTH - 1), (uint16_t)(ST7789_HEIGHT - 1));
}

void st7789_blit(const uint8_t *data, size_t len)
{
    static uint8_t swap_buf[512];

    while (len) {
        size_t chunk = (len > sizeof(swap_buf)) ? sizeof(swap_buf) : len;

        /* swap byte order */
        for (size_t i = 0; i < chunk; i += 2) {
            swap_buf[i]     = data[i + 1];
            swap_buf[i + 1] = data[i];
        }

#if (ST7789_USE_DMA == 1)
        st7789_hw_write_data_dma(swap_buf, chunk);
#else
        st7789_hw_write_data(swap_buf, chunk);
#endif
        data += chunk;
        len  -= chunk;
    }
}
