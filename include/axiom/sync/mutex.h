#ifndef AXIOM_SYNC_MUTEX_H
#define AXIOM_SYNC_MUTEX_H

#include <stdint.h>

#include <axiom/sync/spinlock.h>
#include <axiom/sync/wait_queue.h>

#define MUTEX_NO_OWNER UINT64_MAX

/* Sleeping, non-recursive kernel mutex with FIFO ownership handoff. */
struct mutex {
    struct spinlock guard;
    struct wait_queue waiters;
    uint64_t owner_id;
    uint64_t acquisitions;
    uint64_t blocks;
    uint64_t wakeups;
};

void mutex_init(struct mutex *mutex);
int mutex_lock(struct mutex *mutex);
int mutex_try_lock(struct mutex *mutex);
int mutex_unlock(struct mutex *mutex);
int mutex_is_locked(const struct mutex *mutex);

#endif
