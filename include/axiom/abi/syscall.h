#ifndef AXIOM_ABI_SYSCALL_H
#define AXIOM_ABI_SYSCALL_H

#include <axiom/abi/errno.h>
#include <axiom/abi/fs.h>
#include <axiom/abi/graphics.h>
#include <axiom/abi/input.h>
#include <axiom/abi/ipc.h>
#include <axiom/abi/process.h>
#include <axiom/abi/network.h>
#include <axiom/abi/scheduler.h>

/* Stable syscall numbers shared by kernel and userspace. */
#define AXIOM_SYS_WRITE      1
#define AXIOM_SYS_READ       2
#define AXIOM_SYS_EXIT       3
#define AXIOM_SYS_SLEEP      4
#define AXIOM_SYS_GETPID     5
#define AXIOM_SYS_YIELD      6
#define AXIOM_SYS_OPEN       7
#define AXIOM_SYS_CLOSE      8
#define AXIOM_SYS_FORK       9
#define AXIOM_SYS_EXEC      10
#define AXIOM_SYS_MMAP      11
#define AXIOM_SYS_LSEEK     12
#define AXIOM_SYS_STAT      13
#define AXIOM_SYS_READDIR   14
#define AXIOM_SYS_MKDIR     15
#define AXIOM_SYS_SPAWN     16
#define AXIOM_SYS_WAITPID   17
#define AXIOM_SYS_CLEAR     18
#define AXIOM_SYS_KBDSTATS  19
#define AXIOM_SYS_PROCINFO  20
#define AXIOM_SYS_KILL      21
#define AXIOM_SYS_GETPPID   22
#define AXIOM_SYS_NETINFO   23
#define AXIOM_SYS_PING      24
#define AXIOM_SYS_DNS       25
#define AXIOM_SYS_HTTPGET   26
#define AXIOM_SYS_GETUID    27
#define AXIOM_SYS_GETGID    28
#define AXIOM_SYS_GETPRIORITY 29
#define AXIOM_SYS_SETPRIORITY 30
#define AXIOM_SYS_GETAFFINITY 31
#define AXIOM_SYS_SETAFFINITY 32
#define AXIOM_SYS_SCHEDSTATS  33
#define AXIOM_SYS_PIPE_CREATE 34
#define AXIOM_SYS_PIPE_WRITE  35
#define AXIOM_SYS_PIPE_READ   36
#define AXIOM_SYS_PIPE_CLOSE  37
#define AXIOM_SYS_SHM_ATTACH  38
#define AXIOM_SYS_SHM_DETACH  39
#define AXIOM_SYS_MSGQ_OPEN   40
#define AXIOM_SYS_MSGQ_SEND   41
#define AXIOM_SYS_MSGQ_RECV   42
#define AXIOM_SYS_MSGQ_CLOSE  43
#define AXIOM_SYS_IPCSTATS    44
#define AXIOM_SYS_GFX_INFO    45
#define AXIOM_SYS_GFX_CLEAR   46
#define AXIOM_SYS_GFX_PIXEL   47
#define AXIOM_SYS_GFX_LINE    48
#define AXIOM_SYS_GFX_RECT    49
#define AXIOM_SYS_GFX_BITMAP  50
#define AXIOM_SYS_GFX_TEXT    51
#define AXIOM_SYS_GFX_STATS   52
#define AXIOM_SYS_HTTPSERVE    53
#define AXIOM_SYS_MOUSESTATE    54
#define AXIOM_SYS_GFX_CURSOR    55

#define AXIOM_SYSCALL_MAX_NUMBER AXIOM_SYS_GFX_CURSOR

#endif
