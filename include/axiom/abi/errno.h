#ifndef AXIOM_ABI_ERRNO_H
#define AXIOM_ABI_ERRNO_H

/* Small errno set shared by kernel and userspace. Syscalls return -errno. */
#define AXIOM_EPERM        1
#define AXIOM_ENOENT       2
#define AXIOM_ESRCH        3
#define AXIOM_EIO          5
#define AXIOM_EBADF        9
#define AXIOM_ECHILD       10
#define AXIOM_ENOMEM      12
#define AXIOM_EACCES      13
#define AXIOM_EFAULT      14
#define AXIOM_EBUSY       16
#define AXIOM_EEXIST      17
#define AXIOM_ENODEV      19
#define AXIOM_ENOTDIR     20
#define AXIOM_EISDIR      21
#define AXIOM_EINVAL      22
#define AXIOM_EMFILE      24
#define AXIOM_ENOSPC      28
#define AXIOM_ESPIPE      29
#define AXIOM_EROFS       30
#define AXIOM_ENAMETOOLONG 36
#define AXIOM_ENOSYS      38
#define AXIOM_EMSGSIZE     90
#define AXIOM_ENETDOWN    100
#define AXIOM_ECONNREFUSED 111
#define AXIOM_ETIMEDOUT   110
#define AXIOM_EHOSTUNREACH 113

#endif
