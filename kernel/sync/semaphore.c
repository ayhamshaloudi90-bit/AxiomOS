#include <stdint.h>

#include <axiom/arch/interrupts.h>
#include <axiom/process/scheduler.h>
#include <axiom/sync/semaphore.h>

void semaphore_init(struct semaphore *semaphore, uint64_t initial_count)
{
    if (semaphore == 0) {
        return;
    }

    spinlock_init(&semaphore->guard);
    wait_queue_init(&semaphore->waiters);
    semaphore->count = initial_count;
    semaphore->waits = 0u;
    semaphore->posts = 0u;
    semaphore->blocks = 0u;
    semaphore->wakeups = 0u;
}

int semaphore_wait(struct semaphore *semaphore)
{
    uint64_t irq_was_enabled;
    uint64_t task_id;
    const struct task *current;

    if (semaphore == 0) {
        return 0;
    }

    current = scheduler_current_task();
    if (current == 0) {
        return 0;
    }
    task_id = current->id;

    irq_was_enabled = spin_lock_irqsave(&semaphore->guard);
    ++semaphore->waits;

    if (semaphore->count != 0u) {
        --semaphore->count;
        spin_unlock_irqrestore(&semaphore->guard, irq_was_enabled);
        return 1;
    }

    if (!wait_queue_enqueue_locked(&semaphore->waiters, task_id) ||
        !scheduler_prepare_block_current(task_id)) {
        (void)wait_queue_remove_locked(&semaphore->waiters, task_id);
        spin_unlock_irqrestore(&semaphore->guard, irq_was_enabled);
        return 0;
    }

    ++semaphore->blocks;
    spin_unlock(&semaphore->guard);

    if (!scheduler_park_current(task_id)) {
        if (irq_was_enabled != 0u) {
            interrupts_enable();
        }
        return 0;
    }

    if (irq_was_enabled != 0u) {
        interrupts_enable();
    }

    /* post() directly handed this waiter one token. */
    return 1;
}

int semaphore_try_wait(struct semaphore *semaphore)
{
    uint64_t irq_was_enabled;
    int acquired = 0;

    if (semaphore == 0) {
        return 0;
    }

    irq_was_enabled = spin_lock_irqsave(&semaphore->guard);
    ++semaphore->waits;
    if (semaphore->count != 0u) {
        --semaphore->count;
        acquired = 1;
    }
    spin_unlock_irqrestore(&semaphore->guard, irq_was_enabled);
    return acquired;
}

void semaphore_post(struct semaphore *semaphore)
{
    uint64_t irq_was_enabled;
    uint64_t task_id;

    if (semaphore == 0) {
        return;
    }

    irq_was_enabled = spin_lock_irqsave(&semaphore->guard);
    ++semaphore->posts;

    task_id = wait_queue_dequeue_locked(&semaphore->waiters);
    if (task_id != UINT64_MAX) {
        if (scheduler_wake_task(task_id)) {
            ++semaphore->wakeups;
        } else {
            ++semaphore->count;
        }
    } else {
        ++semaphore->count;
    }

    spin_unlock_irqrestore(&semaphore->guard, irq_was_enabled);
}

uint64_t semaphore_value(struct semaphore *semaphore)
{
    uint64_t irq_was_enabled;
    uint64_t value;

    if (semaphore == 0) {
        return 0u;
    }

    irq_was_enabled = spin_lock_irqsave(&semaphore->guard);
    value = semaphore->count;
    spin_unlock_irqrestore(&semaphore->guard, irq_was_enabled);
    return value;
}
