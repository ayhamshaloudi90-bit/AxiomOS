#ifndef AXIOM_FILESYSTEM_RAMFS_H
#define AXIOM_FILESYSTEM_RAMFS_H

#include <axiom/filesystem/vfs.h>

/* Allocate an empty writable in-memory filesystem. */
struct vfs_filesystem *ramfs_create(const char *name);

#endif
