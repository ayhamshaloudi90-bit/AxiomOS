#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/gdt.h>

/* Long-mode TSS: RSP0 is used when Ring 3 enters the kernel. */
struct task_state_segment {
    uint32_t reserved0;
    uint64_t rsp[3];
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

struct descriptor_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

_Static_assert(sizeof(struct task_state_segment) == 104, "TSS layout");
_Static_assert(offsetof(struct task_state_segment, ist) == 36, "TSS IST offset");
_Static_assert(sizeof(struct descriptor_pointer) == 10, "GDTR layout");

/* Null, ring-0 code/data, ring-3 data/code, and a two-slot TSS. */
static uint64_t gdt[7] __attribute__((aligned(16)));
static struct task_state_segment tss;
static uint8_t emergency_stacks[3][16384] __attribute__((aligned(16)));

#define GDT_SECONDARY_MAX_CPUS 8u
static uint64_t secondary_gdt[GDT_SECONDARY_MAX_CPUS][7]
    __attribute__((aligned(16)));
static struct task_state_segment secondary_tss[GDT_SECONDARY_MAX_CPUS];
static uint8_t secondary_emergency_stacks[GDT_SECONDARY_MAX_CPUS][3][16384]
    __attribute__((aligned(16)));

extern uint8_t kernel_stack_top[];
extern void gdt_load(const struct descriptor_pointer *pointer);

void gdt_init(void)
{
    const uint64_t base = (uint64_t)(uintptr_t)&tss;
    const uint64_t limit = sizeof(tss) - 1u;
    const struct descriptor_pointer pointer = {
        .limit = sizeof(gdt) - 1u,
        .base = (uint64_t)(uintptr_t)gdt
    };
    uint8_t i;

    gdt[0] = 0;
    gdt[1] = 0x00AF9A000000FFFFULL; /* Ring-0 code, present, L=1. */
    gdt[2] = 0x00CF92000000FFFFULL; /* Ring-0 writable data. */
    gdt[3] = 0x00CFF2000000FFFFULL; /* Ring-3 writable data. */
    gdt[4] = 0x00AFFA000000FFFFULL; /* Ring-3 code, present, L=1. */
    gdt[5] = (limit & 0xFFFFu)
           | ((base & 0xFFFFFFu) << 16)
           | (0x89ULL << 40) /* Present available 64-bit TSS. */
           | (((limit >> 16) & 0xFu) << 48)
           | (((base >> 24) & 0xFFu) << 56);
    gdt[6] = base >> 32;

    tss.rsp[0] = (uint64_t)(uintptr_t)kernel_stack_top;

    for (i = 0u; i < 3u; ++i) {
        tss.ist[i] = (uint64_t)(uintptr_t)&emergency_stacks[i][16384];
    }

    /* No I/O permission bitmap; its offset lies beyond the TSS limit. */
    tss.iomap_base = sizeof(tss);
    gdt_load(&pointer);
}

void gdt_set_kernel_stack(uintptr_t stack_top)
{
    tss.rsp[0] = (uint64_t)stack_top;
}

uintptr_t gdt_kernel_stack(void)
{
    return (uintptr_t)tss.rsp[0];
}

int gdt_ist_contains(uint8_t index, uintptr_t address)
{
    uintptr_t begin;

    if (index < 1u || index > 3u) {
        return 0;
    }

    begin = (uintptr_t)emergency_stacks[index - 1u];
    return address >= begin && address < begin + 16384u;
}


int gdt_init_secondary(uint32_t cpu_index, uintptr_t stack_top)
{
    uint64_t *cpu_gdt;
    struct task_state_segment *cpu_tss;
    uint64_t base;
    uint64_t limit;
    struct descriptor_pointer pointer;
    uint8_t i;

    if (cpu_index == 0u || cpu_index >= GDT_SECONDARY_MAX_CPUS ||
        stack_top == 0u) {
        return 0;
    }

    cpu_gdt = secondary_gdt[cpu_index];
    cpu_tss = &secondary_tss[cpu_index];
    base = (uint64_t)(uintptr_t)cpu_tss;
    limit = sizeof(*cpu_tss) - 1u;

    for (i = 0u; i < 7u; ++i) {
        cpu_gdt[i] = 0u;
    }
    for (i = 0u; i < sizeof(*cpu_tss); ++i) {
        ((uint8_t *)cpu_tss)[i] = 0u;
    }

    cpu_gdt[0] = 0;
    cpu_gdt[1] = 0x00AF9A000000FFFFULL;
    cpu_gdt[2] = 0x00CF92000000FFFFULL;
    cpu_gdt[3] = 0x00CFF2000000FFFFULL;
    cpu_gdt[4] = 0x00AFFA000000FFFFULL;
    cpu_gdt[5] = (limit & 0xFFFFu)
               | ((base & 0xFFFFFFu) << 16)
               | (0x89ULL << 40)
               | (((limit >> 16) & 0xFu) << 48)
               | (((base >> 24) & 0xFFu) << 56);
    cpu_gdt[6] = base >> 32;

    cpu_tss->rsp[0] = (uint64_t)stack_top;
    for (i = 0u; i < 3u; ++i) {
        cpu_tss->ist[i] = (uint64_t)(uintptr_t)
            &secondary_emergency_stacks[cpu_index][i][16384];
    }
    cpu_tss->iomap_base = sizeof(*cpu_tss);

    pointer.limit = sizeof(secondary_gdt[cpu_index]) - 1u;
    pointer.base = (uint64_t)(uintptr_t)cpu_gdt;
    gdt_load(&pointer);
    return 1;
}
