#ifndef AXIOM_PROCESS_TASK_H
#define AXIOM_PROCESS_TASK_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/user_layout.h>
#include <axiom/arch/interrupts.h>
#include <axiom/memory/address.h>

#define TASK_USER_CODE_BASE   ((vaddr_t)AXIOM_USER_CODE_BASE)
#define TASK_USER_DATA_BASE   ((vaddr_t)AXIOM_USER_DATA_BASE)
#define TASK_USER_STACK_TOP   ((vaddr_t)AXIOM_USER_STACK_TOP)
#define TASK_USER_STACK_PAGES 4u
#define TASK_USER_STACK_GUARD_PAGES 1u
#define TASK_USER_STACK_GUARD_BASE \
    (TASK_USER_STACK_TOP - (vaddr_t)((TASK_USER_STACK_PAGES + TASK_USER_STACK_GUARD_PAGES) * 4096ULL))
#define TASK_USER_ELF_MAX_PAGES 32u
#define TASK_USER_MMAP_MAX_PAGES 128u
#define TASK_USER_MMAP_BASE ((vaddr_t)0x0000100000000000ULL)
#define TASK_MAX_FILES 16u
#define TASK_DEFAULT_UID 1000u
#define TASK_DEFAULT_GID 1000u
#define TASK_KERNEL_STACK_CANARY 0xA8105EC0FFEE20ULL

#define TASK_USER_MESSAGE_MAGIC_OFFSET 64u
#define TASK_USER_MESSAGE_MAGIC 0x4158494F4D555345ULL

typedef void (*task_entry_t)(void *argument);

struct vfs_file;

enum task_state {
    TASK_RUNNING = 0,
    TASK_READY,
    TASK_BLOCKED,
    TASK_SLEEPING,
    TASK_TERMINATED,
};

enum task_privilege {
    TASK_PRIVILEGE_KERNEL = 0,
    TASK_PRIVILEGE_USER = 3,
};

struct task {
    uint64_t id;
    uint64_t parent_id;
    const char *name;
    enum task_state state;
    enum task_privilege privilege;
    uint32_t uid;
    uint32_t gid;

    /* Actual frame restored by the common ISR epilogue. */
    struct interrupt_frame *saved_frame;

    void *kernel_stack_base;
    size_t kernel_stack_size;
    uintptr_t kernel_stack_top;
    uint64_t kernel_stack_canary_expected;
    paddr_t address_space;

    task_entry_t entry;
    void *argument;

    paddr_t user_code_page;
    paddr_t user_data_page;
    paddr_t user_stack_pages[TASK_USER_STACK_PAGES];

    /* Phase 11 ELF-backed process image pages. */
    paddr_t user_elf_pages[TASK_USER_ELF_MAX_PAGES];
    vaddr_t user_elf_virtual_pages[TASK_USER_ELF_MAX_PAGES];
    uint64_t user_elf_page_flags[TASK_USER_ELF_MAX_PAGES];
    size_t user_elf_page_count;

    /* Phase 19 anonymous userspace mappings backing libc malloc(). */
    paddr_t user_mmap_pages[TASK_USER_MMAP_MAX_PAGES];
    vaddr_t user_mmap_virtual_pages[TASK_USER_MMAP_MAX_PAGES];
    size_t user_mmap_page_count;
    vaddr_t user_mmap_next;
    uint16_t elf_program_headers;
    uint16_t elf_load_segments;
    uint64_t elf_file_bytes;
    uint64_t elf_memory_bytes;
    size_t elf_image_size;
    int elf_backed;

    vaddr_t user_entry;
    vaddr_t user_stack_top;

    /* Phase 12 per-process descriptor table. 0/1/2 remain stdin/out/err. */
    struct vfs_file *file_descriptors[TASK_MAX_FILES];

    uint64_t fault_vector;
    uint64_t fault_error_code;
    vaddr_t fault_address;
    int64_t exit_code;
    uint64_t wake_tick;
    uint64_t wait_target_id;
    int wait_collected;
    uint32_t termination_signal;

    uint64_t quantum_ticks;
    uint64_t ticks_in_slice;
    uint64_t runtime_ticks;
    uint64_t context_switches;

    /* Phase 21 scheduler metadata. Larger priority values run first. */
    uint32_t priority;
    uint32_t effective_priority;
    uint64_t affinity_mask;
    uint64_t ready_since_tick;
    uint64_t accumulated_ready_ticks;
};

const char *task_state_name(enum task_state state);
const char *task_privilege_name(enum task_privilege privilege);

int task_install_file(struct task *task, struct vfs_file *file);
struct vfs_file *task_file(struct task *task, int fd);
struct vfs_file *task_take_file(struct task *task, int fd);
void task_close_all_files(struct task *task);
int task_share_files(struct task *destination, const struct task *source);

#endif
