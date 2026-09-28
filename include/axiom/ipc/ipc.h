#ifndef AXIOM_IPC_IPC_H
#define AXIOM_IPC_IPC_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/ipc.h>
#include <axiom/memory/address.h>

int ipc_init(void);
int ipc_initialized(void);

int ipc_pipe_create(uint32_t *pipe_id_out);
int64_t ipc_pipe_write(uint32_t pipe_id, const void *buffer, size_t count);
int64_t ipc_pipe_read(uint32_t pipe_id, void *buffer, size_t count);
int ipc_pipe_close(uint32_t pipe_id);

int ipc_shm_attach(
    uint64_t task_id,
    paddr_t address_space,
    uint64_t key,
    vaddr_t *address_out
);
int ipc_shm_detach(uint64_t task_id, paddr_t address_space, vaddr_t address);
void ipc_task_detach_all(uint64_t task_id, paddr_t address_space);
int ipc_task_clone_shared(
    uint64_t parent_task_id,
    uint64_t child_task_id,
    paddr_t child_address_space
);

int ipc_msgq_open(uint64_t key, uint32_t *queue_id_out);
int64_t ipc_msgq_send(uint32_t queue_id, const void *message, size_t length);
int64_t ipc_msgq_receive(uint32_t queue_id, void *buffer, size_t capacity);
int ipc_msgq_close(uint32_t queue_id);

struct axiom_ipc_stats ipc_get_stats(void);

#endif
