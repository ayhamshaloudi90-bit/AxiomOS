#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/syscall.h>
#include <axiom/graphics.h>
#include "syscall_internal.h"

long axiom_gfx_info(struct axiom_gfx_info *info)
{
    return __axiom_syscall1(AXIOM_SYS_GFX_INFO, (long)(uintptr_t)info);
}

long axiom_gfx_clear(uint32_t rgb)
{
    return __axiom_syscall1(AXIOM_SYS_GFX_CLEAR, (long)rgb);
}

long axiom_draw_pixel(uint32_t x, uint32_t y, uint32_t rgb)
{
    return __axiom_syscall3(AXIOM_SYS_GFX_PIXEL, (long)x, (long)y, (long)rgb);
}

long axiom_draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t rgb)
{
    return __axiom_syscall5(
        AXIOM_SYS_GFX_LINE, (long)x0, (long)y0, (long)x1, (long)y1, (long)rgb);
}

long axiom_draw_rectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t rgb)
{
    return __axiom_syscall5(
        AXIOM_SYS_GFX_RECT, (long)x, (long)y, (long)width, (long)height, (long)rgb);
}

long axiom_draw_bitmap(
    uint32_t x, uint32_t y, uint32_t width, uint32_t height, const uint32_t *pixels)
{
    const struct axiom_gfx_bitmap_request request = {
        .x = x,
        .y = y,
        .width = width,
        .height = height,
        .pixels = (uint64_t)(uintptr_t)pixels,
    };
    return __axiom_syscall1(AXIOM_SYS_GFX_BITMAP, (long)(uintptr_t)&request);
}

long axiom_draw_text(uint32_t x, uint32_t y, uint32_t rgb, const char *text)
{
    return __axiom_syscall5(
        AXIOM_SYS_GFX_TEXT, (long)x, (long)y, (long)rgb, (long)(uintptr_t)text, 0);
}

long axiom_gfx_stats(struct axiom_gfx_stats *stats)
{
    return __axiom_syscall1(AXIOM_SYS_GFX_STATS, (long)(uintptr_t)stats);
}

long axiom_gfx_cursor(uint32_t x, uint32_t y, int visible)
{
    return __axiom_syscall3(AXIOM_SYS_GFX_CURSOR, (long)x, (long)y, (long)(visible != 0));
}
