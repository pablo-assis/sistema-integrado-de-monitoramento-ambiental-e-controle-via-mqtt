#pragma once

#include "hardware/i2c.h"
#include <stdbool.h>

// SSD1306 128x64 I2C driver (minimal)

bool ssd1306_init(i2c_inst_t *i2c, uint8_t addr);
void ssd1306_clear(void);
void ssd1306_update(void);
void ssd1306_set_pixel(int x, int y, bool on);
void ssd1306_draw_line(int x0, int y0, int x1, int y1, bool on);
void ssd1306_draw_circle(int cx, int cy, int r, bool on);
void ssd1306_draw_bitmap(int x, int y, const uint8_t *bmp, int w, int h, bool white);
