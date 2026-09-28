#ifndef AXIOM_SYNC_WAIT_QUEUE_H
#define AXIOM_SYNC_WAIT_QUEUE_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/process/scheduler.h>

#define WAIT_QUEUE_MAX_WAITERS SCHEDULER_MAX_TASKS

/*
 * FIFO of scheduler task IDs. The queue itself is intentionally lock-free:
 * callers protect it with the same spinlock that protects their condition.
 * That lets "condition false -> enqueue -> mark BLOCKED" happen atomically.
 */
struct wait_queue {
    uint64_t task_ids[WAIT_QUEUE_MAX_WAITERS];
    size_t head;
    size_t count;
    uint64_t sleeps;
    uint64_t wakeups;
};

void wait_queue_init(struct wait_queue *queue);
int wait_queue_enqueue_locked(struct wait_queue *queue, uint64_t task_id);
uint64_t wait_queue_dequeue_locked(struct wait_queue *queue);
int wait_queue_remove_locked(struct wait_queue *queue, uint64_t task_id);
size_t wait_queue_count(const struct wait_queue *queue);

#endif
