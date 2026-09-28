#ifndef AXIOM_LIBC_GRAPHICS_H
#define AXIOM_LIBC_GRAPHICS_H

#include <stdint.h>
#include <axiom/abi/graphics.h>

long axiom_gfx_info(struct axiom_gfx_info *info);
long axiom_gfx_clear(uint32_t rgb);
long axiom_draw_pixel(uint32_t x, uint32_t y, uint32_t rgb);
long axiom_draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t rgb);
long axiom_draw_rectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t rgb);
long axiom_draw_bitmap(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const uint32_t *pixels);
long axiom_draw_text(uint32_t x, uint32_t y, uint32_t rgb, const char *text);
long axiom_gfx_stats(struct axiom_gfx_stats *stats);
long axiom_gfx_cursor(uint32_t x, uint32_t y, int visible);

#endif
