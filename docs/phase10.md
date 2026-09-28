# Phase 10 — x86-64 system calls

## Goal

Give Ring-3 programs a controlled way to request kernel services without being
allowed to call kernel functions or touch kernel memory directly.

AxiomOS Phase 10 uses the architectural 64-bit `SYSCALL` / `SYSRETQ` mechanism.
It does not use a DPL-3 `int 0x80` compatibility shortcut.

## Entry path

At boot, `syscall_init()` programs:

- `IA32_EFER.SCE` to enable SYSCALL/SYSRET;
- `IA32_STAR` with AxiomOS kernel/user GDT selector relationships;
- `IA32_LSTAR` with `syscall_entry`;
- `IA32_FMASK` so TF, IF, and DF are cleared on entry.

Unlike an interrupt gate, `SYSCALL` does not switch stacks. AxiomOS therefore
maintains the currently scheduled task's trusted kernel-stack top in
`syscall_kernel_stack_top`. The assembly entry stub moves off the user stack
before calling C.

```text
Ring 3
  |
  | syscall
  v
CPU -> CPL0, RCX=user RIP, R11=user RFLAGS
  |
  v
switch RSP to current task's kernel stack
  |
  v
save syscall frame
  |
  v
syscall_dispatch()
  |
  v
restore registers and user RSP
  |
  | sysretq
  v
Ring 3
```

The project is still single-CPU, so the tiny pre-stack-switch scratch slot is a
single-CPU implementation detail. SMP will require per-CPU syscall entry state.

## ABI

Phase 10 follows a Linux-like x86-64 register convention:

```text
RAX  syscall number
RDI  arg0
RSI  arg1
RDX  arg2
R10  arg3
R8   arg4
R9   arg5
RAX  return value
```

`RCX` and `R11` are clobbered by the CPU's SYSCALL/SYSRET mechanism.

Implemented calls:

| Number | Call | Phase-10 behavior |
|---:|---|---|
| 1 | `write(fd, buf, len)` | fd 1/2 -> kernel terminal |
| 2 | `read(fd, buf, len)` | fd 0 -> non-blocking keyboard buffer |
| 3 | `_exit(status)` | terminate current task |
| 4 | `sleep(ms)` | scheduler-backed sleeping state |
| 5 | `getpid()` | current task ID |
| 6 | `yield()` | force one round-robin reschedule |
| 7 | `open(path, flags)` | reserved; `-ENOSYS` until VFS |
| 8 | `close(fd)` | reserved; `-ENOSYS` until VFS |
| 9 | `fork()` | reserved; `-ENOSYS` |
| 10 | `exec()` | reserved; `-ENOSYS` |
| 11 | `mmap()` | reserved; `-ENOSYS` |

Errors are returned as negative integers (`-EBADF`, `-EFAULT`, `-EINVAL`,
`-ENOSYS`).

## User-pointer safety

The syscall layer never trusts a Ring-3 pointer. Phase 10 adds VMM helpers that
walk the task's page tables and require every page in a requested range to be:

- lower-half/canonical;
- present;
- user accessible;
- writable when the kernel intends to copy data into userspace.

After validation, bytes are copied through the HHDM using the translated
physical pages. The kernel does not simply dereference an arbitrary user
virtual address while running in Ring 0.

The demo intentionally calls `write()` with `0xFFFFFFFF80000000`. The syscall
must return `-EFAULT` instead of generating a kernel-mode page fault.

## Scheduler integration

Phase 10 turns `TASK_SLEEPING` into an active scheduler state. `sleep(ms)` marks
the current task sleeping until a future scheduling tick, then the timer wakes
it back to READY. `yield()` expires the task's quantum and waits until the
round-robin scheduler has switched away and eventually back.

A task inside a syscall can therefore be preempted safely on its own kernel
stack and later resume the syscall before `SYSRETQ` returns to Ring 3.

## Userspace wrapper shim

`userspace/phase10_program.S` contains the first libc-style wrappers:

```text
write()
read()
_exit()
sleep()
getpid()
yield()
open()
close()
```

`libc/include/axiom/unistd.h` records their C-facing signatures. This is not yet
the full userspace libc planned for Phase 19; it establishes the ABI that that
library will later wrap normally.

## Phase-10 demo

The Ring-3 demo:

1. calls `getpid()`;
2. prints `Hello via SYS_write from Ring 3!`;
3. passes a kernel pointer to `write()` and expects `-EFAULT`;
4. calls `open()` and expects `-ENOSYS` because no VFS exists yet;
5. calls `yield()`;
6. sleeps for 30 ms;
7. polls `read()` for a bounded period;
8. if a key arrives, prints `Phase 10 userspace read: <key>`;
9. exits with status 37.

The read wait is deliberately short and bounded so older regression tests can still boot the newest
kernel without requiring manual input. `make test-phase10` injects a real QEMU
keyboard event and requires the user task to read it through `SYS_read`.

## Acceptance

Run:

```bash
make clean
make
make test-phase10
make test
```

Phase 10 is accepted only after the dedicated syscall test and the full Phase
1-10 regression suite pass under QEMU.
