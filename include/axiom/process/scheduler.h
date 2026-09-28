#ifndef AXIOM_PROCESS_SCHEDULER_H
#define AXIOM_PROCESS_SCHEDULER_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/interrupts.h>
#include <axiom/abi/scheduler.h>
#include <axiom/process/task.h>

#define SCHEDULER_MAX_TASKS 16u
#define SCHEDULER_TASK_STACK_SIZE (16u * 1024u)
#define SCHEDULER_DEFAULT_QUANTUM_TICKS 5u
#define SCHEDULER_AGING_INTERVAL_TICKS 20u

struct syscall_frame;

struct scheduler_stats {
    uint64_t task_count;
    uint64_t runnable_tasks;
    uint64_t context_switches;
    uint64_t preemptions;
    uint64_t priority_preemptions;
    uint64_t scheduling_ticks;
    uint64_t aging_promotions;
    uint64_t voluntary_yields;
    uint64_t user_fault_terminations;
    uint64_t current_task_id;
    uint64_t current_affinity_mask;
    uint32_t policy;
    uint32_t current_priority;
    uint32_t current_effective_priority;
};

struct scheduler_benchmark_result {
    uint64_t round_robin[3];
    uint64_t priority_aging[3];
    uint64_t decisions;
};

int scheduler_init(void);

int task_create(
    const char *name,
    task_entry_t entry,
    void *argument,
    uint64_t *task_id_out
);

/* Create a Ring-3 task in its own address space from a <=4 KiB code image. */
int user_task_create(
    const char *name,
    const void *image,
    size_t image_size,
    uint64_t *task_id_out
);

/* Phase 11: create a Ring-3 task by parsing a standalone ELF64 ET_EXEC image. */
int user_task_create_elf(
    const char *name,
    const void *elf_image,
    size_t elf_size,
    uint64_t *task_id_out
);

/* Replace the currently running Ring-3 process image with a new ELF. IF=0. */
int scheduler_exec_current_elf(
    const void *elf_image,
    size_t elf_size,
    vaddr_t *entry_out,
    vaddr_t *stack_out
);


/* Phase 15 process-management primitives. */
int scheduler_fork_current(
    const struct syscall_frame *parent_frame,
    uint64_t *child_id_out
);
int scheduler_wait_current_child(uint64_t child_id, int64_t *status_out);
int scheduler_terminate_task(uint64_t task_id, uint32_t signal_number);

/* Phase 19: anonymous writable/NX user mappings for libc heap growth. */
int scheduler_mmap_current(size_t length, vaddr_t *address_out);

/* Phase 16: generic blocking/wakeup hooks used by kernel wait queues. IF=0. */
int scheduler_prepare_block_current(uint64_t expected_task_id);
int scheduler_park_current(uint64_t expected_task_id);
int scheduler_wake_task(uint64_t task_id);

int scheduler_start(void);
int scheduler_running(void);

/* Phase 21 advanced-scheduler controls. Call with interrupts disabled. */
int scheduler_set_policy(uint32_t policy);
uint32_t scheduler_get_policy(void);
int scheduler_set_current_priority(uint32_t priority);
uint32_t scheduler_get_current_priority(void);
uint32_t scheduler_get_current_effective_priority(void);
int scheduler_set_current_affinity(uint64_t affinity_mask);
uint64_t scheduler_get_current_affinity(void);
void scheduler_benchmark_compare(struct scheduler_benchmark_result *result);

/* Return the interrupt frame that the assembly epilogue should restore. */
struct interrupt_frame *scheduler_on_timer_interrupt(
    struct interrupt_frame *frame
);

/* Kill a faulting Ring-3 task and return the next runnable task's frame. */
struct interrupt_frame *scheduler_handle_user_fault(
    struct interrupt_frame *frame,
    uint64_t vector,
    uint64_t error_code,
    vaddr_t fault_address
);

int scheduler_sleep_current(uint64_t ticks);
int scheduler_yield_current(void);
_Noreturn void task_exit_current(int64_t status);

struct task *scheduler_current_task_mutable(void);
const struct task *scheduler_current_task(void);

struct scheduler_stats scheduler_get_stats(void);
size_t scheduler_task_count(void);
const struct task *scheduler_task_at(size_t index);
const struct task *scheduler_task_by_id(uint64_t id);
struct task *scheduler_task_by_id_mutable(uint64_t id);

#endif
