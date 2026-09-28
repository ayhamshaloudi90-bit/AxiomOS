#include <stdint.h>

#include <axiom/arch/interrupts.h>
#include <axiom/process/scheduler.h>
#include <axiom/sync/mutex.h>

void mutex_init(struct mutex *mutex)
{
    if (mutex == 0) {
        return;
    }

    spinlock_init(&mutex->guard);
    wait_queue_init(&mutex->waiters);
    mutex->owner_id = MUTEX_NO_OWNER;
    mutex->acquisitions = 0u;
    mutex->blocks = 0u;
    mutex->wakeups = 0u;
}

int mutex_lock(struct mutex *mutex)
{
    uint64_t irq_was_enabled;
    uint64_t task_id;
    const struct task *current;

    if (mutex == 0) {
        return 0;
    }

    current = scheduler_current_task();
    if (current == 0) {
        return 0;
    }
    task_id = current->id;

    irq_was_enabled = spin_lock_irqsave(&mutex->guard);

    if (mutex->owner_id == MUTEX_NO_OWNER) {
        mutex->owner_id = task_id;
        ++mutex->acquisitions;
        spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
        return 1;
    }

    /* Non-recursive by design. */
    if (mutex->owner_id == task_id) {
        spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
        return 0;
    }

    if (!wait_queue_enqueue_locked(&mutex->waiters, task_id) ||
        !scheduler_prepare_block_current(task_id)) {
        (void)wait_queue_remove_locked(&mutex->waiters, task_id);
        spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
        return 0;
    }

    ++mutex->blocks;

    /* Leave IF=0 until the scheduler has parked this blocked task. */
    spin_unlock(&mutex->guard);

    if (!scheduler_park_current(task_id)) {
        if (irq_was_enabled != 0u) {
            interrupts_enable();
        }
        return 0;
    }

    if (irq_was_enabled != 0u) {
        interrupts_enable();
    }

    /* unlock() directly hands ownership to the FIFO waiter it wakes. */
    irq_was_enabled = spin_lock_irqsave(&mutex->guard);
    if (mutex->owner_id != task_id) {
        spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
        return 0;
    }
    ++mutex->acquisitions;
    spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
    return 1;
}

int mutex_try_lock(struct mutex *mutex)
{
    uint64_t irq_was_enabled;
    const struct task *current;
    int acquired = 0;

    if (mutex == 0) {
        return 0;
    }

    current = scheduler_current_task();
    if (current == 0) {
        return 0;
    }

    irq_was_enabled = spin_lock_irqsave(&mutex->guard);
    if (mutex->owner_id == MUTEX_NO_OWNER) {
        mutex->owner_id = current->id;
        ++mutex->acquisitions;
        acquired = 1;
    }
    spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
    return acquired;
}

int mutex_unlock(struct mutex *mutex)
{
    uint64_t irq_was_enabled;
    uint64_t next_id;
    const struct task *current;

    if (mutex == 0) {
        return 0;
    }

    current = scheduler_current_task();
    if (current == 0) {
        return 0;
    }

    irq_was_enabled = spin_lock_irqsave(&mutex->guard);
    if (mutex->owner_id != current->id) {
        spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
        return 0;
    }

    next_id = wait_queue_dequeue_locked(&mutex->waiters);
    if (next_id == UINT64_MAX) {
        mutex->owner_id = MUTEX_NO_OWNER;
    } else {
        mutex->owner_id = next_id;
        if (!scheduler_wake_task(next_id)) {
            mutex->owner_id = MUTEX_NO_OWNER;
            spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
            return 0;
        }
        ++mutex->wakeups;
    }

    spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
    return 1;
}

int mutex_is_locked(const struct mutex *mutex)
{
    return mutex != 0 && mutex->owner_id != MUTEX_NO_OWNER;
}
