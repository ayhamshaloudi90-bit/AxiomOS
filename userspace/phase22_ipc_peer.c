#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <axiom/syscalls.h>

#define PHASE22_SHM_KEY  0x220022ULL
#define PHASE22_MSGQ_KEY 0x220023ULL

int main(void)
{
    char *shared = (char *)axiom_shm_attach(PHASE22_SHM_KEY);
    long queue;
    static const char response[] = "queue->parent";

    if (shared == 0) {
        printf("Phase 22 peer shared-memory attach: FAILED\n");
        return 21;
    }
    if (strcmp(shared, "parent->peer") != 0) {
        printf("Phase 22 peer shared-memory read: FAILED\n");
        (void)axiom_shm_detach(shared);
        return 22;
    }

    strcpy(shared, "peer->parent");
    queue = axiom_msgq_open(PHASE22_MSGQ_KEY);
    if (queue < 0) {
        printf("Phase 22 peer message-queue open: FAILED\n");
        (void)axiom_shm_detach(shared);
        return 23;
    }
    if (axiom_msgq_send(queue, response, sizeof(response)) != (long)sizeof(response)) {
        printf("Phase 22 peer message-queue send: FAILED\n");
        (void)axiom_msgq_close(queue);
        (void)axiom_shm_detach(shared);
        return 24;
    }

    (void)axiom_msgq_close(queue);
    (void)axiom_shm_detach(shared);
    printf("Phase 22 peer cross-program IPC: OK\n");
    return 0;
}
