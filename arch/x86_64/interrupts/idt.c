#include <stdint.h>

#include <axiom/arch/apic.h>
#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/arch/pic.h>
#include <axiom/process/scheduler.h>
#include <axiom/terminal/kprintf.h>

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t attributes;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

_Static_assert(sizeof(struct idt_entry) == 16, "IDT gate size");
_Static_assert(sizeof(struct idt_pointer) == 10, "IDTR size");

static struct idt_entry idt[256] __attribute__((aligned(16)));
static irq_handler_t irq_handlers[16];
static uint64_t vector_counts[256];
static uint64_t total_interrupt_count;

extern void (*const isr_stub_table[256])(void);

static uint64_t read_cr2(void)
{
    uint64_t value;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(value));
    return value;
}

void interrupts_load_idt(void)
{
    const struct idt_pointer pointer = {
        .limit = sizeof(idt) - 1u,
        .base = (uint64_t)(uintptr_t)idt
    };

    __asm__ volatile ("lidt %0" : : "m"(pointer) : "memory");
}


void interrupts_init(void)
{
    uint16_t vector;
    __asm__ volatile ("cli" ::: "memory");

    total_interrupt_count = 0u;
    for (vector = 0; vector < 256; ++vector) {
        vector_counts[vector] = 0u;
        const uintptr_t address = (uintptr_t)isr_stub_table[vector];
        struct idt_entry *entry = &idt[vector];

        entry->offset_low = (uint16_t)address;
        entry->selector = GDT_KERNEL_CODE;
        entry->ist =
            vector == 8 ? 1 :
            vector == 2 ? 2 :
            vector == 18 ? 3 :
            0;
        entry->attributes = 0x8E;
        entry->offset_middle = (uint16_t)(address >> 16);
        entry->offset_high = (uint32_t)(address >> 32);
        entry->reserved = 0;
    }

    interrupts_load_idt();
    pic_init_masked();
}

void interrupts_enable(void)
{
    __asm__ volatile ("sti" ::: "memory");
}

void interrupts_disable(void)
{
    __asm__ volatile ("cli" ::: "memory");
}

int interrupts_enabled(void)
{
    uint64_t flags;
    __asm__ volatile ("pushfq; popq %0" : "=r"(flags));
    return (flags & (1ULL << 9)) != 0ULL;
}

int irq_register(uint8_t irq, irq_handler_t handler)
{
    uint64_t flags;

    __asm__ volatile ("pushfq; popq %0" : "=r"(flags));

    if (irq >= 16u || handler == 0 || (flags & (1ULL << 9)) != 0ULL) {
        return -1;
    }

    irq_handlers[irq] = handler;
    return 0;
}

struct interrupt_frame *interrupt_dispatch(struct interrupt_frame *frame)
{
    if (frame != 0) {
        ++vector_counts[(uint8_t)frame->vector];
        ++total_interrupt_count;
    }

    if (frame->vector == 3u) {
        kprintf("Breakpoint: resumed safely.\n");
        return frame;
    }

    if (frame->vector == 14u) {
        if ((frame->cs & 3ULL) == 3ULL && scheduler_running()) {
            struct interrupt_frame *next = scheduler_handle_user_fault(
                frame,
                14u,
                frame->error_code,
                read_cr2()
            );

            if (next != 0) {
                return next;
            }
        }

        page_fault_panic(frame);
    }

    if (frame->vector < 32u) {
        exception_panic(frame);
    }

    if (frame->vector < 48u) {
        const uint8_t irq = (uint8_t)(frame->vector - 32u);
        struct interrupt_frame *resume = frame;

        if (apic_active()) {
            if (irq_handlers[irq] != 0) {
                resume = irq_handlers[irq](frame);
                if (resume == 0) {
                    resume = frame;
                }
            }

            apic_eoi();
            return resume;
        }

        if (pic_is_spurious(irq)) {
            return frame;
        }

        if (irq_handlers[irq] != 0) {
            resume = irq_handlers[irq](frame);
            if (resume == 0) {
                resume = frame;
            }
        }

        pic_eoi(irq);
        return resume;
    }

    if (frame->vector == 0x80u) {
        kprintf("Software interrupt 0x80: returned safely.\n");
        return frame;
    }

    if (frame->vector == 0xFFu) {
        return frame;
    }

    exception_panic(frame);
}


uint64_t interrupt_count(uint8_t vector)
{
    return vector_counts[vector];
}

uint64_t interrupt_total_count(void)
{
    return total_interrupt_count;
}

#ifdef AXIOM_TEST_DOUBLE_FAULT
void phase3_disable_gp_gate(void)
{
    idt[13].attributes &= (uint8_t)~0x80u;
}
#endif
