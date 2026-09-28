#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <axiom/graphics.h>
#include <axiom/syscalls.h>

#define DESKTOP_POLL_MS 10u
#define DESKTOP_MOUSE_SCALE 2LL

#define COLOR_WALL_0 0x08111Fu
#define COLOR_WALL_1 0x0B1830u
#define COLOR_WALL_2 0x0E2040u
#define COLOR_WALL_3 0x122950u
#define COLOR_TOP    0x0A1020u
#define COLOR_TASK   0x111827u
#define COLOR_CARD   0x172033u
#define COLOR_CARD_H 0x1E2D49u
#define COLOR_BORDER 0x334155u
#define COLOR_ACCENT 0x55C2FFu
#define COLOR_ACCENT2 0x8B5CF6u
#define COLOR_WHITE  0xF8FAFCu
#define COLOR_MUTED  0x94A3B8u
#define COLOR_GREEN  0x4ADE80u
#define COLOR_BLACK  0x020617u

struct rect {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
};

enum launcher {
    LAUNCH_NONE = 0,
    LAUNCH_TERMINAL,
    LAUNCH_SYSTEM,
    LAUNCH_NETWORK,
    LAUNCH_GRAPHICS,
};

static size_t text_length(const char *text)
{
    return strlen(text);
}

static void console_write(const char *text)
{
    (void)write(1, text, text_length(text));
}

static int contains(const struct rect *r, uint32_t x, uint32_t y)
{
    return r != 0 && x >= r->x && y >= r->y &&
        (uint64_t)x < (uint64_t)r->x + r->w &&
        (uint64_t)y < (uint64_t)r->y + r->h;
}

static void frame(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    if (w < 2u || h < 2u) return;
    (void)axiom_draw_rectangle(x, y, w, 2u, color);
    (void)axiom_draw_rectangle(x, y + h - 2u, w, 2u, color);
    (void)axiom_draw_rectangle(x, y, 2u, h, color);
    (void)axiom_draw_rectangle(x + w - 2u, y, 2u, h, color);
}

static void draw_logo(uint32_t x, uint32_t y)
{
    (void)axiom_draw_rectangle(x, y, 34u, 34u, COLOR_ACCENT);
    (void)axiom_draw_rectangle(x + 4u, y + 4u, 26u, 26u, COLOR_TOP);
    (void)axiom_draw_line(x + 9u, y + 25u, x + 17u, y + 8u, COLOR_WHITE);
    (void)axiom_draw_line(x + 17u, y + 8u, x + 26u, y + 25u, COLOR_WHITE);
    (void)axiom_draw_line(x + 12u, y + 20u, x + 23u, y + 20u, COLOR_WHITE);
}

static void draw_terminal_icon(uint32_t x, uint32_t y)
{
    (void)axiom_draw_rectangle(x, y, 48u, 38u, COLOR_BLACK);
    frame(x, y, 48u, 38u, COLOR_ACCENT);
    (void)axiom_draw_line(x + 9u, y + 11u, x + 17u, y + 19u, COLOR_GREEN);
    (void)axiom_draw_line(x + 17u, y + 19u, x + 9u, y + 27u, COLOR_GREEN);
    (void)axiom_draw_line(x + 23u, y + 27u, x + 36u, y + 27u, COLOR_WHITE);
}

static void draw_system_icon(uint32_t x, uint32_t y)
{
    (void)axiom_draw_rectangle(x + 3u, y + 3u, 42u, 30u, 0x0F172Au);
    frame(x + 3u, y + 3u, 42u, 30u, COLOR_ACCENT2);
    (void)axiom_draw_rectangle(x + 12u, y + 11u, 24u, 3u, COLOR_WHITE);
    (void)axiom_draw_rectangle(x + 12u, y + 18u, 18u, 3u, COLOR_MUTED);
    (void)axiom_draw_rectangle(x + 12u, y + 25u, 27u, 3u, COLOR_GREEN);
}

static void draw_network_icon(uint32_t x, uint32_t y)
{
    (void)axiom_draw_rectangle(x + 20u, y + 4u, 8u, 8u, COLOR_ACCENT);
    (void)axiom_draw_rectangle(x + 4u, y + 27u, 8u, 8u, COLOR_GREEN);
    (void)axiom_draw_rectangle(x + 36u, y + 27u, 8u, 8u, COLOR_GREEN);
    (void)axiom_draw_line(x + 24u, y + 12u, x + 8u, y + 27u, COLOR_WHITE);
    (void)axiom_draw_line(x + 24u, y + 12u, x + 40u, y + 27u, COLOR_WHITE);
    (void)axiom_draw_line(x + 8u, y + 31u, x + 40u, y + 31u, COLOR_MUTED);
}

static void draw_graphics_icon(uint32_t x, uint32_t y)
{
    (void)axiom_draw_rectangle(x + 3u, y + 3u, 42u, 34u, 0x0F172Au);
    frame(x + 3u, y + 3u, 42u, 34u, COLOR_ACCENT2);
    (void)axiom_draw_line(x + 8u, y + 30u, x + 18u, y + 19u, COLOR_GREEN);
    (void)axiom_draw_line(x + 18u, y + 19u, x + 25u, y + 25u, COLOR_GREEN);
    (void)axiom_draw_line(x + 25u, y + 25u, x + 36u, y + 12u, COLOR_ACCENT);
    (void)axiom_draw_rectangle(x + 32u, y + 8u, 5u, 5u, 0xFDE047u);
}

static void layout(
    const struct axiom_gfx_info *info,
    struct rect *terminal,
    struct rect *system,
    struct rect *network,
    struct rect *graphics,
    struct rect *task_terminal
)
{
    uint32_t card_w = 220u;
    uint32_t card_h = 120u;
    uint32_t gap = 24u;
    uint32_t left = 54u;
    uint32_t top = 110u;

    if (info->width < 900u) {
        card_w = (info->width > 120u) ? (info->width - 108u) / 2u : 120u;
        if (card_w > 220u) card_w = 220u;
    }

    *terminal = (struct rect){left, top, card_w, card_h};
    *system = (struct rect){left + card_w + gap, top, card_w, card_h};
    *network = (struct rect){left, top + card_h + gap, card_w, card_h};
    *graphics = (struct rect){left + card_w + gap, top + card_h + gap, card_w, card_h};
    *task_terminal = (struct rect){78u, info->height > 50u ? info->height - 46u : 0u, 150u, 34u};
}

static enum launcher hovered_launcher(
    const struct axiom_gfx_info *info,
    uint32_t x,
    uint32_t y
)
{
    struct rect terminal, system, network, graphics, task_terminal;
    layout(info, &terminal, &system, &network, &graphics, &task_terminal);
    if (contains(&terminal, x, y) || contains(&task_terminal, x, y)) return LAUNCH_TERMINAL;
    if (contains(&system, x, y)) return LAUNCH_SYSTEM;
    if (contains(&network, x, y)) return LAUNCH_NETWORK;
    if (contains(&graphics, x, y)) return LAUNCH_GRAPHICS;
    return LAUNCH_NONE;
}

static void draw_card(
    const struct rect *r,
    enum launcher id,
    enum launcher hover,
    const char *title,
    const char *subtitle
)
{
    const uint32_t fill = (id == hover) ? COLOR_CARD_H : COLOR_CARD;
    const uint32_t border = (id == hover) ? COLOR_ACCENT : COLOR_BORDER;

    /* Shadow, body and accent strip. */
    (void)axiom_draw_rectangle(r->x + 6u, r->y + 7u, r->w, r->h, 0x050A14u);
    (void)axiom_draw_rectangle(r->x, r->y, r->w, r->h, fill);
    frame(r->x, r->y, r->w, r->h, border);
    (void)axiom_draw_rectangle(r->x, r->y, 5u, r->h, id == LAUNCH_TERMINAL ? COLOR_ACCENT : COLOR_ACCENT2);

    if (id == LAUNCH_TERMINAL) draw_terminal_icon(r->x + 22u, r->y + 20u);
    else if (id == LAUNCH_SYSTEM) draw_system_icon(r->x + 22u, r->y + 20u);
    else if (id == LAUNCH_NETWORK) draw_network_icon(r->x + 22u, r->y + 20u);
    else draw_graphics_icon(r->x + 22u, r->y + 20u);

    (void)axiom_draw_text(r->x + 82u, r->y + 23u, COLOR_WHITE, title);
    (void)axiom_draw_text(r->x + 82u, r->y + 47u, COLOR_MUTED, subtitle);
}

static void draw_desktop(
    const struct axiom_gfx_info *info,
    uint32_t cursor_x,
    uint32_t cursor_y,
    int mouse_online
)
{
    static const uint32_t bands[] = {
        COLOR_WALL_0, COLOR_WALL_0, COLOR_WALL_1, COLOR_WALL_1,
        COLOR_WALL_2, COLOR_WALL_2, COLOR_WALL_3, COLOR_WALL_2,
        COLOR_WALL_1, COLOR_WALL_0
    };
    struct rect terminal, system, network, graphics, task_terminal;
    enum launcher hover = mouse_online ? hovered_launcher(info, cursor_x, cursor_y) : LAUNCH_NONE;
    uint32_t band_h = info->height / (uint32_t)(sizeof(bands) / sizeof(bands[0]));
    uint32_t i;

    if (band_h == 0u) band_h = 1u;
    for (i = 0u; i < (uint32_t)(sizeof(bands) / sizeof(bands[0])); ++i) {
        const uint32_t y = i * band_h;
        uint32_t h = band_h + 1u;
        if ((uint64_t)y + h > info->height) h = info->height - y;
        (void)axiom_draw_rectangle(0u, y, info->width, h, bands[i]);
    }

    /* Subtle wallpaper geometry. */
    if (info->width > 640u && info->height > 420u) {
        (void)axiom_draw_line(info->width - 360u, 70u, info->width - 80u, 350u, 0x173A62u);
        (void)axiom_draw_line(info->width - 320u, 70u, info->width - 40u, 350u, 0x152F55u);
        (void)axiom_draw_line(info->width - 360u, 350u, info->width - 80u, 70u, 0x1A335Bu);
    }

    /* Top bar. */
    (void)axiom_draw_rectangle(0u, 0u, info->width, 54u, COLOR_TOP);
    (void)axiom_draw_rectangle(0u, 52u, info->width, 2u, 0x21324Bu);
    draw_logo(18u, 10u);
    (void)axiom_draw_text(64u, 13u, COLOR_WHITE, "AxiomOS");
    (void)axiom_draw_text(64u, 31u, COLOR_MUTED, "Desktop");
    if (info->width > 450u) {
        (void)axiom_draw_text(info->width - 190u, 20u, COLOR_MUTED, "PHASE 26  |  GUI");
    }

    (void)axiom_draw_text(54u, 73u, COLOR_WHITE, "Welcome to AxiomOS");
    (void)axiom_draw_text(54u, 91u, COLOR_MUTED, "Launch a tool, or open Terminal for the full shell.");

    layout(info, &terminal, &system, &network, &graphics, &task_terminal);
    draw_card(&terminal, LAUNCH_TERMINAL, hover, "Terminal", "Full AxiomOS shell");
    draw_card(&system, LAUNCH_SYSTEM, hover, "System", "Live /proc telemetry");
    draw_card(&network, LAUNCH_NETWORK, hover, "Network", "E1000 + IPv4 status");
    draw_card(&graphics, LAUNCH_GRAPHICS, hover, "Graphics", "Framebuffer demo");

    /* Bottom taskbar. */
    if (info->height >= 58u) {
        const uint32_t y = info->height - 58u;
        (void)axiom_draw_rectangle(0u, y, info->width, 58u, COLOR_TASK);
        (void)axiom_draw_rectangle(0u, y, info->width, 2u, 0x263A55u);
        draw_logo(24u, y + 12u);
        (void)axiom_draw_rectangle(task_terminal.x, task_terminal.y, task_terminal.w, task_terminal.h,
            hover == LAUNCH_TERMINAL ? 0x243B5Au : 0x1B2638u);
        frame(task_terminal.x, task_terminal.y, task_terminal.w, task_terminal.h,
            hover == LAUNCH_TERMINAL ? COLOR_ACCENT : COLOR_BORDER);
        (void)axiom_draw_text(task_terminal.x + 15u, task_terminal.y + 9u, COLOR_WHITE, ">_  Terminal");
        if (info->width > 620u) {
            (void)axiom_draw_text(info->width - 355u, y + 20u, COLOR_MUTED,
                mouse_online ? "Mouse online | T Terminal | Q Quit" : "Keyboard | T Terminal | Q Quit");
        }
    }

}

static int64_t scaled_mouse_delta(int64_t delta)
{
    /* QEMU PS/2 motion is deliberately conservative; 2x feels much closer
     * to the host pointer while remaining predictable on a touchpad. */
    return delta * DESKTOP_MOUSE_SCALE;
}

static void wait_for_return_key(void)
{
    console_write("\nPress Enter to return to the AxiomOS desktop...\n");
    for (;;) {
        char ch = '\0';
        const long got = read(0, &ch, 1u);
        if (got > 0 && (ch == '\n' || ch == '\r' || ch == 'q' || ch == 'Q')) return;
        (void)sleep(10u);
    }
}

static void launch_child(const char *path, int pause_after)
{
    long status = 0;
    long pid;

    (void)axiom_clear();
    pid = axiom_spawn(path);
    if (pid < 0) {
        console_write("desktop: failed to launch application\n");
        wait_for_return_key();
        return;
    }
    (void)waitpid(pid, &status);
    if (pause_after) wait_for_return_key();
}

static void activate(enum launcher which)
{
    switch (which) {
        case LAUNCH_TERMINAL:
            console_write("Phase 26 desktop launching terminal. Type 'exit' to return.\n");
            launch_child("/bin/axiomsh", 0);
            console_write("Phase 26 terminal returned to desktop.\n");
            break;
        case LAUNCH_SYSTEM:
            launch_child("/bin/sysinfo", 1);
            break;
        case LAUNCH_NETWORK:
            launch_child("/bin/ifconfig", 1);
            break;
        case LAUNCH_GRAPHICS:
            launch_child("/bin/gfxdemo", 1);
            break;
        default:
            break;
    }
}

int main(void)
{
    struct axiom_gfx_info info;
    struct axiom_mouse_state mouse;
    int mouse_online = 0;
    int64_t mouse_x_last = 0;
    int64_t mouse_y_last = 0;
    uint8_t buttons_last = 0u;
    uint32_t cursor_x;
    uint32_t cursor_y;
    enum launcher hover_last = LAUNCH_NONE;

    if (axiom_gfx_info(&info) < 0 || info.width < 320u || info.height < 240u) {
        console_write("Phase 26 desktop: framebuffer unavailable.\n");
        return 1;
    }

    cursor_x = info.width / 2u;
    cursor_y = info.height / 2u;

    if (axiom_mousestate(&mouse) == 0 && mouse.available != 0u) {
        mouse_online = 1;
        mouse_x_last = mouse.x;
        mouse_y_last = mouse.y;
        buttons_last = mouse.buttons;
    }

    console_write("AxiomOS Phase 26 desktop ready.\n");
    console_write("Desktop controls: T/Enter Terminal, S System, N Network, G Graphics, Q Quit.\n");
    console_write(mouse_online ? "Phase 26 PS/2 mouse input: ONLINE\n" : "Phase 26 PS/2 mouse input: unavailable; keyboard fallback active.\n");
    draw_desktop(&info, cursor_x, cursor_y, mouse_online);
    if (mouse_online) {
        hover_last = hovered_launcher(&info, cursor_x, cursor_y);
        (void)axiom_gfx_cursor(cursor_x, cursor_y, 1);
    }

    for (;;) {
        char key = '\0';
        long got = read(0, &key, 1u);
        enum launcher requested = LAUNCH_NONE;
        int cursor_moved = 0;
        int hover_changed = 0;

        if (got > 0) {
            if (key == 't' || key == 'T' || key == '\n' || key == '\r') requested = LAUNCH_TERMINAL;
            else if (key == 's' || key == 'S') requested = LAUNCH_SYSTEM;
            else if (key == 'n' || key == 'N') requested = LAUNCH_NETWORK;
            else if (key == 'g' || key == 'G') requested = LAUNCH_GRAPHICS;
            else if (key == 'q' || key == 'Q') {
                if (mouse_online) (void)axiom_gfx_cursor(cursor_x, cursor_y, 0);
                (void)axiom_clear();
                console_write("Phase 26 desktop exited to shell.\n");
                return 0;
            }
        }

        if (mouse_online && axiom_mousestate(&mouse) == 0 && mouse.available != 0u) {
            const int64_t dx = mouse.x - mouse_x_last;
            const int64_t dy = mouse.y - mouse_y_last;
            const uint8_t pressed = (uint8_t)((mouse.buttons & 1u) != 0u && (buttons_last & 1u) == 0u);

            mouse_x_last = mouse.x;
            mouse_y_last = mouse.y;

            if (dx != 0 || dy != 0) {
                int64_t nx = (int64_t)cursor_x + scaled_mouse_delta(dx);
                int64_t ny = (int64_t)cursor_y + scaled_mouse_delta(dy);
                enum launcher hover_now;
                if (nx < 0) nx = 0;
                if (ny < 0) ny = 0;
                if (nx >= (int64_t)info.width) nx = (int64_t)info.width - 1;
                if (ny >= (int64_t)info.height) ny = (int64_t)info.height - 1;
                cursor_x = (uint32_t)nx;
                cursor_y = (uint32_t)ny;
                cursor_moved = 1;
                hover_now = hovered_launcher(&info, cursor_x, cursor_y);
                hover_changed = hover_now != hover_last;
                hover_last = hover_now;
            }

            if (pressed != 0u) requested = hovered_launcher(&info, cursor_x, cursor_y);
            buttons_last = mouse.buttons;
        }

        if (requested != LAUNCH_NONE) {
            if (mouse_online) (void)axiom_gfx_cursor(cursor_x, cursor_y, 0);
            activate(requested);
            draw_desktop(&info, cursor_x, cursor_y, mouse_online);
            if (mouse_online) (void)axiom_gfx_cursor(cursor_x, cursor_y, 1);
        } else if (hover_changed) {
            /* Hover changes are rare, so repaint the cards only at boundaries.
             * Ordinary pointer motion uses the tiny save-under cursor below. */
            if (mouse_online) (void)axiom_gfx_cursor(cursor_x, cursor_y, 0);
            draw_desktop(&info, cursor_x, cursor_y, mouse_online);
            if (mouse_online) (void)axiom_gfx_cursor(cursor_x, cursor_y, 1);
        } else if (cursor_moved && mouse_online) {
            (void)axiom_gfx_cursor(cursor_x, cursor_y, 1);
        }

        (void)sleep(DESKTOP_POLL_MS);
    }
}
