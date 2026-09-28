#include <stddef.h>
#include <stdint.h>

#include <axiom/boot/limine.h>
#include <axiom/graphics/framebuffer.h>

#define GRAPHICS_CURSOR_WIDTH 12u
#define GRAPHICS_CURSOR_HEIGHT 19u

struct graphics_state {
    struct limine_framebuffer *framebuffer;
    const uint8_t *font;
    uint32_t font_width;
    uint32_t font_height;
    int initialized;
    struct axiom_gfx_stats stats;

    int cursor_visible;
    uint32_t cursor_x;
    uint32_t cursor_y;
    uint32_t cursor_under[GRAPHICS_CURSOR_WIDTH * GRAPHICS_CURSOR_HEIGHT];
};

static struct graphics_state graphics;

static uint32_t scale_channel(uint8_t value, uint8_t bits)
{
    if (bits == 0u) return 0u;
    if (bits >= 8u) return (uint32_t)value;
    return (uint32_t)(value >> (8u - bits));
}

static uint32_t pack_rgb(uint32_t rgb)
{
    const uint8_t red = (uint8_t)((rgb >> 16) & 0xFFu);
    const uint8_t green = (uint8_t)((rgb >> 8) & 0xFFu);
    const uint8_t blue = (uint8_t)(rgb & 0xFFu);

    return
        (scale_channel(red, graphics.framebuffer->red_mask_size)
            << graphics.framebuffer->red_mask_shift) |
        (scale_channel(green, graphics.framebuffer->green_mask_size)
            << graphics.framebuffer->green_mask_shift) |
        (scale_channel(blue, graphics.framebuffer->blue_mask_size)
            << graphics.framebuffer->blue_mask_shift);
}

static int in_bounds(uint32_t x, uint32_t y)
{
    return graphics.initialized &&
        (uint64_t)x < graphics.framebuffer->width &&
        (uint64_t)y < graphics.framebuffer->height;
}

static volatile uint32_t *pixel_address(uint32_t x, uint32_t y)
{
    return &((volatile uint32_t *)((uint8_t *)graphics.framebuffer->address +
        ((uint64_t)y * graphics.framebuffer->pitch)))[x];
}

int graphics_init(void)
{
    struct limine_framebuffer *framebuffer;
    struct limine_flanterm_fb_init_params *font_params;

    if (graphics.initialized) return 1;
    if (!limine_get_terminal_boot_info(&framebuffer, &font_params)) return 0;
    if (framebuffer == 0 || framebuffer->address == 0 ||
        framebuffer->bpp != 32u || framebuffer->memory_model != LIMINE_FRAMEBUFFER_RGB ||
        framebuffer->width == 0u || framebuffer->height == 0u ||
        framebuffer->width > UINT32_MAX || framebuffer->height > UINT32_MAX ||
        framebuffer->pitch > UINT32_MAX) {
        return 0;
    }
    if (font_params == 0 || font_params->font == 0 ||
        font_params->font_width != 8u || font_params->font_height == 0u ||
        font_params->font_height > UINT32_MAX || font_params->rotation != 0u) {
        return 0;
    }

    graphics.framebuffer = framebuffer;
    graphics.font = (const uint8_t *)font_params->font;
    graphics.font_width = (uint32_t)font_params->font_width;
    graphics.font_height = (uint32_t)font_params->font_height;
    graphics.stats = (struct axiom_gfx_stats){0};
    graphics.initialized = 1;
    return 1;
}

int graphics_initialized(void)
{
    return graphics.initialized;
}

void graphics_get_info(struct axiom_gfx_info *info)
{
    if (info == 0) return;
    *info = (struct axiom_gfx_info){0};
    if (!graphics.initialized) return;
    info->width = (uint32_t)graphics.framebuffer->width;
    info->height = (uint32_t)graphics.framebuffer->height;
    info->pitch = (uint32_t)graphics.framebuffer->pitch;
    info->bpp = graphics.framebuffer->bpp;
    info->font_width = graphics.font_width;
    info->font_height = graphics.font_height;
}

struct axiom_gfx_stats graphics_get_stats(void)
{
    return graphics.stats;
}

void draw_pixel(uint32_t x, uint32_t y, uint32_t rgb)
{
    if (!in_bounds(x, y)) {
        if (graphics.initialized) ++graphics.stats.clipped_pixels;
        return;
    }
    *pixel_address(x, y) = pack_rgb(rgb);
    ++graphics.stats.pixels;
}

void graphics_clear(uint32_t rgb)
{
    uint32_t y;
    uint32_t x;
    uint32_t packed;

    if (!graphics.initialized) return;
    packed = pack_rgb(rgb);
    for (y = 0u; (uint64_t)y < graphics.framebuffer->height; ++y) {
        volatile uint32_t *row = (volatile uint32_t *)((uint8_t *)graphics.framebuffer->address +
            ((uint64_t)y * graphics.framebuffer->pitch));
        for (x = 0u; (uint64_t)x < graphics.framebuffer->width; ++x) row[x] = packed;
    }
    ++graphics.stats.clears;
}

void draw_line(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, uint32_t rgb)
{
    int64_t x = (int64_t)x0;
    int64_t y = (int64_t)y0;
    const int64_t end_x = (int64_t)x1;
    const int64_t end_y = (int64_t)y1;
    const int64_t dx = end_x >= x ? end_x - x : x - end_x;
    const int64_t sx = x < end_x ? 1 : -1;
    const int64_t dy_abs = end_y >= y ? end_y - y : y - end_y;
    const int64_t dy = -dy_abs;
    const int64_t sy = y < end_y ? 1 : -1;
    int64_t error = dx + dy;

    if (!graphics.initialized) return;
    ++graphics.stats.lines;
    for (;;) {
        if (x >= 0 && y >= 0 && x <= UINT32_MAX && y <= UINT32_MAX) {
            draw_pixel((uint32_t)x, (uint32_t)y, rgb);
        }
        if (x == end_x && y == end_y) break;
        {
            const int64_t doubled = error * 2;
            if (doubled >= dy) { error += dy; x += sx; }
            if (doubled <= dx) { error += dx; y += sy; }
        }
    }
}

void draw_rectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t rgb)
{
    uint64_t end_x;
    uint64_t end_y;
    uint64_t clipped_end_x;
    uint64_t clipped_end_y;
    uint64_t drawn_width;
    uint64_t drawn_height;
    uint64_t drawn_pixels;
    const uint64_t requested_pixels = (uint64_t)width * height;
    uint32_t packed;
    uint32_t row;
    uint32_t column;

    if (!graphics.initialized || width == 0u || height == 0u) return;
    ++graphics.stats.rectangles;

    if ((uint64_t)x >= graphics.framebuffer->width ||
        (uint64_t)y >= graphics.framebuffer->height) {
        graphics.stats.clipped_pixels += requested_pixels;
        return;
    }

    end_x = (uint64_t)x + width;
    end_y = (uint64_t)y + height;
    clipped_end_x = end_x < graphics.framebuffer->width
        ? end_x : graphics.framebuffer->width;
    clipped_end_y = end_y < graphics.framebuffer->height
        ? end_y : graphics.framebuffer->height;
    drawn_width = clipped_end_x - x;
    drawn_height = clipped_end_y - y;
    drawn_pixels = drawn_width * drawn_height;
    packed = pack_rgb(rgb);

    for (row = 0u; row < (uint32_t)drawn_height; ++row) {
        volatile uint32_t *target = pixel_address(x, y + row);
        for (column = 0u; column < (uint32_t)drawn_width; ++column) {
            target[column] = packed;
        }
    }

    graphics.stats.pixels += drawn_pixels;
    graphics.stats.clipped_pixels += requested_pixels - drawn_pixels;
}

void draw_bitmap(uint32_t x, uint32_t y, uint32_t width, uint32_t height, const uint32_t *pixels)
{
    uint32_t row;
    uint32_t column;

    if (!graphics.initialized || pixels == 0 || width == 0u || height == 0u) return;
    ++graphics.stats.bitmaps;
    for (row = 0u; row < height; ++row) {
        const uint64_t py = (uint64_t)y + row;
        for (column = 0u; column < width; ++column) {
            const uint64_t px = (uint64_t)x + column;
            if (px <= UINT32_MAX && py <= UINT32_MAX) {
                draw_pixel((uint32_t)px, (uint32_t)py,
                    pixels[(size_t)row * width + column]);
            } else {
                ++graphics.stats.clipped_pixels;
            }
            ++graphics.stats.bitmap_pixels;
        }
    }
}

static void draw_glyph(uint32_t x, uint32_t y, uint32_t rgb, uint8_t character)
{
    const uint8_t *glyph = graphics.font + ((size_t)character * graphics.font_height);
    uint32_t row;
    uint32_t column;

    for (row = 0u; row < graphics.font_height; ++row) {
        const uint8_t bits = glyph[row];
        for (column = 0u; column < graphics.font_width; ++column) {
            if ((bits & (uint8_t)(0x80u >> column)) != 0u) {
                const uint64_t px = (uint64_t)x + column;
                const uint64_t py = (uint64_t)y + row;
                if (px <= UINT32_MAX && py <= UINT32_MAX) {
                    draw_pixel((uint32_t)px, (uint32_t)py, rgb);
                } else {
                    ++graphics.stats.clipped_pixels;
                }
            }
        }
    }
}

void draw_text(uint32_t x, uint32_t y, uint32_t rgb, const char *text)
{
    uint32_t cursor_x = x;
    uint32_t cursor_y = y;

    if (!graphics.initialized || text == 0) return;
    ++graphics.stats.text_calls;
    while (*text != '\0') {
        if (*text == '\n') {
            cursor_x = x;
            if ((uint64_t)cursor_y + graphics.font_height <= UINT32_MAX) {
                cursor_y += graphics.font_height;
            } else {
                cursor_y = UINT32_MAX;
            }
        } else {
            draw_glyph(cursor_x, cursor_y, rgb, (uint8_t)*text);
            if ((uint64_t)cursor_x + graphics.font_width <= UINT32_MAX) {
                cursor_x += graphics.font_width;
            } else {
                cursor_x = UINT32_MAX;
            }
            ++graphics.stats.glyphs;
        }
        ++text;
    }
}


static void cursor_restore(void)
{
    uint32_t row;
    uint32_t column;

    if (!graphics.initialized || !graphics.cursor_visible) return;

    for (row = 0u; row < GRAPHICS_CURSOR_HEIGHT; ++row) {
        const uint32_t py = graphics.cursor_y + row;
        if ((uint64_t)py >= graphics.framebuffer->height) break;
        for (column = 0u; column < GRAPHICS_CURSOR_WIDTH; ++column) {
            const uint32_t px = graphics.cursor_x + column;
            if ((uint64_t)px >= graphics.framebuffer->width) break;
            *pixel_address(px, py) =
                graphics.cursor_under[(size_t)row * GRAPHICS_CURSOR_WIDTH + column];
        }
    }
}

static void cursor_save(uint32_t x, uint32_t y)
{
    uint32_t row;
    uint32_t column;

    for (row = 0u; row < GRAPHICS_CURSOR_HEIGHT; ++row) {
        const uint32_t py = y + row;
        for (column = 0u; column < GRAPHICS_CURSOR_WIDTH; ++column) {
            const uint32_t px = x + column;
            uint32_t value = 0u;
            if ((uint64_t)px < graphics.framebuffer->width &&
                (uint64_t)py < graphics.framebuffer->height) {
                value = *pixel_address(px, py);
            }
            graphics.cursor_under[(size_t)row * GRAPHICS_CURSOR_WIDTH + column] = value;
        }
    }
}

static void cursor_plot(uint32_t x, uint32_t y, uint32_t rgb)
{
    if (in_bounds(x, y)) *pixel_address(x, y) = pack_rgb(rgb);
}

static void cursor_draw(uint32_t x, uint32_t y)
{
    /* 12x19 arrow: black outline with white interior. */
    static const uint16_t black_rows[GRAPHICS_CURSOR_HEIGHT] = {
        0x001u, 0x003u, 0x007u, 0x00Fu, 0x01Fu, 0x03Fu, 0x07Fu,
        0x0FFu, 0x1FFu, 0x3FFu, 0x7FFu, 0x1FFu, 0x0EFu, 0x1C7u,
        0x383u, 0x301u, 0x200u, 0x000u, 0x000u
    };
    static const uint16_t white_rows[GRAPHICS_CURSOR_HEIGHT] = {
        0x000u, 0x001u, 0x003u, 0x007u, 0x00Fu, 0x01Fu, 0x03Fu,
        0x07Fu, 0x0FFu, 0x1FFu, 0x07Fu, 0x067u, 0x043u, 0x081u,
        0x100u, 0x000u, 0x000u, 0x000u, 0x000u
    };
    uint32_t row;
    uint32_t column;

    for (row = 0u; row < GRAPHICS_CURSOR_HEIGHT; ++row) {
        for (column = 0u; column < GRAPHICS_CURSOR_WIDTH; ++column) {
            const uint16_t bit = (uint16_t)(1u << column);
            if ((black_rows[row] & bit) != 0u) {
                cursor_plot(x + column, y + row, 0x020617u);
            }
        }
    }
    for (row = 0u; row < GRAPHICS_CURSOR_HEIGHT; ++row) {
        for (column = 0u; column < GRAPHICS_CURSOR_WIDTH; ++column) {
            const uint16_t bit = (uint16_t)(1u << column);
            if ((white_rows[row] & bit) != 0u) {
                cursor_plot(x + column, y + row, 0xF8FAFCu);
            }
        }
    }
}

void graphics_cursor_set(uint32_t x, uint32_t y, int visible)
{
    uint32_t max_x;
    uint32_t max_y;

    if (!graphics.initialized) return;

    cursor_restore();
    graphics.cursor_visible = 0;

    if (!visible) return;

    max_x = graphics.framebuffer->width > GRAPHICS_CURSOR_WIDTH
        ? (uint32_t)graphics.framebuffer->width - GRAPHICS_CURSOR_WIDTH : 0u;
    max_y = graphics.framebuffer->height > GRAPHICS_CURSOR_HEIGHT
        ? (uint32_t)graphics.framebuffer->height - GRAPHICS_CURSOR_HEIGHT : 0u;
    if (x > max_x) x = max_x;
    if (y > max_y) y = max_y;

    cursor_save(x, y);
    graphics.cursor_x = x;
    graphics.cursor_y = y;
    cursor_draw(x, y);
    graphics.cursor_visible = 1;
}

int graphics_selftest(void)
{
    volatile uint32_t *target;
    uint32_t original;
    const uint32_t probe_rgb = 0x12A4E6u;

    if (!graphics.initialized) return 0;
    target = pixel_address(0u, 0u);
    original = *target;
    draw_pixel(0u, 0u, probe_rgb);
    if (*target != pack_rgb(probe_rgb)) {
        *target = original;
        return 0;
    }
    *target = original;

    /* The Phase-26 software cursor must restore exactly what it covered. */
    graphics_cursor_set(0u, 0u, 1);
    graphics_cursor_set(0u, 0u, 0);
    if (*target != original) return 0;

    draw_pixel(UINT32_MAX, UINT32_MAX, probe_rgb);
    return graphics.stats.clipped_pixels != 0u;
}
