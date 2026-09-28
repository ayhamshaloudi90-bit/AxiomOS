#ifndef AXIOM_TERMINAL_TERMINAL_H
#define AXIOM_TERMINAL_TERMINAL_H

#include <stddef.h>
#include <stdint.h>


enum terminal_color {
    TERMINAL_COLOR_BLACK =
        0x000000,

    TERMINAL_COLOR_BLUE =
        0x0000AA,

    TERMINAL_COLOR_GREEN =
        0x00AA00,

    TERMINAL_COLOR_CYAN =
        0x00AAAA,

    TERMINAL_COLOR_RED =
        0xAA0000,

    TERMINAL_COLOR_MAGENTA =
        0xAA00AA,

    TERMINAL_COLOR_BROWN =
        0xAA5500,

    TERMINAL_COLOR_LIGHT_GREY =
        0xAAAAAA,

    TERMINAL_COLOR_DARK_GREY =
        0x555555,

    TERMINAL_COLOR_LIGHT_BLUE =
        0x5555FF,

    TERMINAL_COLOR_LIGHT_GREEN =
        0x55FF55,

    TERMINAL_COLOR_LIGHT_CYAN =
        0x55FFFF,

    TERMINAL_COLOR_LIGHT_RED =
        0xFF5555,

    TERMINAL_COLOR_LIGHT_MAGENTA =
        0xFF55FF,

    TERMINAL_COLOR_YELLOW =
        0xFFFF55,

    TERMINAL_COLOR_WHITE =
        0xFFFFFF
};


int terminal_init(void);


void terminal_clear(void);


void terminal_set_color(
    enum terminal_color foreground,
    enum terminal_color background
);


void terminal_putchar(char character);


void terminal_write(const char *string);


size_t terminal_columns(void);


size_t terminal_rows(void);


#endif
