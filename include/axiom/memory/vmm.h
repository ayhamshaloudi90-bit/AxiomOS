#ifndef AXIOM_MEMORY_VMM_H
#define AXIOM_MEMORY_VMM_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/memory/address.h>

#define VMM_PAGE_SIZE 4096ULL

/* Hardware-compatible leaf mapping flags exposed by the paging API. */
#define VMM_FLAG_WRITABLE      (1ULL << 1)
#define VMM_FLAG_USER          (1ULL << 2)
#define VMM_FLAG_WRITE_THROUGH (1ULL << 3)
#define VMM_FLAG_CACHE_DISABLE (1ULL << 4)
#define VMM_FLAG_GLOBAL        (1ULL << 8)
#define VMM_FLAG_NO_EXECUTE    (1ULL << 63)

/* Reserved lower-half virtual addresses used only by Phase-5 validation. */
#define VMM_SELFTEST_ADDRESS   0x0000600000000000ULL
#define VMM_FAULT_TEST_ADDRESS 0x0000612345600000ULL

struct vmm_stats {
    paddr_t root_table;
    uint64_t page_table_pages;
    uint64_t hhdm_offset;
};

int vmm_init(void);

/* Active-kernel-address-space compatibility API from Phase 5. */
int map_page(vaddr_t virtual_address, paddr_t physical_address, uint64_t flags);
int unmap_page(vaddr_t virtual_address);
paddr_t virt_to_phys(vaddr_t virtual_address);

/* Phase 9: isolated lower-half address spaces sharing supervisor kernel maps. */
paddr_t vmm_create_user_address_space(void);
int vmm_destroy_user_address_space(paddr_t root_table);
int vmm_map_page_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address,
    paddr_t physical_address,
    uint64_t flags
);
int vmm_unmap_page_in_address_space(paddr_t root_table, vaddr_t virtual_address);
paddr_t vmm_virt_to_phys_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address
);
int vmm_activate_address_space(paddr_t root_table);
paddr_t vmm_current_address_space(void);
paddr_t vmm_kernel_address_space(void);

/* Phase 10: validate and copy user buffers without dereferencing them in Ring 0. */
int vmm_user_range_accessible(
    paddr_t root_table,
    vaddr_t user_address,
    size_t length,
    int write_access
);
int vmm_copy_from_user(
    paddr_t root_table,
    void *destination,
    vaddr_t user_source,
    size_t length
);
int vmm_copy_to_user(
    paddr_t root_table,
    vaddr_t user_destination,
    const void *source,
    size_t length
);

/* Phase 11: inspect the effective leaf flags for an existing 4 KiB mapping. */
int vmm_mapping_flags_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address,
    uint64_t *flags_out
);

struct vmm_stats vmm_get_stats(void);
int phase5_vmm_selftest(void);

#endif
