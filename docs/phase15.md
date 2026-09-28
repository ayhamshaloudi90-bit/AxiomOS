# Phase 15 — Process management

Phase 15 turns the Phase-14 foreground `spawn()` mechanism into a fuller
process lifecycle. AxiomOS now supports parent/child relationships, eager-copy
`fork()`, `exec()`, blocking `waitpid()`, exit-status collection, process-table
inspection, sleeping/blocking scheduler states, and a deliberately small
SIGTERM-style termination mechanism.

## Process lifecycle

Every task already has a unique PID. User processes now additionally use:

- `parent_id` — PID of the creating process;
- `TASK_BLOCKED` — used while a parent waits for a child;
- `TASK_SLEEPING` — used by `sleep()` until an APIC-timer deadline;
- `TASK_TERMINATED` — also acts as the zombie state for an unreaped child;
- `wait_target_id` — child a blocked parent is waiting for;
- `wait_collected` — whether the parent collected the child's exit status;
- `termination_signal` — nonzero when the simplified signal path killed it.

A terminated child whose parent is still alive is not immediately reclaimed.
`waitpid()` reads its exit status and marks it reaped; only then may a future
process creation reuse the task slot and release the old address-space pages,
stack, and other resources. Parentless historical test tasks remain immediately
reclaimable so the incremental boot self-tests do not exhaust the table.

The Phase-15 process table has 16 slots. It remains intentionally fixed-size;
dynamic process-table allocation can be introduced later without changing the
userspace ABI.

## `fork()`

AxiomOS implements an eager-copy fork rather than copy-on-write.

When a Ring-3 process calls `fork()`:

1. the scheduler reserves a task slot and private kernel stack;
2. it creates a fresh user CR3/PML4;
3. every mapped ELF/code/data page is copied to a new physical page;
4. every user-stack page is copied to a new physical page;
5. the child's mappings preserve the parent's user/write/NX permissions;
6. open VFS file descriptions are retained and shared, so file offsets are
   shared across parent and child;
7. a synthetic interrupt frame is built from the parent's syscall return state;
8. the parent receives the child PID while the child resumes after the same
   `fork()` syscall with RAX=0.

This uses more RAM than copy-on-write, but makes ownership and teardown simple
and gives a clean baseline before introducing COW page-fault policy later.

## `exec()`

The existing ELF64 `exec()` path remains process-image replacement: it creates a
fresh address space, loads PT_LOAD segments through the Phase-11 ELF loader,
installs a new user stack, switches CR3, frees the previous image, and returns
to the new ELF entry point. PID and `parent_id` stay unchanged.

## Blocking `waitpid()`

Phase 14 implemented foreground waiting by repeatedly yielding. Phase 15 uses a
real blocked state:

```text
parent calls waitpid(child)
        ↓
parent -> TASK_BLOCKED
        ↓
scheduler runs other READY tasks
        ↓
child exits / faults / is killed
        ↓
waiting parent -> TASK_READY
        ↓
parent resumes inside waitpid()
        ↓
exit status copied to userspace
        ↓
child marked reaped
```

This is still a single-CPU scheduler, so state transitions are protected by the
same interrupt-disabled critical sections used by the existing scheduler.

## Simplified signals

Phase 15 adds only `SIGTERM` (`15`). There are no user signal handlers, masks,
pending-signal queues, or asynchronous delivery frames yet.

`kill(pid, SIGTERM)` immediately terminates a Ring-3 target, records exit status
`128 + 15 = 143`, closes its descriptors, and wakes a parent blocked in
`waitpid()`. Kernel tasks cannot be killed from userspace.

## Process-table ABI

`procinfo(index, &info)` exposes one scheduler slot at a time through a validated
Ring-3 buffer. `struct axiom_process_info` contains PID, PPID, state, privilege,
runtime ticks, exit status, terminating signal, and a short process name.

The Ring-3 shell uses it for:

```text
axiom> ps
PID  PPID  STATE       PRIV  TICKS  NAME
...
```

The shell also gains:

```text
spawn <program>   # background launch
wait <pid>        # collect one child
kill <pid>        # send SIGTERM
ps                # inspect process table
```

Normal external commands still run in the foreground through `spawn()+waitpid()`.

## Phase-15 demo

`/bin/phase15-demo` verifies:

- parent receives a positive PID from `fork()`;
- child receives zero;
- child's `getppid()` matches the parent;
- a forked child can `exec("/bin/phase11-demo")`;
- parent blocks and collects exit status 11;
- a second child sleeps and exits with status 42;
- the parent blocks until wakeup;
- a child write to a global variable does not modify the parent's copy.

`/bin/phase15-sleeper` exists for the interactive `ps`/`kill` test.

## Acceptance

```bash
make clean
make
make test-phase15
make test
```

The dedicated QEMU test drives the Ring-3 shell through PS/2 input, executes the
fork demo, checks `ps`, launches a sleeper in the background, confirms it is
SLEEPING, terminates it with SIGTERM, waits for status 143, and launches another
ELF afterward to prove the reaped slot is reusable.

## Deliberate limits

- eager copying instead of copy-on-write fork;
- fixed 16-slot process table;
- only SIGTERM, with immediate termination and no handlers;
- no `argv`/environment yet;
- no pipes, redirection, job-control process groups, sessions, or terminals;
- no SMP synchronization yet;
- scheduler policy remains round-robin.
