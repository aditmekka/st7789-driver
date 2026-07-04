#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "../../src/st7789.h"

static void st7789_demo_task(void *arg)
{
    (void)arg;

    st7789_init();

    vTaskDelay(pdMS_TO_TICKS(200));
    st7789_fill_color(BLACK);

    vTaskDelay(pdMS_TO_TICKS(200));
    st7789_draw_pixel(10, 10, RED);
    vTaskDelay(pdMS_TO_TICKS(200));
    st7789_draw_line(0, 0, (uint16_t)(ST7789_WIDTH - 1), (uint16_t)(ST7789_HEIGHT - 1), GREEN);

    // Simple rectangle
    vTaskDelay(pdMS_TO_TICKS(200));
    st7789_draw_rectangle(20, 20, 100, 60, BLUE);

    // Filled rectangle
    vTaskDelay(pdMS_TO_TICKS(200));
    st7789_draw_filled_rectangle(30, 70, 90, 110, YELLOW);

    // Triangle
    vTaskDelay(pdMS_TO_TICKS(200));
    st7789_draw_triangle(10, 120, 60, 180, 110, 120, MAGENTA);

    // Filled triangle
    vTaskDelay(pdMS_TO_TICKS(200));
    st7789_draw_filled_triangle(130, 20, 200, 60, 160, 110, CYAN);

    // Circles
    vTaskDelay(pdMS_TO_TICKS(200));
    st7789_draw_circle(160, 160, 40, WHITE);
    st7789_draw_filled_circle(160, 160, 20, GRAY);

    // Image/blit demo (optional): provide your own data pointer in RGB565
    // st7789_draw_image(x, y, w, h, data);
    // st7789_blit(byte_swap_already_rgb565_bytes, len_bytes);

    vTaskDelete(NULL);
}

void app_main(void)
{
    xTaskCreate(st7789_demo_task, "st7789_demo", 4096, NULL, 5, NULL);
}
