#ifndef AXIOM_MEMORY_PMM_H
#define AXIOM_MEMORY_PMM_H

#include <stdint.h>

#include <axiom/memory/address.h>

#define PMM_PAGE_SIZE 4096ULL

struct pmm_stats {
    uint64_t memory_map_entries;
    uint64_t total_memory_bytes;
    uint64_t usable_memory_bytes;
    uint64_t usable_pages;
    uint64_t metadata_pages;
    uint64_t allocated_pages;
    uint64_t free_pages;
    paddr_t metadata_base;
    paddr_t highest_usable_address;
};

/* Initialise the allocator from Limine's memory map. Returns 1 on success. */
int pmm_init(void);

/* Allocate/free one 4 KiB physical page. */
paddr_t pmm_alloc_page(void);
int pmm_free_page(paddr_t page);

/* Convert a managed usable physical address through Limine's HHDM. */
void *pmm_phys_to_hhdm(paddr_t address);

/* Snapshot current allocator accounting. */
struct pmm_stats pmm_get_stats(void);

/* Phase-4 destructive allocator self-test; restores baseline accounting. */
int phase4_pmm_selftest(void);

#endif
