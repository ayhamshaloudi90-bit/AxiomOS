#ifndef AXIOM_ARCH_PIC_H
#define AXIOM_ARCH_PIC_H

#include <stdint.h>

void pic_init_masked(void);
int pic_mask_irq(uint8_t irq);
int pic_unmask_irq(uint8_t irq);
int pic_is_spurious(uint8_t irq);
void pic_eoi(uint8_t irq);

#endif
