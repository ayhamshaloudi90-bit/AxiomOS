#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>
#include <axiom/abi/ipc.h>
#include <axiom/ipc/ipc.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>

#define IPC_MAX_PIPES 8u
#define IPC_MAX_SHARED_OBJECTS 8u
#define IPC_MAX_SHARED_ATTACHMENTS 128u
#define IPC_MAX_MESSAGE_QUEUES 8u

struct ipc_pipe {
    int used;
    uint32_t id;
    uint8_t bytes[AXIOM_IPC_PIPE_CAPACITY];
    size_t head;
    size_t count;
};

struct ipc_shared_object {
    int used;
    uint64_t key;
    paddr_t page;
    uint32_t attachments;
};

struct ipc_shared_attachment {
    int used;
    uint64_t task_id;
    paddr_t address_space;
    size_t object_index;
    vaddr_t address;
};

struct ipc_message {
    size_t length;
    uint8_t bytes[AXIOM_IPC_MESSAGE_MAX];
};

struct ipc_message_queue {
    int used;
    uint32_t id;
    uint64_t key;
    uint32_t open_references;
    struct ipc_message messages[AXIOM_IPC_QUEUE_DEPTH];
    size_t head;
    size_t count;
};

static struct ipc_pipe pipes[IPC_MAX_PIPES];
static struct ipc_shared_object shared_objects[IPC_MAX_SHARED_OBJECTS];
static struct ipc_shared_attachment shared_attachments[IPC_MAX_SHARED_ATTACHMENTS];
static struct ipc_message_queue message_queues[IPC_MAX_MESSAGE_QUEUES];
static struct axiom_ipc_stats ipc_stats;
static uint32_t next_pipe_id;
static uint32_t next_queue_id;
static int initialized;

static void bytes_clear(void *memory, size_t count)
{
    uint8_t *bytes = (uint8_t *)memory;
    size_t index;
    for (index = 0u; index < count; ++index) bytes[index] = 0u;
}

static void bytes_copy(void *destination, const void *source, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    size_t index;
    for (index = 0u; index < count; ++index) out[index] = in[index];
}

static struct ipc_pipe *find_pipe(uint32_t id)
{
    size_t index;
    for (index = 0u; index < IPC_MAX_PIPES; ++index) {
        if (pipes[index].used && pipes[index].id == id) return &pipes[index];
    }
    return 0;
}

static struct ipc_message_queue *find_queue_by_id(uint32_t id)
{
    size_t index;
    for (index = 0u; index < IPC_MAX_MESSAGE_QUEUES; ++index) {
        if (message_queues[index].used && message_queues[index].id == id) {
            return &message_queues[index];
        }
    }
    return 0;
}

int ipc_init(void)
{
    if (initialized) return 1;
    bytes_clear(pipes, sizeof(pipes));
    bytes_clear(shared_objects, sizeof(shared_objects));
    bytes_clear(shared_attachments, sizeof(shared_attachments));
    bytes_clear(message_queues, sizeof(message_queues));
    bytes_clear(&ipc_stats, sizeof(ipc_stats));
    next_pipe_id = 1u;
    next_queue_id = 1u;
    initialized = 1;
    return 1;
}

int ipc_initialized(void)
{
    return initialized;
}

int ipc_pipe_create(uint32_t *pipe_id_out)
{
    size_t index;
    if (!initialized || pipe_id_out == 0) return -AXIOM_EINVAL;
    for (index = 0u; index < IPC_MAX_PIPES; ++index) {
        if (!pipes[index].used) {
            bytes_clear(&pipes[index], sizeof(pipes[index]));
            pipes[index].used = 1;
            pipes[index].id = next_pipe_id++;
            if (next_pipe_id == 0u) next_pipe_id = 1u;
            *pipe_id_out = pipes[index].id;
            ++ipc_stats.pipes_created;
            return 0;
        }
    }
    return -AXIOM_ENOSPC;
}

int64_t ipc_pipe_write(uint32_t pipe_id, const void *buffer, size_t count)
{
    struct ipc_pipe *pipe;
    const uint8_t *bytes = (const uint8_t *)buffer;
    size_t writable;
    size_t index;
    if (!initialized || buffer == 0 || count == 0u) return -AXIOM_EINVAL;
    pipe = find_pipe(pipe_id);
    if (pipe == 0) return -AXIOM_EBADF;
    writable = AXIOM_IPC_PIPE_CAPACITY - pipe->count;
    if (writable == 0u) return -AXIOM_EBUSY;
    if (count < writable) writable = count;
    for (index = 0u; index < writable; ++index) {
        const size_t slot = (pipe->head + pipe->count + index) % AXIOM_IPC_PIPE_CAPACITY;
        pipe->bytes[slot] = bytes[index];
    }
    pipe->count += writable;
    ipc_stats.pipe_bytes_written += writable;
    return (int64_t)writable;
}

int64_t ipc_pipe_read(uint32_t pipe_id, void *buffer, size_t count)
{
    struct ipc_pipe *pipe;
    uint8_t *bytes = (uint8_t *)buffer;
    size_t readable;
    size_t index;
    if (!initialized || buffer == 0 || count == 0u) return -AXIOM_EINVAL;
    pipe = find_pipe(pipe_id);
    if (pipe == 0) return -AXIOM_EBADF;
    if (pipe->count == 0u) return -AXIOM_EBUSY;
    readable = count < pipe->count ? count : pipe->count;
    for (index = 0u; index < readable; ++index) {
        bytes[index] = pipe->bytes[(pipe->head + index) % AXIOM_IPC_PIPE_CAPACITY];
    }
    pipe->head = (pipe->head + readable) % AXIOM_IPC_PIPE_CAPACITY;
    pipe->count -= readable;
    ipc_stats.pipe_bytes_read += readable;
    return (int64_t)readable;
}

int ipc_pipe_close(uint32_t pipe_id)
{
    struct ipc_pipe *pipe;
    if (!initialized) return -AXIOM_EINVAL;
    pipe = find_pipe(pipe_id);
    if (pipe == 0) return -AXIOM_EBADF;
    bytes_clear(pipe, sizeof(*pipe));
    return 0;
}

static int find_shared_object(uint64_t key, size_t *index_out)
{
    size_t index;
    for (index = 0u; index < IPC_MAX_SHARED_OBJECTS; ++index) {
        if (shared_objects[index].used && shared_objects[index].key == key) {
            if (index_out != 0) *index_out = index;
            return 1;
        }
    }
    return 0;
}

static int create_shared_object(uint64_t key, size_t *index_out)
{
    size_t index;
    paddr_t page;
    void *bytes;
    for (index = 0u; index < IPC_MAX_SHARED_OBJECTS; ++index) {
        if (!shared_objects[index].used) break;
    }
    if (index == IPC_MAX_SHARED_OBJECTS) return -AXIOM_ENOSPC;
    page = pmm_alloc_page();
    if (page == PADDR_INVALID) return -AXIOM_ENOMEM;
    bytes = pmm_phys_to_hhdm(page);
    if (bytes == 0) {
        (void)pmm_free_page(page);
        return -AXIOM_EIO;
    }
    bytes_clear(bytes, VMM_PAGE_SIZE);
    shared_objects[index].used = 1;
    shared_objects[index].key = key;
    shared_objects[index].page = page;
    shared_objects[index].attachments = 0u;
    *index_out = index;
    ++ipc_stats.shared_objects_created;
    return 0;
}

static int address_used_by_task(uint64_t task_id, vaddr_t address)
{
    size_t index;
    for (index = 0u; index < IPC_MAX_SHARED_ATTACHMENTS; ++index) {
        if (shared_attachments[index].used &&
            shared_attachments[index].task_id == task_id &&
            shared_attachments[index].address == address) return 1;
    }
    return 0;
}

int ipc_shm_attach(
    uint64_t task_id,
    paddr_t address_space,
    uint64_t key,
    vaddr_t *address_out
)
{
    size_t object_index;
    size_t attachment_index;
    size_t slot;
    vaddr_t address = 0u;
    int result;

    if (!initialized || task_id == UINT64_MAX || address_space == PADDR_INVALID ||
        key == 0u || address_out == 0) return -AXIOM_EINVAL;

    for (attachment_index = 0u; attachment_index < IPC_MAX_SHARED_ATTACHMENTS; ++attachment_index) {
        if (shared_attachments[attachment_index].used &&
            shared_attachments[attachment_index].task_id == task_id &&
            shared_objects[shared_attachments[attachment_index].object_index].used &&
            shared_objects[shared_attachments[attachment_index].object_index].key == key) {
            *address_out = shared_attachments[attachment_index].address;
            return 0;
        }
    }

    if (!find_shared_object(key, &object_index)) {
        result = create_shared_object(key, &object_index);
        if (result < 0) return result;
    }

    for (attachment_index = 0u; attachment_index < IPC_MAX_SHARED_ATTACHMENTS; ++attachment_index) {
        if (!shared_attachments[attachment_index].used) break;
    }
    if (attachment_index == IPC_MAX_SHARED_ATTACHMENTS) return -AXIOM_ENOSPC;

    for (slot = 0u; slot < AXIOM_IPC_SHM_MAX_ATTACHMENTS; ++slot) {
        address = (vaddr_t)AXIOM_IPC_SHM_BASE + (vaddr_t)(slot * VMM_PAGE_SIZE);
        if (!address_used_by_task(task_id, address)) break;
    }
    if (slot == AXIOM_IPC_SHM_MAX_ATTACHMENTS) return -AXIOM_ENOSPC;
    if (vmm_virt_to_phys_in_address_space(address_space, address) != PADDR_INVALID) {
        return -AXIOM_EBUSY;
    }
    if (!vmm_map_page_in_address_space(
            address_space,
            address,
            shared_objects[object_index].page,
            VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE
        )) return -AXIOM_ENOMEM;

    shared_attachments[attachment_index].used = 1;
    shared_attachments[attachment_index].task_id = task_id;
    shared_attachments[attachment_index].address_space = address_space;
    shared_attachments[attachment_index].object_index = object_index;
    shared_attachments[attachment_index].address = address;
    ++shared_objects[object_index].attachments;
    ++ipc_stats.shared_attachments;
    *address_out = address;
    return 0;
}

static void release_shared_attachment(size_t attachment_index, int unmap)
{
    struct ipc_shared_attachment *attachment;
    struct ipc_shared_object *object;
    if (attachment_index >= IPC_MAX_SHARED_ATTACHMENTS ||
        !shared_attachments[attachment_index].used) return;
    attachment = &shared_attachments[attachment_index];
    object = &shared_objects[attachment->object_index];
    if (unmap && attachment->address_space != PADDR_INVALID) {
        (void)vmm_unmap_page_in_address_space(attachment->address_space, attachment->address);
    }
    if (object->used && object->attachments != 0u) --object->attachments;
    bytes_clear(attachment, sizeof(*attachment));
    if (object->used && object->attachments == 0u) {
        if (object->page != PADDR_INVALID) (void)pmm_free_page(object->page);
        bytes_clear(object, sizeof(*object));
    }
}

int ipc_shm_detach(uint64_t task_id, paddr_t address_space, vaddr_t address)
{
    size_t index;
    if (!initialized || task_id == UINT64_MAX || address_space == PADDR_INVALID) {
        return -AXIOM_EINVAL;
    }
    for (index = 0u; index < IPC_MAX_SHARED_ATTACHMENTS; ++index) {
        if (shared_attachments[index].used &&
            shared_attachments[index].task_id == task_id &&
            shared_attachments[index].address_space == address_space &&
            shared_attachments[index].address == address) {
            release_shared_attachment(index, 1);
            return 0;
        }
    }
    return -AXIOM_EINVAL;
}

void ipc_task_detach_all(uint64_t task_id, paddr_t address_space)
{
    size_t index;
    if (!initialized) return;
    for (index = 0u; index < IPC_MAX_SHARED_ATTACHMENTS; ++index) {
        if (shared_attachments[index].used &&
            shared_attachments[index].task_id == task_id &&
            shared_attachments[index].address_space == address_space) {
            release_shared_attachment(index, 1);
        }
    }
}

int ipc_task_clone_shared(
    uint64_t parent_task_id,
    uint64_t child_task_id,
    paddr_t child_address_space
)
{
    size_t index;
    size_t cloned[AXIOM_IPC_SHM_MAX_ATTACHMENTS];
    size_t cloned_count = 0u;

    if (!initialized) return 1;
    if (child_task_id == UINT64_MAX || child_address_space == PADDR_INVALID) return 0;

    for (index = 0u; index < IPC_MAX_SHARED_ATTACHMENTS; ++index) {
        size_t free_index;
        struct ipc_shared_attachment *source;
        struct ipc_shared_object *object;
        if (!shared_attachments[index].used ||
            shared_attachments[index].task_id != parent_task_id) continue;
        if (cloned_count >= AXIOM_IPC_SHM_MAX_ATTACHMENTS) goto fail;
        source = &shared_attachments[index];
        object = &shared_objects[source->object_index];
        if (!object->used) goto fail;
        for (free_index = 0u; free_index < IPC_MAX_SHARED_ATTACHMENTS; ++free_index) {
            if (!shared_attachments[free_index].used) break;
        }
        if (free_index == IPC_MAX_SHARED_ATTACHMENTS) goto fail;
        if (!vmm_map_page_in_address_space(
                child_address_space,
                source->address,
                object->page,
                VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE
            )) goto fail;
        shared_attachments[free_index].used = 1;
        shared_attachments[free_index].task_id = child_task_id;
        shared_attachments[free_index].address_space = child_address_space;
        shared_attachments[free_index].object_index = source->object_index;
        shared_attachments[free_index].address = source->address;
        ++object->attachments;
        ++ipc_stats.shared_attachments;
        cloned[cloned_count++] = free_index;
    }
    return 1;

fail:
    while (cloned_count != 0u) {
        --cloned_count;
        release_shared_attachment(cloned[cloned_count], 1);
    }
    return 0;
}

int ipc_msgq_open(uint64_t key, uint32_t *queue_id_out)
{
    size_t index;
    if (!initialized || key == 0u || queue_id_out == 0) return -AXIOM_EINVAL;
    for (index = 0u; index < IPC_MAX_MESSAGE_QUEUES; ++index) {
        if (message_queues[index].used && message_queues[index].key == key) {
            if (message_queues[index].open_references == UINT32_MAX) return -AXIOM_EBUSY;
            ++message_queues[index].open_references;
            *queue_id_out = message_queues[index].id;
            return 0;
        }
    }
    for (index = 0u; index < IPC_MAX_MESSAGE_QUEUES; ++index) {
        if (!message_queues[index].used) {
            bytes_clear(&message_queues[index], sizeof(message_queues[index]));
            message_queues[index].used = 1;
            message_queues[index].id = next_queue_id++;
            if (next_queue_id == 0u) next_queue_id = 1u;
            message_queues[index].key = key;
            message_queues[index].open_references = 1u;
            *queue_id_out = message_queues[index].id;
            ++ipc_stats.message_queues_created;
            return 0;
        }
    }
    return -AXIOM_ENOSPC;
}

int64_t ipc_msgq_send(uint32_t queue_id, const void *message, size_t length)
{
    struct ipc_message_queue *queue;
    size_t slot;
    if (!initialized || message == 0 || length == 0u) return -AXIOM_EINVAL;
    if (length > AXIOM_IPC_MESSAGE_MAX) return -AXIOM_EMSGSIZE;
    queue = find_queue_by_id(queue_id);
    if (queue == 0) return -AXIOM_EBADF;
    if (queue->count >= AXIOM_IPC_QUEUE_DEPTH) return -AXIOM_EBUSY;
    slot = (queue->head + queue->count) % AXIOM_IPC_QUEUE_DEPTH;
    queue->messages[slot].length = length;
    bytes_copy(queue->messages[slot].bytes, message, length);
    ++queue->count;
    ++ipc_stats.messages_sent;
    return (int64_t)length;
}

int64_t ipc_msgq_receive(uint32_t queue_id, void *buffer, size_t capacity)
{
    struct ipc_message_queue *queue;
    struct ipc_message *message;
    if (!initialized || buffer == 0 || capacity == 0u) return -AXIOM_EINVAL;
    queue = find_queue_by_id(queue_id);
    if (queue == 0) return -AXIOM_EBADF;
    if (queue->count == 0u) return -AXIOM_EBUSY;
    message = &queue->messages[queue->head];
    if (capacity < message->length) return -AXIOM_EMSGSIZE;
    bytes_copy(buffer, message->bytes, message->length);
    capacity = message->length;
    bytes_clear(message, sizeof(*message));
    queue->head = (queue->head + 1u) % AXIOM_IPC_QUEUE_DEPTH;
    --queue->count;
    ++ipc_stats.messages_received;
    return (int64_t)capacity;
}

int ipc_msgq_close(uint32_t queue_id)
{
    struct ipc_message_queue *queue;
    if (!initialized) return -AXIOM_EINVAL;
    queue = find_queue_by_id(queue_id);
    if (queue == 0) return -AXIOM_EBADF;
    if (queue->open_references != 0u) --queue->open_references;
    if (queue->open_references == 0u) bytes_clear(queue, sizeof(*queue));
    return 0;
}

struct axiom_ipc_stats ipc_get_stats(void)
{
    return ipc_stats;
}
