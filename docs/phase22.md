# Phase 22 — Inter-process communication

## Goal

Add useful kernel-mediated communication between isolated Ring-3 processes
without weakening the Phase-20 user/kernel boundary.

Phase 22 implements three IPC mechanisms from the original roadmap:

1. byte-stream pipes;
2. page-backed shared memory;
3. bounded message queues.

The implementation intentionally remains small enough to audit. Normal task
execution is still BSP-only, so IPC syscalls execute with the syscall entry
interrupt mask in effect and do not yet require cross-CPU object locking.

## Pipes

`pipe_create` creates a kernel byte-stream object with a 1024-byte circular
buffer. `pipe_write` and `pipe_read` transfer up to 512 bytes per syscall.
The current ABI uses an integer IPC object ID rather than a VFS file descriptor.
Empty reads and full writes are non-blocking and report `-EBUSY`.

`/bin/ipcdemo` creates a pipe, calls `fork()`, lets the child write a message,
waits for the child, and reads the exact bytes in the parent. This proves that
separate process images can communicate through a kernel-owned stream object.

## Shared memory

`shm_attach(key)` creates or finds a keyed one-page shared object and maps the
same physical page into the caller at a reserved lower-half IPC address.
Mappings are:

- user accessible;
- writable;
- NX;
- 4 KiB;
- separate from the Phase-19 anonymous `mmap` range.

Up to eight shared-memory mappings are allowed per process. Each attachment is
tracked by task ID, address-space root, object, and virtual address.

`fork()` inherits shared mappings by mapping the same physical IPC pages into
the child at the same virtual addresses. `exec()` and process teardown detach
old shared mappings before destroying the replaced address space. The last
attachment releases the physical page.

## Message queues

`msgq_open(key)` creates or opens a keyed kernel queue. Each queue holds up to
eight messages, each at most 64 bytes. `msgq_send` preserves message boundaries;
`msgq_receive` removes exactly one complete message. Empty/full conditions are
non-blocking and return `-EBUSY`; too-small receive buffers return `-EMSGSIZE`.

## Cross-program demonstration

Phase 22 installs two executables:

```text
/bin/ipcdemo
/bin/ipc-peer
```

`ipcdemo` attaches keyed shared memory, writes `parent->peer`, opens a keyed
message queue, and spawns `ipc-peer`. The independent peer executable attaches
the same shared page, verifies the parent's bytes, writes `peer->parent`, and
sends `queue->parent` through the message queue. The parent waits, verifies both
channels, and prints the Phase-22 success markers.

This deliberately demonstrates communication between two different Ring-3 ELF
programs, not just two kernel threads.

## ABI

Phase 22 adds syscall numbers 34–44:

```text
34 pipe_create
35 pipe_write
36 pipe_read
37 pipe_close
38 shm_attach
39 shm_detach
40 msgq_open
41 msgq_send
42 msgq_recv
43 msgq_close
44 ipcstats
```

`libaxiom.a` exposes matching `axiom_*` wrappers. All pointer-bearing IPC
syscalls validate Ring-3 ranges with the VMM and copy data across the boundary;
raw userspace pointers are never dereferenced by the kernel.

## Observability

`ipcstats` reports cumulative counters for object creation, pipe bytes,
shared-memory attachments, and message send/receive operations.

## Deliberate limits

- pipes and message queues are non-blocking; there is no `poll`, `select`, or
  wait-queue integration yet;
- pipe IDs are IPC object IDs, not VFS file descriptors;
- shared objects are exactly one page and keyed by a 64-bit value;
- queues are bounded to eight 64-byte messages;
- IPC object IDs/keys currently have no per-user namespaces, ACLs, or ownership
  policy;
- there is no zero-copy pipe/message transport;
- normal process execution remains BSP-only, so cross-CPU IPC locking is left
  for a future full SMP scheduler.

These are intentional Phase-22 boundaries rather than claims of POSIX-complete
IPC semantics.

## Acceptance

```bash
make clean
make
make test-phase22
make test
```

At the shell, manual validation is:

```text
axiom> ipcdemo
axiom> ipcstats
```

Expected userspace markers include:

```text
Phase 22 pipe process communication: OK
Phase 22 peer cross-program IPC: OK
Phase 22 shared memory communication: OK
Phase 22 message queue communication: OK
Phase 22 two-program IPC demonstration: OK
Phase 22 IPC statistics: OK
Phase 22 IPC userspace demo complete.
```
