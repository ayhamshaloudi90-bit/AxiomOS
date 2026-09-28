#ifndef AXIOM_DRIVERS_MOUSE_H
#define AXIOM_DRIVERS_MOUSE_H

#include <stdint.h>

struct mouse_state {
    int64_t x;
    int64_t y;
    uint64_t irq_count;
    uint64_t packet_count;
    uint8_t buttons;
    uint8_t available;
};

/* Best-effort PS/2 second-port mouse setup. Must be called with IF=0. */
int mouse_init(void);
int mouse_available(void);
struct mouse_state mouse_get_state(void);

#endif
