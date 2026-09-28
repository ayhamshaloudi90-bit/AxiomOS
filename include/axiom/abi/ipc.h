#ifndef AXIOM_ABI_IPC_H
#define AXIOM_ABI_IPC_H

#define AXIOM_IPC_PIPE_CAPACITY 1024u
#define AXIOM_IPC_PIPE_IO_MAX 512u
#define AXIOM_IPC_MESSAGE_MAX 64u
#define AXIOM_IPC_QUEUE_DEPTH 8u
#define AXIOM_IPC_SHM_SIZE 4096u
#define AXIOM_IPC_SHM_BASE 0x0000200000000000ULL
#define AXIOM_IPC_SHM_MAX_ATTACHMENTS 8u

#ifndef __ASSEMBLER__
#include <stdint.h>

struct axiom_ipc_stats {
    uint64_t pipes_created;
    uint64_t pipe_bytes_written;
    uint64_t pipe_bytes_read;
    uint64_t shared_objects_created;
    uint64_t shared_attachments;
    uint64_t message_queues_created;
    uint64_t messages_sent;
    uint64_t messages_received;
};

#endif

#endif
