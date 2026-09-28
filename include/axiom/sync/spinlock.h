#ifndef AXIOM_SYNC_SPINLOCK_H
#define AXIOM_SYNC_SPINLOCK_H

#include <stdint.h>

/*
 * Phase-16/17 IRQ-save SMP spinlock.
 *
 * Holding the lock with interrupts disabled prevents the current CPU from
 * being preempted while it owns the lock. Disabling local interrupts prevents the owner CPU from being
 * preempted while the atomic x86 XCHG provides exclusion against other CPUs.
 */
struct spinlock {
    volatile uint32_t locked;
    uint64_t acquisitions;
    uint64_t contentions;
};

void spinlock_init(struct spinlock *lock);

/* Acquire while saving whether interrupts were enabled on entry. */
uint64_t spin_lock_irqsave(struct spinlock *lock);

/* Release but deliberately leave interrupts disabled. */
void spin_unlock(struct spinlock *lock);

/* Release and restore the IF state returned by spin_lock_irqsave(). */
void spin_unlock_irqrestore(struct spinlock *lock, uint64_t irq_was_enabled);

#endif
