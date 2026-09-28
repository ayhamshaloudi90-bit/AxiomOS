# Tasks, scheduling, and process management — Phase 21

AxiomOS uses the BSP local APIC's 100 Hz timer for preemptive scheduling. The
default quantum remains five ticks (about 50 ms). Phase 21 keeps round-robin as
the historical Phase-8 boot policy, then switches the live system to priority
scheduling with aging after Phase 20 initializes.

Each `struct task` records PID/PPID, RUNNING/READY/BLOCKED/SLEEPING/TERMINATED
state, Ring-0/Ring-3 privilege, saved CPU context, private kernel stack, CR3,
user image/stack physical pages, open files, fault/exit information, wait state,
runtime counters, Phase-21 base/effective priority, READY wait time, and a
64-bit CPU-affinity mask.

Kernel threads share the kernel CR3. Ring-3 tasks own separate lower-half
address spaces while sharing supervisor-only higher-half kernel mappings.

## Process creation

`spawn(path)` creates a fresh ELF-backed Ring-3 task from a VFS executable.
`fork()` instead clones the currently running userspace process. Phase 15 uses
an eager copy: user image pages and stack pages receive new physical frames and
identical mappings. The child's CPU return state is synthesized so the parent
sees the child PID and the child sees zero from the same syscall.

Open VFS file descriptions are reference-counted and shared across fork, so the
file offset is shared until descriptors are closed.

## Exec and exit

`exec(path)` replaces the current process image while preserving PID and PPID.
`exit(status)` closes descriptors, records status, changes the task to
TERMINATED, and wakes a parent waiting for that PID.

A terminated child with a live parent is retained as an unreaped zombie until
`waitpid()` collects its status. Reaped or orphaned terminated tasks can be
reclaimed by later task creation.

## Waiting, sleeping, and blocking

`sleep(ms)` places a task in SLEEPING until the APIC scheduling tick reaches its
wake deadline. `waitpid()` places a parent in BLOCKED with a target child PID.
The scheduler excludes BLOCKED and SLEEPING tasks from READY selection. Child
termination promotes the blocked parent back to READY.

This replaces Phase 14's yield-polling wait loop with a real scheduler state
transition.

## Simplified signal termination

`kill(pid, SIGTERM)` immediately terminates a Ring-3 target with status 143 and
wakes any waiting parent. Kernel tasks are protected. This is intentionally only
a first signal mechanism: no handler registration, masks, pending queues, or
signal-return frames exist yet.

## Process inspection

`procinfo(index, &info)` exposes scheduler process data to Ring 3. `/bin/axiomsh`
uses it for `ps` and also provides background `spawn`, `wait`, and `kill`
commands.

## Phase 16 generic wait queues

Synchronization now reuses the scheduler's BLOCKED state outside `waitpid()`.
A kernel primitive can mark its current task BLOCKED while interrupts are off,
park it, then later wake its task ID back to READY. FIFO wait queues use these
hooks for sleeping mutexes and semaphores.

This matters beyond locks: future pipes, networking sockets, device completion,
and IPC can block callers on the same scheduler mechanism instead of polling.


## Phase 21 advanced scheduling

Priorities range from 0 to 7, with larger values preferred and 4 as the default.
Every 20 ticks spent READY increases a task's effective priority by one until it
reaches 7. A selected task loses the temporary boost and returns to its base
priority. This gives high-priority tasks preferential CPU access while ensuring
that continuously READY low-priority tasks eventually run.

Equal effective priorities use circular scan order, retaining round-robin
fairness inside a priority class. A higher-effective-priority READY task may
preempt the current task before the current five-tick quantum expires.

Fresh tasks start with BSP affinity (`mask 1`). Forked children inherit both
priority and affinity. Because Phase-17 APs still park after their SMP proof,
Phase 21 requires CPU0 in a runnable affinity mask. Full per-CPU run queues,
task migration, reschedule IPIs, and TLB shootdowns remain future SMP scheduler
work.

The Ring-3 `ps` command exposes base/effective priority and affinity, while
`schedstats` reports context switches, preemptions, priority preemptions, aging
promotions, voluntary yields, and current policy/task metadata.

## Phase 22 IPC lifecycle

Processes can now communicate through kernel IPC objects without sharing their
ordinary private code/data/heap pages.

- Pipes are bounded kernel circular byte buffers addressed by IPC ID.
- Shared memory maps the same PMM page into multiple isolated address spaces at
  reserved user/RW/NX virtual addresses.
- Message queues preserve message boundaries in bounded kernel storage.

Shared-memory attachment records include the task ID and address-space root.
`fork()` clones these mappings as shared physical pages rather than private
copies. `exec()` drops inherited IPC shared mappings from the replaced image,
and task teardown detaches them before its page tables are destroyed.

Phase 22 does not yet turn pipes into VFS descriptors and does not block tasks
on empty/full IPC objects. Those extensions can reuse Phase-16 wait queues in a
later refinement.

## Phase 23 graphical userspace

`/bin/gfxdemo` is an ordinary isolated Ring-3 ELF. It owns no privileged display
mapping: all rendering goes through Phase-23 graphics syscalls and `libaxiom.a`.
This keeps graphics applications subject to the same address-space, NX/W^X,
credentials, scheduler, and lifecycle rules as every other userspace process.


## Phase 24 network processes

The five networking tools are ordinary Ring-3 ELF processes loaded from `/bin`
and governed by the same scheduler, W^X/NX rules, VFS execute permissions, and
process lifecycle as every other application. No process receives direct NIC
DMA-ring access. Network I/O crosses the validated syscall boundary and remains
kernel-owned. `/bin/httpd` blocks synchronously inside `httpserve` until one
inbound request completes, then exits normally.


## Phase 25 process observability

`/proc/processes` exposes PID/PPID, state, privilege, base/effective priority,
affinity, runtime and accumulated ready time for every task. `/proc/files`
walks each task's descriptor table, and `/proc/pagemap` describes the current
process address-space root plus ELF, stack and mmap mappings. These views are
read-only snapshots generated through procfs.
