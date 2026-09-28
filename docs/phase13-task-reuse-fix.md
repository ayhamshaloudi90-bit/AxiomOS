# Phase 13 scheduler slot-reuse fix

Phase 13 originally failed before AHCI testing because the scheduler had a fixed
8-slot task table and only appended new tasks. By the end of Phase 12, task IDs
0 through 7 had already been created, so the Phase 13 ELF task could not be
allocated even though several older user tasks were already TERMINATED.

The scheduler now reuses terminated non-current slots on the next task creation.
Before reuse it releases the old task's open VFS files, user address space, ELF
pages, user stack pages, and private kernel stack. The bootstrap task is never
reclaimed. New tasks still receive monotonically increasing task IDs.

This is intentionally better than merely increasing SCHEDULER_MAX_TASKS: the
shell and later process-management phases will create many programs over the
lifetime of one boot, so dead task slots must be reclaimable.
