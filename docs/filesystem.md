# AxiomOS filesystem architecture

Phase 12 introduced the virtual filesystem layer; Phase 13 adds a persistent
backend without changing the VFS or userspace file ABI.

## VFS

The VFS lives in `filesystem/vfs.c` and exposes generic filesystem/node/file
objects. Filesystem backends supply vnode operations for lookup, creation,
reading, writing, truncation, and directory enumeration.

Current generic operations include:

```text
vfs_mount
vfs_lookup
vfs_mkdir
vfs_readdir
vfs_open
vfs_read
vfs_write
vfs_close
vfs_seek
vfs_stat
vfs_read_all
vfs_write_file
```

Paths are normalized and mount selection uses the longest matching prefix.

## Current mounts

```text
/       -> root RAMFS
/tmp    -> secondary RAMFS
/disk   -> persistent diskfs when an AHCI disk is attached
```

`/bin`, `/etc`, `/dev`, `/home`, `/tmp`, and the `/disk` mountpoint live in the
RAM root filesystem. Boot ELF modules are still copied into `/bin`; persistent
user data can now live under `/disk`.

## RAM filesystem

`filesystem/ramfs.c` stores directory nodes, metadata, and file contents in the
kernel heap. RAMFS contents disappear on reboot.

## Persistent diskfs

`filesystem/diskfs.c` is a small Phase-13 backend built on the generic block
API. It uses a one-sector superblock, eight sectors of fixed directory entries,
and contiguous append-only data extents. The filesystem is deliberately simple
so the storage-driver path remains understandable.

The persistent path is:

```text
open/read/write/lseek/stat
        -> VFS
        -> diskfs
        -> block API
        -> AHCI DMA
        -> QEMU raw disk image
```

## File descriptors

Every task owns 16 descriptor slots. Slots 0, 1 and 2 retain stdin/stdout/
stderr semantics; VFS opens allocate from descriptor 3 upward. Descriptors hold
vnode, current offset, and open flags. Task exit/fault cleanup closes all
remaining regular file descriptors.

## Next step

Phase 14 adds the shell, allowing the human user to interact with these existing
file and executable APIs through commands rather than only automated Ring-3
demos.

## Phase 14 shell integration

The VFS is now directly usable from `/bin/axiomsh`. Directory enumeration is
exposed through `SYS_readdir`, and `mkdir`, `open`, `read`, `write`, `lseek`,
`stat`, and executable lookup all travel through the same VFS layer. The shell
can therefore use RAMFS (`/`, `/tmp`) and persistent diskfs (`/disk`) without
backend-specific code.


## Phase 25 procfs

AxiomOS mounts a third virtual backend at `/proc`. Unlike RAMFS and diskfs,
procfs is read-only and generates file contents from live kernel state at read
time. It supports normal VFS lookup, readdir, open, and read operations, so
existing userspace tools can inspect it without a new syscall family.

Files: `meminfo`, `processes`, `interrupts`, `files`, `net`, `scheduler`,
`cpuinfo`, and `pagemap`.
