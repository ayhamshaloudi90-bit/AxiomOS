#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <axiom/syscalls.h>

#define PHASE22_SHM_KEY  0x220022ULL
#define PHASE22_MSGQ_KEY 0x220023ULL

static int pipe_demo(void)
{
    static const char message[] = "pipe child->parent";
    char buffer[64];
    long pipe_id = axiom_pipe_create();
    long child;
    long status = -1;
    long got;

    if (pipe_id < 0) return 0;
    child = fork();
    if (child < 0) {
        (void)axiom_pipe_close(pipe_id);
        return 0;
    }
    if (child == 0) {
        const long written = axiom_pipe_write(pipe_id, message, sizeof(message));
        _exit(written == (long)sizeof(message) ? 0 : 31);
    }

    if (waitpid(child, &status) != child || status != 0) {
        (void)axiom_pipe_close(pipe_id);
        return 0;
    }
    got = axiom_pipe_read(pipe_id, buffer, sizeof(buffer));
    (void)axiom_pipe_close(pipe_id);
    return got == (long)sizeof(message) && strcmp(buffer, message) == 0;
}

static int cross_program_demo(void)
{
    char *shared;
    char message[AXIOM_IPC_MESSAGE_MAX];
    long queue;
    long peer;
    long status = -1;
    long got;
    int ok;

    shared = (char *)axiom_shm_attach(PHASE22_SHM_KEY);
    if (shared == 0) return 0;
    strcpy(shared, "parent->peer");

    queue = axiom_msgq_open(PHASE22_MSGQ_KEY);
    if (queue < 0) {
        (void)axiom_shm_detach(shared);
        return 0;
    }

    peer = axiom_spawn("/bin/ipc-peer");
    if (peer < 0 || waitpid(peer, &status) != peer || status != 0) {
        (void)axiom_msgq_close(queue);
        (void)axiom_shm_detach(shared);
        return 0;
    }

    got = axiom_msgq_receive(queue, message, sizeof(message));
    ok = strcmp(shared, "peer->parent") == 0 &&
        got == (long)sizeof("queue->parent") &&
        strcmp(message, "queue->parent") == 0;

    (void)axiom_msgq_close(queue);
    (void)axiom_shm_detach(shared);
    return ok;
}

int main(void)
{
    struct axiom_ipc_stats stats;

    printf("AxiomOS Phase 22 IPC userspace demo.\n");

    if (!pipe_demo()) {
        printf("Phase 22 pipe process communication: FAILED\n");
        return 41;
    }
    printf("Phase 22 pipe process communication: OK\n");

    if (!cross_program_demo()) {
        printf("Phase 22 shared/message cross-program communication: FAILED\n");
        return 42;
    }
    printf("Phase 22 shared memory communication: OK\n");
    printf("Phase 22 message queue communication: OK\n");
    printf("Phase 22 two-program IPC demonstration: OK\n");

    if (axiom_ipcstats(&stats) < 0 ||
        stats.pipes_created == 0u ||
        stats.pipe_bytes_written == 0u ||
        stats.pipe_bytes_read == 0u ||
        stats.shared_objects_created == 0u ||
        stats.shared_attachments < 2u ||
        stats.message_queues_created == 0u ||
        stats.messages_sent == 0u ||
        stats.messages_received == 0u) {
        printf("Phase 22 IPC statistics: FAILED\n");
        return 43;
    }

    printf("Phase 22 IPC statistics: OK\n");
    printf("Phase 22 IPC userspace demo complete.\n");
    return 0;
}
