#ifndef AXIOM_LIBC_SYSCALLS_H
#define AXIOM_LIBC_SYSCALLS_H

#include <stddef.h>
#include <stdint.h>
#include <axiom/abi/fs.h>
#include <axiom/abi/graphics.h>
#include <axiom/abi/input.h>
#include <axiom/abi/ipc.h>
#include <axiom/abi/network.h>
#include <axiom/abi/process.h>
#include <axiom/abi/scheduler.h>

long axiom_readdir(const char *path, unsigned long index, struct axiom_dirent *entry);
long axiom_mkdir(const char *path);
long axiom_spawn(const char *path);
long axiom_clear(void);
long axiom_kbdstats(struct axiom_keyboard_stats *stats);
long axiom_mousestate(struct axiom_mouse_state *state);
long axiom_procinfo(unsigned long index, struct axiom_process_info *info);
long axiom_kill(long pid, unsigned long signal_number);
long axiom_netinfo(struct axiom_net_info *info);
long axiom_ping(const char *target, struct axiom_ping_result *result);
long axiom_dns(const char *host, uint32_t *address);
long axiom_httpget(const char *host, const char *path, void *body, size_t capacity, struct axiom_http_result *result);
long axiom_httpserve(uint16_t port, const void *body, size_t length, struct axiom_http_server_result *result);
void *axiom_mmap(size_t length);
long axiom_getpriority(void);
long axiom_setpriority(unsigned long priority);
long axiom_getaffinity(void);
long axiom_setaffinity(uint64_t affinity_mask);
long axiom_schedstats(struct axiom_scheduler_stats *stats);
long axiom_pipe_create(void);
long axiom_pipe_write(long pipe_id, const void *buffer, size_t count);
long axiom_pipe_read(long pipe_id, void *buffer, size_t count);
long axiom_pipe_close(long pipe_id);
void *axiom_shm_attach(uint64_t key);
long axiom_shm_detach(void *address);
long axiom_msgq_open(uint64_t key);
long axiom_msgq_send(long queue_id, const void *message, size_t length);
long axiom_msgq_receive(long queue_id, void *buffer, size_t capacity);
long axiom_msgq_close(long queue_id);
long axiom_ipcstats(struct axiom_ipc_stats *stats);

#endif
