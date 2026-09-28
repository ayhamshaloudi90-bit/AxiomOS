#ifndef AXIOM_SYNC_SEMAPHORE_H
#define AXIOM_SYNC_SEMAPHORE_H

#include <stdint.h>

#include <axiom/sync/spinlock.h>
#include <axiom/sync/wait_queue.h>

/* Counting semaphore. Waiters sleep and tokens are handed off FIFO. */
struct semaphore {
    struct spinlock guard;
    struct wait_queue waiters;
    uint64_t count;
    uint64_t waits;
    uint64_t posts;
    uint64_t blocks;
    uint64_t wakeups;
};

void semaphore_init(struct semaphore *semaphore, uint64_t initial_count);
int semaphore_wait(struct semaphore *semaphore);
int semaphore_try_wait(struct semaphore *semaphore);
void semaphore_post(struct semaphore *semaphore);
uint64_t semaphore_value(struct semaphore *semaphore);

#endif
