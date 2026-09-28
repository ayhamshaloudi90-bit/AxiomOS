# Phase 25 — Developer tooling and observability

## Goal

Finish the original AxiomOS Phase 0–25 roadmap with live debugging and
observability interfaces that reuse the normal VFS and Ring-3 process model.

## `/proc` pseudo-filesystem

Phase 25 mounts a read-only `procfs` at `/proc`. Files are generated on demand
from live kernel state rather than stored on disk:

- `/proc/meminfo` — PMM, heap, and VMM usage;
- `/proc/processes` — process state, priorities, affinity, runtime, ready time;
- `/proc/interrupts` — total/per-vector interrupt counts;
- `/proc/files` — open descriptors across tasks, offsets, flags, references;
- `/proc/net` — NIC and protocol counters plus IPv4 configuration;
- `/proc/scheduler` — policy, switches, preemptions, aging, yields, timer data;
- `/proc/cpuinfo` — CPUID vendor, APIC and SMP topology;
- `/proc/pagemap` — current process CR3 and mapped ELF/stack/mmap pages.

The mount is read-only (`0444` files, `0555` root). A kernel boot self-test
opens and reads every telemetry file and verifies writes are rejected.

## Interrupt instrumentation

The common x86-64 interrupt dispatcher now increments a counter for each IDT
vector plus a total interrupt counter. The counters include timer and keyboard
interrupts and are exported through `/proc/interrupts`.

## Ring-3 developer dashboard

`/bin/sysinfo` is a normal isolated ELF that opens and reads the `/proc` files
through existing file syscalls. No privileged debug syscall is required.

Interactive examples:

```text
axiom> ls /proc
axiom> cat /proc/meminfo
axiom> cat /proc/processes
axiom> cat /proc/interrupts
axiom> cat /proc/files
axiom> cat /proc/net
axiom> cat /proc/scheduler
axiom> cat /proc/cpuinfo
axiom> cat /proc/pagemap
axiom> sysinfo
```

## Acceptance

```bash
make test-phase25
make test
```

The phase-specific test verifies the boot markers, `/proc` directory contents,
live telemetry from every requested category, execution of `/bin/sysinfo`, and
read-only enforcement.

Phase 25 completes the original AxiomOS Phase 0–25 roadmap. Further work can be
planned as post-roadmap extensions rather than being silently added to the
original scope.
