#include <stdint.h>

#include <axiom/arch/interrupts.h>
#include <axiom/sync/spinlock.h>

static uint32_t exchange_locked(volatile uint32_t *memory, uint32_t value)
{
    __asm__ volatile (
        "xchgl %0, %1"
        : "+r" (value), "+m" (*memory)
        :
        : "memory"
    );
    return value;
}

void spinlock_init(struct spinlock *lock)
{
    if (lock == 0) {
        return;
    }

    lock->locked = 0u;
    lock->acquisitions = 0u;
    lock->contentions = 0u;
}

uint64_t spin_lock_irqsave(struct spinlock *lock)
{
    const uint64_t irq_was_enabled = interrupts_enabled() ? 1u : 0u;

    if (lock == 0) {
        return irq_was_enabled;
    }

    interrupts_disable();

    for (;;) {
        if (exchange_locked(&lock->locked, 1u) == 0u) {
            (void)__atomic_add_fetch(&lock->acquisitions, 1u, __ATOMIC_RELAXED);
            return irq_was_enabled;
        }

        (void)__atomic_add_fetch(&lock->contentions, 1u, __ATOMIC_RELAXED);
        while (lock->locked != 0u) {
            __asm__ volatile ("pause" ::: "memory");
        }
    }
}

void spin_unlock(struct spinlock *lock)
{
    if (lock == 0) {
        return;
    }

    __atomic_store_n(&lock->locked, 0u, __ATOMIC_RELEASE);
}

void spin_unlock_irqrestore(struct spinlock *lock, uint64_t irq_was_enabled)
{
    spin_unlock(lock);

    if (irq_was_enabled != 0u) {
        interrupts_enable();
    }
}
