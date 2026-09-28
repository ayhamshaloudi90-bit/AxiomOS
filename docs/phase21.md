# Phase 21 — Advanced scheduler

## Goal

Extend Phase 8's preemptive round-robin scheduler without breaking the previous
regression suite. Phase 21 adds priorities, starvation prevention, affinity
metadata, observability, and a benchmark while keeping the Phase-17 decision to
run normal tasks only on the BSP.

## Policy

A task has a base priority from `0` through `7`; larger values have more
scheduling preference and `4` is the default. READY tasks accumulate wait time.
Every 20 READY ticks raises their effective priority by one, capped at 7.
Scheduling always selects the READY task with the largest effective priority.
Equal effective priorities are resolved by circular scan order, preserving
round-robin fairness within a priority class.

When a task is selected, its temporary aging boost is reset to its base
priority. This means a low-priority task can eventually catch a continuously
runnable high-priority task, run, and then return to its normal priority.

## Preemption

The APIC timer still supplies 100 Hz scheduling ticks and the normal quantum is
five ticks. Under the Phase-21 policy a newly READY task with a higher effective
priority can preempt the current task before the current five-tick quantum
expires. At quantum expiration the current task competes again; if it remains
the highest-priority runnable task it may receive another slice.

## CPU affinity

Each task now stores a 64-bit affinity mask. Phase 17 APs are still parked after
the SMP proof, so Phase 21 requires bit 0 (the BSP) in every runnable mask.
Additional bits are retained as affinity intent for a later fully SMP scheduler.
This is deliberate groundwork, not a claim that task migration exists yet.

## Userspace ABI

| Number | Call | Purpose |
|---:|---|---|
| 29 | `getpriority` | Read the current process base priority |
| 30 | `setpriority` | Set the current process priority (`0..7`) |
| 31 | `getaffinity` | Read the current process affinity mask |
| 32 | `setaffinity` | Set the current affinity mask; BSP bit is required |
| 33 | `schedstats` | Copy live scheduler statistics into Ring 3 |

Forked children inherit their parent's priority and affinity mask. Fresh tasks
start with priority 4 and BSP affinity.

## Observability

`ps` now shows base/effective priority and affinity. `schedstats` reports the
active policy, runnable count, switches, timer preemptions, priority-driven
preemptions, aging promotions, voluntary yields, and current task metadata.

`/bin/schedbench` deterministically compares 96 scheduling decisions for three
synthetic priorities under round-robin and priority+aging. The fixed benchmark
produces an equal `32/32/32` split for round-robin and a high-priority-biased,
but non-starving, split for priority+aging. It also exercises the live priority,
affinity, and scheduler-statistics syscall interfaces.

## Compatibility

`scheduler_init()` still begins in round-robin mode so the historical Phase-8
acceptance test continues to validate the scheduler that Phase 8 actually
introduced. Phase 21 switches to priority+aging only after the Phase-20 boot
milestone has completed.

## Acceptance

```bash
make test-phase21
make test
```

Expected Phase-21 boot markers include:

```text
AxiomOS Phase 21 advanced scheduler online.
Phase 21 policy: priority + aging
Phase 21 priority scheduling: OK
Phase 21 starvation-prevention aging: OK
Phase 21 CPU affinity groundwork: OK
Phase 21 scheduler benchmark comparison: OK
Phase 21 advanced scheduler initialization complete.
```

The test then runs `schedstats` and `/bin/schedbench` from the Ring-3 shell.

## Deliberate limits

- Normal processes still run only on the BSP; affinity does not migrate tasks to APs yet.
- MLFQ is not implemented in this phase; priority+aging is the chosen advanced policy.
- Mutex priority inheritance is not implemented yet, so priority inversion is still possible.
- A process can change only its own scheduling priority/affinity through the Phase-21 ABI.
- The benchmark compares scheduler decision distributions, not real-time throughput.
