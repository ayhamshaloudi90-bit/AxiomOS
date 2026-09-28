# AxiomOS memory management — Phase 6

AxiomOS now has three memory-management layers:

```text
physical RAM
    ↓
Phase 4 PMM — allocates 4 KiB physical frames
    ↓
Phase 5 VMM — maps frames into virtual address space
    ↓
Phase 6 heap — allocates arbitrary-sized kernel objects
```

## Physical memory

The PMM manages only Limine `USABLE` regions with 4 KiB frames. It distinguishes
physical addresses with `paddr_t` and exposes HHDM aliases explicitly.
Bootloader-reclaimable memory is still not reclaimed.

## Virtual memory

AxiomOS owns the active 4-level PML4/PDPT/PD/PT hierarchy. The VMM can map and
unmap 4 KiB pages and translate 4 KiB plus inherited 2 MiB/1 GiB leaves.
Changed mappings invalidate the appropriate TLB entry.

## Kernel heap

The Phase-6 heap begins at:

```text
0xFFFFC00000000000
```

and is limited to 64 MiB in this phase. The heap starts with four mapped pages.
When no free block is large enough, the heap allocates additional PMM frames and
maps them writable + NX at consecutive heap virtual addresses.

The mapped heap is partitioned into contiguous variable-sized blocks. A 64-byte
header precedes each payload. Allocated blocks are aligned to 16 bytes and keep
a 16-byte tail guard immediately after the requested payload.

The allocator uses first-fit search, splitting, and immediate coalescing.
`kcalloc()` checks multiplication overflow and zero-fills. `krealloc()` shrinks
in place, grows into the next free block when possible, otherwise moves and
copies the allocation.

The heap is grow-only for now. `kfree()` returns space to the allocator but not
to the PMM. A later optimization may trim completely unused top pages.

See [Phase 6](phase6.md) for block layout, corruption checks, fragmentation,
and the full test plan.

## Phase 9 user address spaces

Phase 9 introduces per-user-task CR3 roots. Each user root starts with an empty
lower canonical half and shares the kernel's upper-half PML4 entries with USER
cleared. User code/data/stack mappings are created only in the lower half with
`VMM_FLAG_USER`.

New VMM interfaces include:

```c
paddr_t vmm_create_user_address_space(void);
int vmm_destroy_user_address_space(paddr_t root_table);
int vmm_map_page_in_address_space(...);
paddr_t vmm_virt_to_phys_in_address_space(...);
int vmm_activate_address_space(paddr_t root_table);
paddr_t vmm_current_address_space(void);
paddr_t vmm_kernel_address_space(void);
```

The scheduler changes CR3 when switching between tasks with different address
spaces. Because the higher-half kernel mapping is present in every user CR3,
interrupt handling can continue safely after a Ring-3 interrupt switches to the
per-task TSS kernel stack.
