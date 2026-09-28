#ifndef AXIOM_LIBC_UNISTD_H
#define AXIOM_LIBC_UNISTD_H

#include <stddef.h>
#include <stdint.h>

long write(int fd, const void *buffer, size_t count);
long read(int fd, void *buffer, size_t count);
_Noreturn void _exit(int status);
long sleep(uint64_t milliseconds);
long getpid(void);
long getppid(void);
long getuid(void);
long getgid(void);
long yield(void);
long fork(void);
long exec(const char *path);
long close(int fd);
long lseek(int fd, long offset, int whence);
long waitpid(long pid, long *status);

#endif
