#ifndef AXIOM_FILESYSTEM_PROCFS_H
#define AXIOM_FILESYSTEM_PROCFS_H

#include <axiom/filesystem/vfs.h>

/* Phase 25 read-only live observability filesystem mounted at /proc. */
struct vfs_filesystem *procfs_create(void);
int procfs_selftest(void);

#endif
