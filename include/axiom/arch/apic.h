#ifndef AXIOM_ARCH_APIC_H
#define AXIOM_ARCH_APIC_H

#include <stdint.h>

/*
 * Phase 7 uses the xAPIC MMIO interface on the q35 machine model.
 * The local APIC handles the periodic timer; the I/O APIC routes ISA IRQs.
 */
int apic_init(void);
int apic_active(void);
uint8_t apic_local_id(void);

/* Route one legacy ISA IRQ/GSI to an IDT vector on the bootstrap CPU. */
int apic_route_isa_irq(uint8_t irq, uint8_t vector);
int apic_mask_isa_irq(uint8_t irq);

/* Signal completion of a non-spurious local/I/O APIC interrupt. */
void apic_eoi(void);

/* Configure the local APIC timer in periodic mode. */
int apic_timer_start(uint8_t vector, uint32_t frequency_hz);
uint32_t apic_timer_frequency(void);

#endif
