#ifndef AXIOM_DRIVERS_TIMER_H
#define AXIOM_DRIVERS_TIMER_H

#include <stdint.h>

/* Initialise the local APIC periodic timer on vector 32. Call while IF=0. */
int timer_init(uint32_t frequency_hz);

/* Monotonic count of periodic vector-32 timer deliveries since timer_init(). */
uint64_t timer_ticks(void);

/* Configured Local APIC timer frequency. */
uint32_t timer_frequency(void);

/* Sleep this CPU until at least count more timer interrupts have arrived. */
void timer_wait_ticks(uint64_t count);

#endif
