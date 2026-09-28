#include <stddef.h>
#include <stdint.h>

#include <axiom/boot/limine.h>
#include <axiom/drivers/serial.h>
#include <axiom/terminal/terminal.h>


#define TERMINAL_MARGIN_PIXELS 8ULL
#define TERMINAL_TAB_WIDTH     4ULL


struct terminal_state {
    struct limine_framebuffer *framebuffer;

    const uint8_t *font;

    uint64_t font_width;
    uint64_t font_height;
    uint64_t font_spacing;

    uint64_t cell_width;

    uint64_t margin;

    size_t columns;
    size_t rows;

    size_t column;
    size_t row;

    uint32_t foreground;
    uint32_t background;

    int initialized;
};


static struct terminal_state terminal;


static uint32_t scale_channel(
    uint8_t value,
    uint8_t bits
)
{
    if (bits == 0) {
        return 0;
    }


    if (bits >= 8) {
        return (uint32_t)value;
    }


    return (uint32_t)(
        value >> (8u - bits)
    );
}


static uint32_t pack_rgb(uint32_t rgb)
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;


    red = (uint8_t)(
        (rgb >> 16) & 0xFFu
    );

    green = (uint8_t)(
        (rgb >> 8) & 0xFFu
    );

    blue = (uint8_t)(
        rgb & 0xFFu
    );


    return
        (
            scale_channel(
                red,
                terminal.framebuffer->red_mask_size
            )
            << terminal.framebuffer->red_mask_shift
        )
        |
        (
            scale_channel(
                green,
                terminal.framebuffer->green_mask_size
            )
            << terminal.framebuffer->green_mask_shift
        )
        |
        (
            scale_channel(
                blue,
                terminal.framebuffer->blue_mask_size
            )
            << terminal.framebuffer->blue_mask_shift
        );
}


static void put_pixel(
    uint64_t x,
    uint64_t y,
    uint32_t color
)
{
    volatile uint32_t *row;


    if (x >= terminal.framebuffer->width ||
        y >= terminal.framebuffer->height) {

        return;
    }


    row = (volatile uint32_t *)(
        (uint8_t *)terminal.framebuffer->address
        +
        (y * terminal.framebuffer->pitch)
    );


    row[x] = color;
}


static void fill_rectangle(
    uint64_t x,
    uint64_t y,
    uint64_t width,
    uint64_t height,
    uint32_t color
)
{
    uint64_t current_y;
    uint64_t current_x;


    for (
        current_y = 0;
        current_y < height;
        ++current_y
    ) {

        for (
            current_x = 0;
            current_x < width;
            ++current_x
        ) {

            put_pixel(
                x + current_x,
                y + current_y,
                color
            );
        }
    }
}


static void draw_character(
    char character,
    size_t column,
    size_t row
)
{
    const uint8_t *glyph;

    uint64_t origin_x;
    uint64_t origin_y;

    uint64_t y;
    uint64_t x;


    /*
     * The Limine-provided font contains 256 glyphs.
     *
     * Every glyph occupies font_height bytes because the
     * font width is eight pixels.
     */
    glyph =
        terminal.font
        +
        (
            (uint64_t)(uint8_t)character
            *
            terminal.font_height
        );


    origin_x =
        terminal.margin
        +
        (
            (uint64_t)column
            *
            terminal.cell_width
        );


    origin_y =
        terminal.margin
        +
        (
            (uint64_t)row
            *
            terminal.font_height
        );


    for (y = 0; y < terminal.font_height; ++y) {
        uint8_t bits;


        bits = glyph[y];


        for (x = 0; x < terminal.font_width; ++x) {
            uint8_t mask;

            uint32_t color;


            mask = (uint8_t)(
                0x80u >> x
            );


            color =
                (bits & mask) != 0u
                    ? terminal.foreground
                    : terminal.background;


            put_pixel(
                origin_x + x,
                origin_y + y,
                color
            );
        }


        /*
         * Clear the spacing pixels after the glyph.
         */
        for (
            x = terminal.font_width;
            x < terminal.cell_width;
            ++x
        ) {

            put_pixel(
                origin_x + x,
                origin_y + y,
                terminal.background
            );
        }
    }
}


static void scroll_one_line(void)
{
    uint64_t text_width;
    uint64_t text_height;

    uint64_t y;
    uint64_t x;


    text_width =
        (uint64_t)terminal.columns
        *
        terminal.cell_width;


    text_height =
        (uint64_t)terminal.rows
        *
        terminal.font_height;


    /*
     * Copy every pixel row upward by exactly one
     * character height.
     */
    for (
        y = terminal.font_height;
        y < text_height;
        ++y
    ) {

        volatile uint32_t *source;
        volatile uint32_t *destination;


        source = (volatile uint32_t *)(
            (uint8_t *)terminal.framebuffer->address
            +
            (
                (terminal.margin + y)
                *
                terminal.framebuffer->pitch
            )
        );


        destination = (volatile uint32_t *)(
            (uint8_t *)terminal.framebuffer->address
            +
            (
                (
                    terminal.margin
                    +
                    y
                    -
                    terminal.font_height
                )
                *
                terminal.framebuffer->pitch
            )
        );


        for (x = 0; x < text_width; ++x) {
            destination[terminal.margin + x] =
                source[terminal.margin + x];
        }
    }


    /*
     * The last row is now stale.
     * Paint it with the background color.
     */
    fill_rectangle(
        terminal.margin,

        terminal.margin
            +
            text_height
            -
            terminal.font_height,

        text_width,

        terminal.font_height,

        terminal.background
    );


    terminal.row =
        terminal.rows - 1u;
}


static void newline(void)
{
    terminal.column = 0;

    ++terminal.row;


    if (terminal.row >= terminal.rows) {
        scroll_one_line();
    }
}


int terminal_init(void)
{
    struct limine_framebuffer *framebuffer;

    struct limine_flanterm_fb_init_params *font_params;

    uint64_t usable_width;
    uint64_t usable_height;


    if (!limine_get_terminal_boot_info(
            &framebuffer,
            &font_params
        )) {

        return 0;
    }


    /*
     * This first renderer supports a normal 32-bit RGB
     * framebuffer.
     */
    if (framebuffer->address == 0 ||
        framebuffer->bpp != 32u ||
        framebuffer->memory_model !=
            LIMINE_FRAMEBUFFER_RGB) {

        return 0;
    }


    /*
     * Limine describes this as a VGA-style font.
     * VGA fonts are 8 pixels wide.
     */
    if (font_params->font == 0 ||
        font_params->font_width != 8u ||
        font_params->font_height == 0u ||
        font_params->rotation != 0u) {

        return 0;
    }


    terminal.framebuffer =
        framebuffer;


    terminal.font =
        (const uint8_t *)font_params->font;


    terminal.font_width =
        font_params->font_width;


    terminal.font_height =
        font_params->font_height;


    terminal.font_spacing =
        font_params->font_spacing;


    terminal.cell_width =
        terminal.font_width
        +
        terminal.font_spacing;


    terminal.margin =
        TERMINAL_MARGIN_PIXELS;


    if (terminal.cell_width == 0u ||
        framebuffer->width <=
            (terminal.margin * 2u) ||
        framebuffer->height <=
            (terminal.margin * 2u)) {

        return 0;
    }


    usable_width =
        framebuffer->width
        -
        (terminal.margin * 2u);


    usable_height =
        framebuffer->height
        -
        (terminal.margin * 2u);


    terminal.columns =
        (size_t)(
            usable_width
            /
            terminal.cell_width
        );


    terminal.rows =
        (size_t)(
            usable_height
            /
            terminal.font_height
        );


    if (terminal.columns == 0u ||
        terminal.rows == 0u) {

        return 0;
    }


    terminal.column = 0;
    terminal.row = 0;

    terminal.initialized = 1;


    terminal.foreground =
        pack_rgb(
            TERMINAL_COLOR_LIGHT_GREY
        );


    terminal.background =
        pack_rgb(
            TERMINAL_COLOR_BLACK
        );


    terminal_clear();


    return 1;
}


void terminal_clear(void)
{
    if (!terminal.initialized) {
        return;
    }


    fill_rectangle(
        0,
        0,

        terminal.framebuffer->width,
        terminal.framebuffer->height,

        terminal.background
    );


    terminal.column = 0;
    terminal.row = 0;
}


void terminal_set_color(
    enum terminal_color foreground,
    enum terminal_color background
)
{
    if (!terminal.initialized) {
        return;
    }


    terminal.foreground =
        pack_rgb(
            (uint32_t)foreground
        );


    terminal.background =
        pack_rgb(
            (uint32_t)background
        );
}


void terminal_putchar(char character)
{
    size_t spaces;


    /*
     * A tab is implemented as spaces so both framebuffer
     * and serial output behave consistently.
     */
    if (character == '\t') {
        spaces =
            TERMINAL_TAB_WIDTH
            -
            (
                terminal.column
                %
                TERMINAL_TAB_WIDTH
            );


        while (spaces-- > 0u) {
            terminal_putchar(' ');
        }


        return;
    }


    /*
     * Backspace needs to erase the previous cell rather than draw glyph 0x08.
     * Mirror the conventional "backspace, space, backspace" sequence to the
     * serial console so interactive use looks sensible there as well.
     */
    if (character == '\b') {
        serial_write_string("\b \b");

        if (!terminal.initialized) {
            return;
        }

        if (terminal.column > 0u) {
            --terminal.column;
        } else if (terminal.row > 0u) {
            --terminal.row;
            terminal.column = terminal.columns - 1u;
        } else {
            return;
        }

        draw_character(
            ' ',
            terminal.column,
            terminal.row
        );

        return;
    }


    /*
     * Mirror every character to our Phase-1 serial driver.
     */
    serial_write_char(character);


    if (!terminal.initialized) {
        return;
    }


    switch (character) {
        case '\n':
            newline();
            return;


        case '\r':
            terminal.column = 0;
            return;


        default:
            break;
    }


    draw_character(
        character,
        terminal.column,
        terminal.row
    );


    ++terminal.column;


    if (terminal.column >= terminal.columns) {
        newline();
    }
}


void terminal_write(const char *string)
{
    if (string == 0) {
        terminal_write("(null)");
        return;
    }


    while (*string != '\0') {
        terminal_putchar(*string);

        ++string;
    }
}


size_t terminal_columns(void)
{
    return terminal.columns;
}


size_t terminal_rows(void)
{
    return terminal.rows;
}
