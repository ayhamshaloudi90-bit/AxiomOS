# Phase 5 — Virtual memory

## Goal

Move from bootloader-owned paging to an AxiomOS-owned x86-64 address-space root
and provide tested 4 KiB mapping primitives.

## Design choice: clone before replacing

AxiomOS already depends on live mappings for its higher-half kernel, bootstrap
stack, Limine response objects, HHDM aliases, and framebuffer. Rebuilding an
entire address space from a hand-written list would risk omitting a live
mapping. Editing Limine's table pages in place would also leave AxiomOS tied to
bootloader-reclaimable memory.

Phase 5 therefore clones the current hierarchy. The clone preserves every
existing leaf mapping but places every paging-structure page in PMM-owned
`USABLE` RAM. Only after the recursive clone succeeds is CR3 replaced.

## API

```c
int vmm_init(void);
int map_page(vaddr_t virtual_address, paddr_t physical_address, uint64_t flags);
int unmap_page(vaddr_t virtual_address);
paddr_t virt_to_phys(vaddr_t virtual_address);
```

Mapping flags currently include writable, user, write-through, cache-disable,
global, and NX.

## Failure policy

`map_page()` rejects unaligned/non-canonical addresses, duplicate mappings, and
attempts to descend through an existing huge-page leaf. Phase 5 intentionally
does not implement huge-page splitting.

`unmap_page()` rejects missing/unaligned mappings. Empty intermediate tables are
returned to the PMM.

## Test addresses

- normal map/unmap test: `0x0000600000000000`;
- deliberate page fault: `0x0000612345600000`.

Both lie in the canonical lower half and are checked to be unmapped before use.

## Page-fault report

The deliberate fault should contain approximately:

```text
KERNEL PANIC
Exception: Page Fault
Vector: 14  Error: 0x0
CR2: 0x612345600000
Cause: Non-present page
Access: Read
Privilege: Supervisor
Reserved-bit violation: no
Instruction fetch: no
...
CPU halted.
```
