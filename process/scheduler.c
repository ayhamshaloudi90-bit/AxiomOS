#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/elf/elf64.h>
#include <axiom/kernel/panic.h>
#include <axiom/kernel/syscall.h>
#include <axiom/ipc/ipc.h>
#include <axiom/memory/heap.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>
#include <axiom/process/scheduler.h>

static struct task tasks[SCHEDULER_MAX_TASKS];
static size_t task_count_value;
static size_t current_index;
static uint64_t next_task_id;
static uint64_t total_context_switches;
static uint64_t total_preemptions;
static uint64_t total_priority_preemptions;
static uint64_t total_scheduling_ticks;
static uint64_t total_aging_promotions;
static uint64_t total_voluntary_yields;
static uint64_t total_user_fault_terminations;
static uint32_t scheduler_policy;
static int force_voluntary_rotation;
static int initialized;
static int running;

extern uint8_t kernel_stack_bottom[];
extern uint8_t kernel_stack_top[];

static void free_user_task_resources(struct task *task);

static void bytes_clear(void *memory, size_t count)
{
    uint8_t *bytes = (uint8_t *)memory;
    size_t index;

    for (index = 0u; index < count; ++index) {
        bytes[index] = 0u;
    }
}

static void bytes_copy(void *destination, const void *source, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    size_t index;

    for (index = 0u; index < count; ++index) {
        out[index] = in[index];
    }
}

static void frame_clear(struct interrupt_frame *frame)
{
    bytes_clear(frame, sizeof(*frame));
}

static void task_reset(struct task *task)
{
    size_t index;

    task->id = 0u;
    task->parent_id = UINT64_MAX;
    task->name = 0;
    task->state = TASK_TERMINATED;
    task->privilege = TASK_PRIVILEGE_KERNEL;
    task->uid = 0u;
    task->gid = 0u;
    task->saved_frame = 0;
    task->kernel_stack_base = 0;
    task->kernel_stack_size = 0u;
    task->kernel_stack_top = 0u;
    task->kernel_stack_canary_expected = 0u;
    task->address_space = PADDR_INVALID;
    task->entry = 0;
    task->argument = 0;
    task->user_code_page = PADDR_INVALID;
    task->user_data_page = PADDR_INVALID;

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        task->user_stack_pages[index] = PADDR_INVALID;
    }

    for (index = 0u; index < TASK_USER_ELF_MAX_PAGES; ++index) {
        task->user_elf_pages[index] = PADDR_INVALID;
        task->user_elf_virtual_pages[index] = 0u;
        task->user_elf_page_flags[index] = 0u;
    }

    for (index = 0u; index < TASK_USER_MMAP_MAX_PAGES; ++index) {
        task->user_mmap_pages[index] = PADDR_INVALID;
        task->user_mmap_virtual_pages[index] = 0u;
    }

    for (index = 0u; index < TASK_MAX_FILES; ++index) {
        task->file_descriptors[index] = 0;
    }

    task->user_elf_page_count = 0u;
    task->user_mmap_page_count = 0u;
    task->user_mmap_next = TASK_USER_MMAP_BASE;
    task->elf_program_headers = 0u;
    task->elf_load_segments = 0u;
    task->elf_file_bytes = 0u;
    task->elf_memory_bytes = 0u;
    task->elf_image_size = 0u;
    task->elf_backed = 0;

    task->user_entry = 0u;
    task->user_stack_top = 0u;
    task->fault_vector = 0u;
    task->fault_error_code = 0u;
    task->fault_address = 0u;
    task->exit_code = 0;
    task->wake_tick = 0u;
    task->wait_target_id = UINT64_MAX;
    task->wait_collected = 0;
    task->termination_signal = 0u;
    task->quantum_ticks = SCHEDULER_DEFAULT_QUANTUM_TICKS;
    task->ticks_in_slice = 0u;
    task->runtime_ticks = 0u;
    task->context_switches = 0u;
    task->priority = AXIOM_SCHED_PRIORITY_DEFAULT;
    task->effective_priority = AXIOM_SCHED_PRIORITY_DEFAULT;
    task->affinity_mask = AXIOM_CPU_AFFINITY_BSP;
    task->ready_since_tick = 0u;
    task->accumulated_ready_ticks = 0u;
}

static int allocate_kernel_stack(struct task *task)
{
    uintptr_t top;
    uint64_t *guard;
    void *stack = kmalloc(SCHEDULER_TASK_STACK_SIZE);

    if (stack == 0) {
        return 0;
    }

    /* Reserve the first 8 bytes as a kernel-stack overflow canary. */
    guard = (uint64_t *)stack;
    *guard = TASK_KERNEL_STACK_CANARY;
    top = ((uintptr_t)stack + SCHEDULER_TASK_STACK_SIZE) & ~(uintptr_t)0xFu;
    task->kernel_stack_base = stack;
    task->kernel_stack_size = SCHEDULER_TASK_STACK_SIZE;
    task->kernel_stack_top = top;
    task->kernel_stack_canary_expected = TASK_KERNEL_STACK_CANARY;
    return 1;
}

static void verify_kernel_stack_canary(const struct task *task)
{
    const uint64_t *guard;

    if (task == 0 || task->kernel_stack_base == 0 ||
        task->kernel_stack_base == (void *)kernel_stack_bottom ||
        task->kernel_stack_canary_expected == 0u) {
        return;
    }

    guard = (const uint64_t *)task->kernel_stack_base;
    if (*guard != task->kernel_stack_canary_expected) {
        kernel_panic("kernel task stack canary corrupted");
    }
}

static _Noreturn void task_bootstrap(struct task *task)
{
    if (task == 0 || task->entry == 0) {
        kernel_panic("scheduler entered an invalid task bootstrap");
    }

    task->entry(task->argument);
    task_exit_current(0);
}

static struct interrupt_frame *build_kernel_initial_frame(struct task *task)
{
    uintptr_t address;
    uintptr_t initial_rsp;
    struct interrupt_frame *frame;

    /*
     * In 64-bit mode the CPU saves SS:RSP for every interrupt frame, even
     * without a privilege change, and IRETQ restores them. Build the synthetic
     * kernel-thread frame with a real private-stack RSP and kernel-data SS.
     *
     * A normal SysV C function sees RSP == 8 (mod 16) at entry because CALL
     * pushed a return address. IRETQ does not do that, so reserve one dummy
     * quadword at the top of the task stack and restore RSP to that slot.
     */
    initial_rsp = task->kernel_stack_top - sizeof(uint64_t);
    *(uint64_t *)initial_rsp = 0u;

    address = initial_rsp - sizeof(struct interrupt_frame);
    frame = (struct interrupt_frame *)address;
    frame_clear(frame);

    frame->rip = (uint64_t)(uintptr_t)&task_bootstrap;
    frame->cs = GDT_KERNEL_CODE;
    frame->rflags = 0x202ULL;
    frame->rsp = (uint64_t)initial_rsp;
    frame->ss = GDT_KERNEL_DATA;
    frame->rdi = (uint64_t)(uintptr_t)task;
    return frame;
}

static struct interrupt_frame *build_user_initial_frame(struct task *task)
{
    uintptr_t address;
    struct interrupt_frame *frame;

    address = task->kernel_stack_top - sizeof(struct interrupt_frame);
    frame = (struct interrupt_frame *)address;
    frame_clear(frame);

    frame->rip = task->user_entry;
    frame->cs = GDT_USER_CODE;
    frame->rflags = 0x202ULL;
    frame->rsp = task->user_stack_top - 8ULL;
    frame->ss = GDT_USER_DATA;
    return frame;
}

static void mark_task_ready(struct task *task)
{
    if (task == 0) {
        return;
    }

    task->state = TASK_READY;
    task->ready_since_tick = total_scheduling_ticks;
    task->effective_priority = task->priority;
}

static void age_ready_tasks(void)
{
    size_t index;

    if (scheduler_policy != AXIOM_SCHED_POLICY_PRIORITY_AGING) {
        return;
    }

    for (index = 0u; index < task_count_value; ++index) {
        struct task *task = &tasks[index];
        uint64_t waited;
        uint32_t boosted;

        if (task->state != TASK_READY ||
            (task->affinity_mask & AXIOM_CPU_AFFINITY_BSP) == 0u) {
            continue;
        }

        waited = total_scheduling_ticks - task->ready_since_tick;
        task->accumulated_ready_ticks += 1u;
        boosted = task->priority +
            (uint32_t)(waited / (uint64_t)SCHEDULER_AGING_INTERVAL_TICKS);
        if (boosted > AXIOM_SCHED_PRIORITY_MAX) {
            boosted = AXIOM_SCHED_PRIORITY_MAX;
        }

        if (boosted > task->effective_priority) {
            total_aging_promotions +=
                (uint64_t)(boosted - task->effective_priority);
            task->effective_priority = boosted;
        }
    }
}

static size_t find_next_ready_round_robin(size_t after)
{
    size_t step;

    for (step = 1u; step <= task_count_value; ++step) {
        const size_t index = (after + step) % task_count_value;

        if (tasks[index].state == TASK_READY &&
            (tasks[index].affinity_mask & AXIOM_CPU_AFFINITY_BSP) != 0u) {
            return index;
        }
    }

    return SCHEDULER_MAX_TASKS;
}

static size_t find_next_ready_priority(size_t after)
{
    size_t step;
    size_t selected = SCHEDULER_MAX_TASKS;
    uint32_t selected_priority = 0u;

    for (step = 1u; step <= task_count_value; ++step) {
        const size_t index = (after + step) % task_count_value;
        const struct task *task = &tasks[index];

        if (task->state != TASK_READY ||
            (task->affinity_mask & AXIOM_CPU_AFFINITY_BSP) == 0u) {
            continue;
        }

        if (selected == SCHEDULER_MAX_TASKS ||
            task->effective_priority > selected_priority) {
            selected = index;
            selected_priority = task->effective_priority;
        }
    }

    return selected;
}

static size_t find_next_ready(size_t after)
{
    if (scheduler_policy == AXIOM_SCHED_POLICY_PRIORITY_AGING) {
        return find_next_ready_priority(after);
    }

    return find_next_ready_round_robin(after);
}

static int higher_priority_ready_than(const struct task *current)
{
    size_t index;

    if (current == 0 ||
        scheduler_policy != AXIOM_SCHED_POLICY_PRIORITY_AGING) {
        return 0;
    }

    for (index = 0u; index < task_count_value; ++index) {
        const struct task *task = &tasks[index];

        if (task->state == TASK_READY &&
            (task->affinity_mask & AXIOM_CPU_AFFINITY_BSP) != 0u &&
            task->effective_priority > current->priority) {
            return 1;
        }
    }

    return 0;
}

static struct interrupt_frame *switch_to(size_t next_index)
{
    struct task *next;

    if (next_index >= task_count_value) {
        kernel_panic("scheduler selected invalid task index");
    }

    next = &tasks[next_index];
    verify_kernel_stack_canary(next);

    if (next->saved_frame == 0 ||
        next->address_space == PADDR_INVALID ||
        next->kernel_stack_top == 0u) {
        kernel_panic("scheduler selected incomplete task context");
    }

    if (vmm_current_address_space() != next->address_space &&
        !vmm_activate_address_space(next->address_space)) {
        kernel_panic("scheduler failed to switch address spaces");
    }

    gdt_set_kernel_stack(next->kernel_stack_top);
    syscall_set_kernel_stack(next->kernel_stack_top);
    next->state = TASK_RUNNING;
    next->ticks_in_slice = 0u;
    next->effective_priority = next->priority;
    next->ready_since_tick = total_scheduling_ticks;
    ++next->context_switches;

    current_index = next_index;
    ++total_context_switches;
    return next->saved_frame;
}

int scheduler_init(void)
{
    size_t index;
    struct task *bootstrap;
    const paddr_t kernel_root = vmm_kernel_address_space();

    if (initialized || interrupts_enabled() || kernel_root == PADDR_INVALID) {
        return 0;
    }

    for (index = 0u; index < SCHEDULER_MAX_TASKS; ++index) {
        task_reset(&tasks[index]);
    }

    bootstrap = &tasks[0];
    bootstrap->id = 0u;
    bootstrap->name = "bootstrap";
    bootstrap->state = TASK_RUNNING;
    bootstrap->privilege = TASK_PRIVILEGE_KERNEL;
    bootstrap->kernel_stack_base = kernel_stack_bottom;
    bootstrap->kernel_stack_size = (size_t)(kernel_stack_top - kernel_stack_bottom);
    bootstrap->kernel_stack_top = (uintptr_t)kernel_stack_top;
    bootstrap->address_space = kernel_root;

    gdt_set_kernel_stack(bootstrap->kernel_stack_top);
    syscall_set_kernel_stack(bootstrap->kernel_stack_top);

    task_count_value = 1u;
    current_index = 0u;
    next_task_id = 1u;
    total_context_switches = 0u;
    total_preemptions = 0u;
    total_priority_preemptions = 0u;
    total_scheduling_ticks = 0u;
    total_aging_promotions = 0u;
    total_voluntary_yields = 0u;
    total_user_fault_terminations = 0u;
    scheduler_policy = AXIOM_SCHED_POLICY_ROUND_ROBIN;
    force_voluntary_rotation = 0;
    running = 0;
    initialized = 1;
    return 1;
}

static void release_terminated_task_slot(struct task *task)
{
    if (task == 0) {
        return;
    }

    if (task->privilege == TASK_PRIVILEGE_USER) {
        free_user_task_resources(task);
        return;
    }

    /* Slot zero is the permanent bootstrap task and is never reclaimed. */
    if (task->kernel_stack_base != 0 &&
        task->kernel_stack_base != (void *)kernel_stack_bottom) {
        kfree(task->kernel_stack_base);
    }

    task_reset(task);
}

static int task_parent_is_alive(const struct task *task)
{
    size_t index;

    if (task == 0 || task->parent_id == UINT64_MAX) {
        return 0;
    }

    for (index = 0u; index < task_count_value; ++index) {
        if (tasks[index].id == task->parent_id &&
            tasks[index].state != TASK_TERMINATED) {
            return 1;
        }
    }

    return 0;
}

static int task_slot_reclaimable(const struct task *task)
{
    if (task == 0 || task->state != TASK_TERMINATED) {
        return 0;
    }

    /*
     * Parentless historical phase tasks can be reused immediately. A real
     * child stays as a small zombie until waitpid() collects its status, unless
     * its parent has already died.
     */
    return task->parent_id == UINT64_MAX || task->wait_collected ||
        !task_parent_is_alive(task);
}

static struct task *prepare_creation_slot(size_t *slot_index_out)
{
    size_t index;

    if (slot_index_out == 0) {
        return 0;
    }

    for (index = 1u; index < task_count_value; ++index) {
        if (index != current_index && task_slot_reclaimable(&tasks[index])) {
            release_terminated_task_slot(&tasks[index]);
            *slot_index_out = index;
            return &tasks[index];
        }
    }

    if (task_count_value >= SCHEDULER_MAX_TASKS) {
        return 0;
    }

    index = task_count_value;
    task_reset(&tasks[index]);
    *slot_index_out = index;
    return &tasks[index];
}

static void commit_creation_slot(size_t slot_index)
{
    if (slot_index == task_count_value) {
        ++task_count_value;
    }
}

int task_create(
    const char *name,
    task_entry_t entry,
    void *argument,
    uint64_t *task_id_out
)
{
    struct task *task;
    size_t slot_index;

    if (!initialized || interrupts_enabled() || entry == 0) {
        return 0;
    }

    task = prepare_creation_slot(&slot_index);
    if (task == 0) {
        return 0;
    }

    if (!allocate_kernel_stack(task)) {
        return 0;
    }

    task->id = next_task_id++;
    task->name = name != 0 ? name : "kernel-thread";
    mark_task_ready(task);
    task->privilege = TASK_PRIVILEGE_KERNEL;
    task->address_space = vmm_kernel_address_space();
    task->entry = entry;
    task->argument = argument;
    task->saved_frame = build_kernel_initial_frame(task);

    if (task_id_out != 0) {
        *task_id_out = task->id;
    }

    commit_creation_slot(slot_index);
    return 1;
}

static void free_user_task_resources(struct task *task)
{
    size_t index;

    task_close_all_files(task);
    ipc_task_detach_all(task->id, task->address_space);

    if (task->address_space != PADDR_INVALID &&
        task->address_space != vmm_kernel_address_space()) {
        (void)vmm_destroy_user_address_space(task->address_space);
    }

    if (task->user_code_page != PADDR_INVALID) {
        (void)pmm_free_page(task->user_code_page);
    }

    if (task->user_data_page != PADDR_INVALID) {
        (void)pmm_free_page(task->user_data_page);
    }

    for (index = 0u; index < task->user_elf_page_count; ++index) {
        if (task->user_elf_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(task->user_elf_pages[index]);
        }
    }

    for (index = 0u; index < task->user_mmap_page_count; ++index) {
        if (task->user_mmap_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(task->user_mmap_pages[index]);
        }
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        if (task->user_stack_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(task->user_stack_pages[index]);
        }
    }

    if (task->kernel_stack_base != 0) {
        kfree(task->kernel_stack_base);
    }

    task_reset(task);
}

int user_task_create(
    const char *name,
    const void *image,
    size_t image_size,
    uint64_t *task_id_out
)
{
    struct task *task;
    uint8_t *code_bytes;
    uint8_t *data_bytes;
    size_t index;
    size_t slot_index;

    if (!initialized || interrupts_enabled() || image == 0 || image_size == 0u ||
        image_size > VMM_PAGE_SIZE) {
        return 0;
    }

    task = prepare_creation_slot(&slot_index);
    if (task == 0) {
        return 0;
    }

    if (!allocate_kernel_stack(task)) {
        return 0;
    }

    task->address_space = vmm_create_user_address_space();
    if (task->address_space == PADDR_INVALID) {
        free_user_task_resources(task);
        return 0;
    }

    task->user_code_page = pmm_alloc_page();
    task->user_data_page = pmm_alloc_page();

    if (task->user_code_page == PADDR_INVALID ||
        task->user_data_page == PADDR_INVALID) {
        free_user_task_resources(task);
        return 0;
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        task->user_stack_pages[index] = pmm_alloc_page();

        if (task->user_stack_pages[index] == PADDR_INVALID) {
            free_user_task_resources(task);
            return 0;
        }
    }

    code_bytes = (uint8_t *)pmm_phys_to_hhdm(task->user_code_page);
    data_bytes = (uint8_t *)pmm_phys_to_hhdm(task->user_data_page);

    if (code_bytes == 0 || data_bytes == 0) {
        free_user_task_resources(task);
        return 0;
    }

    bytes_clear(code_bytes, VMM_PAGE_SIZE);
    bytes_clear(data_bytes, VMM_PAGE_SIZE);
    bytes_copy(code_bytes, image, image_size);

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        void *stack_page = pmm_phys_to_hhdm(task->user_stack_pages[index]);

        if (stack_page == 0) {
            free_user_task_resources(task);
            return 0;
        }

        bytes_clear(stack_page, VMM_PAGE_SIZE);
    }

    if (!vmm_map_page_in_address_space(
            task->address_space,
            TASK_USER_CODE_BASE,
            task->user_code_page,
            VMM_FLAG_USER
        ) ||
        !vmm_map_page_in_address_space(
            task->address_space,
            TASK_USER_DATA_BASE,
            task->user_data_page,
            VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE
        )) {
        free_user_task_resources(task);
        return 0;
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        const vaddr_t address =
            TASK_USER_STACK_TOP -
            (vaddr_t)((TASK_USER_STACK_PAGES - index) * VMM_PAGE_SIZE);

        if (!vmm_map_page_in_address_space(
                task->address_space,
                address,
                task->user_stack_pages[index],
                VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE
            )) {
            free_user_task_resources(task);
            return 0;
        }
    }

    task->id = next_task_id++;
    task->name = name != 0 ? name : "user-task";
    mark_task_ready(task);
    task->privilege = TASK_PRIVILEGE_USER;
    task->uid = TASK_DEFAULT_UID;
    task->gid = TASK_DEFAULT_GID;
    task->user_entry = TASK_USER_CODE_BASE;
    task->user_stack_top = TASK_USER_STACK_TOP;
    task->saved_frame = build_user_initial_frame(task);

    if (task_id_out != 0) {
        *task_id_out = task->id;
    }

    commit_creation_slot(slot_index);
    return 1;
}


int user_task_create_elf(
    const char *name,
    const void *elf_image,
    size_t elf_size,
    uint64_t *task_id_out
)
{
    struct task *task;
    struct elf64_load_result loaded;
    size_t index;
    size_t slot_index;

    if (!initialized || interrupts_enabled() || elf_image == 0 ||
        elf_size == 0u) {
        return 0;
    }

    task = prepare_creation_slot(&slot_index);
    if (task == 0) {
        return 0;
    }

    if (!allocate_kernel_stack(task)) {
        return 0;
    }

    task->address_space = vmm_create_user_address_space();
    if (task->address_space == PADDR_INVALID) {
        free_user_task_resources(task);
        return 0;
    }

    if (!elf64_load_executable(
            task->address_space,
            elf_image,
            elf_size,
            &loaded
        )) {
        free_user_task_resources(task);
        return 0;
    }

    if (loaded.page_count > TASK_USER_ELF_MAX_PAGES) {
        free_user_task_resources(task);
        return 0;
    }

    task->user_elf_page_count = loaded.page_count;
    for (index = 0u; index < loaded.page_count; ++index) {
        task->user_elf_pages[index] = loaded.pages[index].physical_address;
        task->user_elf_virtual_pages[index] = loaded.pages[index].virtual_address;
        task->user_elf_page_flags[index] = loaded.pages[index].vmm_flags;
    }
    for (index = 0u; index < TASK_USER_MMAP_MAX_PAGES; ++index) {
        task->user_mmap_pages[index] = PADDR_INVALID;
        task->user_mmap_virtual_pages[index] = 0u;
    }
    task->user_mmap_page_count = 0u;
    task->user_mmap_next = TASK_USER_MMAP_BASE;

    task->elf_program_headers = loaded.program_headers;
    task->elf_load_segments = loaded.load_segments;
    task->elf_file_bytes = loaded.file_bytes;
    task->elf_memory_bytes = loaded.memory_bytes;
    task->elf_image_size = elf_size;
    task->elf_backed = 1;

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        void *stack_page;
        const vaddr_t address =
            TASK_USER_STACK_TOP -
            (vaddr_t)((TASK_USER_STACK_PAGES - index) * VMM_PAGE_SIZE);

        task->user_stack_pages[index] = pmm_alloc_page();
        if (task->user_stack_pages[index] == PADDR_INVALID) {
            free_user_task_resources(task);
            return 0;
        }

        stack_page = pmm_phys_to_hhdm(task->user_stack_pages[index]);
        if (stack_page == 0) {
            free_user_task_resources(task);
            return 0;
        }
        bytes_clear(stack_page, VMM_PAGE_SIZE);

        if (!vmm_map_page_in_address_space(
                task->address_space,
                address,
                task->user_stack_pages[index],
                VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE
            )) {
            free_user_task_resources(task);
            return 0;
        }
    }

    task->id = next_task_id++;
    task->name = name != 0 ? name : "elf-user-task";
    mark_task_ready(task);
    task->privilege = TASK_PRIVILEGE_USER;
    task->uid = TASK_DEFAULT_UID;
    task->gid = TASK_DEFAULT_GID;
    task->user_entry = loaded.entry;
    task->user_stack_top = TASK_USER_STACK_TOP;
    task->saved_frame = build_user_initial_frame(task);

    if (task_id_out != 0) {
        *task_id_out = task->id;
    }

    commit_creation_slot(slot_index);
    return 1;
}


int scheduler_exec_current_elf(
    const void *elf_image,
    size_t elf_size,
    vaddr_t *entry_out,
    vaddr_t *stack_out
)
{
    struct task *task;
    struct elf64_load_result loaded;
    paddr_t new_root = PADDR_INVALID;
    paddr_t new_stack_pages[TASK_USER_STACK_PAGES];
    paddr_t old_stack_pages[TASK_USER_STACK_PAGES];
    paddr_t old_elf_pages[TASK_USER_ELF_MAX_PAGES];
    paddr_t old_mmap_pages[TASK_USER_MMAP_MAX_PAGES];
    paddr_t old_root;
    paddr_t old_code;
    paddr_t old_data;
    size_t old_elf_count;
    size_t old_mmap_count;
    size_t index;

    if (!initialized || !running || interrupts_enabled() ||
        current_index >= task_count_value || elf_image == 0 || elf_size == 0u ||
        entry_out == 0 || stack_out == 0) {
        return 0;
    }

    task = &tasks[current_index];
    if (task->privilege != TASK_PRIVILEGE_USER ||
        task->address_space == PADDR_INVALID) {
        return 0;
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        new_stack_pages[index] = PADDR_INVALID;
    }

    new_root = vmm_create_user_address_space();
    if (new_root == PADDR_INVALID) {
        return 0;
    }

    if (!elf64_load_executable(new_root, elf_image, elf_size, &loaded)) {
        (void)vmm_destroy_user_address_space(new_root);
        return 0;
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        void *stack_page;
        const vaddr_t address =
            TASK_USER_STACK_TOP -
            (vaddr_t)((TASK_USER_STACK_PAGES - index) * VMM_PAGE_SIZE);

        new_stack_pages[index] = pmm_alloc_page();
        if (new_stack_pages[index] == PADDR_INVALID) {
            goto fail_new_image;
        }

        stack_page = pmm_phys_to_hhdm(new_stack_pages[index]);
        if (stack_page == 0) {
            goto fail_new_image;
        }
        bytes_clear(stack_page, VMM_PAGE_SIZE);

        if (!vmm_map_page_in_address_space(
                new_root,
                address,
                new_stack_pages[index],
                VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE
            )) {
            goto fail_new_image;
        }
    }

    old_root = task->address_space;
    old_code = task->user_code_page;
    old_data = task->user_data_page;
    old_elf_count = task->user_elf_page_count;

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        old_stack_pages[index] = task->user_stack_pages[index];
    }
    for (index = 0u; index < TASK_USER_ELF_MAX_PAGES; ++index) {
        old_elf_pages[index] = task->user_elf_pages[index];
    }
    old_mmap_count = task->user_mmap_page_count;
    for (index = 0u; index < TASK_USER_MMAP_MAX_PAGES; ++index) {
        old_mmap_pages[index] = task->user_mmap_pages[index];
    }

    if (!vmm_activate_address_space(new_root)) {
        goto fail_new_image;
    }

    task->address_space = new_root;
    task->user_code_page = PADDR_INVALID;
    task->user_data_page = PADDR_INVALID;

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        task->user_stack_pages[index] = new_stack_pages[index];
    }
    for (index = 0u; index < TASK_USER_ELF_MAX_PAGES; ++index) {
        task->user_elf_pages[index] = PADDR_INVALID;
        task->user_elf_virtual_pages[index] = 0u;
        task->user_elf_page_flags[index] = 0u;
    }
    task->user_elf_page_count = loaded.page_count;
    for (index = 0u; index < loaded.page_count; ++index) {
        task->user_elf_pages[index] = loaded.pages[index].physical_address;
        task->user_elf_virtual_pages[index] = loaded.pages[index].virtual_address;
        task->user_elf_page_flags[index] = loaded.pages[index].vmm_flags;
    }
    for (index = 0u; index < TASK_USER_MMAP_MAX_PAGES; ++index) {
        task->user_mmap_pages[index] = PADDR_INVALID;
        task->user_mmap_virtual_pages[index] = 0u;
    }
    task->user_mmap_page_count = 0u;
    task->user_mmap_next = TASK_USER_MMAP_BASE;

    task->elf_program_headers = loaded.program_headers;
    task->elf_load_segments = loaded.load_segments;
    task->elf_file_bytes = loaded.file_bytes;
    task->elf_memory_bytes = loaded.memory_bytes;
    task->elf_image_size = elf_size;
    task->elf_backed = 1;
    task->user_entry = loaded.entry;
    task->user_stack_top = TASK_USER_STACK_TOP;
    task->fault_vector = 0u;
    task->fault_error_code = 0u;
    task->fault_address = 0u;
    task->exit_code = 0;
    task->wake_tick = 0u;

    /* Exec drops shared-memory attachments from the replaced image. */
    ipc_task_detach_all(task->id, old_root);

    /* The new root is active, so the old user page tables can now be released. */
    if (old_root != vmm_kernel_address_space()) {
        (void)vmm_destroy_user_address_space(old_root);
    }
    if (old_code != PADDR_INVALID) {
        (void)pmm_free_page(old_code);
    }
    if (old_data != PADDR_INVALID) {
        (void)pmm_free_page(old_data);
    }
    for (index = 0u; index < old_elf_count; ++index) {
        if (old_elf_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(old_elf_pages[index]);
        }
    }
    for (index = 0u; index < old_mmap_count; ++index) {
        if (old_mmap_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(old_mmap_pages[index]);
        }
    }
    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        if (old_stack_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(old_stack_pages[index]);
        }
    }

    *entry_out = loaded.entry;
    *stack_out = TASK_USER_STACK_TOP - 8ULL;
    return 1;

fail_new_image:
    /* Tear down page tables first, then release the leaf physical frames. */
    if (vmm_current_address_space() == new_root) {
        (void)vmm_activate_address_space(task->address_space);
    }
    (void)vmm_destroy_user_address_space(new_root);

    for (index = 0u; index < loaded.page_count; ++index) {
        if (loaded.pages[index].physical_address != PADDR_INVALID) {
            (void)pmm_free_page(loaded.pages[index].physical_address);
        }
    }
    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        if (new_stack_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(new_stack_pages[index]);
        }
    }
    return 0;
}


static int clone_leaf_page(
    paddr_t destination_root,
    vaddr_t virtual_address,
    paddr_t source_page,
    uint64_t flags,
    paddr_t *destination_page_out
)
{
    paddr_t destination_page;
    void *source_bytes;
    void *destination_bytes;

    if (source_page == PADDR_INVALID || destination_page_out == 0) {
        return 0;
    }

    destination_page = pmm_alloc_page();
    if (destination_page == PADDR_INVALID) {
        return 0;
    }

    source_bytes = pmm_phys_to_hhdm(source_page);
    destination_bytes = pmm_phys_to_hhdm(destination_page);
    if (source_bytes == 0 || destination_bytes == 0) {
        (void)pmm_free_page(destination_page);
        return 0;
    }

    bytes_copy(destination_bytes, source_bytes, VMM_PAGE_SIZE);

    if (!vmm_map_page_in_address_space(
            destination_root,
            virtual_address,
            destination_page,
            flags
        )) {
        (void)pmm_free_page(destination_page);
        return 0;
    }

    *destination_page_out = destination_page;
    return 1;
}

int scheduler_mmap_current(size_t length, vaddr_t *address_out)
{
    struct task *task;
    size_t pages;
    size_t start_index;
    size_t mapped = 0u;
    vaddr_t base;
    size_t index;

    if (!initialized || !running || interrupts_enabled() || length == 0u ||
        address_out == 0 || current_index >= task_count_value) {
        return 0;
    }

    task = &tasks[current_index];
    if (task->privilege != TASK_PRIVILEGE_USER || task->address_space == PADDR_INVALID) {
        return 0;
    }

    if (length > (size_t)TASK_USER_MMAP_MAX_PAGES * VMM_PAGE_SIZE) return 0;
    pages = (length + VMM_PAGE_SIZE - 1u) / VMM_PAGE_SIZE;
    if (pages == 0u || task->user_mmap_page_count + pages > TASK_USER_MMAP_MAX_PAGES) return 0;
    if (task->user_mmap_next > TASK_USER_STACK_TOP - (vaddr_t)(pages * VMM_PAGE_SIZE)) return 0;

    base = task->user_mmap_next;
    start_index = task->user_mmap_page_count;

    for (index = 0u; index < pages; ++index) {
        paddr_t page = pmm_alloc_page();
        void *bytes;
        vaddr_t address = base + (vaddr_t)(index * VMM_PAGE_SIZE);
        if (page == PADDR_INVALID) goto fail;
        bytes = pmm_phys_to_hhdm(page);
        if (bytes == 0) { (void)pmm_free_page(page); goto fail; }
        bytes_clear(bytes, VMM_PAGE_SIZE);
        if (!vmm_map_page_in_address_space(
                task->address_space,
                address,
                page,
                VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE
            )) {
            (void)pmm_free_page(page);
            goto fail;
        }
        task->user_mmap_pages[start_index + index] = page;
        task->user_mmap_virtual_pages[start_index + index] = address;
        ++mapped;
    }

    task->user_mmap_page_count += pages;
    task->user_mmap_next += (vaddr_t)(pages * VMM_PAGE_SIZE);
    *address_out = base;
    return 1;

fail:
    for (index = 0u; index < mapped; ++index) {
        size_t slot = start_index + index;
        (void)vmm_unmap_page_in_address_space(task->address_space, task->user_mmap_virtual_pages[slot]);
        if (task->user_mmap_pages[slot] != PADDR_INVALID) (void)pmm_free_page(task->user_mmap_pages[slot]);
        task->user_mmap_pages[slot] = PADDR_INVALID;
        task->user_mmap_virtual_pages[slot] = 0u;
    }
    return 0;
}

static struct interrupt_frame *build_fork_child_frame(
    struct task *child,
    const struct syscall_frame *parent_frame
)
{
    uintptr_t address;
    struct interrupt_frame *frame;

    address = child->kernel_stack_top - sizeof(struct interrupt_frame);
    frame = (struct interrupt_frame *)address;
    frame_clear(frame);

    frame->r15 = parent_frame->r15;
    frame->r14 = parent_frame->r14;
    frame->r13 = parent_frame->r13;
    frame->r12 = parent_frame->r12;
    frame->r11 = parent_frame->user_rflags;
    frame->r10 = parent_frame->r10;
    frame->r9 = parent_frame->r9;
    frame->r8 = parent_frame->r8;
    frame->rbp = parent_frame->rbp;
    frame->rdi = parent_frame->rdi;
    frame->rsi = parent_frame->rsi;
    frame->rdx = parent_frame->rdx;
    frame->rcx = parent_frame->user_rip;
    frame->rbx = parent_frame->rbx;
    frame->rax = 0u; /* fork() returns zero in the child. */
    frame->rip = parent_frame->user_rip;
    frame->cs = GDT_USER_CODE;
    frame->rflags = parent_frame->user_rflags;
    frame->rsp = parent_frame->user_rsp;
    frame->ss = GDT_USER_DATA;
    return frame;
}

int scheduler_fork_current(
    const struct syscall_frame *parent_frame,
    uint64_t *child_id_out
)
{
    struct task *parent;
    struct task *child;
    size_t slot_index;
    size_t index;

    if (!initialized || !running || interrupts_enabled() ||
        parent_frame == 0 || current_index >= task_count_value) {
        return 0;
    }

    parent = &tasks[current_index];
    if (parent->privilege != TASK_PRIVILEGE_USER ||
        parent->address_space == PADDR_INVALID) {
        return 0;
    }

    child = prepare_creation_slot(&slot_index);
    if (child == 0 || !allocate_kernel_stack(child)) {
        return 0;
    }

    child->address_space = vmm_create_user_address_space();
    if (child->address_space == PADDR_INVALID) {
        free_user_task_resources(child);
        return 0;
    }

    if (parent->user_code_page != PADDR_INVALID &&
        !clone_leaf_page(
            child->address_space,
            TASK_USER_CODE_BASE,
            parent->user_code_page,
            VMM_FLAG_USER,
            &child->user_code_page
        )) {
        free_user_task_resources(child);
        return 0;
    }

    if (parent->user_data_page != PADDR_INVALID &&
        !clone_leaf_page(
            child->address_space,
            TASK_USER_DATA_BASE,
            parent->user_data_page,
            VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE,
            &child->user_data_page
        )) {
        free_user_task_resources(child);
        return 0;
    }

    child->user_elf_page_count = parent->user_elf_page_count;
    for (index = 0u; index < parent->user_elf_page_count; ++index) {
        if (!clone_leaf_page(
                child->address_space,
                parent->user_elf_virtual_pages[index],
                parent->user_elf_pages[index],
                parent->user_elf_page_flags[index],
                &child->user_elf_pages[index]
            )) {
            free_user_task_resources(child);
            return 0;
        }
        child->user_elf_virtual_pages[index] =
            parent->user_elf_virtual_pages[index];
        child->user_elf_page_flags[index] = parent->user_elf_page_flags[index];
    }

    child->user_mmap_page_count = parent->user_mmap_page_count;
    child->user_mmap_next = parent->user_mmap_next;
    for (index = 0u; index < parent->user_mmap_page_count; ++index) {
        if (!clone_leaf_page(
                child->address_space,
                parent->user_mmap_virtual_pages[index],
                parent->user_mmap_pages[index],
                VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE,
                &child->user_mmap_pages[index]
            )) {
            free_user_task_resources(child);
            return 0;
        }
        child->user_mmap_virtual_pages[index] = parent->user_mmap_virtual_pages[index];
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        const vaddr_t address =
            TASK_USER_STACK_TOP -
            (vaddr_t)((TASK_USER_STACK_PAGES - index) * VMM_PAGE_SIZE);

        if (!clone_leaf_page(
                child->address_space,
                address,
                parent->user_stack_pages[index],
                VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE,
                &child->user_stack_pages[index]
            )) {
            free_user_task_resources(child);
            return 0;
        }
    }

    if (!task_share_files(child, parent)) {
        free_user_task_resources(child);
        return 0;
    }

    child->id = next_task_id++;
    child->parent_id = parent->id;
    child->name = parent->name != 0 ? parent->name : "fork-child";
    mark_task_ready(child);
    child->privilege = TASK_PRIVILEGE_USER;
    child->uid = parent->uid;
    child->gid = parent->gid;
    child->priority = parent->priority;
    child->effective_priority = parent->priority;
    child->affinity_mask = parent->affinity_mask;
    child->ready_since_tick = total_scheduling_ticks;
    child->elf_program_headers = parent->elf_program_headers;
    child->elf_load_segments = parent->elf_load_segments;
    child->elf_file_bytes = parent->elf_file_bytes;
    child->elf_memory_bytes = parent->elf_memory_bytes;
    child->elf_image_size = parent->elf_image_size;
    child->elf_backed = parent->elf_backed;
    child->user_entry = parent->user_entry;
    child->user_stack_top = parent->user_stack_top;
    child->saved_frame = build_fork_child_frame(child, parent_frame);
    child->wait_collected = 0;

    /* POSIX-like fork semantics for Phase-22 shared memory: the child sees
     * the same physical IPC pages at the same virtual addresses. */
    if (!ipc_task_clone_shared(parent->id, child->id, child->address_space)) {
        free_user_task_resources(child);
        return 0;
    }

    if (child_id_out != 0) {
        *child_id_out = child->id;
    }

    commit_creation_slot(slot_index);
    return 1;
}

static void wake_waiters_for_child(uint64_t child_id)
{
    size_t index;

    for (index = 0u; index < task_count_value; ++index) {
        if (tasks[index].state == TASK_BLOCKED &&
            tasks[index].wait_target_id == child_id) {
            mark_task_ready(&tasks[index]);
            tasks[index].wait_target_id = UINT64_MAX;
        }
    }
}

int scheduler_wait_current_child(uint64_t child_id, int64_t *status_out)
{
    struct task *parent;
    struct task *child;
    const uint64_t parent_id =
        current_index < task_count_value ? tasks[current_index].id : UINT64_MAX;

    if (!initialized || !running || interrupts_enabled() ||
        current_index >= task_count_value || status_out == 0) {
        return 0;
    }

    parent = &tasks[current_index];

    for (;;) {
        child = scheduler_task_by_id_mutable(child_id);
        if (child == 0 || child->parent_id != parent_id || child->wait_collected) {
            return 0;
        }

        if (child->state == TASK_TERMINATED) {
            *status_out = child->exit_code;
            child->wait_collected = 1;
            return 1;
        }

        parent->state = TASK_BLOCKED;
        parent->wait_target_id = child_id;
        parent->ticks_in_slice = parent->quantum_ticks;

        interrupts_enable();
        for (;;) {
            __asm__ volatile ("hlt" ::: "memory");

            if (current_index < task_count_value &&
                tasks[current_index].id == parent_id &&
                tasks[current_index].state == TASK_RUNNING) {
                break;
            }
        }
        interrupts_disable();
    }
}

int scheduler_prepare_block_current(uint64_t expected_task_id)
{
    struct task *current;

    if (!initialized || !running || interrupts_enabled() ||
        current_index >= task_count_value) {
        return 0;
    }

    current = &tasks[current_index];
    if (current->id != expected_task_id || current->state != TASK_RUNNING) {
        return 0;
    }

    current->state = TASK_BLOCKED;
    current->wait_target_id = UINT64_MAX;
    current->ticks_in_slice = current->quantum_ticks;
    return 1;
}

int scheduler_park_current(uint64_t expected_task_id)
{
    if (!initialized || !running || interrupts_enabled()) {
        return 0;
    }

    /*
     * The caller has already made the current task non-runnable while IF=0.
     * Enabling interrupts lets the next APIC timer interrupt switch away. A
     * wakeup changes this task to READY; once scheduled again it is RUNNING
     * and execution resumes here.
     */
    interrupts_enable();

    for (;;) {
        if (current_index < task_count_value &&
            tasks[current_index].id == expected_task_id &&
            tasks[current_index].state == TASK_RUNNING) {
            break;
        }

        __asm__ volatile ("hlt" ::: "memory");
    }

    interrupts_disable();
    return 1;
}

int scheduler_wake_task(uint64_t task_id)
{
    struct task *task;

    if (!initialized || !running || interrupts_enabled()) {
        return 0;
    }

    task = scheduler_task_by_id_mutable(task_id);
    if (task == 0 || task->state != TASK_BLOCKED) {
        return 0;
    }

    mark_task_ready(task);
    task->wait_target_id = UINT64_MAX;
    return 1;
}

int scheduler_terminate_task(uint64_t task_id, uint32_t signal_number)
{
    struct task *target;

    if (!initialized || !running || interrupts_enabled() || signal_number == 0u) {
        return 0;
    }

    target = scheduler_task_by_id_mutable(task_id);
    if (target == 0 || target->privilege != TASK_PRIVILEGE_USER ||
        target->state == TASK_TERMINATED) {
        return 0;
    }

    if (current_index < task_count_value && target == &tasks[current_index]) {
        task_exit_current((int64_t)(128u + signal_number));
    }

    task_close_all_files(target);
    target->termination_signal = signal_number;
    target->exit_code = (int64_t)(128u + signal_number);
    target->state = TASK_TERMINATED;
    target->wake_tick = 0u;
    target->wait_target_id = UINT64_MAX;
    wake_waiters_for_child(target->id);
    return 1;
}


int scheduler_start(void)
{
    if (!initialized || running || interrupts_enabled() || task_count_value < 2u) {
        return 0;
    }

    running = 1;
    return 1;
}

int scheduler_running(void)
{
    return running;
}

int scheduler_set_policy(uint32_t policy)
{
    size_t index;

    if (!initialized || interrupts_enabled() ||
        (policy != AXIOM_SCHED_POLICY_ROUND_ROBIN &&
         policy != AXIOM_SCHED_POLICY_PRIORITY_AGING)) {
        return 0;
    }

    scheduler_policy = policy;
    for (index = 0u; index < task_count_value; ++index) {
        struct task *task = &tasks[index];

        task->effective_priority = task->priority;
        if (task->state == TASK_READY) {
            task->ready_since_tick = total_scheduling_ticks;
        }
    }
    if (current_index < task_count_value) {
        tasks[current_index].ticks_in_slice = 0u;
    }
    return 1;
}

uint32_t scheduler_get_policy(void)
{
    return scheduler_policy;
}

int scheduler_set_current_priority(uint32_t priority)
{
    struct task *current;

    if (!initialized || interrupts_enabled() ||
        current_index >= task_count_value ||
        priority < AXIOM_SCHED_PRIORITY_MIN ||
        priority > AXIOM_SCHED_PRIORITY_MAX) {
        return 0;
    }

    current = &tasks[current_index];
    current->priority = priority;
    current->effective_priority = priority;
    return 1;
}

uint32_t scheduler_get_current_priority(void)
{
    if (!initialized || current_index >= task_count_value) {
        return AXIOM_SCHED_PRIORITY_DEFAULT;
    }
    return tasks[current_index].priority;
}

uint32_t scheduler_get_current_effective_priority(void)
{
    if (!initialized || current_index >= task_count_value) {
        return AXIOM_SCHED_PRIORITY_DEFAULT;
    }
    return tasks[current_index].effective_priority;
}

int scheduler_set_current_affinity(uint64_t affinity_mask)
{
    if (!initialized || interrupts_enabled() ||
        current_index >= task_count_value ||
        affinity_mask == 0u ||
        (affinity_mask & AXIOM_CPU_AFFINITY_BSP) == 0u) {
        return 0;
    }

    /*
     * Phase 17 brought APs online for validation and then parked them. Until
     * normal task scheduling becomes multicore, every runnable task must keep
     * CPU0 in its mask. The extra bits are retained as future affinity intent.
     */
    tasks[current_index].affinity_mask = affinity_mask;
    return 1;
}

uint64_t scheduler_get_current_affinity(void)
{
    if (!initialized || current_index >= task_count_value) {
        return AXIOM_CPU_AFFINITY_BSP;
    }
    return tasks[current_index].affinity_mask;
}

void scheduler_benchmark_compare(struct scheduler_benchmark_result *result)
{
    static const uint32_t base_priority[3] = {1u, 4u, 7u};
    uint64_t waiting_ticks[3] = {0u, 0u, 0u};
    size_t previous = 2u;
    size_t decision;
    size_t task;

    if (result == 0) {
        return;
    }

    for (task = 0u; task < 3u; ++task) {
        result->round_robin[task] = 0u;
        result->priority_aging[task] = 0u;
    }
    result->decisions = 96u;

    for (decision = 0u; decision < result->decisions; ++decision) {
        result->round_robin[decision % 3u] += 1u;
    }

    for (decision = 0u; decision < result->decisions; ++decision) {
        size_t selected = 3u;
        uint32_t selected_priority = 0u;
        size_t step;

        for (step = 1u; step <= 3u; ++step) {
            const size_t index = (previous + step) % 3u;
            uint32_t effective = base_priority[index] +
                (uint32_t)(waiting_ticks[index] /
                    (uint64_t)SCHEDULER_AGING_INTERVAL_TICKS);

            if (effective > AXIOM_SCHED_PRIORITY_MAX) {
                effective = AXIOM_SCHED_PRIORITY_MAX;
            }
            if (selected == 3u || effective > selected_priority) {
                selected = index;
                selected_priority = effective;
            }
        }

        result->priority_aging[selected] += 1u;
        for (task = 0u; task < 3u; ++task) {
            if (task == selected) {
                waiting_ticks[task] = 0u;
            } else {
                waiting_ticks[task] += SCHEDULER_DEFAULT_QUANTUM_TICKS;
            }
        }
        previous = selected;
    }
}

static void wake_sleeping_tasks(void)
{
    size_t index;

    for (index = 0u; index < task_count_value; ++index) {
        struct task *task = &tasks[index];

        if (task->state == TASK_SLEEPING &&
            task->wake_tick <= total_scheduling_ticks) {
            mark_task_ready(task);
            task->wake_tick = 0u;
        }
    }
}


struct interrupt_frame *scheduler_on_timer_interrupt(struct interrupt_frame *frame)
{
    struct task *current;
    size_t next_index;
    int priority_preemption = 0;

    if (!running || frame == 0 || task_count_value == 0u) {
        return frame;
    }

    current = &tasks[current_index];
    verify_kernel_stack_canary(current);
    current->saved_frame = frame;

    ++total_scheduling_ticks;
    wake_sleeping_tasks();
    age_ready_tasks();
    ++current->runtime_ticks;
    ++current->ticks_in_slice;

    if (force_voluntary_rotation && current->state == TASK_RUNNING) {
        force_voluntary_rotation = 0;
        next_index = find_next_ready_round_robin(current_index);
        if (next_index == SCHEDULER_MAX_TASKS) {
            current->ticks_in_slice = 0u;
            return frame;
        }
        mark_task_ready(current);
        current->ticks_in_slice = 0u;
        ++total_preemptions;
        return switch_to(next_index);
    }

    if (current->state == TASK_RUNNING &&
        current->ticks_in_slice < current->quantum_ticks) {
        if (!higher_priority_ready_than(current)) {
            return frame;
        }
        priority_preemption = 1;
    }

    /*
     * Under priority+aging the current task competes again when its quantum
     * expires. A highest-priority task may therefore receive another slice;
     * lower priorities eventually catch it through aging. Round-robin keeps
     * the original Phase-8 behaviour and always chooses another READY task.
     */
    if (scheduler_policy == AXIOM_SCHED_POLICY_PRIORITY_AGING &&
        current->state == TASK_RUNNING) {
        mark_task_ready(current);
    }

    next_index = find_next_ready(current_index);

    if (next_index == SCHEDULER_MAX_TASKS) {
        if (current->state == TASK_RUNNING) {
            current->ticks_in_slice = 0u;
            return frame;
        }

        kernel_panic("scheduler has no runnable task");
    }

    if (scheduler_policy == AXIOM_SCHED_POLICY_PRIORITY_AGING &&
        next_index == current_index) {
        current->state = TASK_RUNNING;
        current->ticks_in_slice = 0u;
        current->effective_priority = current->priority;
        current->ready_since_tick = total_scheduling_ticks;
        return frame;
    }

    if (current->state == TASK_RUNNING) {
        mark_task_ready(current);
    }
    current->ticks_in_slice = 0u;
    if (priority_preemption) {
        ++total_priority_preemptions;
    }
    ++total_preemptions;
    return switch_to(next_index);
}

struct interrupt_frame *scheduler_handle_user_fault(
    struct interrupt_frame *frame,
    uint64_t vector,
    uint64_t error_code,
    vaddr_t fault_address
)
{
    struct task *current;
    size_t next_index;

    if (!running || frame == 0 || current_index >= task_count_value) {
        return 0;
    }

    current = &tasks[current_index];

    if (current->privilege != TASK_PRIVILEGE_USER ||
        (frame->cs & 3ULL) != 3ULL) {
        return 0;
    }

    current->saved_frame = frame;
    task_close_all_files(current);
    current->state = TASK_TERMINATED;
    current->fault_vector = vector;
    current->fault_error_code = error_code;
    current->fault_address = fault_address;
    current->exit_code = (int64_t)(128u + vector);
    current->termination_signal = 0u;
    wake_waiters_for_child(current->id);
    ++total_user_fault_terminations;

    next_index = find_next_ready(current_index);
    if (next_index == SCHEDULER_MAX_TASKS) {
        return 0;
    }

    return switch_to(next_index);
}

int scheduler_sleep_current(uint64_t ticks)
{
    struct task *current;
    const uint64_t task_id =
        (current_index < task_count_value) ? tasks[current_index].id : UINT64_MAX;

    if (!running || !initialized || current_index >= task_count_value ||
        interrupts_enabled()) {
        return 0;
    }

    if (ticks == 0u) {
        return scheduler_yield_current();
    }

    current = &tasks[current_index];
    current->state = TASK_SLEEPING;
    current->wake_tick = total_scheduling_ticks + ticks;
    current->ticks_in_slice = current->quantum_ticks;

    interrupts_enable();

    for (;;) {
        __asm__ volatile ("hlt" ::: "memory");

        if (current_index < task_count_value &&
            tasks[current_index].id == task_id &&
            tasks[current_index].state == TASK_RUNNING &&
            total_scheduling_ticks >= current->wake_tick) {
            break;
        }
    }

    interrupts_disable();
    current->wake_tick = 0u;
    return 1;
}

int scheduler_yield_current(void)
{
    struct task *current;
    const uint64_t before_switches = total_context_switches;
    const uint64_t before_ticks = total_scheduling_ticks;
    const uint64_t task_id =
        (current_index < task_count_value) ? tasks[current_index].id : UINT64_MAX;

    if (!running || !initialized || current_index >= task_count_value ||
        interrupts_enabled()) {
        return 0;
    }

    current = &tasks[current_index];
    current->ticks_in_slice = current->quantum_ticks;
    force_voluntary_rotation = 1;
    ++total_voluntary_yields;

    interrupts_enable();

    for (;;) {
        if (current_index < task_count_value &&
            tasks[current_index].id == task_id) {
            if (total_context_switches != before_switches ||
                total_scheduling_ticks != before_ticks) {
                break;
            }
        }
        __asm__ volatile ("hlt" ::: "memory");
    }

    interrupts_disable();
    return 1;
}

_Noreturn void task_exit_current(int64_t status)
{
    if (!running || !initialized || current_index >= task_count_value) {
        kernel_panic("task_exit_current called without a running scheduler");
    }

    interrupts_disable();
    task_close_all_files(&tasks[current_index]);
    tasks[current_index].exit_code = status;
    tasks[current_index].termination_signal = 0u;
    tasks[current_index].state = TASK_TERMINATED;
    tasks[current_index].ticks_in_slice = tasks[current_index].quantum_ticks;
    wake_waiters_for_child(tasks[current_index].id);

    for (;;) {
        __asm__ volatile ("sti; hlt" ::: "memory");
    }
}

struct task *scheduler_current_task_mutable(void)
{
    if (!initialized || current_index >= task_count_value) {
        return 0;
    }

    return &tasks[current_index];
}

const struct task *scheduler_current_task(void)
{
    return scheduler_current_task_mutable();
}

struct scheduler_stats scheduler_get_stats(void)
{
    struct scheduler_stats stats;
    uint64_t runnable = 0u;
    size_t index;

    for (index = 0u; index < task_count_value; ++index) {
        if (tasks[index].state == TASK_RUNNING ||
            tasks[index].state == TASK_READY) {
            ++runnable;
        }
    }

    stats.task_count = task_count_value;
    stats.runnable_tasks = runnable;
    stats.context_switches = total_context_switches;
    stats.preemptions = total_preemptions;
    stats.priority_preemptions = total_priority_preemptions;
    stats.scheduling_ticks = total_scheduling_ticks;
    stats.aging_promotions = total_aging_promotions;
    stats.voluntary_yields = total_voluntary_yields;
    stats.user_fault_terminations = total_user_fault_terminations;
    stats.current_task_id =
        task_count_value != 0u ? tasks[current_index].id : UINT64_MAX;
    stats.current_affinity_mask =
        task_count_value != 0u ? tasks[current_index].affinity_mask : 0u;
    stats.policy = scheduler_policy;
    stats.current_priority =
        task_count_value != 0u ? tasks[current_index].priority : 0u;
    stats.current_effective_priority =
        task_count_value != 0u ? tasks[current_index].effective_priority : 0u;
    return stats;
}

size_t scheduler_task_count(void)
{
    return task_count_value;
}

const struct task *scheduler_task_at(size_t index)
{
    if (index >= task_count_value) {
        return 0;
    }

    return &tasks[index];
}

struct task *scheduler_task_by_id_mutable(uint64_t id)
{
    size_t index;

    for (index = 0u; index < task_count_value; ++index) {
        if (tasks[index].id == id) {
            return &tasks[index];
        }
    }

    return 0;
}

const struct task *scheduler_task_by_id(uint64_t id)
{
    return scheduler_task_by_id_mutable(id);
}
