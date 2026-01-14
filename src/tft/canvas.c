/*
 *
 *
 *
 */

#include "canvas.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/display.h>

#define RGB565_SIZE 2

#define fill_color(_buf, _idx, _color)                                                             \
	(_buf)[_idx] = (_color) >> 8;                                                              \
	(_buf)[_idx + 1] = (_color) & 0xFFu;

void draw_fill_rect(const struct device *dev, uint16_t x, uint16_t y, uint16_t w, uint16_t h,
		    uint16_t color)
{

	uint8_t h_step = 4;
	if (h_step > h) {
		h_step = h;
	}
	struct display_buffer_descriptor desc;
	desc.buf_size = w * h_step * RGB565_SIZE;
	desc.pitch = w;
	desc.width = w;
	desc.height = h_step;

	char *buf = k_malloc(desc.buf_size);

	// fill buf
	for (uint16_t y = 0, idx = 0; y < h_step; y++) {
		for (uint16_t x = 0; x < w; x++) {
			fill_color(buf, idx, color);
			idx += 2;
		}
	}
	for (uint16_t hy = 0; hy < h; hy += h_step) {
		if (hy + h_step > h) {
			desc.height = h - hy;
		}
		display_write(dev, x, y + hy, &desc, buf);
	}
	k_free(buf);
}

void draw_fill_screen(const struct device *dev, uint16_t color)
{
	struct display_capabilities caps;
	display_get_capabilities(dev, &caps);
	draw_fill_rect(dev, 0, 0, caps.x_resolution, caps.y_resolution, color);
}

static void fill_char(char ch, FontDef font, uint16_t fore, uint16_t back, uint8_t *buf)
{
	uint32_t b;

	uint16_t idx = 0;
	for (uint32_t i = 0; i < font.height; i++) {
		b = font.data[(ch - 32) * font.height + i];
		for (uint32_t j = 0; j < font.width; j++) {
			if ((b << j) & 0x8000) {
				fill_color(buf, idx, fore);
			} else {
				fill_color(buf, idx, back);
			}
			idx += 2;
		}
	}
}

uint16_t draw_text(const struct device *dev, const char *str, uint16_t x, uint16_t y, FontDef font,
		   uint16_t fore, uint16_t back)
{
	if (str == NULL) {
		return 0;
	}

	struct display_buffer_descriptor desc;
	desc.buf_size = font.width * font.height * RGB565_SIZE;
	desc.pitch = font.width;
	desc.width = font.width;
	desc.height = font.height;

	char *buf = k_malloc(desc.buf_size);
	if (buf == NULL) {
		return 0;
	}

	uint16_t count = 0;
	for (uint16_t startx = x; *str; str++, startx += font.width, count++) {
		fill_char(*str, font, fore, back, buf);
		display_write(dev, startx, y, &desc, buf);
	}
	k_free(buf);
	return count;
}
