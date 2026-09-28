# Phase 16 — Synchronization

Phase 16 adds kernel synchronization primitives and connects them to the
preemptive scheduler. The purpose is not merely to define lock types: AxiomOS
first demonstrates a real lost-update race and then proves that the new
primitives repair or control the same concurrency.

## Why synchronization is needed even on one CPU

AxiomOS still runs on one CPU, but the APIC timer can preempt one task between a
load and a later store. Two tasks that both execute:

```text
value = counter
counter = value + 1
```

can therefore both read the same old value and lose one increment. Phase 16's
race test makes that interleaving deterministic with a barrier: two workers
read the same counter value before either stores it. 128 intended increments
become 64, proving the bug instead of relying on luck.

## IRQ-save spinlocks

`struct spinlock` uses x86 `xchg` as the atomic acquire operation. Acquisition
also saves and disables interrupts. On the current single-CPU kernel this is an
important rule: a task holding a spinlock cannot be preempted, so another task
cannot run on the same CPU and spin forever waiting for a descheduled owner.

Spinlocks are therefore for very short kernel critical sections. They must not
be held across sleeping/blocking operations. Phase 16 uses one to protect a
shared counter; two workers produce exactly the expected 2000 increments.

## Wait queues and scheduler blocking

Phase 16 adds generic scheduler hooks:

```text
RUNNING
   ↓ prepare block while IF=0
BLOCKED
   ↓ park / context switch
another task runs
   ↓ wake task
READY
   ↓ scheduler chooses it
RUNNING
```

A `wait_queue` is a bounded FIFO of task IDs. The queue is protected by the
same spinlock as the condition being waited on, which prevents the classic
lost-wakeup race between checking a condition and going to sleep.

These hooks are intentionally generic because later pipes, sockets, device I/O
and IPC can use the same BLOCKED/READY mechanism.

## Sleeping mutex

`struct mutex` is non-recursive and uses:

- a short internal spinlock for metadata;
- an owner PID/task ID;
- a FIFO wait queue.

If free, ownership is immediate. If owned, the caller is enqueued, marked
BLOCKED, and parked. `mutex_unlock()` hands ownership directly to the oldest
waiter before waking it. Direct handoff prevents a newly arriving task from
stealing the mutex ahead of a task that was already waiting.

The self-test deliberately yields while holding the mutex so the second worker
must actually block. The final shared counter must still be exact.

## Counting semaphore

`struct semaphore` maintains a token count plus a FIFO wait queue. `wait()`
consumes a token or blocks. `post()` either increments the available count or
directly hands the token to the oldest blocked waiter.

The Phase-16 test starts three workers with a semaphore count of two. Workers
yield while holding a token, so two tasks are observed inside the protected
region while the third is forced to block. The measured concurrency peak must
be exactly two.

## Deadlock and starvation

Phase 16 does not attempt a general deadlock detector. A deadlock can still
occur if, for example, task A holds mutex X and waits for Y while task B holds Y
and waits for X. The engineering rule is to use a consistent lock ordering and
never sleep while holding a spinlock.

FIFO mutex/semaphore wait queues reduce starvation by waking waiters in arrival
order. They do not provide formal real-time fairness, especially once priorities
or multiple CPUs are introduced later.


## Phase-8 worker retirement

Phase 8 used two intentionally CPU-bound kernel threads to prove timer-driven
preemption. They are acceptance-test fixtures rather than kernel services.
Phase 16 now asks them to exit immediately after the Phase-8 test succeeds.
This prevents two synthetic busy loops from consuming scheduler quanta forever
and makes later blocking/synchronization timing representative of real work.

The synchronization worker timeout is also a safety bound only; it is now 1600
timer ticks so a slow emulator does not turn correct scheduling into a false
negative. On failure, the kernel prints the failing synchronization stage
number before the generic failure line.

## Acceptance output

A normal boot should include lines similar to:

```text
AxiomOS Phase 16 synchronization online.
Phase 16 race expected/observed: 128/64
Phase 16 race condition demonstrated: OK
Phase 16 spinlock counter expected/actual: 2000/2000
Phase 16 spinlock protected counter: OK
Phase 16 mutex counter expected/actual: 128/128
Phase 16 mutex blocks/wakeups: .../...
Phase 16 mutex protected counter: OK
Phase 16 semaphore limit/peak: 2/2
Phase 16 semaphore blocks/wakeups: .../...
Phase 16 semaphore limit: OK
Phase 16 wait queue wakeups: OK
Phase 16 synchronization self-test: OK
Phase 16 synchronization complete.
```

The exact block/wakeup counts are scheduler-interleaving dependent; the test
requires them to be nonzero rather than hardcoding a particular value.

Run:

```bash
make clean
make
make test-phase16
make test
```

`test-phase16` boots QEMU, validates the deliberate race, exact protected
counters, real mutex/semaphore blocking, the semaphore concurrency limit, then
types `echo phase16-ok` into the Ring-3 shell to ensure normal userspace still
works after the kernel synchronization tests.

## Deliberate limits

- single CPU only; SMP locking comes in Phase 17;
- fixed-size wait queues bounded by the fixed process/task table;
- spinlocks are non-recursive;
- mutexes are non-recursive;
- no priority inheritance;
- no deadlock detector;
- no timed mutex/semaphore waits yet.
