
#ifndef __CANVAS_H_
#define __CANVAS_H_

#include <stdint.h>
#include "fonts.h"
#include <zephyr/device.h>

#define COLOR_BLACK 0x0000
#define COLOR_BLUE  0x001F
#define COLOR_RED   0xF800
#define COLOR_GREEN 0x07E0

#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_YELLOW  0xFFE0
#define COLOR_WHITE   0xFFFF
#define COLOR_RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))





void draw_fill_rect(const struct device *dev, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
		    uint16_t color);
void draw_fill_screen(const struct device *dev, uint16_t color);

uint16_t draw_text(const struct device *dev, const char *str, uint16_t x, uint16_t y, FontDef font,
	       uint16_t fore, uint16_t back);
#endif //__CANVAS_H_
