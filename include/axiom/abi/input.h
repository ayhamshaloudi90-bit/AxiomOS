#ifndef AXIOM_ABI_INPUT_H
#define AXIOM_ABI_INPUT_H

#ifndef __ASSEMBLER__
#include <stdint.h>

struct axiom_keyboard_stats {
    uint64_t irq_count;
    uint64_t scancode_count;
    uint64_t character_count;
    uint64_t dropped_characters;
};

/* Phase 26: cumulative PS/2 mouse state. x/y are relative-count accumulators. */
struct axiom_mouse_state {
    int64_t x;
    int64_t y;
    uint64_t irq_count;
    uint64_t packet_count;
    uint8_t buttons;
    uint8_t available;
    uint8_t reserved[6];
};
#endif

#endif
