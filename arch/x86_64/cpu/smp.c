#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/arch/smp.h>
#include <axiom/boot/limine.h>
#include <axiom/memory/vmm.h>
#include <axiom/sync/spinlock.h>

_Static_assert(offsetof(struct smp_cpu, kernel_cr3) == 16u, "smp_cpu CR3 offset");
_Static_assert(offsetof(struct smp_cpu, stack_top) == 24u, "smp_cpu stack offset");

#define SMP_READY_SPIN_LIMIT 200000000ULL
#define SMP_DONE_SPIN_LIMIT  400000000ULL

static struct smp_cpu cpus[SMP_MAX_CPUS];
static uint8_t ap_stacks[SMP_MAX_CPUS][SMP_AP_STACK_SIZE]
    __attribute__((aligned(16)));
static struct smp_stats stats;

static struct spinlock work_lock;
static volatile uint64_t locked_counter;
static volatile uint64_t aps_ready;
static volatile uint64_t aps_done;
static volatile uint64_t release_barrier;
static volatile uint64_t participant_mask;
static volatile int initialized;

static uint64_t atomic_load_u64(volatile uint64_t *value)
{
    return __atomic_load_n(value, __ATOMIC_ACQUIRE);
}

static void atomic_store_u64(volatile uint64_t *value, uint64_t new_value)
{
    __atomic_store_n(value, new_value, __ATOMIC_RELEASE);
}

static uint64_t atomic_add_u64(volatile uint64_t *value, uint64_t amount)
{
    return __atomic_add_fetch(value, amount, __ATOMIC_ACQ_REL);
}

static void atomic_or_u64(volatile uint64_t *value, uint64_t mask)
{
    (void)__atomic_fetch_or(value, mask, __ATOMIC_ACQ_REL);
}

static int wait_until_at_least(
    volatile uint64_t *value,
    uint64_t target,
    uint64_t spin_limit
)
{
    uint64_t spins = 0u;

    while (atomic_load_u64(value) < target) {
        if (spins++ >= spin_limit) {
            return 0;
        }
        __asm__ volatile ("pause" ::: "memory");
    }

    return 1;
}

static void perform_locked_work(struct smp_cpu *cpu)
{
    uint64_t iteration;

    if (cpu == 0) {
        return;
    }

    atomic_or_u64(&participant_mask, 1ULL << cpu->logical_index);

    for (iteration = 0u; iteration < SMP_TEST_ITERATIONS; ++iteration) {
        const uint64_t flags = spin_lock_irqsave(&work_lock);
        ++locked_counter;
        spin_unlock_irqrestore(&work_lock, flags);
    }

    cpu->work_iterations = SMP_TEST_ITERATIONS;
    cpu->work_complete = 1u;
}

_Noreturn void smp_ap_entry_c(struct smp_cpu *cpu)
{
    if (cpu == 0 || cpu->logical_index == 0u ||
        cpu->logical_index >= SMP_MAX_CPUS) {
        for (;;) {
            __asm__ volatile ("cli; hlt" ::: "memory");
        }
    }

    interrupts_disable();

    /*
     * Each AP receives its own GDT/TSS and loads the already-built shared IDT.
     * Interrupts remain disabled: the BSP alone owns the scheduler and timer
     * in this foundational SMP phase.
     */
    if (!gdt_init_secondary((uint32_t)cpu->logical_index, cpu->stack_top)) {
        for (;;) {
            __asm__ volatile ("cli; hlt" ::: "memory");
        }
    }
    interrupts_load_idt();

    cpu->online = 1u;
    atomic_add_u64(&aps_ready, 1u);

    while (atomic_load_u64(&release_barrier) == 0u) {
        __asm__ volatile ("pause" ::: "memory");
    }

    perform_locked_work(cpu);
    atomic_add_u64(&aps_done, 1u);

    /* AP scheduling/timers are intentionally deferred; park this CPU. */
    for (;;) {
        __asm__ volatile ("cli; hlt" ::: "memory");
    }
}

int smp_init(void)
{
    struct limine_mp_response *response;
    uint64_t managed = 0u;
    uint64_t ap_count = 0u;
    uint64_t index;
    uint64_t logical = 1u;
    struct smp_cpu *bsp = 0;

    if (initialized) {
        return stats.multicore_test_ran ? stats.multicore_test_passed : 1;
    }

    response = limine_get_mp_response();
    if (response == 0 || response->cpu_count == 0u || response->cpus == 0) {
        return 0;
    }

    stats.detected_cpus = response->cpu_count;
    stats.managed_cpus = response->cpu_count < SMP_MAX_CPUS
        ? response->cpu_count : SMP_MAX_CPUS;
    managed = stats.managed_cpus;

    for (index = 0u; index < response->cpu_count; ++index) {
        struct limine_mp_info *info = response->cpus[index];

        if (info != 0 && info->lapic_id == response->bsp_lapic_id) {
            bsp = &cpus[0];
            bsp->processor_id = info->processor_id;
            bsp->lapic_id = info->lapic_id;
            bsp->logical_index = 0u;
            bsp->kernel_cr3 = vmm_kernel_address_space();
            bsp->stack_top = gdt_kernel_stack();
            bsp->online = 1u;
            bsp->work_complete = 0u;
            bsp->work_iterations = 0u;
            bsp->role = SMP_CPU_BSP;
            break;
        }
    }

    if (bsp == 0) {
        return 0;
    }

    spinlock_init(&work_lock);
    locked_counter = 0u;
    aps_ready = 0u;
    aps_done = 0u;
    release_barrier = 0u;
    participant_mask = 0u;

    for (index = 0u; index < response->cpu_count && logical < managed; ++index) {
        struct limine_mp_info *info = response->cpus[index];
        struct smp_cpu *cpu;

        if (info == 0 || info->lapic_id == response->bsp_lapic_id) {
            continue;
        }

        cpu = &cpus[logical];
        cpu->processor_id = info->processor_id;
        cpu->lapic_id = info->lapic_id;
        cpu->logical_index = logical;
        cpu->kernel_cr3 = vmm_kernel_address_space();
        cpu->stack_top = (uintptr_t)&ap_stacks[logical][SMP_AP_STACK_SIZE];
        cpu->online = 0u;
        cpu->work_complete = 0u;
        cpu->work_iterations = 0u;
        cpu->role = SMP_CPU_AP;

        info->extra_argument = (uint64_t)(uintptr_t)cpu;
        __atomic_store_n(&info->goto_address, smp_ap_entry_asm, __ATOMIC_RELEASE);

        ++logical;
        ++ap_count;
    }

    stats.aps_released = ap_count;

    if (ap_count == 0u) {
        stats.online_cpus = 1u;
        stats.participant_mask = 1u;
        stats.multicore_test_ran = 0;
        stats.multicore_test_passed = 0;
        initialized = 1;
        return 1;
    }

    if (!wait_until_at_least(&aps_ready, ap_count, SMP_READY_SPIN_LIMIT)) {
        return 0;
    }

    stats.online_cpus = 1u + atomic_load_u64(&aps_ready);
    atomic_store_u64(&release_barrier, 1u);

    /* BSP participates at the same time as the released APs. */
    perform_locked_work(bsp);

    if (!wait_until_at_least(&aps_done, ap_count, SMP_DONE_SPIN_LIMIT)) {
        return 0;
    }

    stats.aps_completed = atomic_load_u64(&aps_done);
    stats.participant_mask = atomic_load_u64(&participant_mask);
    stats.expected_locked_count = stats.online_cpus * SMP_TEST_ITERATIONS;
    stats.actual_locked_count = locked_counter;
    stats.spinlock_acquisitions = work_lock.acquisitions;
    stats.spinlock_contentions = work_lock.contentions;
    stats.multicore_test_ran = 1;
    stats.multicore_test_passed =
        stats.online_cpus >= 2u &&
        stats.aps_completed == ap_count &&
        stats.actual_locked_count == stats.expected_locked_count &&
        stats.participant_mask == ((1ULL << stats.online_cpus) - 1ULL);

    initialized = 1;
    return stats.multicore_test_passed;
}

const struct smp_stats *smp_get_stats(void)
{
    return &stats;
}

const struct smp_cpu *smp_cpu_at(uint64_t index)
{
    if (index >= stats.managed_cpus) {
        return 0;
    }
    return &cpus[index];
}
