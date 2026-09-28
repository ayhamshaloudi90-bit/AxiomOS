#ifndef AXIOM_ARCH_GDT_H
#define AXIOM_ARCH_GDT_H

#include <stdint.h>

#define GDT_KERNEL_CODE 0x08u
#define GDT_KERNEL_DATA 0x10u
#define GDT_USER_DATA   0x1Bu
#define GDT_USER_CODE   0x23u
#define GDT_TSS_SELECTOR 0x28u

void gdt_init(void);
int gdt_init_secondary(uint32_t cpu_index, uintptr_t stack_top);
void gdt_set_kernel_stack(uintptr_t stack_top);
uintptr_t gdt_kernel_stack(void);
int gdt_ist_contains(uint8_t index, uintptr_t address);

#endif
