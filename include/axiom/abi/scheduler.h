#ifndef AXIOM_ABI_SCHEDULER_H
#define AXIOM_ABI_SCHEDULER_H

#define AXIOM_SCHED_POLICY_ROUND_ROBIN      0u
#define AXIOM_SCHED_POLICY_PRIORITY_AGING   1u

/* Larger values represent more CPU scheduling preference. */
#define AXIOM_SCHED_PRIORITY_MIN      0u
#define AXIOM_SCHED_PRIORITY_DEFAULT  4u
#define AXIOM_SCHED_PRIORITY_MAX      7u

/* Phase 21 still executes normal tasks on the BSP (logical CPU 0). */
#define AXIOM_CPU_AFFINITY_BSP 0x1ULL

#ifndef __ASSEMBLER__
#include <stdint.h>

struct axiom_scheduler_stats {
    uint64_t task_count;
    uint64_t runnable_tasks;
    uint64_t context_switches;
    uint64_t preemptions;
    uint64_t priority_preemptions;
    uint64_t scheduling_ticks;
    uint64_t aging_promotions;
    uint64_t voluntary_yields;
    uint64_t current_task_id;
    uint64_t current_affinity_mask;
    uint32_t policy;
    uint32_t current_priority;
    uint32_t current_effective_priority;
    uint32_t reserved;
};

#endif

#endif
