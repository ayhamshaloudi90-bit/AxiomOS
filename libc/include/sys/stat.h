#ifndef AXIOM_LIBC_SYS_STAT_H
#define AXIOM_LIBC_SYS_STAT_H

#include <axiom/abi/fs.h>

int stat(const char *path, struct axiom_stat *status);

#endif
