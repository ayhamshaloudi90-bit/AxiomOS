#ifndef AXIOM_DRIVERS_KEYBOARD_H
#define AXIOM_DRIVERS_KEYBOARD_H

#include <stddef.h>
#include <stdint.h>

#define KEYBOARD_BUFFER_CAPACITY 128u

struct keyboard_stats {
    uint64_t irq_count;
    uint64_t scancode_count;
    uint64_t character_count;
    uint64_t dropped_characters;
};

/* Initialise the first PS/2 port and register IRQ1. Call while IF=0. */
int keyboard_init(void);

/* Non-blocking character input. Returns 1 when a character was removed. */
int keyboard_read_char(char *character);

/* Number of decoded characters currently waiting in the ring buffer. */
size_t keyboard_pending(void);

struct keyboard_stats keyboard_get_stats(void);

#endif
