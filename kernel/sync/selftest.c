#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/interrupts.h>
#include <axiom/drivers/timer.h>
#include <axiom/process/scheduler.h>
#include <axiom/sync/mutex.h>
#include <axiom/sync/selftest.h>
#include <axiom/sync/semaphore.h>
#include <axiom/sync/spinlock.h>

#define SYNC_TEST_TIMEOUT_TICKS 3000ULL
#define RACE_ITERATIONS 64ULL
#define SPIN_ITERATIONS 1000ULL
#define MUTEX_ITERATIONS 64ULL
#define SEMAPHORE_ITERATIONS 8ULL
#define SEMAPHORE_WORKERS 3u
#define SEMAPHORE_LIMIT 2ULL

static volatile uint64_t race_counter;
static volatile uint64_t race_loaded[2];
static volatile uint64_t race_stored[2];

static volatile uint64_t spin_counter;
static struct spinlock counter_spinlock;

static volatile uint64_t mutex_counter;
static struct mutex counter_mutex;

static volatile uint64_t semaphore_inside;
static volatile uint64_t semaphore_peak;
static struct semaphore test_semaphore;
static struct spinlock semaphore_monitor_lock;

struct worker_argument {
    uint64_t index;
};

static struct worker_argument two_worker_arguments[2];
static struct worker_argument semaphore_worker_arguments[SEMAPHORE_WORKERS];

static int cooperative_yield(void)
{
    const int restore_interrupts = interrupts_enabled();
    int result;

    interrupts_disable();
    result = scheduler_yield_current();
    if (restore_interrupts) {
        interrupts_enable();
    }
    return result;
}

static void race_worker(void *argument)
{
    const struct worker_argument *worker =
        (const struct worker_argument *)argument;
    const uint64_t index = worker != 0 ? worker->index : 0u;
    const uint64_t other = index == 0u ? 1u : 0u;
    uint64_t iteration;

    for (iteration = 0u; iteration < RACE_ITERATIONS; ++iteration) {
        const uint64_t snapshot = race_counter;

        race_loaded[index] = iteration + 1u;
        while (race_loaded[other] < iteration + 1u) {
            if (!cooperative_yield()) {
                task_exit_current(1);
            }
        }

        /* Both workers intentionally store from the same stale snapshot. */
        race_counter = snapshot + 1u;
        race_stored[index] = iteration + 1u;

        while (race_stored[other] < iteration + 1u) {
            if (!cooperative_yield()) {
                task_exit_current(1);
            }
        }
    }

    task_exit_current(0);
}

static void spin_worker(void *argument)
{
    uint64_t iteration;
    (void)argument;

    for (iteration = 0u; iteration < SPIN_ITERATIONS; ++iteration) {
        const uint64_t flags = spin_lock_irqsave(&counter_spinlock);
        ++spin_counter;
        spin_unlock_irqrestore(&counter_spinlock, flags);

        if ((iteration & 31u) == 0u && !cooperative_yield()) {
            task_exit_current(1);
        }
    }

    task_exit_current(0);
}

static void mutex_worker(void *argument)
{
    uint64_t iteration;
    (void)argument;

    for (iteration = 0u; iteration < MUTEX_ITERATIONS; ++iteration) {
        uint64_t snapshot;

        if (!mutex_lock(&counter_mutex)) {
            task_exit_current(1);
        }

        snapshot = mutex_counter;

        /*
         * Yield while holding the mutex so the other worker must actually
         * enter TASK_BLOCKED and sleep on the mutex wait queue.
         */
        if (!cooperative_yield()) {
            task_exit_current(1);
        }

        mutex_counter = snapshot + 1u;
        if (!mutex_unlock(&counter_mutex)) {
            task_exit_current(1);
        }

        /*
         * No second yield is needed here.  unlock() hands the mutex directly
         * to the oldest waiter.  If this worker reaches the next iteration
         * first, mutex_lock() will block behind that handed-off owner anyway.
         * Avoiding the extra yield keeps the test focused on mutex blocking
         * instead of spending most of its time cycling through bootstrap.
         */
    }

    task_exit_current(0);
}

static void semaphore_worker(void *argument)
{
    uint64_t iteration;
    (void)argument;

    for (iteration = 0u; iteration < SEMAPHORE_ITERATIONS; ++iteration) {
        uint64_t flags;

        if (!semaphore_wait(&test_semaphore)) {
            task_exit_current(1);
        }

        flags = spin_lock_irqsave(&semaphore_monitor_lock);
        ++semaphore_inside;
        if (semaphore_inside > semaphore_peak) {
            semaphore_peak = semaphore_inside;
        }
        spin_unlock_irqrestore(&semaphore_monitor_lock, flags);

        /* Keep a token while another task runs, forcing the third to block. */
        if (!cooperative_yield()) {
            task_exit_current(1);
        }

        flags = spin_lock_irqsave(&semaphore_monitor_lock);
        if (semaphore_inside == 0u) {
            spin_unlock_irqrestore(&semaphore_monitor_lock, flags);
            task_exit_current(1);
        }
        --semaphore_inside;
        spin_unlock_irqrestore(&semaphore_monitor_lock, flags);

        semaphore_post(&test_semaphore);

        if (!cooperative_yield()) {
            task_exit_current(1);
        }
    }

    task_exit_current(0);
}

static int create_workers(
    const char *name_prefix,
    task_entry_t entry,
    struct worker_argument *arguments,
    size_t count,
    uint64_t *ids
)
{
    size_t index;
    int created = 1;

    interrupts_disable();

    for (index = 0u; index < count; ++index) {
        arguments[index].index = index;
        if (!task_create(name_prefix, entry, &arguments[index], &ids[index])) {
            created = 0;
            break;
        }
    }

    interrupts_enable();
    return created;
}

static int wait_for_workers(const uint64_t *ids, size_t count)
{
    const uint64_t start = timer_ticks();

    for (;;) {
        size_t index;
        int complete = 1;

        for (index = 0u; index < count; ++index) {
            const struct task *task = scheduler_task_by_id(ids[index]);

            if (task == 0 || task->state != TASK_TERMINATED) {
                complete = 0;
                break;
            }
            if (task->exit_code != 0) {
                return 0;
            }
        }

        if (complete) {
            return 1;
        }

        if ((timer_ticks() - start) >= SYNC_TEST_TIMEOUT_TICKS) {
            return 0;
        }

        /*
         * Do not let the bootstrap test runner consume an entire 5-tick
         * quantum every time it is scheduled between workers.  Voluntarily
         * yielding makes contention tests deterministic and keeps their
         * timeout independent of the bootstrap task's scheduler quantum.
         */
        if (!cooperative_yield()) {
            return 0;
        }
    }
}

static int run_two_workers(task_entry_t entry)
{
    uint64_t ids[2];

    if (!create_workers(
            "phase16-sync-worker",
            entry,
            two_worker_arguments,
            2u,
            ids
        )) {
        return 0;
    }

    return wait_for_workers(ids, 2u);
}

int sync_run_selftest(struct sync_selftest_result *result)
{
    uint64_t semaphore_ids[SEMAPHORE_WORKERS];

    if (result == 0) {
        return 0;
    }

    result->failure_stage = SYNC_SELFTEST_STAGE_NONE;
    if (!scheduler_running() || !interrupts_enabled()) {
        return 0;
    }

    result->race_expected = RACE_ITERATIONS * 2u;
    result->race_actual = 0u;
    result->spin_expected = SPIN_ITERATIONS * 2u;
    result->spin_actual = 0u;
    result->spin_acquisitions = 0u;
    result->mutex_expected = MUTEX_ITERATIONS * 2u;
    result->mutex_actual = 0u;
    result->mutex_blocks = 0u;
    result->mutex_wakeups = 0u;
    result->semaphore_limit = SEMAPHORE_LIMIT;
    result->semaphore_peak = 0u;
    result->semaphore_blocks = 0u;
    result->semaphore_wakeups = 0u;

    /* Stage 1: deliberately broken read-modify-write critical section. */
    result->failure_stage = SYNC_SELFTEST_STAGE_RACE;
    race_counter = 0u;
    race_loaded[0] = 0u;
    race_loaded[1] = 0u;
    race_stored[0] = 0u;
    race_stored[1] = 0u;

    if (!run_two_workers(race_worker)) {
        return 0;
    }

    result->race_actual = race_counter;
    if (race_counter >= result->race_expected) {
        return 0;
    }

    /* Stage 2: same shared-counter idea protected by a spinlock. */
    result->failure_stage = SYNC_SELFTEST_STAGE_SPINLOCK;
    spin_counter = 0u;
    spinlock_init(&counter_spinlock);
    if (!run_two_workers(spin_worker)) {
        return 0;
    }

    result->spin_actual = spin_counter;
    result->spin_acquisitions = counter_spinlock.acquisitions;
    if (spin_counter != result->spin_expected) {
        return 0;
    }

    /* Stage 3: sleeping mutex. Yielding while locked forces contention. */
    result->failure_stage = SYNC_SELFTEST_STAGE_MUTEX;
    mutex_counter = 0u;
    mutex_init(&counter_mutex);
    if (!run_two_workers(mutex_worker)) {
        /* Preserve partial state so a runtime failure is diagnosable. */
        result->mutex_actual = mutex_counter;
        result->mutex_blocks = counter_mutex.blocks;
        result->mutex_wakeups = counter_mutex.wakeups;
        return 0;
    }

    result->mutex_actual = mutex_counter;
    result->mutex_blocks = counter_mutex.blocks;
    result->mutex_wakeups = counter_mutex.wakeups;
    if (mutex_counter != result->mutex_expected ||
        counter_mutex.blocks == 0u || counter_mutex.wakeups == 0u ||
        mutex_is_locked(&counter_mutex)) {
        return 0;
    }

    /* Stage 4: counting semaphore allows exactly two concurrent entrants. */
    result->failure_stage = SYNC_SELFTEST_STAGE_SEMAPHORE;
    semaphore_inside = 0u;
    semaphore_peak = 0u;
    semaphore_init(&test_semaphore, SEMAPHORE_LIMIT);
    spinlock_init(&semaphore_monitor_lock);

    if (!create_workers(
            "phase16-semaphore-worker",
            semaphore_worker,
            semaphore_worker_arguments,
            SEMAPHORE_WORKERS,
            semaphore_ids
        ) ||
        !wait_for_workers(semaphore_ids, SEMAPHORE_WORKERS)) {
        return 0;
    }

    result->semaphore_peak = semaphore_peak;
    result->semaphore_blocks = test_semaphore.blocks;
    result->semaphore_wakeups = test_semaphore.wakeups;

    if (semaphore_inside != 0u ||
        semaphore_peak != SEMAPHORE_LIMIT ||
        test_semaphore.blocks == 0u || test_semaphore.wakeups == 0u ||
        wait_queue_count(&test_semaphore.waiters) != 0u ||
        semaphore_value(&test_semaphore) != SEMAPHORE_LIMIT) {
        return 0;
    }

    result->failure_stage = SYNC_SELFTEST_STAGE_NONE;
    return 1;
}
