#ifndef AXIOM_ARCH_SMP_H
#define AXIOM_ARCH_SMP_H

#include <stdint.h>

#include <axiom/boot/limine.h>

#define SMP_MAX_CPUS 8u
#define SMP_AP_STACK_SIZE (16u * 1024u)
#define SMP_TEST_ITERATIONS 5000ULL

enum smp_cpu_role {
    SMP_CPU_BSP = 0,
    SMP_CPU_AP = 1,
};

struct smp_cpu {
    uint32_t processor_id;
    uint32_t lapic_id;
    uint64_t logical_index;
    uint64_t kernel_cr3;
    uintptr_t stack_top;
    volatile uint32_t online;
    volatile uint32_t work_complete;
    volatile uint64_t work_iterations;
    enum smp_cpu_role role;
};

struct smp_stats {
    uint64_t detected_cpus;
    uint64_t managed_cpus;
    uint64_t online_cpus;
    uint64_t aps_released;
    uint64_t aps_completed;
    uint64_t participant_mask;
    uint64_t expected_locked_count;
    uint64_t actual_locked_count;
    uint64_t spinlock_acquisitions;
    uint64_t spinlock_contentions;
    int multicore_test_ran;
    int multicore_test_passed;
};

/* Bring up secondary processors and run the Phase-17 parallel-work proof. */
int smp_init(void);

const struct smp_stats *smp_get_stats(void);
const struct smp_cpu *smp_cpu_at(uint64_t index);

/* Assembly entry published through Limine's MP goto_address field. */
void smp_ap_entry_asm(struct limine_mp_info *info);
_Noreturn void smp_ap_entry_c(struct smp_cpu *cpu);

#endif
