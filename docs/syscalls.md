# AxiomOS syscall ABI

Phase 10 introduced the x86-64 `SYSCALL` / `SYSRETQ` boundary. Arguments use
`RDI, RSI, RDX, R10, R8, R9`; `RAX` contains the syscall number on entry and the
signed result on return.

Implemented calls now include:

```text
write
read
_exit
sleep
getpid
yield
open
close
exec
lseek
stat
```

`mmap` retains syscall number 11 and is implemented by Phase 19 as a small anonymous userspace mapping primitive used by `libaxiom` heap growth. `fork` is implemented by Phase 15.

All pointer-bearing calls validate Ring-3 ranges through the VMM before copying
bytes. Bad user pointers return `-EFAULT`; the kernel never trusts a raw user
pointer.

## File syscalls in Phase 12

`open()` resolves an absolute path through the VFS and installs a
`struct vfs_file` into the current task's descriptor table. Regular descriptors
start at 3 because 0/1/2 remain stdin/stdout/stderr.

`read()` and `write()` dispatch either to keyboard/terminal semantics for the
standard descriptors or to VFS file operations for regular descriptors.

`lseek()` changes a descriptor's independent file offset. `stat()` copies a
small `struct axiom_stat` containing size/type/mode back into validated user
memory. `close()` releases the open-file object and descriptor slot.

## `exec`

`SYS_exec` now copies the path from userspace, reads the target ELF bytes using
`vfs_read_all()`, and passes that kernel buffer into the existing Phase-11 ELF
image-replacement path. The temporary hardcoded boot-module executable registry
is gone.

## Phase 14 additions

The interactive Ring-3 shell extends the ABI without changing the x86-64
`SYSCALL/SYSRET` entry mechanism:

| Number | Call | Purpose |
|---:|---|---|
| 14 | `readdir` | Enumerate a VFS directory by index |
| 15 | `mkdir` | Create a directory |
| 16 | `spawn` | Load a VFS ELF into a new Ring-3 task |
| 17 | `waitpid` | Wait for a spawned child and collect exit status |
| 18 | `clear` | Clear the framebuffer terminal |
| 19 | `kbdstats` | Read PS/2 keyboard diagnostic counters |

`spawn()` is deliberately smaller than POSIX `fork()+exec()`: Phase 14 needs a
safe foreground command-launch mechanism, while full process semantics remain
Phase 15 work.

## Phase 15 process calls

| Number | Call | Purpose |
|---:|---|---|
| 9 | `fork` | Eager-copy the current Ring-3 process |
| 20 | `procinfo` | Copy one process-table entry to userspace |
| 21 | `kill` | Send the simplified SIGTERM to a Ring-3 process |
| 22 | `getppid` | Return the current process parent PID |

`waitpid` now blocks the parent in `TASK_BLOCKED` instead of polling with
`yield()`. A terminated child keeps its exit status until the parent reaps it.
Open file descriptions use reference counting so descriptors inherited across
`fork()` share the same VFS offset.

## Phase 18 networking calls

Phase 18 exposes a deliberately small high-level networking ABI while the
in-kernel Ethernet/IP stack is still young. These calls are synchronous and do
not claim to be a POSIX sockets interface.

| Number | Call | Purpose |
|---:|---|---|
| 23 | `netinfo` | Copy the configured MAC/IPv4/gateway/DNS data and counters |
| 24 | `ping` | Resolve an IPv4/host target and perform one ICMP echo exchange |
| 25 | `dns` | Resolve one hostname to an IPv4 A record using the configured DNS server |
| 26 | `httpget` | Perform one HTTP/1.0 GET over the Phase-18 TCP client |

`httpget` accepts host/path strings and copies at most
`AXIOM_NET_HTTP_USER_MAX` bytes of response body into validated Ring-3 memory.
The result structure reports peer IPv4, port, HTTP status, copied byte count,
and truncation. HTTPS/TLS is not implemented in Phase 18.

The ABI still uses the normal AxiomOS syscall register convention. The fifth
`httpget` argument therefore arrives in `R8` after `RDI`, `RSI`, `RDX`, and
`R10`.


## Phase 19 anonymous mmap

| Number | Call | Purpose |
|---:|---|---|
| 11 | `mmap` | Allocate zero-filled anonymous writable/NX pages for the current Ring-3 process |

The Phase-19 ABI intentionally keeps this call smaller than POSIX `mmap(2)`: userspace passes only a byte length in `RDI`. The kernel rounds to 4 KiB pages and returns a page-aligned virtual base or a negative errno. Mappings are process-private, are eagerly copied by `fork()`, and are discarded by `exec()` or process teardown. There is no `munmap`, file-backed mapping, fixed-address mapping, or protection-changing API yet. `malloc()` uses this call internally and recycles freed blocks in userspace.


## Phase 20 credentials

| Number | Call | Purpose |
|---:|---|---|
| 27 | `getuid` | Return the current task's UID |
| 28 | `getgid` | Return the current task's GID |

## Phase 21 scheduler calls

| Number | Call | Purpose |
|---:|---|---|
| 29 | `getpriority` | Return the current process base priority |
| 30 | `setpriority` | Set current-process priority from 0 through 7 |
| 31 | `getaffinity` | Return the current process CPU-affinity mask |
| 32 | `setaffinity` | Set current affinity; BSP bit 0 must remain enabled |
| 33 | `schedstats` | Copy `struct axiom_scheduler_stats` to validated user memory |

Phase 21 intentionally restricts priority/affinity mutation to the calling
process. It does not add a privileged API for rewriting another process's
scheduling parameters. Fork inherits both values. Since general scheduling is
still BSP-only, `setaffinity` rejects masks that exclude logical CPU 0.

## Phase 22 IPC calls

| Number | Call | Purpose |
|---:|---|---|
| 34 | `pipe_create` | Create a 1024-byte kernel byte-stream pipe and return its IPC ID |
| 35 | `pipe_write` | Copy up to 512 validated user bytes into a pipe |
| 36 | `pipe_read` | Copy up to 512 pipe bytes into validated user memory |
| 37 | `pipe_close` | Destroy a pipe IPC object |
| 38 | `shm_attach` | Create/find a keyed 4 KiB shared page and map it user/RW/NX |
| 39 | `shm_detach` | Remove one current-process shared-memory mapping |
| 40 | `msgq_open` | Create/open a keyed bounded message queue |
| 41 | `msgq_send` | Enqueue one message of at most 64 bytes |
| 42 | `msgq_recv` | Dequeue one complete message into validated user memory |
| 43 | `msgq_close` | Drop one queue open reference |
| 44 | `ipcstats` | Copy Phase-22 IPC counters to validated user memory |

Pipe and message-queue operations are deliberately non-blocking in Phase 22.
An empty pipe/queue or full pipe/queue reports `-EBUSY`; oversized messages
report `-EMSGSIZE`. Shared-memory pages are mapped NX and retain ordinary Ring-3
page-table protections. Fork inherits shared pages as true shared mappings;
exec and process teardown detach them.

## Phase 23 graphics calls

| Number | Call | Purpose |
|---:|---|---|
| 45 | `gfx_info` | Copy framebuffer dimensions/pitch/bpp/font information to validated user memory |
| 46 | `gfx_clear` | Fill the framebuffer with one RGB color |
| 47 | `gfx_pixel` | Draw one clipped RGB pixel |
| 48 | `gfx_line` | Draw an in-bounds integer Bresenham line |
| 49 | `gfx_rect` | Draw a bounded filled rectangle |
| 50 | `gfx_bitmap` | Copy a validated RGB32 bitmap of at most 16,384 pixels and blit it |
| 51 | `gfx_text` | Copy at most 128 user characters and render them with the boot bitmap font |
| 52 | `gfx_stats` | Copy cumulative graphics counters to validated user memory |

The framebuffer itself is not mapped into Ring 3. This is deliberate: userspace
submits bounded drawing operations and the kernel retains control of the display
mapping and pointer validation.


## Phase 24 HTTP server call

| Number | Call | Purpose |
|---:|---|---|
| 53 | `httpserve` | Listen on one TCP port, serve one bounded HTTP/1.0 response, and return peer/request statistics |

`httpserve` is intentionally a high-level synchronous syscall rather than a
BSD sockets API. The kernel validates and copies at most 1024 response-body
bytes from Ring 3, waits for one passive-open TCP connection, returns HTTP 200,
then closes the connection and returns. This matches the current single-connection
Phase-18/24 TCP architecture.


## Phase 25 developer tooling

Phase 25 adds no new syscall numbers. Developer telemetry is intentionally
exposed as read-only VFS files under `/proc`, consumed with the existing
`open`, `read`, `readdir`, `stat`, and `close` calls. `/bin/sysinfo` is therefore
a normal Ring-3 program rather than a privileged debugger interface.


## Phase 26 desktop/input call

| Number | Call | Purpose |
|---:|---|---|
| 54 | `mousestate` | Copy cumulative PS/2 mouse motion/button/availability state to validated user memory |
| 55 | `gfx_cursor` | Move/show/hide the kernel save-under software cursor without repainting the desktop |

`mousestate` is intentionally read-only. Userspace receives cumulative relative
motion counts, button bits, and driver counters but no controller I/O access.
The desktop converts the motion deltas into framebuffer coordinates and clamps
the cursor to the current graphics mode. `gfx_cursor` is deliberately tiny: the
kernel restores the previous 12x19 pixel patch, saves the new patch, and draws
the cursor over it. Ordinary mouse movement therefore does not require a full
framebuffer repaint. If the mouse is unavailable, the desktop uses its keyboard
shortcuts without changing the rest of the GUI workflow.
