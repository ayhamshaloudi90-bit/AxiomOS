#ifndef AXIOM_ABI_GRAPHICS_H
#define AXIOM_ABI_GRAPHICS_H

#define AXIOM_GFX_COLOR_BLACK       0x000000u
#define AXIOM_GFX_COLOR_WHITE       0xFFFFFFu
#define AXIOM_GFX_COLOR_RED         0xE53935u
#define AXIOM_GFX_COLOR_GREEN       0x43A047u
#define AXIOM_GFX_COLOR_BLUE        0x1E88E5u
#define AXIOM_GFX_COLOR_YELLOW      0xFDD835u
#define AXIOM_GFX_COLOR_CYAN        0x00ACC1u
#define AXIOM_GFX_COLOR_MAGENTA     0x8E24AAu
#define AXIOM_GFX_COLOR_DARK        0x101820u
#define AXIOM_GFX_COLOR_PANEL       0x263238u

#define AXIOM_GFX_BITMAP_MAX_PIXELS 16384u
#define AXIOM_GFX_TEXT_MAX          128u

#ifndef __ASSEMBLER__
#include <stdint.h>

struct axiom_gfx_info {
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    uint32_t font_width;
    uint32_t font_height;
};

struct axiom_gfx_bitmap_request {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint64_t pixels;
};

struct axiom_gfx_stats {
    uint64_t clears;
    uint64_t pixels;
    uint64_t lines;
    uint64_t rectangles;
    uint64_t bitmaps;
    uint64_t bitmap_pixels;
    uint64_t text_calls;
    uint64_t glyphs;
    uint64_t clipped_pixels;
};
#endif

#endif
