#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/syscall.h>
#include <axiom/abi/user_layout.h>
#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/arch/smp.h>

#include <axiom/boot/limine.h>
#include <axiom/drivers/keyboard.h>
#include <axiom/drivers/mouse.h>
#include <axiom/drivers/block.h>
#include <axiom/drivers/serial.h>
#include <axiom/drivers/timer.h>
#include <axiom/elf/elf64.h>
#include <axiom/filesystem/bootstrap.h>
#include <axiom/filesystem/diskfs.h>
#include <axiom/filesystem/procfs.h>
#include <axiom/filesystem/vfs.h>
#include <axiom/graphics/framebuffer.h>
#include <axiom/kernel/syscall.h>
#include <axiom/ipc/ipc.h>
#include <axiom/memory/heap.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>
#include <axiom/network/net.h>
#include <axiom/process/scheduler.h>
#include <axiom/sync/selftest.h>

#include <axiom/terminal/kprintf.h>
#include <axiom/terminal/terminal.h>



#define PHASE8_WORKER_TARGET 1000ULL
#define PHASE8_TEST_TIMEOUT_TICKS 300ULL
#define PHASE9_TEST_TIMEOUT_TICKS 500ULL
#define PHASE10_TEST_TIMEOUT_TICKS 1500ULL
#define PHASE11_TEST_TIMEOUT_TICKS 1000ULL
#define PHASE12_TEST_TIMEOUT_TICKS 1000ULL
#define PHASE13_TEST_TIMEOUT_TICKS 1500ULL
#define PHASE9_KERNEL_PROBE_ADDRESS 0xFFFFFFFF80000000ULL

static volatile uint64_t phase8_worker_a_count;
static volatile uint64_t phase8_worker_b_count;
static volatile int phase8_workers_stop;

static const char phase13_persistent_message[] =
    "Persistent storage works across AxiomOS reboots!\n";

static int phase13_bytes_equal(
    const void *left_memory,
    const void *right_memory,
    size_t count
)
{
    const uint8_t *left = (const uint8_t *)left_memory;
    const uint8_t *right = (const uint8_t *)right_memory;
    size_t index;

    if (left == 0 || right == 0) {
        return 0;
    }

    for (index = 0u; index < count; ++index) {
        if (left[index] != right[index]) {
            return 0;
        }
    }
    return 1;
}

static int phase13_persistent_file_matches(int *exists_out)
{
    void *data = 0;
    size_t size = 0u;
    int result;

    if (exists_out == 0) {
        return 0;
    }

    result = vfs_read_all("/disk/persist.txt", &data, &size);
    if (result == -AXIOM_ENOENT) {
        *exists_out = 0;
        return 1;
    }
    if (result < 0) {
        return 0;
    }

    *exists_out = 1;
    result = size == sizeof(phase13_persistent_message) - 1u &&
        phase13_bytes_equal(
            data,
            phase13_persistent_message,
            sizeof(phase13_persistent_message) - 1u
        );
    kfree(data);
    return result;
}


extern const uint8_t phase9_user_hello_start[];
extern const uint8_t phase9_user_hello_end[];
extern const uint8_t phase9_user_fault_start[];
extern const uint8_t phase9_user_fault_end[];
extern const uint8_t phase10_user_start[];
extern const uint8_t phase10_user_end[];

static int phase9_message_ready(const struct task *task, char *message, size_t capacity)
{
    const uint8_t *data;
    const uint64_t *magic;
    size_t index;

    if (task == 0 || task->user_data_page == PADDR_INVALID ||
        message == 0 || capacity == 0u) {
        return 0;
    }

    data = (const uint8_t *)pmm_phys_to_hhdm(task->user_data_page);
    if (data == 0) {
        return 0;
    }

    magic = (const uint64_t *)(const void *)(
        data + TASK_USER_MESSAGE_MAGIC_OFFSET
    );

    if (*magic != TASK_USER_MESSAGE_MAGIC) {
        return 0;
    }

    for (index = 0u; index + 1u < capacity; ++index) {
        message[index] = (char)data[index];

        if (message[index] == '\0') {
            return 1;
        }
    }

    message[capacity - 1u] = '\0';
    return 1;
}

static int phase10_results_ready(
    const struct task *task,
    uint64_t *pid_out,
    int64_t *bad_write_out,
    int64_t *open_result_out,
    char *read_character_out
)
{
    const uint8_t *data;
    uint64_t magic;

    if (task == 0 || task->user_data_page == PADDR_INVALID ||
        pid_out == 0 || bad_write_out == 0 || open_result_out == 0 ||
        read_character_out == 0) {
        return 0;
    }

    data = (const uint8_t *)pmm_phys_to_hhdm(task->user_data_page);
    if (data == 0) {
        return 0;
    }

    magic = *(const uint64_t *)(const void *)(
        data + AXIOM_PHASE10_MAGIC_OFFSET
    );

    if (magic != AXIOM_PHASE10_MAGIC) {
        return 0;
    }

    *pid_out = *(const uint64_t *)(const void *)(
        data + AXIOM_PHASE10_PID_OFFSET
    );
    *bad_write_out = *(const int64_t *)(const void *)(
        data + AXIOM_PHASE10_BAD_WRITE_OFFSET
    );
    *open_result_out = *(const int64_t *)(const void *)(
        data + AXIOM_PHASE10_OPEN_RESULT_OFFSET
    );
    *read_character_out = (char)data[AXIOM_PHASE10_READ_CHAR_OFFSET];
    return 1;
}


static int phase11_results_ready(
    const struct task *task,
    uint64_t *pid_out,
    uint64_t *bss_zero_out,
    uint64_t *bss_probe_out
)
{
    uint64_t values[4];

    if (task == 0 || pid_out == 0 || bss_zero_out == 0 || bss_probe_out == 0 ||
        !task->elf_backed || task->address_space == PADDR_INVALID) {
        return 0;
    }

    if (!vmm_copy_from_user(
            task->address_space,
            values,
            (vaddr_t)AXIOM_PHASE11_DATA_BASE,
            sizeof(values)
        )) {
        return 0;
    }

    if (values[1] != AXIOM_PHASE11_MAGIC) {
        return 0;
    }

    *pid_out = values[0];
    *bss_zero_out = values[2];
    *bss_probe_out = values[3];
    return 1;
}

static int phase11_permissions_ok(const struct task *task)
{
    uint64_t text_flags;
    uint64_t rodata_flags;
    uint64_t data_flags;

    if (task == 0 || task->address_space == PADDR_INVALID ||
        !vmm_mapping_flags_in_address_space(
            task->address_space,
            AXIOM_PHASE11_TEXT_BASE,
            &text_flags
        ) ||
        !vmm_mapping_flags_in_address_space(
            task->address_space,
            AXIOM_PHASE11_RODATA_BASE,
            &rodata_flags
        ) ||
        !vmm_mapping_flags_in_address_space(
            task->address_space,
            AXIOM_PHASE11_DATA_BASE,
            &data_flags
        )) {
        return 0;
    }

    return
        (text_flags & VMM_FLAG_USER) != 0ULL &&
        (text_flags & VMM_FLAG_WRITABLE) == 0ULL &&
        (text_flags & VMM_FLAG_NO_EXECUTE) == 0ULL &&
        (rodata_flags & VMM_FLAG_USER) != 0ULL &&
        (rodata_flags & VMM_FLAG_WRITABLE) == 0ULL &&
        (rodata_flags & VMM_FLAG_NO_EXECUTE) != 0ULL &&
        (data_flags & VMM_FLAG_USER) != 0ULL &&
        (data_flags & VMM_FLAG_WRITABLE) != 0ULL &&
        (data_flags & VMM_FLAG_NO_EXECUTE) != 0ULL;
}


static void phase8_counter_worker(void *argument)
{
    volatile uint64_t *counter = (volatile uint64_t *)argument;

    /*
     * These workers exist only to prove timer-driven preemption in Phase 8.
     * Leaving them alive forever would burn two full scheduler quanta in every
     * later round-robin cycle and distort blocking/synchronization tests.
     */
    while (!phase8_workers_stop) {
        ++(*counter);
        __asm__ volatile ("pause" ::: "memory");
    }

    task_exit_current(0);
}


void kernel_main(void)
{
    uint64_t line;
    uint64_t scroll_lines;
    struct pmm_stats memory_stats;


    /*
     * Phase-1 early debugging output remains available.
     */
    serial_init();


    if (!limine_base_revision_supported()) {
        serial_write_string(
            "AxiomOS boot error: "
            "unsupported Limine protocol revision.\n"
        );

        return;
    }


    /*
     * Keep this exact line so the Phase-1 regression test continues to work.
     */
    serial_write_string(
        "AxiomOS kernel booted successfully.\n"
    );


    /*
     * Initialise the Phase-2 framebuffer terminal.
     */
    if (!terminal_init()) {
        serial_write_string(
            "AxiomOS terminal error: "
            "framebuffer terminal unavailable.\n"
        );

        return;
    }


    /*
     * Exercise scrolling by printing slightly more than one screen of text.
     */
    scroll_lines =
        (uint64_t)terminal_rows()
        +
        3ULL;


    for (line = 1; line <= scroll_lines; ++line) {
        kprintf(
            "Scroll exercise line %llu\n",
            (unsigned long long)line
        );
    }


    terminal_clear();


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );


    kprintf(
        "AxiomOS Phase 2 terminal online.\n"
    );


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


    kprintf(
        "Terminal geometry: %llux%llu cells\n",
        (unsigned long long)terminal_columns(),
        (unsigned long long)terminal_rows()
    );


    kprintf(
        "Decimal test: %u\n",
        123456789u
    );


    kprintf(
        "Hex test: 0x%X\n",
        0xDEADBEEFu
    );


    kprintf(
        "64-bit test: %llu\n",
        18446744073709551615ULL
    );


    kprintf(
        "Kernel entry: %p\n",
        (void *)(uintptr_t)&kernel_main
    );


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );


    kprintf(
        "Color output: OK\n"
    );


    terminal_set_color(
        TERMINAL_COLOR_YELLOW,
        TERMINAL_COLOR_BLACK
    );


    kprintf(
        "Phase 2 terminal test complete.\n"
    );


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


    /*
     * Phase 3 CPU architecture initialization and regression tests.
     */
    gdt_init();
    interrupts_init();

    kprintf("GDT/TSS loaded. IDT: 256 gates installed.\n");
    kprintf("PIC remapped: IRQs masked; IF=0.\n");

    phase3_selftest();


    /*
     * Phase 4: discover usable physical RAM and initialise the 4 KiB page
     * allocator. Only LIMINE_MEMMAP_USABLE pages are managed at this stage.
     */
    if (!pmm_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        kprintf("Phase 4 PMM initialization: FAILED\n");
        return;
    }


    memory_stats = pmm_get_stats();


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );

    kprintf("AxiomOS Phase 4 physical memory manager online.\n");


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    kprintf(
        "PMM memory map entries: %llu\n",
        (unsigned long long)memory_stats.memory_map_entries
    );

    kprintf(
        "PMM total RAM-like memory: %llu MiB\n",
        (unsigned long long)(
            memory_stats.total_memory_bytes /
            (1024ULL * 1024ULL)
        )
    );

    kprintf(
        "PMM usable memory: %llu MiB (%llu pages)\n",
        (unsigned long long)(
            memory_stats.usable_memory_bytes /
            (1024ULL * 1024ULL)
        ),
        (unsigned long long)memory_stats.usable_pages
    );

    kprintf(
        "PMM metadata: %llu pages at physical 0x%llX\n",
        (unsigned long long)memory_stats.metadata_pages,
        (unsigned long long)memory_stats.metadata_base
    );

    kprintf(
        "PMM allocated pages: %llu\n",
        (unsigned long long)memory_stats.allocated_pages
    );

    kprintf(
        "PMM free pages: %llu\n",
        (unsigned long long)memory_stats.free_pages
    );


    if (!phase4_pmm_selftest()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        return;
    }


    memory_stats = pmm_get_stats();

    kprintf(
        "PMM post-test allocated pages: %llu\n",
        (unsigned long long)memory_stats.allocated_pages
    );

    kprintf(
        "PMM post-test free pages: %llu\n",
        (unsigned long long)memory_stats.free_pages
    );


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );

    kprintf("Phase 4 physical memory manager complete.\n");


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


    /*
     * Phase 5: clone Limine's active 4-level paging hierarchy into physical
     * pages owned by our PMM, switch CR3, and expose 4 KiB map/unmap/translate
     * primitives. Existing kernel/HHDM/framebuffer mappings are preserved by
     * the clone.
     */
    if (!vmm_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        kprintf("Phase 5 VMM initialization: FAILED\n");
        return;
    }


    {
        const struct vmm_stats virtual_memory = vmm_get_stats();
        const paddr_t kernel_main_physical =
            virt_to_phys((vaddr_t)(uintptr_t)&kernel_main);


        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );

        kprintf("AxiomOS Phase 5 virtual memory manager online.\n");


        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );

        kprintf(
            "VMM root PML4: physical 0x%llX\n",
            (unsigned long long)virtual_memory.root_table
        );

        kprintf(
            "VMM owned page-table pages: %llu\n",
            (unsigned long long)virtual_memory.page_table_pages
        );

        kprintf(
            "VMM HHDM offset: 0x%llX\n",
            (unsigned long long)virtual_memory.hhdm_offset
        );

        kprintf(
            "kernel_main physical address: 0x%llX\n",
            (unsigned long long)kernel_main_physical
        );


        if (kernel_main_physical == PADDR_INVALID) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );

            kprintf("Phase 5 kernel translation check: FAILED\n");
            return;
        }
    }


    if (!phase5_vmm_selftest()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        return;
    }


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );

    kprintf("Phase 5 virtual memory manager complete.\n");


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


#ifdef AXIOM_TEST_PAGE_FAULT
    kprintf("Test: triggering unmapped page read.\n");

    {
        volatile const uint64_t *fault_address =
            (volatile const uint64_t *)(uintptr_t)VMM_FAULT_TEST_ADDRESS;

        volatile uint64_t ignored = *fault_address;
        (void)ignored;
    }

    kprintf("Phase 5 page-fault test: FAILED to fault\n");
#endif

    /*
     * Phase 6: higher-half kernel heap backed by Phase-4 physical pages and
     * Phase-5 virtual mappings. The normal self-test leaves no live
     * allocations but may leave additional heap pages mapped for reuse.
     */
    if (!heap_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        kprintf("Phase 6 heap initialization: FAILED\n");
        return;
    }


    {
        const struct heap_stats initial_heap = heap_get_stats();


        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );

        kprintf("AxiomOS Phase 6 kernel heap online.\n");


        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );

        kprintf(
            "Heap base: 0x%llX\n",
            (unsigned long long)HEAP_BASE_ADDRESS
        );

        kprintf(
            "Heap mapped pages: %llu\n",
            (unsigned long long)initial_heap.mapped_pages
        );

        kprintf(
            "Heap initial free bytes: %llu\n",
            (unsigned long long)initial_heap.free_bytes
        );
    }


    if (!phase6_heap_selftest()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        return;
    }


    {
        const struct heap_stats final_heap = heap_get_stats();

        kprintf(
            "Heap mapped pages after test: %llu\n",
            (unsigned long long)final_heap.mapped_pages
        );

        kprintf(
            "Heap active allocations: %llu\n",
            (unsigned long long)final_heap.active_allocations
        );

        kprintf(
            "Heap bytes in use: %llu\n",
            (unsigned long long)final_heap.bytes_in_use
        );

        kprintf(
            "Heap free bytes: %llu\n",
            (unsigned long long)final_heap.free_bytes
        );

        kprintf(
            "Heap largest free block: %llu\n",
            (unsigned long long)final_heap.largest_free_block
        );

        kprintf(
            "Heap total allocations: %llu\n",
            (unsigned long long)final_heap.total_allocations
        );

        kprintf(
            "Heap total frees: %llu\n",
            (unsigned long long)final_heap.total_frees
        );

        kprintf(
            "Heap reallocations: %llu\n",
            (unsigned long long)final_heap.total_reallocations
        );

        kprintf(
            "Heap failed allocations: %llu\n",
            (unsigned long long)final_heap.failed_allocations
        );
    }


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );

    kprintf("Phase 6 kernel heap complete.\n");


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


#ifdef AXIOM_TEST_HEAP_DOUBLE_FREE
    {
        void *first = kmalloc(64);
        void *second = kmalloc(64);

        if (first == 0 || second == 0) {
            kprintf("Phase 6 double-free setup: FAILED\n");
            return;
        }

        kfree(first);
        kprintf("Test: triggering heap double free.\n");
        kfree(first);

        kprintf("Phase 6 double-free test: FAILED to panic\n");
        kfree(second);
    }
#endif


#ifdef AXIOM_TEST_HEAP_GUARD
    {
        uint8_t *buffer = (uint8_t *)kmalloc(32);

        if (buffer == 0) {
            kprintf("Phase 6 guard-corruption setup: FAILED\n");
            return;
        }

        buffer[32] = 0xA5u;

        kprintf("Test: triggering heap tail-guard corruption.\n");
        kfree(buffer);

        kprintf("Phase 6 guard-corruption test: FAILED to panic\n");
    }
#endif


    /*
     * Phase 7: IRQ-driven time and keyboard input. q35 is an APIC-era
     * platform, so the local APIC provides the periodic timer and the I/O
     * APIC routes the PS/2 keyboard. The legacy PIC remains fully masked.
     */
    if (!timer_init(100u)) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 7 timer initialization: FAILED\n");
        return;
    }

    if (!keyboard_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 7 keyboard initialization: FAILED\n");
        return;
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("AxiomOS Phase 7 timer + keyboard online.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );
    kprintf("APIC timer frequency: %u Hz\n", timer_frequency());
    kprintf("Interrupt controller: Local APIC + I/O APIC\n");

    interrupts_enable();

    if (!interrupts_enabled()) {
        kprintf("Phase 7 interrupt enable: FAILED\n");
        return;
    }

    {
        const uint64_t before = timer_ticks();
        uint64_t elapsed;

        timer_wait_ticks(10u);
        elapsed = timer_ticks() - before;

        if (elapsed < 10u) {
            kprintf("Phase 7 timer test: FAILED\n");
            return;
        }

        kprintf(
            "Timer self-test ticks: %llu\n",
            (unsigned long long)elapsed
        );
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 7 timer test: OK\n");
    kprintf("Phase 7 timer + keyboard drivers complete.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    /*
     * Phase 8: preemptive kernel-thread multitasking. Build every task while
     * IF=0 because task creation allocates private stacks from the Phase-6
     * heap, which is intentionally not interrupt-safe yet.
     */
    interrupts_disable();

    if (!scheduler_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 8 scheduler initialization: FAILED\n");
        return;
    }

    phase8_worker_a_count = 0u;
    phase8_worker_b_count = 0u;
    phase8_workers_stop = 0;

    {
        uint64_t worker_a_id;
        uint64_t worker_b_id;

        if (!task_create(
                "phase8-worker-a",
                phase8_counter_worker,
                (void *)&phase8_worker_a_count,
                &worker_a_id
            ) ||
            !task_create(
                "phase8-worker-b",
                phase8_counter_worker,
                (void *)&phase8_worker_b_count,
                &worker_b_id
            )) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 8 task creation: FAILED\n");
            return;
        }

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 8 preemptive scheduler online.\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Scheduler policy: preemptive round-robin\n");
        kprintf(
            "Scheduler quantum: %u timer ticks\n",
            SCHEDULER_DEFAULT_QUANTUM_TICKS
        );
        kprintf(
            "Phase 8 worker task IDs: %llu, %llu\n",
            (unsigned long long)worker_a_id,
            (unsigned long long)worker_b_id
        );
    }

    if (!scheduler_start()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 8 scheduler start: FAILED\n");
        return;
    }

    interrupts_enable();

    {
        const uint64_t start = timer_ticks();

        while ((phase8_worker_a_count < PHASE8_WORKER_TARGET ||
                phase8_worker_b_count < PHASE8_WORKER_TARGET) &&
               (timer_ticks() - start) < PHASE8_TEST_TIMEOUT_TICKS) {
            /*
             * The bootstrap thread sleeps. Worker A and worker B never yield;
             * only timer preemption can move execution between all three.
             */
            __asm__ volatile ("hlt" ::: "memory");
        }
    }

    {
        const struct scheduler_stats scheduler = scheduler_get_stats();

        if (phase8_worker_a_count < PHASE8_WORKER_TARGET ||
            phase8_worker_b_count < PHASE8_WORKER_TARGET ||
            scheduler.context_switches < 3u ||
            scheduler.preemptions < 3u ||
            scheduler.task_count != 3u) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 8 preemptive multitasking test: FAILED\n");
            return;
        }

        kprintf(
            "Phase 8 task count: %llu\n",
            (unsigned long long)scheduler.task_count
        );
        kprintf(
            "Phase 8 worker A count: %llu\n",
            (unsigned long long)phase8_worker_a_count
        );
        kprintf(
            "Phase 8 worker B count: %llu\n",
            (unsigned long long)phase8_worker_b_count
        );
        kprintf(
            "Phase 8 context switches: %llu\n",
            (unsigned long long)scheduler.context_switches
        );
        kprintf(
            "Phase 8 timer preemptions: %llu\n",
            (unsigned long long)scheduler.preemptions
        );
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 8 preemptive multitasking test: OK\n");
    kprintf("Phase 8 multitasking complete.\n");

    /*
     * The two Phase-8 workers are test fixtures, not kernel services. Stop
     * them after their acceptance criteria have been measured so later phases
     * are not forced to share the CPU with permanent synthetic busy loops.
     */
    phase8_workers_stop = 1;

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    /*
     * Phase 9: create two Ring-3 tasks while interrupts are disabled. The first
     * writes a message into its own user data page. The second deliberately
     * reads a supervisor-only kernel address and must be killed by #PF.
     */
    interrupts_disable();

    {
        uint64_t hello_id;
        uint64_t fault_id;
        const size_t hello_size =
            (size_t)(phase9_user_hello_end - phase9_user_hello_start);
        const size_t fault_size =
            (size_t)(phase9_user_fault_end - phase9_user_fault_start);

        if (!user_task_create(
                "phase9-user-hello",
                phase9_user_hello_start,
                hello_size,
                &hello_id
            ) ||
            !user_task_create(
                "phase9-user-protection",
                phase9_user_fault_start,
                fault_size,
                &fault_id
            )) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 9 user task creation: FAILED\n");
            return;
        }

        {
            const struct task *hello = scheduler_task_by_id(hello_id);
            const struct task *fault = scheduler_task_by_id(fault_id);
            const paddr_t kernel_space = vmm_kernel_address_space();

            if (hello == 0 || fault == 0 ||
                hello->address_space == PADDR_INVALID ||
                fault->address_space == PADDR_INVALID ||
                hello->address_space == fault->address_space ||
                hello->address_space == kernel_space ||
                fault->address_space == kernel_space) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 9 address-space creation: FAILED\n");
                return;
            }

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_CYAN,
                TERMINAL_COLOR_BLACK
            );
            kprintf("AxiomOS Phase 9 Ring 3 userspace online.\n");

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_GREY,
                TERMINAL_COLOR_BLACK
            );
            kprintf(
                "Phase 9 user hello task ID: %llu\n",
                (unsigned long long)hello_id
            );
            kprintf(
                "Phase 9 protection task ID: %llu\n",
                (unsigned long long)fault_id
            );
            kprintf("Phase 9 user privilege: Ring 3\n");
            kprintf(
                "Phase 9 hello address space: 0x%llX\n",
                (unsigned long long)hello->address_space
            );
            kprintf(
                "Phase 9 protection address space: 0x%llX\n",
                (unsigned long long)fault->address_space
            );
            kprintf("Phase 9 separate address spaces: OK\n");
        }

        interrupts_enable();

        {
            const uint64_t start = timer_ticks();
            char user_message[64];
            int hello_ready = 0;
            int protection_ready = 0;

            user_message[0] = '\0';

            while ((timer_ticks() - start) < PHASE9_TEST_TIMEOUT_TICKS) {
                const struct task *hello = scheduler_task_by_id(hello_id);
                const struct task *fault = scheduler_task_by_id(fault_id);

                hello_ready = phase9_message_ready(
                    hello,
                    user_message,
                    sizeof(user_message)
                );

                protection_ready =
                    fault != 0 &&
                    fault->state == TASK_TERMINATED &&
                    fault->fault_vector == 14u &&
                    fault->fault_address == PHASE9_KERNEL_PROBE_ADDRESS &&
                    (fault->fault_error_code & 0x5ULL) == 0x5ULL;

                if (hello_ready && protection_ready) {
                    break;
                }

                __asm__ volatile ("hlt" ::: "memory");
            }

            if (!hello_ready || !protection_ready) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 9 userspace isolation test: FAILED\n");
                return;
            }

            {
                const struct task *fault = scheduler_task_by_id(fault_id);
                const struct scheduler_stats stats = scheduler_get_stats();

                kprintf("%s\n", user_message);
                kprintf("Phase 9 kernel protection fault: OK\n");
                kprintf(
                    "Phase 9 protection fault vector: %llu\n",
                    (unsigned long long)fault->fault_vector
                );
                kprintf(
                    "Phase 9 protection fault address: 0x%llX\n",
                    (unsigned long long)fault->fault_address
                );
                kprintf(
                    "Phase 9 protection fault error: 0x%llX\n",
                    (unsigned long long)fault->fault_error_code
                );
                kprintf(
                    "Phase 9 user fault terminations: %llu\n",
                    (unsigned long long)stats.user_fault_terminations
                );
            }
        }
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 9 userspace isolation test: OK\n");
    kprintf("Phase 9 userspace complete.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    /*
     * Phase 10: install the x86-64 SYSCALL/SYSRET ABI and launch a Ring-3
     * program that exercises write/read/getpid/sleep/yield/exit. open() is
     * intentionally reserved but returns -ENOSYS until the VFS exists.
     */
    interrupts_disable();

    if (!syscall_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 10 syscall initialization: FAILED\n");
        return;
    }

    {
        uint64_t syscall_task_id;
        const size_t image_size =
            (size_t)(phase10_user_end - phase10_user_start);

        if (!user_task_create(
                "phase10-syscall-demo",
                phase10_user_start,
                image_size,
                &syscall_task_id
            )) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 10 user task creation: FAILED\n");
            return;
        }

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 10 syscall interface online.\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Syscall mechanism: x86-64 SYSCALL/SYSRET\n");
        kprintf(
            "Phase 10 user task ID: %llu\n",
            (unsigned long long)syscall_task_id
        );
        kprintf("Phase 10 demo: type one key when userspace asks for input.\n");

        interrupts_enable();

        {
            const uint64_t start = timer_ticks();
            int results_ready = 0;
            uint64_t pid_result = 0u;
            int64_t bad_write_result = 0;
            int64_t open_result = 0;
            char read_character = '\0';

            while ((timer_ticks() - start) < PHASE10_TEST_TIMEOUT_TICKS) {
                const struct task *task = scheduler_task_by_id(syscall_task_id);

                results_ready = phase10_results_ready(
                    task,
                    &pid_result,
                    &bad_write_result,
                    &open_result,
                    &read_character
                );

                if (results_ready && task != 0 &&
                    task->state == TASK_TERMINATED) {
                    break;
                }

                __asm__ volatile ("hlt" ::: "memory");
            }

            {
                const struct task *task = scheduler_task_by_id(syscall_task_id);
                const struct syscall_stats syscall_stats = syscall_get_stats();

                if (!results_ready || task == 0 ||
                    task->state != TASK_TERMINATED ||
                    task->exit_code != AXIOM_PHASE10_EXIT_STATUS ||
                    pid_result != syscall_task_id ||
                    bad_write_result != -(int64_t)AXIOM_EFAULT ||
                    open_result != -(int64_t)AXIOM_ENOSYS ||
                    syscall_stats.write_calls < 2u ||
                    syscall_stats.read_calls < 1u ||
                    syscall_stats.sleep_calls < 1u ||
                    syscall_stats.getpid_calls < 1u ||
                    syscall_stats.yield_calls < 1u ||
                    syscall_stats.exit_calls < 1u ||
                    syscall_stats.rejected_pointers < 1u ||
                    syscall_stats.unimplemented_calls < 1u) {
                    terminal_set_color(
                        TERMINAL_COLOR_LIGHT_RED,
                        TERMINAL_COLOR_BLACK
                    );
                    kprintf("Phase 10 syscall self-test: FAILED\n");
                    return;
                }

                kprintf(
                    "Phase 10 getpid result: %llu\n",
                    (unsigned long long)pid_result
                );
                kprintf(
                    "Phase 10 invalid pointer result: %lld\n",
                    (long long)bad_write_result
                );
                kprintf(
                    "Phase 10 open result: %lld\n",
                    (long long)open_result
                );
                if (read_character != '\0') {
                    kprintf(
                        "Phase 10 read character: %c\n",
                        read_character
                    );
                } else {
                    kprintf("Phase 10 read character: <none>\n");
                }
                kprintf(
                    "Phase 10 exit status: %lld\n",
                    (long long)task->exit_code
                );
                kprintf(
                    "Phase 10 syscall calls: %llu\n",
                    (unsigned long long)syscall_stats.total_calls
                );
                kprintf(
                    "Phase 10 bytes written/read: %llu/%llu\n",
                    (unsigned long long)syscall_stats.bytes_written,
                    (unsigned long long)syscall_stats.bytes_read
                );
                kprintf(
                    "Phase 10 rejected user pointers: %llu\n",
                    (unsigned long long)syscall_stats.rejected_pointers
                );
                kprintf("Phase 10 user-copy validation: OK\n");
            }
        }
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 10 syscall self-test: OK\n");
    kprintf("Phase 10 system calls complete.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


    /*
     * Phase 12's VFS is initialized here so Phase 11's accepted exec() test
     * now resolves /bin/phase11-demo through normal VFS lookup rather than a
     * syscall-local boot-module registry. The visible Phase-12 acceptance test
     * still runs after Phase 11, preserving the roadmap/output order.
     */
    interrupts_disable();
    if (!filesystem_phase12_bootstrap()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 12 VFS bootstrap: FAILED\n");
        return;
    }

    /*
     * Phase 11: start from a standalone ELF launcher, then prove SYS_exec by
     * replacing that process with /bin/phase11-demo, another standalone ELF.
     * Phase 12 is already bootstrapped, so exec now resolves the target through
     * the VFS while the launcher itself remains a Limine-delivered test input.
     */
    interrupts_disable();

    {
        const void *launcher_image;
        uint64_t launcher_size_u64;
        const char *launcher_path;
        const void *demo_image;
        uint64_t demo_size_u64;
        const char *demo_path;
        uint64_t elf_task_id;

        if (!limine_get_module(
                "phase11-launcher",
                &launcher_image,
                &launcher_size_u64,
                &launcher_path
            ) ||
            !limine_get_module(
                "phase11-demo",
                &demo_image,
                &demo_size_u64,
                &demo_path
            ) ||
            launcher_size_u64 > (uint64_t)SIZE_MAX ||
            demo_size_u64 > (uint64_t)SIZE_MAX ||
            !user_task_create_elf(
                "phase11-exec-launcher",
                launcher_image,
                (size_t)launcher_size_u64,
                &elf_task_id
            )) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 11 ELF task creation: FAILED (%s)\n", elf64_last_error());
            return;
        }

        (void)demo_image;
        (void)demo_size_u64;

        {
            const struct task *task = scheduler_task_by_id(elf_task_id);

            if (task == 0 || !task->elf_backed ||
                task->user_entry != AXIOM_PHASE11_TEXT_BASE ||
                !phase11_permissions_ok(task)) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 11 ELF mapping validation: FAILED\n");
                return;
            }

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_CYAN,
                TERMINAL_COLOR_BLACK
            );
            kprintf("AxiomOS Phase 11 ELF64 loader online.\n");

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_GREY,
                TERMINAL_COLOR_BLACK
            );
            kprintf("ELF launcher source: %s (Limine module)\n", launcher_path);
            kprintf("ELF exec target: /bin/phase11-demo -> %s\n", demo_path);
            kprintf("ELF type: ET_EXEC x86-64\n");
            kprintf(
                "Phase 11 ELF task ID: %llu\n",
                (unsigned long long)elf_task_id
            );
        }

        interrupts_enable();

        {
            const uint64_t start = timer_ticks();
            int results_ready = 0;
            uint64_t pid_result = 0u;
            uint64_t bss_was_zero = 0u;
            uint64_t bss_probe = UINT64_MAX;

            while ((timer_ticks() - start) < PHASE11_TEST_TIMEOUT_TICKS) {
                const struct task *task = scheduler_task_by_id(elf_task_id);

                results_ready = phase11_results_ready(
                    task,
                    &pid_result,
                    &bss_was_zero,
                    &bss_probe
                );

                if (results_ready && task != 0 &&
                    task->state == TASK_TERMINATED) {
                    break;
                }

                __asm__ volatile ("hlt" ::: "memory");
            }

            {
                const struct task *task = scheduler_task_by_id(elf_task_id);
                const struct syscall_stats syscall_stats = syscall_get_stats();

                if (!results_ready || task == 0 ||
                    task->state != TASK_TERMINATED ||
                    task->exit_code != AXIOM_PHASE11_EXIT_STATUS ||
                    pid_result != elf_task_id ||
                    bss_was_zero != 1u || bss_probe != 0u ||
                    !task->elf_backed ||
                    task->user_entry != AXIOM_PHASE11_TEXT_BASE ||
                    task->elf_program_headers != 3u ||
                    task->elf_load_segments != 3u ||
                    task->user_elf_page_count != 3u ||
                    !phase11_permissions_ok(task) ||
                    syscall_stats.exec_calls < 1u ||
                    syscall_stats.exec_successes < 1u) {
                    terminal_set_color(
                        TERMINAL_COLOR_LIGHT_RED,
                        TERMINAL_COLOR_BLACK
                    );
                    kprintf("Phase 11 ELF executable test: FAILED\n");
                    return;
                }

                kprintf(
                    "ELF entry point: 0x%llX\n",
                    (unsigned long long)task->user_entry
                );
                kprintf(
                    "ELF program headers: %u\n",
                    (unsigned)task->elf_program_headers
                );
                kprintf(
                    "ELF PT_LOAD segments: %u\n",
                    (unsigned)task->elf_load_segments
                );
                kprintf(
                    "ELF mapped pages: %llu\n",
                    (unsigned long long)task->user_elf_page_count
                );
                kprintf(
                    "ELF file/memory bytes: %llu/%llu\n",
                    (unsigned long long)task->elf_file_bytes,
                    (unsigned long long)task->elf_memory_bytes
                );
                kprintf("Phase 11 segment permissions: RX/R/RW+NX OK\n");
                kprintf(
                    "Phase 11 exec transitions: %llu/%llu\n",
                    (unsigned long long)syscall_stats.exec_successes,
                    (unsigned long long)syscall_stats.exec_calls
                );
                kprintf(
                    "Phase 11 getpid result: %llu\n",
                    (unsigned long long)pid_result
                );
                kprintf("Phase 11 exec preserved task ID: OK\n");
                kprintf("Phase 11 BSS zero-fill: OK\n");
                kprintf(
                    "Phase 11 exit status: %lld\n",
                    (long long)task->exit_code
                );
            }
        }
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 11 ELF executable test: OK\n");
    kprintf("Phase 11 ELF loader complete.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    /*
     * Phase 12: load a standalone ELF through the VFS itself. The program
     * exercises open/read/write/close/lseek/stat on rootfs and the separately
     * mounted /tmp RAM filesystem, then execs /bin/phase11-demo through VFS.
     */
    interrupts_disable();

    {
        struct axiom_stat bin_stat;
        struct axiom_stat tmp_stat;
        void *phase12_image = 0;
        size_t phase12_image_size = 0u;
        uint64_t phase12_task_id;
        const struct syscall_stats syscall_before = syscall_get_stats();
        const struct vfs_stats vfs_before = vfs_get_stats();

        if (vfs_stat("/bin", &bin_stat) < 0 ||
            vfs_stat("/tmp", &tmp_stat) < 0 ||
            bin_stat.type != AXIOM_DT_DIR ||
            tmp_stat.type != AXIOM_DT_DIR ||
            vfs_read_all(
                "/bin/phase12-demo",
                &phase12_image,
                &phase12_image_size
            ) < 0 ||
            !user_task_create_elf(
                "phase12-vfs-demo",
                phase12_image,
                phase12_image_size,
                &phase12_task_id
            )) {
            if (phase12_image != 0) {
                kfree(phase12_image);
            }
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 12 VFS task creation: FAILED\n");
            return;
        }

        kfree(phase12_image);

        {
            const struct vfs_stats mounted = vfs_get_stats();

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_CYAN,
                TERMINAL_COLOR_BLACK
            );
            kprintf("AxiomOS Phase 12 virtual filesystem online.\n");

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_GREY,
                TERMINAL_COLOR_BLACK
            );
            kprintf("VFS root filesystem: ramfs mounted at /\n");
            kprintf("VFS secondary mount: ramfs mounted at /tmp\n");
            kprintf(
                "VFS mount count: %llu\n",
                (unsigned long long)mounted.mount_count
            );
            kprintf("VFS /bin type: DIRECTORY\n");
            kprintf("VFS /tmp type: DIRECTORY (mount point)\n");
            kprintf(
                "Phase 12 ELF task ID: %llu\n",
                (unsigned long long)phase12_task_id
            );
            kprintf("Phase 12 ELF source: /bin/phase12-demo (VFS)\n");
        }

        interrupts_enable();

        {
            const uint64_t start = timer_ticks();
            int target_ready = 0;
            uint64_t pid_result = 0u;
            uint64_t bss_was_zero = 0u;
            uint64_t bss_probe = UINT64_MAX;

            while ((timer_ticks() - start) < PHASE12_TEST_TIMEOUT_TICKS) {
                const struct task *task = scheduler_task_by_id(phase12_task_id);

                target_ready = phase11_results_ready(
                    task,
                    &pid_result,
                    &bss_was_zero,
                    &bss_probe
                );

                if (target_ready && task != 0 &&
                    task->state == TASK_TERMINATED) {
                    break;
                }

                __asm__ volatile ("hlt" ::: "memory");
            }

            interrupts_disable();

            {
                const struct task *task = scheduler_task_by_id(phase12_task_id);
                const struct syscall_stats syscall_after = syscall_get_stats();
                const struct vfs_stats vfs_after = vfs_get_stats();
                void *roundtrip_data = 0;
                size_t roundtrip_size = 0u;
                static const char expected_roundtrip[] =
                    "RAM filesystem round-trip works!\n";
                int roundtrip_ok = 0;

                if (vfs_read_all(
                        "/tmp/phase12.txt",
                        &roundtrip_data,
                        &roundtrip_size
                    ) == 0 &&
                    roundtrip_size == sizeof(expected_roundtrip) - 1u) {
                    size_t index;

                    roundtrip_ok = 1;
                    for (index = 0u; index < roundtrip_size; ++index) {
                        if (((const char *)roundtrip_data)[index] !=
                            expected_roundtrip[index]) {
                            roundtrip_ok = 0;
                            break;
                        }
                    }
                }

                if (roundtrip_data != 0) {
                    kfree(roundtrip_data);
                }

                if (!target_ready || task == 0 ||
                    task->state != TASK_TERMINATED ||
                    task->exit_code != AXIOM_PHASE11_EXIT_STATUS ||
                    pid_result != phase12_task_id ||
                    bss_was_zero != 1u || bss_probe != 0u ||
                    !roundtrip_ok ||
                    vfs_after.mount_count != 2u ||
                    syscall_after.open_calls < syscall_before.open_calls + 2u ||
                    syscall_after.close_calls < syscall_before.close_calls + 2u ||
                    syscall_after.lseek_calls < syscall_before.lseek_calls + 1u ||
                    syscall_after.stat_calls < syscall_before.stat_calls + 2u ||
                    syscall_after.exec_successes < syscall_before.exec_successes + 1u ||
                    vfs_after.reads <= vfs_before.reads ||
                    vfs_after.writes <= vfs_before.writes) {
                    terminal_set_color(
                        TERMINAL_COLOR_LIGHT_RED,
                        TERMINAL_COLOR_BLACK
                    );
                    kprintf("Phase 12 VFS self-test: FAILED\n");
                    return;
                }

                kprintf("Phase 12 file descriptor base: 3\n");
                kprintf("Phase 12 /tmp file round-trip: OK\n");
                kprintf("Phase 12 seek/stat operations: OK\n");
                kprintf("Phase 12 exec through VFS: OK\n");
                kprintf(
                    "Phase 12 VFS opens/closes: %llu/%llu\n",
                    (unsigned long long)vfs_after.opens,
                    (unsigned long long)vfs_after.closes
                );
                kprintf(
                    "Phase 12 VFS bytes read/written: %llu/%llu\n",
                    (unsigned long long)vfs_after.bytes_read,
                    (unsigned long long)vfs_after.bytes_written
                );
            }
        }
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 12 VFS self-test: OK\n");
    kprintf("Phase 12 virtual filesystem complete.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    /*
     * Phase 13: discover the q35 AHCI controller through PCI, expose its SATA
     * disk as a 512-byte block device, mount a tiny persistent filesystem at
     * /disk, and run a Ring-3 program against that mount. Earlier regression
     * tests intentionally boot without a hard disk, so absence is non-fatal.
     */
    interrupts_disable();

    if (block_init()) {
        struct block_device *disk = block_primary();
        struct diskfs_info disk_info;
        struct vfs_filesystem *persistent_fs;
        int persistent_before = 0;
        int mkdir_result;
        void *phase13_image = 0;
        size_t phase13_image_size = 0u;
        uint64_t phase13_task_id = 0u;

        mkdir_result = vfs_mkdir("/disk");
        if (mkdir_result < 0 && mkdir_result != -AXIOM_EEXIST) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 13 diskfs mount: FAILED\n");
            return;
        }

        persistent_fs = diskfs_create(disk, &disk_info);
        if (persistent_fs == 0 || vfs_mount("/disk", persistent_fs) < 0) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 13 diskfs mount: FAILED\n");
            return;
        }

        if (!phase13_persistent_file_matches(&persistent_before)) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 13 persistent file validation: FAILED\n");
            return;
        }

        if (vfs_read_all(
                "/bin/phase13-demo",
                &phase13_image,
                &phase13_image_size
            ) < 0 ||
            !user_task_create_elf(
                "phase13-disk-demo",
                phase13_image,
                phase13_image_size,
                &phase13_task_id
            )) {
            if (phase13_image != 0) {
                kfree(phase13_image);
            }
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 13 disk task creation: FAILED\n");
            return;
        }
        kfree(phase13_image);

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 13 persistent storage online.\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Storage controller: PCI AHCI (q35 SATA)\n");
        kprintf("Block device: %s (512-byte sectors)\n", disk->name);
        kprintf(
            "Phase 13 disk sectors: %llu\n",
            (unsigned long long)disk->sector_count
        );
        kprintf("VFS disk mount: diskfs mounted at /disk\n");
        kprintf(
            "Phase 13 disk freshly formatted: %s\n",
            disk_info.freshly_formatted ? "yes" : "no"
        );
        kprintf(
            "Phase 13 persistent file before user: %s\n",
            persistent_before ? "present" : "absent"
        );
        kprintf(
            "Phase 13 disk task ID: %llu\n",
            (unsigned long long)phase13_task_id
        );

        interrupts_enable();

        {
            const uint64_t start = timer_ticks();

            while ((timer_ticks() - start) < PHASE13_TEST_TIMEOUT_TICKS) {
                const struct task *task = scheduler_task_by_id(phase13_task_id);
                if (task != 0 && task->state == TASK_TERMINATED) {
                    break;
                }
                __asm__ volatile ("hlt" ::: "memory");
            }
        }

        interrupts_disable();

        {
            const struct task *task = scheduler_task_by_id(phase13_task_id);
            struct block_stats block_stats;
            struct diskfs_info final_disk_info;
            int persistent_after = 0;

            if (task == 0 || task->state != TASK_TERMINATED ||
                task->exit_code != 13 ||
                !phase13_persistent_file_matches(&persistent_after) ||
                !persistent_after) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 13 disk round-trip: FAILED\n");
                return;
            }

            if (!block_flush(disk)) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 13 disk flush: FAILED\n");
                return;
            }

            block_stats = block_get_stats();
            final_disk_info = diskfs_get_info();

            kprintf("Phase 13 disk round-trip: OK\n");
            kprintf(
                "Phase 13 diskfs files: %u\n",
                (unsigned)final_disk_info.file_count
            );
            kprintf(
                "Phase 13 block sectors read: %llu\n",
                (unsigned long long)block_stats.sectors_read
            );
            kprintf(
                "Phase 13 block sectors written: %llu\n",
                (unsigned long long)block_stats.sectors_written
            );
            kprintf(
                "Phase 13 block flushes: %llu\n",
                (unsigned long long)block_stats.flush_commands
            );
        }

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREEN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 13 persistent storage complete.\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
    } else {
        kprintf("Phase 13 block device: not present; persistent demo skipped.\n");
    }

    interrupts_enable();

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("AxiomOS Phase 15 process management online.\n");
    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Process table capacity: %u tasks\n", (unsigned)SCHEDULER_MAX_TASKS);
    kprintf("fork policy: eager private-page copy\n");
    kprintf("wait policy: blocking parent + zombie reap\n");
    kprintf("signal policy: SIGTERM immediate termination\n");

    /*
     * Phase 16: synchronization primitives. First demonstrate a lost-update
     * race on purpose, then prove spinlocks, sleeping mutexes, semaphores, and
     * FIFO wait queues repair/control the same kind of concurrency.
     */
    {
        struct sync_selftest_result sync_result;

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 16 synchronization online.\n");
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Synchronization CPU model: BSP only; APs remain parked until Phase 17\n");
        kprintf("Spinlock policy: IRQ-save busy lock for short critical sections\n");
        kprintf("Mutex policy: blocking FIFO handoff\n");
        kprintf("Semaphore policy: counting tokens + blocking FIFO waiters\n");

        if (!sync_run_selftest(&sync_result)) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf(
                "Phase 16 synchronization failure stage: %llu\n",
                (unsigned long long)sync_result.failure_stage
            );
            if (sync_result.failure_stage == SYNC_SELFTEST_STAGE_MUTEX) {
                kprintf(
                    "Phase 16 failure mutex expected/actual: %llu/%llu\n",
                    (unsigned long long)sync_result.mutex_expected,
                    (unsigned long long)sync_result.mutex_actual
                );
                kprintf(
                    "Phase 16 failure mutex blocks/wakeups: %llu/%llu\n",
                    (unsigned long long)sync_result.mutex_blocks,
                    (unsigned long long)sync_result.mutex_wakeups
                );
            }
            kprintf("Phase 16 synchronization self-test: FAILED\n");
            return;
        }

        kprintf(
            "Phase 16 race expected/observed: %llu/%llu\n",
            (unsigned long long)sync_result.race_expected,
            (unsigned long long)sync_result.race_actual
        );
        kprintf("Phase 16 race condition demonstrated: OK\n");
        kprintf(
            "Phase 16 spinlock counter expected/actual: %llu/%llu\n",
            (unsigned long long)sync_result.spin_expected,
            (unsigned long long)sync_result.spin_actual
        );
        kprintf(
            "Phase 16 spinlock acquisitions: %llu\n",
            (unsigned long long)sync_result.spin_acquisitions
        );
        kprintf("Phase 16 spinlock protected counter: OK\n");
        kprintf(
            "Phase 16 mutex counter expected/actual: %llu/%llu\n",
            (unsigned long long)sync_result.mutex_expected,
            (unsigned long long)sync_result.mutex_actual
        );
        kprintf(
            "Phase 16 mutex blocks/wakeups: %llu/%llu\n",
            (unsigned long long)sync_result.mutex_blocks,
            (unsigned long long)sync_result.mutex_wakeups
        );
        kprintf("Phase 16 mutex protected counter: OK\n");
        kprintf(
            "Phase 16 semaphore limit/peak: %llu/%llu\n",
            (unsigned long long)sync_result.semaphore_limit,
            (unsigned long long)sync_result.semaphore_peak
        );
        kprintf(
            "Phase 16 semaphore blocks/wakeups: %llu/%llu\n",
            (unsigned long long)sync_result.semaphore_blocks,
            (unsigned long long)sync_result.semaphore_wakeups
        );
        kprintf("Phase 16 semaphore limit: OK\n");
        kprintf("Phase 16 wait queue wakeups: OK\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREEN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 16 synchronization self-test: OK\n");
        kprintf("Phase 16 synchronization complete.\n");
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
    }

    /*
     * Phase 17: release Limine-parked application processors.  APs switch to
     * the AxiomOS kernel CR3, private kernel stacks, and private TSS instances.
     * The general scheduler deliberately remains BSP-only in this foundation
     * phase; APs run a parallel spinlock proof and then park with IF=0.
     */
    {
        const struct smp_stats *smp;
        uint64_t cpu_index;

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 17 SMP / multicore online.\n");
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );

        if (!smp_init()) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 17 SMP initialization/self-test: FAILED\n");
            return;
        }

        smp = smp_get_stats();
        kprintf(
            "Phase 17 CPUs detected/managed/online: %llu/%llu/%llu\n",
            (unsigned long long)smp->detected_cpus,
            (unsigned long long)smp->managed_cpus,
            (unsigned long long)smp->online_cpus
        );
        kprintf(
            "Phase 17 APs released/completed: %llu/%llu\n",
            (unsigned long long)smp->aps_released,
            (unsigned long long)smp->aps_completed
        );

        for (cpu_index = 0u; cpu_index < smp->managed_cpus; ++cpu_index) {
            const struct smp_cpu *cpu = smp_cpu_at(cpu_index);

            if (cpu == 0) {
                continue;
            }

            kprintf(
                "Phase 17 CPU %llu: processor=%u LAPIC=%u role=%s online=%u\n",
                (unsigned long long)cpu->logical_index,
                (unsigned)cpu->processor_id,
                (unsigned)cpu->lapic_id,
                cpu->role == SMP_CPU_BSP ? "BSP" : "AP",
                (unsigned)cpu->online
            );
        }

        if (smp->multicore_test_ran) {
            kprintf(
                "Phase 17 parallel participant mask: 0x%llX\n",
                (unsigned long long)smp->participant_mask
            );
            kprintf(
                "Phase 17 locked counter expected/actual: %llu/%llu\n",
                (unsigned long long)smp->expected_locked_count,
                (unsigned long long)smp->actual_locked_count
            );
            kprintf(
                "Phase 17 cross-CPU spinlock acquisitions/contentions: %llu/%llu\n",
                (unsigned long long)smp->spinlock_acquisitions,
                (unsigned long long)smp->spinlock_contentions
            );

            if (!smp->multicore_test_passed) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 17 multicore parallel-work test: FAILED\n");
                return;
            }

            kprintf("Phase 17 AP bring-up: OK\n");
            kprintf("Phase 17 cross-CPU spinlock: OK\n");
            kprintf("Phase 17 multicore parallel-work test: OK\n");
        } else {
            kprintf(
                "Phase 17 multicore self-test: SKIPPED (single CPU boot)\n"
            );
        }

        kprintf("Phase 17 scheduler policy: BSP-only; APs parked after self-test\n");
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREEN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 17 SMP initialization complete.\n");
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
    }

    /*
     * Phase 18: initialise the first real network interface and protocol stack.
     * Networking is optional for older regression boots: if no supported E1000
     * NIC is attached, the shell still launches and network syscalls return
     * ENETDOWN. Normal `make run` and test-phase18 attach an E1000 explicitly.
     */
    {
        const struct net_config *network_config;

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 18 networking online.\n");
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );

        if (!net_init()) {
            kprintf("Phase 18 E1000 NIC: not present; networking unavailable.\n");
            kprintf("Phase 18 networking initialization: SKIPPED\n");
        } else {
            network_config = net_get_config();
            kprintf("Network device: Intel E1000 82540EM (PCI/MMIO, polled DMA)\n");
            kprintf(
                "Phase 18 MAC: %X:%X:%X:%X:%X:%X\n",
                (unsigned)network_config->mac[0],
                (unsigned)network_config->mac[1],
                (unsigned)network_config->mac[2],
                (unsigned)network_config->mac[3],
                (unsigned)network_config->mac[4],
                (unsigned)network_config->mac[5]
            );
            kprintf(
                "Phase 18 IPv4: %u.%u.%u.%u/24 gateway %u.%u.%u.%u DNS %u.%u.%u.%u\n",
                (unsigned)((network_config->address >> 24) & 0xFFu),
                (unsigned)((network_config->address >> 16) & 0xFFu),
                (unsigned)((network_config->address >> 8) & 0xFFu),
                (unsigned)(network_config->address & 0xFFu),
                (unsigned)((network_config->gateway >> 24) & 0xFFu),
                (unsigned)((network_config->gateway >> 16) & 0xFFu),
                (unsigned)((network_config->gateway >> 8) & 0xFFu),
                (unsigned)(network_config->gateway & 0xFFu),
                (unsigned)((network_config->dns_server >> 24) & 0xFFu),
                (unsigned)((network_config->dns_server >> 16) & 0xFFu),
                (unsigned)((network_config->dns_server >> 8) & 0xFFu),
                (unsigned)(network_config->dns_server & 0xFFu)
            );
            kprintf("Phase 18 stack: Ethernet -> ARP -> IPv4 -> ICMP/UDP/TCP -> DNS/HTTP\n");
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_GREEN,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 18 networking initialization complete.\n");
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_GREY,
                TERMINAL_COLOR_BLACK
            );
        }
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("AxiomOS Phase 19 userspace libc available.\n");
    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );
    kprintf("libaxiom: stdio + stdlib + string + ctype + unistd-like wrappers\n");
    kprintf("Phase 19 heap backend: anonymous user mmap pages\n");
    kprintf("Phase 19 libc linkage: axiomsh + Phase 15 C programs + /bin/phase19-demo\n");
    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 19 userspace library initialization complete.\n");
    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    terminal_set_color(TERMINAL_COLOR_LIGHT_CYAN, TERMINAL_COLOR_BLACK);
    kprintf("AxiomOS Phase 20 security hardening online.\n");
    terminal_set_color(TERMINAL_COLOR_LIGHT_GREY, TERMINAL_COLOR_BLACK);
    kprintf("Security policy: Ring-3 isolation + W^X + NX + guarded stacks\n");
    kprintf("File policy: read/write/execute mode enforcement\n");
    kprintf("Credentials: uid=%u gid=%u for normal userspace tasks\n",
            (unsigned)TASK_DEFAULT_UID, (unsigned)TASK_DEFAULT_GID);
    kprintf("Kernel stack policy: per-task overflow canary checked on scheduling\n");
    terminal_set_color(TERMINAL_COLOR_LIGHT_GREEN, TERMINAL_COLOR_BLACK);
    kprintf("Phase 20 security initialization complete.\n");
    terminal_set_color(TERMINAL_COLOR_LIGHT_GREY, TERMINAL_COLOR_BLACK);

    /*
     * Phase 21: preserve the Phase-8 round-robin regression during early boot,
     * then switch the live scheduler to priority scheduling with starvation
     * prevention through aging. Normal tasks still execute on the BSP; per-task
     * affinity masks are recorded now so multicore scheduling can consume the
     * same ABI in a later phase.
     */
    {
        struct scheduler_benchmark_result benchmark;
        const int irq_was_enabled = interrupts_enabled();

        terminal_set_color(TERMINAL_COLOR_LIGHT_CYAN, TERMINAL_COLOR_BLACK);
        kprintf("AxiomOS Phase 21 advanced scheduler online.\n");
        terminal_set_color(TERMINAL_COLOR_LIGHT_GREY, TERMINAL_COLOR_BLACK);

        interrupts_disable();
        if (!scheduler_set_policy(AXIOM_SCHED_POLICY_PRIORITY_AGING) ||
            !scheduler_set_current_priority(AXIOM_SCHED_PRIORITY_DEFAULT) ||
            !scheduler_set_current_affinity(AXIOM_CPU_AFFINITY_BSP)) {
            terminal_set_color(TERMINAL_COLOR_LIGHT_RED, TERMINAL_COLOR_BLACK);
            kprintf("Phase 21 scheduler activation: FAILED\n");
            return;
        }
        scheduler_benchmark_compare(&benchmark);
        if (irq_was_enabled) {
            interrupts_enable();
        }

        kprintf("Phase 21 policy: priority + aging\n");
        kprintf(
            "Phase 21 priority range/default: %u..%u/%u\n",
            (unsigned)AXIOM_SCHED_PRIORITY_MIN,
            (unsigned)AXIOM_SCHED_PRIORITY_MAX,
            (unsigned)AXIOM_SCHED_PRIORITY_DEFAULT
        );
        kprintf(
            "Phase 21 aging interval: %u timer ticks\n",
            (unsigned)SCHEDULER_AGING_INTERVAL_TICKS
        );
        kprintf("Phase 21 affinity policy: BSP execution + per-task masks\n");
        kprintf(
            "Phase 21 round-robin benchmark: %llu/%llu/%llu\n",
            (unsigned long long)benchmark.round_robin[0],
            (unsigned long long)benchmark.round_robin[1],
            (unsigned long long)benchmark.round_robin[2]
        );
        kprintf(
            "Phase 21 priority-aging benchmark: %llu/%llu/%llu\n",
            (unsigned long long)benchmark.priority_aging[0],
            (unsigned long long)benchmark.priority_aging[1],
            (unsigned long long)benchmark.priority_aging[2]
        );

        if (benchmark.round_robin[0] != benchmark.round_robin[1] ||
            benchmark.round_robin[1] != benchmark.round_robin[2] ||
            benchmark.priority_aging[0] == 0u ||
            benchmark.priority_aging[1] == 0u ||
            benchmark.priority_aging[2] == 0u ||
            benchmark.priority_aging[2] <= benchmark.priority_aging[0]) {
            terminal_set_color(TERMINAL_COLOR_LIGHT_RED, TERMINAL_COLOR_BLACK);
            kprintf("Phase 21 scheduler benchmark: FAILED\n");
            return;
        }

        terminal_set_color(TERMINAL_COLOR_LIGHT_GREEN, TERMINAL_COLOR_BLACK);
        kprintf("Phase 21 priority scheduling: OK\n");
        kprintf("Phase 21 starvation-prevention aging: OK\n");
        kprintf("Phase 21 CPU affinity groundwork: OK\n");
        kprintf("Phase 21 scheduler benchmark comparison: OK\n");
        kprintf("Phase 21 advanced scheduler initialization complete.\n");
        terminal_set_color(TERMINAL_COLOR_LIGHT_GREY, TERMINAL_COLOR_BLACK);
    }

    /* Phase 22: kernel-owned IPC objects exposed to Ring-3 through syscalls. */
    {
        uint32_t pipe_id = 0u;
        uint32_t queue_id = 0u;
        char pipe_buffer[8];
        char queue_buffer[8];
        static const char probe[] = "ipc22";
        int64_t transferred;

        terminal_set_color(TERMINAL_COLOR_LIGHT_CYAN, TERMINAL_COLOR_BLACK);
        kprintf("AxiomOS Phase 22 inter-process communication online.\n");
        terminal_set_color(TERMINAL_COLOR_LIGHT_GREY, TERMINAL_COLOR_BLACK);

        if (!ipc_init()) {
            terminal_set_color(TERMINAL_COLOR_LIGHT_RED, TERMINAL_COLOR_BLACK);
            kprintf("Phase 22 IPC initialization: FAILED\n");
            return;
        }

        if (ipc_pipe_create(&pipe_id) < 0 ||
            ipc_pipe_write(pipe_id, probe, sizeof(probe)) != (int64_t)sizeof(probe) ||
            ipc_pipe_read(pipe_id, pipe_buffer, sizeof(probe)) != (int64_t)sizeof(probe) ||
            !phase13_bytes_equal(pipe_buffer, probe, sizeof(probe)) ||
            ipc_pipe_close(pipe_id) < 0) {
            terminal_set_color(TERMINAL_COLOR_LIGHT_RED, TERMINAL_COLOR_BLACK);
            kprintf("Phase 22 pipe kernel self-test: FAILED\n");
            return;
        }

        transferred = ipc_msgq_open(0xA221ULL, &queue_id);
        if (transferred < 0 ||
            ipc_msgq_send(queue_id, probe, sizeof(probe)) != (int64_t)sizeof(probe) ||
            ipc_msgq_receive(queue_id, queue_buffer, sizeof(queue_buffer)) != (int64_t)sizeof(probe) ||
            !phase13_bytes_equal(queue_buffer, probe, sizeof(probe)) ||
            ipc_msgq_close(queue_id) < 0) {
            terminal_set_color(TERMINAL_COLOR_LIGHT_RED, TERMINAL_COLOR_BLACK);
            kprintf("Phase 22 message-queue kernel self-test: FAILED\n");
            return;
        }

        kprintf("Phase 22 IPC mechanisms: pipes + shared memory + message queues\n");
        kprintf("Phase 22 shared-memory policy: keyed 4 KiB user/NX mappings\n");
        terminal_set_color(TERMINAL_COLOR_LIGHT_GREEN, TERMINAL_COLOR_BLACK);
        kprintf("Phase 22 pipe kernel self-test: OK\n");
        kprintf("Phase 22 message-queue kernel self-test: OK\n");
        kprintf("Phase 22 shared-memory mapper: READY\n");
        kprintf("Phase 22 IPC initialization complete.\n");
        terminal_set_color(TERMINAL_COLOR_LIGHT_GREY, TERMINAL_COLOR_BLACK);
    }

    /* Phase 23: reusable framebuffer graphics primitives + userspace ABI. */
    {
        struct axiom_gfx_info gfx_info;

        terminal_set_color(TERMINAL_COLOR_LIGHT_CYAN, TERMINAL_COLOR_BLACK);
        kprintf("AxiomOS Phase 23 graphical framebuffer online.\n");
        terminal_set_color(TERMINAL_COLOR_LIGHT_GREY, TERMINAL_COLOR_BLACK);

        if (!graphics_init()) {
            terminal_set_color(TERMINAL_COLOR_LIGHT_RED, TERMINAL_COLOR_BLACK);
            kprintf("Phase 23 graphics initialization: FAILED\n");
            return;
        }
        graphics_get_info(&gfx_info);
        kprintf(
            "Phase 23 framebuffer mode: %ux%u pitch=%u bpp=%u\n",
            (unsigned)gfx_info.width,
            (unsigned)gfx_info.height,
            (unsigned)gfx_info.pitch,
            (unsigned)gfx_info.bpp
        );
        kprintf(
            "Phase 23 bitmap font: %ux%u\n",
            (unsigned)gfx_info.font_width,
            (unsigned)gfx_info.font_height
        );
        kprintf("Phase 23 primitives: pixel + line + rectangle + bitmap + text\n");
        kprintf("Phase 23 userspace graphics ABI: syscall-mediated framebuffer access\n");

        if (!graphics_selftest()) {
            terminal_set_color(TERMINAL_COLOR_LIGHT_RED, TERMINAL_COLOR_BLACK);
            kprintf("Phase 23 framebuffer read/write self-test: FAILED\n");
            return;
        }

        terminal_set_color(TERMINAL_COLOR_LIGHT_GREEN, TERMINAL_COLOR_BLACK);
        kprintf("Phase 23 framebuffer read/write self-test: OK\n");
        kprintf("Phase 23 clipping protection: OK\n");
        kprintf("Phase 23 graphics library initialization complete.\n");
        terminal_set_color(TERMINAL_COLOR_LIGHT_GREY, TERMINAL_COLOR_BLACK);
    }

    /* Phase 24: standalone Ring-3 networking applications and passive TCP. */
    {
        terminal_set_color(TERMINAL_COLOR_LIGHT_CYAN, TERMINAL_COLOR_BLACK);
        kprintf("AxiomOS Phase 24 networking applications online.\n");
        terminal_set_color(TERMINAL_COLOR_LIGHT_GREY, TERMINAL_COLOR_BLACK);
        kprintf("Phase 24 userspace tools: ifconfig + ping + dnslookup + httpget + httpd\n");
        kprintf("Phase 24 TCP passive-open HTTP server: READY\n");
        kprintf("Phase 24 application ABI: existing network calls + one-shot httpserve\n");
        terminal_set_color(TERMINAL_COLOR_LIGHT_GREEN, TERMINAL_COLOR_BLACK);
        kprintf("Phase 24 networking applications initialization complete.\n");
        terminal_set_color(TERMINAL_COLOR_LIGHT_GREY, TERMINAL_COLOR_BLACK);
    }

    /* Phase 25: live developer observability through a read-only procfs. */
    {
        struct vfs_filesystem *procfs;
        const int irq_was_enabled = interrupts_enabled();

        terminal_set_color(TERMINAL_COLOR_LIGHT_CYAN, TERMINAL_COLOR_BLACK);
        kprintf("AxiomOS Phase 25 developer tooling online.\n");
        terminal_set_color(TERMINAL_COLOR_LIGHT_GREY, TERMINAL_COLOR_BLACK);

        interrupts_disable();
        procfs = procfs_create();
        if (procfs == 0 || vfs_mount("/proc", procfs) < 0 || !procfs_selftest()) {
            terminal_set_color(TERMINAL_COLOR_LIGHT_RED, TERMINAL_COLOR_BLACK);
            kprintf("Phase 25 /proc observability initialization: FAILED\n");
            return;
        }
        if (irq_was_enabled) interrupts_enable();

        kprintf("Phase 25 /proc files: meminfo processes interrupts files net scheduler cpuinfo pagemap\n");
        kprintf("Phase 25 userspace tool: /bin/sysinfo\n");
        kprintf("Phase 25 diagnostics: memory + processes + mappings + IRQs + FDs + network + scheduler + CPUs\n");
        terminal_set_color(TERMINAL_COLOR_LIGHT_GREEN, TERMINAL_COLOR_BLACK);
        kprintf("Phase 25 procfs read-only self-test: OK\n");
        kprintf("Phase 25 developer tooling initialization complete.\n");
        kprintf("AxiomOS original Phase 0-25 roadmap complete.\n");
        terminal_set_color(TERMINAL_COLOR_LIGHT_GREY, TERMINAL_COLOR_BLACK);
    }

    /* Phase 26: optional desktop input groundwork. The desktop itself is a
     * normal Ring-3 application launched from the shell, keeping the accepted
     * Phase 1-25 boot/test path unchanged. */
    {
        const int irq_was_enabled = interrupts_enabled();
        int mouse_ok;

        interrupts_disable();
        mouse_ok = mouse_init();
        if (irq_was_enabled) interrupts_enable();

        terminal_set_color(TERMINAL_COLOR_LIGHT_CYAN, TERMINAL_COLOR_BLACK);
        kprintf("AxiomOS Phase 26 desktop GUI available.\n");
        terminal_set_color(TERMINAL_COLOR_LIGHT_GREY, TERMINAL_COLOR_BLACK);
        kprintf("Desktop executable: /bin/desktop (launch with: desktop)\n");
        kprintf("Desktop terminal workflow: open Terminal, type exit to return\n");
        kprintf("Phase 26 PS/2 mouse: %s\n", mouse_ok ? "online" : "unavailable - keyboard fallback");
        terminal_set_color(TERMINAL_COLOR_LIGHT_GREEN, TERMINAL_COLOR_BLACK);
        kprintf("Phase 26 desktop integration complete.\n");
        terminal_set_color(TERMINAL_COLOR_LIGHT_GREY, TERMINAL_COLOR_BLACK);
    }

    /*
     * Phase 14: launch the interactive command shell as a normal Ring-3 ELF.
     * The shell consumes keyboard input through SYS_read, uses the VFS through
     * file syscalls, and launches other ELF programs through spawn/waitpid.
     */
    {
        void *shell_image = 0;
        size_t shell_image_size = 0u;
        uint64_t shell_task_id = 0u;

        interrupts_disable();

        if (vfs_read_all(
                "/bin/axiomsh",
                &shell_image,
                &shell_image_size
            ) < 0 ||
            !user_task_create_elf(
                "axiomsh",
                shell_image,
                shell_image_size,
                &shell_task_id
            )) {
            if (shell_image != 0) {
                kfree(shell_image);
            }
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 14 shell task creation: FAILED\n");
            return;
        }

        kfree(shell_image);

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 14 interactive shell online.\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Shell executable: /bin/axiomsh\n");
        kprintf("Shell privilege: Ring 3\n");
        kprintf(
            "Phase 14 shell task ID: %llu\n",
            (unsigned long long)shell_task_id
        );
        kprintf("Phase 14 shell launch complete.\n");

        interrupts_enable();
    }

    /* Bootstrap task becomes an idle host while the Ring-3 shell runs. */
    for (;;) {
        __asm__ volatile ("hlt" ::: "memory");
    }
}
