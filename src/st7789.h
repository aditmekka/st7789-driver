#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "../user_setup.h"

/* Common 16-bit RGB565 colors */
#define WHITE       0xFFFFu
#define BLACK       0x0000u
#define BLUE        0x001Fu
#define RED         0xF800u
#define MAGENTA     0xF81Fu
#define GREEN       0x07E0u
#define CYAN        0x7FFFu
#define YELLOW      0xFFE0u
#define GRAY        0x8430u
#define LIGHTBLUE   0x7D7Cu
#define LIGHTGREEN  0x841Fu

/* ST7789 init/draw API */
void st7789_init(void);
void st7789_set_rotation(uint8_t m);

void st7789_fill_color(uint16_t color);
void st7789_fill(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color);
void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

void st7789_draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void st7789_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void st7789_draw_filled_rectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

void st7789_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color);
void st7789_draw_filled_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

void st7789_draw_triangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                           uint16_t x3, uint16_t y3, uint16_t color);
void st7789_draw_filled_triangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                                  uint16_t x3, uint16_t y3, uint16_t color);

void st7789_draw_image(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);

void st7789_invert_colors(uint8_t invert);
void st7789_tear_effect(bool tear);

void st7789_set_window_full(void);
void st7789_blit(const uint8_t *data, size_t len);

#endif /* ST7789_H */
