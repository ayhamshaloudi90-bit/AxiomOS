#ifndef AXIOM_ABI_PROCESS_H
#define AXIOM_ABI_PROCESS_H

#define AXIOM_PROCESS_NAME_MAX 32

#define AXIOM_PROC_RUNNING    0
#define AXIOM_PROC_READY      1
#define AXIOM_PROC_BLOCKED    2
#define AXIOM_PROC_SLEEPING   3
#define AXIOM_PROC_TERMINATED 4

#define AXIOM_PRIV_KERNEL 0
#define AXIOM_PRIV_USER   3

/* Phase 15 intentionally starts with one non-catchable termination signal. */
#define AXIOM_SIGTERM 15

#ifndef __ASSEMBLER__
#include <stdint.h>

struct axiom_process_info {
    uint64_t pid;
    uint64_t ppid;
    uint64_t runtime_ticks;
    int64_t exit_status;
    uint32_t state;
    uint32_t privilege;
    uint32_t termination_signal;
    uint32_t priority;
    uint32_t effective_priority;
    uint32_t reserved;
    uint64_t affinity_mask;
    uint64_t ready_ticks;
    char name[AXIOM_PROCESS_NAME_MAX];
};
#endif

#endif
