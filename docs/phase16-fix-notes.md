# AxiomOS Phase 16 runtime fix

Observed failure:

```text
AxiomOS Phase 16 synchronization online.
...
Phase 16 synchronization self-test: FAILED
```

The Phase-16 self-test used a fixed 800-tick worker timeout. Two Phase-8 CPU-bound
preemption-test threads were intentionally infinite loops and were still alive in
every later phase. Because the scheduler is round-robin with a five-tick quantum,
those synthetic workers consumed ten ticks of every cycle. Phase 16 deliberately
yields frequently to force races and blocking, so a correct synchronization test
could exceed the timeout and be reported as a false failure.

The correction is architectural rather than merely hiding the symptom:

1. Phase-8 workers now observe `phase8_workers_stop` and terminate after the
   Phase-8 acceptance metrics have been recorded. They are test fixtures, not
   permanent kernel services.
2. Phase 16's worker safety timeout is increased from 800 to 1600 timer ticks to
   tolerate slower emulation without making timing itself part of correctness.
3. The synchronization result now records the active stage. If a real failure
   remains, the kernel prints `Phase 16 synchronization failure stage: N` where
   1=race, 2=spinlock, 3=mutex, and 4=semaphore.

The synchronization behavior and acceptance values remain unchanged: the race
still expects 128 attempted increments, spinlock expects 2000 exact increments,
mutex expects 128 exact increments with real block/wakeup activity, and the
semaphore peak remains exactly two.

## Stage-3 mutex runtime fix

A subsequent QEMU run correctly narrowed the remaining failure to synchronization self-test stage 3 (mutex). The mutex test runner itself was still consuming too much scheduler time: the bootstrap task waited with `hlt`, so every time it was scheduled between contending workers it could occupy a full five-tick quantum. The mutex test deliberately yields while holding the mutex, making that artificial bootstrap load repeat many times.

The second runtime fix:

- changes the self-test worker waiter to voluntarily call the scheduler yield path rather than consuming a full bootstrap quantum;
- removes the redundant post-unlock yield from the mutex worker (FIFO ownership handoff already forces the old owner to block if it reaches the next lock first);
- increases the self-test safety timeout to 3000 ticks as a guard, not as the primary fix;
- preserves partial mutex counter/block/wakeup statistics on failure and prints them from the kernel for easier diagnosis.

The synchronization semantics themselves are unchanged: mutex waiters still block in `TASK_BLOCKED`, unlock still hands ownership directly to the oldest FIFO waiter, and the scheduler wakes that task to `TASK_READY`.
