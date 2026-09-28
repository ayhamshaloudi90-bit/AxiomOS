#ifndef AXIOM_GRAPHICS_FRAMEBUFFER_H
#define AXIOM_GRAPHICS_FRAMEBUFFER_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/graphics.h>

int graphics_init(void);
int graphics_initialized(void);
void graphics_get_info(struct axiom_gfx_info *info);
struct axiom_gfx_stats graphics_get_stats(void);

void draw_pixel(uint32_t x, uint32_t y, uint32_t rgb);
void draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t rgb);
void draw_rectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t rgb);
void draw_bitmap(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const uint32_t *pixels);
void draw_text(uint32_t x, uint32_t y, uint32_t rgb, const char *text);
void graphics_clear(uint32_t rgb);

/* Small save-under software cursor for interactive desktop use. */
void graphics_cursor_set(uint32_t x, uint32_t y, int visible);

int graphics_selftest(void);

#endif
