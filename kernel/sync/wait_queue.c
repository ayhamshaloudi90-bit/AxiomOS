#include <stddef.h>
#include <stdint.h>

#include <axiom/sync/wait_queue.h>

void wait_queue_init(struct wait_queue *queue)
{
    size_t index;

    if (queue == 0) {
        return;
    }

    for (index = 0u; index < WAIT_QUEUE_MAX_WAITERS; ++index) {
        queue->task_ids[index] = UINT64_MAX;
    }

    queue->head = 0u;
    queue->count = 0u;
    queue->sleeps = 0u;
    queue->wakeups = 0u;
}

int wait_queue_enqueue_locked(struct wait_queue *queue, uint64_t task_id)
{
    size_t index;
    size_t tail;

    if (queue == 0 || task_id == UINT64_MAX ||
        queue->count >= WAIT_QUEUE_MAX_WAITERS) {
        return 0;
    }

    for (index = 0u; index < queue->count; ++index) {
        const size_t slot = (queue->head + index) % WAIT_QUEUE_MAX_WAITERS;
        if (queue->task_ids[slot] == task_id) {
            return 0;
        }
    }

    tail = (queue->head + queue->count) % WAIT_QUEUE_MAX_WAITERS;
    queue->task_ids[tail] = task_id;
    ++queue->count;
    ++queue->sleeps;
    return 1;
}

uint64_t wait_queue_dequeue_locked(struct wait_queue *queue)
{
    uint64_t task_id;

    if (queue == 0 || queue->count == 0u) {
        return UINT64_MAX;
    }

    task_id = queue->task_ids[queue->head];
    queue->task_ids[queue->head] = UINT64_MAX;
    queue->head = (queue->head + 1u) % WAIT_QUEUE_MAX_WAITERS;
    --queue->count;
    ++queue->wakeups;
    return task_id;
}

int wait_queue_remove_locked(struct wait_queue *queue, uint64_t task_id)
{
    size_t index;

    if (queue == 0 || queue->count == 0u) {
        return 0;
    }

    for (index = 0u; index < queue->count; ++index) {
        const size_t slot = (queue->head + index) % WAIT_QUEUE_MAX_WAITERS;

        if (queue->task_ids[slot] == task_id) {
            size_t shift;

            for (shift = index; shift + 1u < queue->count; ++shift) {
                const size_t from =
                    (queue->head + shift + 1u) % WAIT_QUEUE_MAX_WAITERS;
                const size_t to =
                    (queue->head + shift) % WAIT_QUEUE_MAX_WAITERS;
                queue->task_ids[to] = queue->task_ids[from];
            }

            queue->task_ids[
                (queue->head + queue->count - 1u) % WAIT_QUEUE_MAX_WAITERS
            ] = UINT64_MAX;
            --queue->count;
            return 1;
        }
    }

    return 0;
}

size_t wait_queue_count(const struct wait_queue *queue)
{
    return queue != 0 ? queue->count : 0u;
}
