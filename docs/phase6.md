# Phase 6 — Kernel heap

## Goal

Build a freestanding dynamic allocator on top of the Phase-4 physical memory
manager and Phase-5 virtual memory manager.

Phase 6 implements:

```c
void *kmalloc(size_t size);
void *kcalloc(size_t count, size_t size);
void *krealloc(void *pointer, size_t new_size);
void kfree(void *pointer);
```

The allocator does not call host Linux, libc, `malloc()`, or a bootloader
allocator.

## Virtual heap region

The kernel heap begins at the canonical higher-half address:

```text
0xFFFFC00000000000
```

The Phase-6 heap is capped at 64 MiB. It starts with four 4 KiB pages and grows
on demand. Growth works by:

1. allocating a physical frame with `pmm_alloc_page()`;
2. zeroing it through its HHDM alias;
3. mapping it at the next heap virtual page with `map_page()`;
4. adding the newly mapped bytes to the free-list allocator.

Heap mappings are writable and NX.

Phase 6 is intentionally grow-only: `kfree()` makes memory reusable inside the
heap, but does not yet trim top pages back to the PMM. That keeps page-return
policy separate from the first allocator implementation.

## Allocator design

AxiomOS uses a doubly linked, address-ordered block list with a first-fit search.
Every mapped byte in the heap belongs to exactly one block.

Conceptually:

```text
+---------+------------------+---------+----------------+---------+
| header  | allocation A     | header  | free space     | ...     |
+---------+------------------+---------+----------------+---------+
```

Each block has a 64-byte header containing:

- integrity magic;
- payload capacity;
- currently requested byte count;
- previous/next block links;
- free/allocated state;
- an integrity cookie and its inverse.

Allocated payloads are 16-byte aligned.

## Splitting

If a free block is substantially larger than a request, it is split:

```text
Before:

+-----------------------------------------------+
|                  free block                   |
+-----------------------------------------------+

After allocation:

+--------------------+--------------------------+
| allocated block    |       free remainder     |
+--------------------+--------------------------+
```

A split is performed only when the remainder is large enough to hold another
64-byte header plus a useful minimum payload.

## Coalescing

When a block is freed, adjacent free blocks are merged immediately:

```text
+---------+---------+---------+
| free A  | free B  | used C  |
+---------+---------+---------+

          becomes

+-------------------+---------+
|      free A+B     | used C  |
+-------------------+---------+
```

This directly reduces external fragmentation and is tested in the Phase-6
self-test.

## `krealloc()`

`krealloc()` uses three paths:

1. **Shrink in place** when the current block is already large enough. A useful
   remainder is split into a new free block.
2. **Grow in place** when the immediately following block is free and combining
   the two creates enough capacity.
3. **Move** when neither in-place option works: allocate a new block, copy the
   old payload byte-for-byte, and free the old block.

`krealloc(NULL, n)` behaves like `kmalloc(n)`. `krealloc(ptr, 0)` frees the
allocation and returns `NULL`.

## `kcalloc()`

`kcalloc()` rejects integer multiplication overflow before allocating. A valid
allocation is zero-filled explicitly by the kernel.

## Corruption checks

The heap performs intentionally expensive validation in Phase 6 because
correctness is more important than allocation speed at this stage.

### Header integrity

Each header contains an address-dependent magic value plus a cookie derived
from its size, links, requested size, and flags. The cookie inverse must also
match.

The allocator validates that blocks:

- are contiguous and 16-byte aligned;
- remain inside the currently mapped heap;
- have consistent previous/next links;
- have sane capacities;
- partition the entire mapped heap without gaps or overlap.

### Tail guards

Each live allocation reserves 16 bytes immediately after the caller-requested
payload and stores a two-word guard there. `kfree()` and `krealloc()` verify the
guard before modifying the block.

A one-byte write past the requested size therefore corrupts the guard and causes
an intentional kernel panic.

### Double free

A second `kfree()` of a block that is still represented as free is detected and
panics rather than corrupting allocator state.

## Fragmentation

Two different forms matter:

### Internal fragmentation

The caller may request 17 bytes, but AxiomOS must align the allocation and
reserve guard space. The block therefore consumes more than exactly 17 bytes.
That unused space inside an allocated block is internal fragmentation.

### External fragmentation

The heap might contain 8 KiB total free space, but split into four separate 2
KiB holes. A 5 KiB request cannot use those holes even though the total free
space is larger than 5 KiB. That is external fragmentation.

Phase 6 fights external fragmentation using immediate coalescing. First-fit is
simple and easy to audit, but may still fragment over long-running workloads.
Later kernels could add segregated free lists or slab allocators for common
object sizes.

## Phase-6 self-test

The normal self-test verifies:

1. 16-byte allocation alignment;
2. distinct blocks do not overlap;
3. write/read integrity for small, medium, and 20 KiB allocations;
4. dynamic heap page growth;
5. `kcalloc()` zero-fill;
6. `kcalloc()` multiplication-overflow rejection;
7. first-fit reuse of a freed block;
8. `krealloc()` growth with data preservation;
9. `krealloc()` shrink with data preservation;
10. adjacent-free-block coalescing;
11. no live allocations remain afterward;
12. allocation/free counters balance;
13. the final heap collapses back into one free block.

Two additional QEMU modes validate corruption protection:

```bash
make MODE=heap_double_free run
make MODE=heap_guard run
```

The first intentionally frees an allocation twice. The second writes one byte
past a 32-byte allocation and then frees it. Both must panic and halt.
