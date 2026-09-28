#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <axiom/graphics.h>
#include <axiom/syscalls.h>

#define ICON_W 8u
#define ICON_H 8u

static const uint32_t icon[ICON_W * ICON_H] = {
    0x000000,0x000000,0x1E88E5,0x1E88E5,0x1E88E5,0x1E88E5,0x000000,0x000000,
    0x000000,0x1E88E5,0xFFFFFF,0xFFFFFF,0xFFFFFF,0xFFFFFF,0x1E88E5,0x000000,
    0x1E88E5,0xFFFFFF,0x00ACC1,0x00ACC1,0x00ACC1,0x00ACC1,0xFFFFFF,0x1E88E5,
    0x1E88E5,0xFFFFFF,0x00ACC1,0xFFFFFF,0xFFFFFF,0x00ACC1,0xFFFFFF,0x1E88E5,
    0x1E88E5,0xFFFFFF,0x00ACC1,0xFFFFFF,0xFFFFFF,0x00ACC1,0xFFFFFF,0x1E88E5,
    0x1E88E5,0xFFFFFF,0x00ACC1,0x00ACC1,0x00ACC1,0x00ACC1,0xFFFFFF,0x1E88E5,
    0x000000,0x1E88E5,0xFFFFFF,0xFFFFFF,0xFFFFFF,0xFFFFFF,0x1E88E5,0x000000,
    0x000000,0x000000,0x1E88E5,0x1E88E5,0x1E88E5,0x1E88E5,0x000000,0x000000,
};

int main(void)
{
    struct axiom_gfx_info info;
    struct axiom_gfx_stats before;
    struct axiom_gfx_stats after;
    uint32_t panel_x;
    uint32_t panel_y;
    uint32_t panel_w;
    uint32_t panel_h;
    uint32_t box_w;
    uint32_t box_h;
    int ok = 1;

    puts("AxiomOS Phase 23 framebuffer graphics userspace demo.");
    if (axiom_gfx_info(&info) < 0 || info.width == 0u || info.height == 0u || info.bpp != 32u) {
        puts("Phase 23 framebuffer discovery: FAILED");
        return 23;
    }
    if (axiom_gfx_stats(&before) < 0) return 24;

    printf("Phase 23 framebuffer: %ux%u pitch=%u bpp=%u font=%ux%u\n",
        info.width, info.height, info.pitch, info.bpp, info.font_width, info.font_height);

    panel_w = info.width > 536u ? 520u : info.width - 16u;
    panel_h = info.height > 316u ? 300u : info.height - 16u;
    panel_x = (info.width - panel_w) / 2u;
    panel_y = (info.height - panel_h) / 2u;
    box_w = panel_w > 200u ? 160u : panel_w / 3u;
    box_h = panel_h > 130u ? 90u : panel_h / 3u;

    ok &= axiom_gfx_clear(AXIOM_GFX_COLOR_DARK) == 0;
    ok &= axiom_draw_rectangle(panel_x, panel_y, panel_w, panel_h, AXIOM_GFX_COLOR_PANEL) == 0;
    ok &= axiom_draw_line(panel_x, panel_y, panel_x + panel_w - 1u, panel_y + panel_h - 1u, AXIOM_GFX_COLOR_CYAN) == 0;
    ok &= axiom_draw_line(panel_x + panel_w - 1u, panel_y, panel_x, panel_y + panel_h - 1u, AXIOM_GFX_COLOR_MAGENTA) == 0;
    ok &= axiom_draw_rectangle(panel_x + 8u, panel_y + 48u, box_w, box_h, AXIOM_GFX_COLOR_BLUE) == 0;
    ok &= axiom_draw_bitmap(panel_x + panel_w / 2u, panel_y + panel_h / 3u, ICON_W, ICON_H, icon) == 0;
    ok &= axiom_draw_text(panel_x + 8u, panel_y + 8u, AXIOM_GFX_COLOR_WHITE,
        "AxiomOS Phase 23 Graphics") == 0;
    ok &= axiom_draw_text(panel_x + 8u, panel_y + panel_h * 2u / 3u, AXIOM_GFX_COLOR_YELLOW,
        "pixel  line  rectangle  bitmap  text") == 0;
    ok &= axiom_draw_pixel(info.width - 1u, info.height - 1u, AXIOM_GFX_COLOR_GREEN) == 0;

    if (axiom_gfx_stats(&after) < 0) ok = 0;
    if (after.clears <= before.clears || after.lines < before.lines + 2u ||
        after.rectangles < before.rectangles + 2u || after.bitmaps <= before.bitmaps ||
        after.text_calls < before.text_calls + 2u || after.glyphs <= before.glyphs) ok = 0;

    puts(ok ? "Phase 23 draw_pixel: OK" : "Phase 23 draw_pixel: FAILED");
    puts(ok ? "Phase 23 draw_line: OK" : "Phase 23 draw_line: FAILED");
    puts(ok ? "Phase 23 draw_rectangle: OK" : "Phase 23 draw_rectangle: FAILED");
    puts(ok ? "Phase 23 draw_bitmap: OK" : "Phase 23 draw_bitmap: FAILED");
    puts(ok ? "Phase 23 draw_text: OK" : "Phase 23 draw_text: FAILED");
    puts(ok ? "Phase 23 userspace graphics library: OK" : "Phase 23 userspace graphics library: FAILED");

    (void)sleep(350u);
    (void)axiom_clear();
    puts(ok ? "Phase 23 framebuffer graphics demo complete." : "Phase 23 framebuffer graphics demo FAILED.");
    return ok ? 0 : 25;
}
