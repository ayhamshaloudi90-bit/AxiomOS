#ifndef AXIOM_LIBC_FCNTL_H
#define AXIOM_LIBC_FCNTL_H

#include <axiom/abi/fs.h>

#define O_RDONLY AXIOM_O_RDONLY
#define O_WRONLY AXIOM_O_WRONLY
#define O_RDWR   AXIOM_O_RDWR
#define O_CREAT  AXIOM_O_CREAT
#define O_TRUNC  AXIOM_O_TRUNC
#define O_APPEND AXIOM_O_APPEND

int open(const char *path, int flags);

#endif
