#ifndef AXIOM_ARCH_INTERRUPTS_H
#define AXIOM_ARCH_INTERRUPTS_H

#include <stddef.h>
#include <stdint.h>

/* Exact layout made by isr_stubs.asm, followed by the long-mode CPU frame. */
struct interrupt_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

_Static_assert(sizeof(struct interrupt_frame) == 176, "ISR frame size");
_Static_assert(offsetof(struct interrupt_frame, vector) == 120, "ISR vector offset");
_Static_assert(offsetof(struct interrupt_frame, rip) == 136, "ISR RIP offset");
_Static_assert(offsetof(struct interrupt_frame, rsp) == 160, "ISR RSP offset");

typedef struct interrupt_frame *(*irq_handler_t)(struct interrupt_frame *frame);

void interrupts_init(void);
void interrupts_load_idt(void);
void interrupts_enable(void);
void interrupts_disable(void);
int interrupts_enabled(void);

int irq_register(uint8_t irq, irq_handler_t handler);

/* Phase 25 observability counters. */
uint64_t interrupt_count(uint8_t vector);
uint64_t interrupt_total_count(void);

/* Returns the frame/stack context that the ISR epilogue must restore. */
struct interrupt_frame *interrupt_dispatch(struct interrupt_frame *frame);
void phase3_selftest(void);

_Noreturn void exception_panic(const struct interrupt_frame *frame);
_Noreturn void page_fault_panic(const struct interrupt_frame *frame);

#endif
