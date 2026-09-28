# Phase 17 — SMP / Multicore Bring-up

Phase 17 moves AxiomOS from a one-CPU execution model to a real multiprocessor
machine. The kernel now requests Limine's MP feature, discovers every x86-64
CPU exposed by QEMU, releases application processors (APs), switches them onto
AxiomOS-owned page tables, and gives each AP private kernel CPU state.

This phase deliberately does **not** schedule normal processes on APs yet. The
existing scheduler remains BSP-only while the APs prove cross-core execution
and then park. This separates CPU bring-up from the much larger problem of a
fully SMP scheduler.

## QEMU machine

Normal `make run` and `make debug` now use:

```text
-smp 4
```

so QEMU exposes four virtual CPUs. Older regression tests may still boot with
one CPU. On a one-CPU boot Phase 17 reports discovery and skips only the
multicore parallel-work proof; this preserves the historical phase tests.

`make test-phase17` always boots with four CPUs and requires the complete SMP
proof.

## Limine MP handoff

AxiomOS adds a Limine MP request with flags `0`. That intentionally keeps xAPIC
mode because the existing Phase-7 local APIC implementation uses the MMIO
xAPIC interface rather than x2APIC MSRs.

Limine provides:

```text
cpu_count
bsp_lapic_id
cpus[]
  └─ processor_id
  └─ lapic_id
  └─ goto_address
  └─ extra_argument
```

Secondary CPUs are already bootstrapped and parked by Limine. AxiomOS fills
`extra_argument` with a pointer to that CPU's `struct smp_cpu` and publishes
`smp_ap_entry_asm` through `goto_address` with release ordering.

## AP entry sequence

An AP cannot simply call normal kernel C code on Limine's temporary execution
state. Its entry sequence is:

```text
Limine parked AP
      ↓ publish goto_address
smp_ap_entry_asm
      ↓ CLI
read per-CPU pointer
      ↓
load AxiomOS kernel CR3
      ↓
switch to private 16 KiB AP stack
      ↓
smp_ap_entry_c
      ↓
load private GDT + private TSS
      ↓
load shared AxiomOS IDT
      ↓
mark CPU online
```

The CR3 change happens before using the permanent AxiomOS stack. The assembly
contains no stack memory accesses between the CR3 write and the RSP switch.

## Per-CPU state

AxiomOS currently supports up to eight managed CPUs in Phase 17. Each managed
CPU records:

```text
processor ID
LAPIC ID
logical AxiomOS CPU index
kernel CR3
private stack top
BSP/AP role
online state
work-completion state
```

Every AP also receives:

- a private 16 KiB kernel stack;
- a private GDT;
- a private 64-bit TSS;
- private emergency IST stacks.

The IDT itself is read-only after initialization and is shared between CPUs.

## Why the TSS must be per-CPU

`LTR` marks a TSS descriptor busy. Two processors must not share one mutable TSS
instance and one Ring-0 stack pointer. Even though Phase-17 APs do not enter
Ring 3, assigning private TSS state now establishes the correct architecture
for later multicore user scheduling.

## Cross-CPU spinlock proof

Phase 16's spinlock previously only had to prevent timer-preempted tasks on one
CPU from interleaving. Phase 17 makes it a true SMP lock.

The acquire path already uses x86 atomic `XCHG`. Phase 17 also makes the lock's
statistics atomic and uses a release store when unlocking.

All online CPUs synchronize at a start barrier. The BSP and every released AP
then perform:

```text
5,000 iterations each
      ↓
spin_lock_irqsave()
      ↓
shared_counter++
      ↓
spin_unlock_irqrestore()
```

For four CPUs the exact expected result is:

```text
4 × 5,000 = 20,000
```

A participant bitmask also proves that each online logical CPU entered the
parallel-work region.

## Current scheduler policy

The important deliberate limit is:

```text
BSP:
  timer
  interrupts
  scheduler
  kernel threads
  Ring-3 processes

APs:
  Phase-17 parallel proof
  then CLI + HLT park
```

This means AxiomOS is now an SMP-capable kernel at the CPU bring-up and shared
memory synchronization level, but **not yet a multicore process scheduler**.
That distinction is intentional and should not be exaggerated.

Moving the general scheduler onto all CPUs requires at least:

- per-CPU current-task state instead of one global `current_index`;
- a scheduler/run-queue lock or per-CPU run queues;
- per-CPU local APIC timers;
- safe task migration;
- cross-core reschedule IPIs;
- TLB shootdowns when mappings shared by another CPU change;
- auditing PMM/VMM/heap/VFS global state for SMP locking.

Those mechanisms are not faked in Phase 17.

## Acceptance output

With `-smp 4`, boot output should include roughly:

```text
AxiomOS Phase 17 SMP / multicore online.
Phase 17 CPUs detected/managed/online: 4/4/4
Phase 17 APs released/completed: 3/3
Phase 17 CPU 0: ... role=BSP online=1
Phase 17 CPU 1: ... role=AP online=1
Phase 17 CPU 2: ... role=AP online=1
Phase 17 CPU 3: ... role=AP online=1
Phase 17 parallel participant mask: 0xF
Phase 17 locked counter expected/actual: 20000/20000
Phase 17 AP bring-up: OK
Phase 17 cross-CPU spinlock: OK
Phase 17 multicore parallel-work test: OK
Phase 17 scheduler policy: BSP-only; APs parked after self-test
Phase 17 SMP initialization complete.
```

Then the normal Ring-3 shell must still launch.

Run:

```bash
make clean
make
make test-phase17
make test
```

## Deliberate limits

- maximum eight managed CPUs in the current static per-CPU table;
- scheduler remains BSP-only;
- AP local timers are not started;
- no inter-processor interrupts yet;
- no TLB shootdown protocol yet;
- no process/task migration between CPUs;
- APs park permanently after the Phase-17 proof;
- kernel subsystems other than the spinlock test are not yet advertised as
  safe for arbitrary AP execution.
