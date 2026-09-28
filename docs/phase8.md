# Phase 8 — Preemptive multitasking

## Goal

Turn the Phase-7 100 Hz Local APIC timer into a real preemptive scheduler. AxiomOS
now runs multiple ring-0 kernel threads with independent stacks and CPU contexts.
The policy is intentionally simple: fixed-quantum round robin.

## Task model

`struct task` contains:

- task ID and human-readable name;
- state: `RUNNING`, `READY`, `BLOCKED`, `SLEEPING`, or `TERMINATED`;
- a complete x86-64 `interrupt_frame` CPU context;
- private kernel stack base/size;
- address-space/root-page-table identifier;
- entry function and argument;
- quantum, ticks consumed, runtime ticks, and switch count.

Phase 8 implements and exercises `RUNNING`, `READY`, and the termination path.
`BLOCKED` and `SLEEPING` are represented now so later synchronization/sleep APIs do
not need to redesign the task structure.

## Why kernel threads first

All Phase-8 tasks execute in ring 0 and share the current AxiomOS page tables. This
separates two hard problems:

1. prove CPU context switching and preemptive scheduling;
2. later add ring-3 privilege changes and per-process address spaces in Phase 9.

Mixing both into one phase would make failures much harder to isolate.

## Context switch

The Local APIC timer still fires at 100 Hz. The scheduler gives each task a
five-tick quantum (about 50 ms at the configured frequency).

The CPU already creates a complete interrupt frame on every timer interrupt. The
common ISR stub then saves all general-purpose registers. Phase 8 uses that frame
as the task context:

```text
running task
   |
   v
Local APIC timer interrupt
   |
   v
ISR saves GPRs + CPU saved RIP/RSP/RFLAGS/etc.
   |
   v
scheduler copies frame -> current task context
   |
   v
round-robin chooses next READY task
   |
   v
scheduler copies next task context -> ISR frame
   |
   v
IRETQ restores the next task
```

There is no host thread API and the worker threads do not voluntarily yield.

## New-task bootstrap

A new task receives a 16 KiB stack from the Phase-6 heap while interrupts are
disabled. A synthetic interrupt context points RIP at `task_bootstrap()` and RSP
at the new stack. Its RFLAGS has IF set, so once `IRETQ` selects it the task runs
like any normally interrupted kernel thread.

If a task entry returns, the bootstrap marks it `TERMINATED`. Phase 8 deliberately
does not free its stack while executing on it; stack reaping/reclamation is later
scheduler work.

## Round robin

The scheduler scans the fixed task table after the current task has consumed its
quantum and chooses the next `READY` task. With the Phase-8 demo:

```text
bootstrap -> worker A -> worker B -> bootstrap -> ...
```

The bootstrap task is the original `kernel_main()` execution context.

## Acceptance demonstration

Two worker threads run infinite loops and increment separate volatile counters.
They contain no `yield()`, `hlt`, blocking call, or scheduler call. Therefore both
counters can advance and the bootstrap task can regain control only if timer
preemption and context restoration work.

The normal boot requires:

- exactly three schedulable tasks for the demo;
- both worker counters to exceed 1000;
- at least three context switches/preemptions;
- the Phase-7 keyboard input loop to continue working after the scheduler starts.

Run:

```bash
make test-phase8
```

Then run all regressions:

```bash
make test
```

## Current limitations

- single CPU only;
- kernel threads only; no ring-3 processes yet;
- every Phase-8 task shares the same CR3/address space;
- fixed five-tick quantum, no priorities;
- `BLOCKED`/`SLEEPING` states exist but wait queues and sleeping APIs are later;
- terminated task stacks are not reclaimed yet;
- no FPU/SIMD context exists because AxiomOS still builds with those facilities
  disabled;
- heap allocation remains forbidden from interrupt context.

## Phase-9 context-switch refinement

Phase 9 refined the stack-switch mechanism used by the Phase-8 scheduler. In
64-bit mode, the interrupt frame includes saved `SS:RSP` even for same-CPL
interrupts, and `IRETQ` restores them. The ISR contract now returns the selected
task's saved-frame pointer; each synthetic kernel-thread frame carries a real
private-stack `RSP` and kernel `SS`. The round-robin policy remains unchanged.
