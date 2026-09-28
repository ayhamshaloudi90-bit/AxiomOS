# Phase 12 — Virtual filesystem and RAM filesystem

## Goal

Introduce a real filesystem abstraction without depending on a disk driver yet.
AxiomOS now has a VFS layer, path lookup, mount points, per-process file
descriptors, file metadata, and a writable in-memory filesystem.

The actual block-device-backed filesystem is intentionally deferred until Phase
13, because a filesystem cannot read persistent disk sectors before a block
driver exists. Phase 13 can attach a FAT-like or other on-disk filesystem to the
same VFS API instead of replacing the VFS.

## Architecture

```text
Ring-3 program
    |
    | open/read/write/close/lseek/stat/exec
    v
SYSCALL layer
    |
    v
per-process descriptor table
    |
    v
VFS
    |
    +-- path normalization
    +-- longest-prefix mount selection
    +-- vnode operations
    +-- file offsets/open flags
    |
    +-----------------------+
    |                       |
    v                       v
rootfs ramfs mounted /      tmpfs ramfs mounted /tmp
```

The VFS exposes generic nodes and node operations so future filesystems can
implement lookup/create/read/write/truncate/readdir without changing the
syscall layer.

## Paths and mount points

Phase 12 creates this initial hierarchy:

```text
/
├── bin/
│   ├── phase11-launcher
│   ├── phase11-demo
│   └── phase12-demo
├── etc/
│   └── motd
├── dev/
├── home/
│   └── readme.txt
└── tmp/        <- second ramfs mounted here
```

The executable files are initially seeded from Limine modules into rootfs.
That is a temporary boot-time source only. Once a disk driver/filesystem exists,
`/bin/*` can come from persistent storage while `SYS_exec` keeps using ordinary
VFS reads.

Paths are absolute and normalized. Repeated slashes, `.` and `..` are handled,
and mount resolution chooses the longest matching mount prefix.

## Per-process file descriptors

Each task owns a fixed 16-entry descriptor table.

- `0` is stdin;
- `1` is stdout;
- `2` is stderr;
- ordinary VFS files begin at descriptor `3`.

Descriptors keep an independent current file offset and open flags. They are
closed automatically when a task exits or is terminated by a userspace fault.
They survive `exec()` for now because close-on-exec flags are not implemented.

## System calls implemented in this phase

The previously reserved calls are now wired to the VFS:

- `open(path, flags)`;
- `read(fd, buffer, count)` for regular files as well as existing stdin;
- `write(fd, buffer, count)` for regular files as well as stdout/stderr;
- `close(fd)`;
- `lseek(fd, offset, whence)`;
- `stat(path, struct axiom_stat *)`.

`fork()` and `mmap()` remain `-ENOSYS` because their owning phases are still in
the future.

All userspace path/buffer/stat pointers are copied through the existing VMM
user-copy validation instead of being dereferenced directly in Ring 0.

## `exec()` now uses the VFS

Phase 11 used a small syscall-local registry that translated
`/bin/phase11-demo` directly to a Limine module. Phase 12 removes that special
lookup.

`SYS_exec` now does:

```text
copy path from Ring 3
        |
        v
vfs_read_all(path)
        |
        v
kernel heap buffer containing ELF file
        |
        v
scheduler_exec_current_elf()
        |
        v
fresh CR3 + ELF segments + user stack
```

The ELF loader itself did not need to be redesigned.

## Phase-12 userspace test

`phase12_demo.elf` is loaded from `/bin/phase12-demo` through the VFS. It:

1. opens `/etc/motd` read-only;
2. calls `stat()` on it;
3. reads and prints its contents;
4. creates `/tmp/phase12.txt` on the second mounted ramfs;
5. writes `RAM filesystem round-trip works!`;
6. seeks back to offset zero;
7. reads the bytes back and prints them;
8. stats and closes the file;
9. calls `exec("/bin/phase11-demo")`;
10. the Phase-11 ELF runs under the same PID and exits normally.

The kernel additionally reopens `/tmp/phase12.txt` after the process finishes
and byte-compares its contents.

## Current limitations

- RAM filesystem contents disappear on reboot.
- No disk-backed filesystem yet; that depends on Phase 13.
- No unlink/rename/mkdir syscalls yet.
- No permissions/users/groups enforcement yet; mode bits are metadata only.
- No file locking or VFS synchronization yet; AxiomOS is still single-CPU and
  synchronization is a later phase.
- Descriptor table size is fixed at 16 per task.
- Relative paths/current working directories are not exposed to userspace yet.

## Acceptance

```bash
make clean
make
make test-phase12
make test
```

The dedicated test verifies the standalone Phase-12 ELF, mount count, path
lookup, descriptor allocation, RAM-file write/read/seek/stat, and VFS-backed
`exec()` before the old interactive keyboard prompt is reached.
