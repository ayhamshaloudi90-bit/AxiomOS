#include <stddef.h>
#include <stdint.h>

#include <axiom/boot/limine.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>

#define PAGE_ENTRY_PRESENT       (1ULL << 0)
#define PAGE_ENTRY_WRITABLE      (1ULL << 1)
#define PAGE_ENTRY_USER          (1ULL << 2)
#define PAGE_ENTRY_LARGE         (1ULL << 7)

#define PAGE_ADDRESS_MASK_4K     0x000FFFFFFFFFF000ULL
#define PAGE_ADDRESS_MASK_2M     0x000FFFFFFFE00000ULL
#define PAGE_ADDRESS_MASK_1G     0x000FFFFFC0000000ULL

#define PAGE_OFFSET_MASK_4K      (VMM_PAGE_SIZE - 1ULL)
#define PAGE_OFFSET_MASK_2M      ((1ULL << 21) - 1ULL)
#define PAGE_OFFSET_MASK_1G      ((1ULL << 30) - 1ULL)

#define PAGE_TABLE_ENTRIES       512ULL


static struct {
    paddr_t root_table;
    uint64_t hhdm_offset;
    uint64_t page_table_pages;
    int initialized;
} vmm;


static uint64_t pml4_index(vaddr_t address)
{
    return (address >> 39) & 0x1FFULL;
}


static uint64_t pdpt_index(vaddr_t address)
{
    return (address >> 30) & 0x1FFULL;
}


static uint64_t pd_index(vaddr_t address)
{
    return (address >> 21) & 0x1FFULL;
}


static uint64_t pt_index(vaddr_t address)
{
    return (address >> 12) & 0x1FFULL;
}


static int is_page_aligned(uint64_t address)
{
    return (address & PAGE_OFFSET_MASK_4K) == 0ULL;
}


static int is_canonical(vaddr_t address)
{
    const uint64_t upper = address >> 48;
    const uint64_t sign = (address >> 47) & 1ULL;

    return sign != 0ULL ? upper == 0xFFFFULL : upper == 0ULL;
}


static uint64_t read_cr3(void)
{
    uint64_t value;

    __asm__ volatile ("mov %%cr3, %0" : "=r"(value));
    return value;
}


static void write_cr3(paddr_t root)
{
    __asm__ volatile ("mov %0, %%cr3" : : "r"(root) : "memory");
}


static void invalidate_page(vaddr_t address)
{
    __asm__ volatile ("invlpg (%0)" : : "r"((uintptr_t)address) : "memory");
}


static uint64_t *raw_hhdm_table(paddr_t physical_address)
{
    if (physical_address == PADDR_INVALID ||
        UINT64_MAX - vmm.hhdm_offset < physical_address) {

        return 0;
    }

    return (uint64_t *)(uintptr_t)(vmm.hhdm_offset + physical_address);
}


static uint64_t *owned_table(paddr_t physical_address)
{
    return (uint64_t *)pmm_phys_to_hhdm(physical_address);
}


static void clear_table(uint64_t *table)
{
    uint64_t index;

    for (index = 0; index < PAGE_TABLE_ENTRIES; ++index) {
        table[index] = 0ULL;
    }
}


static int table_is_empty(const uint64_t *table)
{
    uint64_t index;

    for (index = 0; index < PAGE_TABLE_ENTRIES; ++index) {
        if ((table[index] & PAGE_ENTRY_PRESENT) != 0ULL) {
            return 0;
        }
    }

    return 1;
}


static int entry_is_large(uint64_t entry, unsigned level)
{
    return (level == 3U || level == 2U) &&
           (entry & PAGE_ENTRY_LARGE) != 0ULL;
}


static void free_owned_tree(paddr_t table_physical, unsigned level);
static paddr_t virt_to_phys_in_root(paddr_t root_table, vaddr_t virtual_address);


static paddr_t clone_table_recursive(paddr_t source_physical, unsigned level)
{
    uint64_t *source;
    uint64_t *destination;
    paddr_t destination_physical;
    uint64_t index;


    source = raw_hhdm_table(source_physical);

    if (source == 0) {
        return PADDR_INVALID;
    }


    destination_physical = pmm_alloc_page();

    if (destination_physical == PADDR_INVALID) {
        return PADDR_INVALID;
    }


    destination = owned_table(destination_physical);

    if (destination == 0) {
        (void)pmm_free_page(destination_physical);
        return PADDR_INVALID;
    }


    clear_table(destination);
    ++vmm.page_table_pages;


    for (index = 0; index < PAGE_TABLE_ENTRIES; ++index) {
        const uint64_t entry = source[index];


        if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
            destination[index] = entry;
            continue;
        }


        if (level == 1U || entry_is_large(entry, level)) {
            destination[index] = entry;
            continue;
        }


        {
            const paddr_t child_source = entry & PAGE_ADDRESS_MASK_4K;
            const paddr_t child_destination =
                clone_table_recursive(child_source, level - 1U);


            if (child_destination == PADDR_INVALID) {
                free_owned_tree(destination_physical, level);
                return PADDR_INVALID;
            }


            destination[index] =
                child_destination |
                (entry & ~PAGE_ADDRESS_MASK_4K);
        }
    }


    return destination_physical;
}


static void free_owned_tree(paddr_t table_physical, unsigned level)
{
    uint64_t *table;
    uint64_t index;


    table = owned_table(table_physical);

    if (table == 0) {
        return;
    }


    if (level > 1U) {
        for (index = 0; index < PAGE_TABLE_ENTRIES; ++index) {
            const uint64_t entry = table[index];


            if ((entry & PAGE_ENTRY_PRESENT) == 0ULL ||
                entry_is_large(entry, level)) {

                continue;
            }


            free_owned_tree(
                entry & PAGE_ADDRESS_MASK_4K,
                level - 1U
            );
        }
    }


    if (pmm_free_page(table_physical)) {
        if (vmm.page_table_pages > 0ULL) {
            --vmm.page_table_pages;
        }
    }
}


static int allocate_child_table(
    uint64_t *parent_entry,
    uint64_t leaf_flags,
    paddr_t *new_physical
)
{
    paddr_t physical;
    uint64_t *table;
    uint64_t flags;


    physical = pmm_alloc_page();

    if (physical == PADDR_INVALID) {
        return 0;
    }


    table = owned_table(physical);

    if (table == 0) {
        (void)pmm_free_page(physical);
        return 0;
    }


    clear_table(table);


    flags = PAGE_ENTRY_PRESENT | PAGE_ENTRY_WRITABLE;

    if ((leaf_flags & VMM_FLAG_USER) != 0ULL) {
        flags |= PAGE_ENTRY_USER;
    }


    *parent_entry = physical | flags;
    *new_physical = physical;
    ++vmm.page_table_pages;

    return 1;
}


static void rollback_new_tables(
    uint64_t **parent_entries,
    paddr_t *physical_pages,
    uint64_t count
)
{
    while (count > 0ULL) {
        --count;

        *parent_entries[count] = 0ULL;

        if (pmm_free_page(physical_pages[count]) &&
            vmm.page_table_pages > 0ULL) {

            --vmm.page_table_pages;
        }
    }
}


int vmm_init(void)
{
    struct limine_memmap_response *memory_map;
    uint64_t hhdm_offset;
    paddr_t source_root;
    paddr_t cloned_root;


    if (vmm.initialized) {
        return 1;
    }


    if (!limine_get_memory_boot_info(&memory_map, &hhdm_offset)) {
        return 0;
    }


    if (memory_map == 0) {
        return 0;
    }


    vmm.hhdm_offset = hhdm_offset;
    vmm.page_table_pages = 0ULL;


    source_root = read_cr3() & PAGE_ADDRESS_MASK_4K;

    if (source_root == 0ULL) {
        return 0;
    }


    cloned_root = clone_table_recursive(source_root, 4U);

    if (cloned_root == PADDR_INVALID) {
        return 0;
    }


    vmm.root_table = cloned_root;

    /*
     * The cloned hierarchy contains all current kernel, HHDM and framebuffer
     * mappings. Switching CR3 therefore preserves the currently executing
     * instruction stream and stack while making AxiomOS the page-table owner.
     */
    write_cr3(vmm.root_table);


    if ((read_cr3() & PAGE_ADDRESS_MASK_4K) != vmm.root_table) {
        write_cr3(source_root);
        free_owned_tree(vmm.root_table, 4U);
        vmm.root_table = PADDR_INVALID;
        return 0;
    }


    vmm.initialized = 1;
    return 1;
}


static int map_page_in_root(
    paddr_t root_table,
    vaddr_t virtual_address,
    paddr_t physical_address,
    uint64_t flags
)
{
    uint64_t *pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;

    uint64_t *parent_entries[3];
    paddr_t new_tables[3];
    uint64_t *user_entries[3];
    uint64_t created;
    uint64_t user_entry_count;

    uint64_t index4;
    uint64_t index3;
    uint64_t index2;
    uint64_t index1;


    if (!vmm.initialized ||
        !is_page_aligned(virtual_address) ||
        !is_page_aligned(physical_address) ||
        physical_address == PADDR_INVALID ||
        (physical_address & ~PAGE_ADDRESS_MASK_4K) != 0ULL ||
        !is_canonical(virtual_address)) {

        return 0;
    }


    index4 = pml4_index(virtual_address);
    index3 = pdpt_index(virtual_address);
    index2 = pd_index(virtual_address);
    index1 = pt_index(virtual_address);

    created = 0ULL;
    user_entry_count = 0ULL;


    pml4 = owned_table(root_table);

    if (pml4 == 0) {
        return 0;
    }


    if ((pml4[index4] & PAGE_ENTRY_PRESENT) == 0ULL) {
        parent_entries[created] = &pml4[index4];

        if (!allocate_child_table(
                &pml4[index4],
                flags,
                &new_tables[created])) {

            return 0;
        }

        ++created;
    } else if ((flags & VMM_FLAG_USER) != 0ULL) {
        user_entries[user_entry_count++] = &pml4[index4];
    }


    pdpt = owned_table(pml4[index4] & PAGE_ADDRESS_MASK_4K);

    if (pdpt == 0) {
        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    if ((pdpt[index3] & PAGE_ENTRY_PRESENT) != 0ULL &&
        (pdpt[index3] & PAGE_ENTRY_LARGE) != 0ULL) {

        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    if ((pdpt[index3] & PAGE_ENTRY_PRESENT) == 0ULL) {
        parent_entries[created] = &pdpt[index3];

        if (!allocate_child_table(
                &pdpt[index3],
                flags,
                &new_tables[created])) {

            rollback_new_tables(parent_entries, new_tables, created);
            return 0;
        }

        ++created;
    } else if ((flags & VMM_FLAG_USER) != 0ULL) {
        user_entries[user_entry_count++] = &pdpt[index3];
    }


    pd = owned_table(pdpt[index3] & PAGE_ADDRESS_MASK_4K);

    if (pd == 0) {
        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    if ((pd[index2] & PAGE_ENTRY_PRESENT) != 0ULL &&
        (pd[index2] & PAGE_ENTRY_LARGE) != 0ULL) {

        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    if ((pd[index2] & PAGE_ENTRY_PRESENT) == 0ULL) {
        parent_entries[created] = &pd[index2];

        if (!allocate_child_table(
                &pd[index2],
                flags,
                &new_tables[created])) {

            rollback_new_tables(parent_entries, new_tables, created);
            return 0;
        }

        ++created;
    } else if ((flags & VMM_FLAG_USER) != 0ULL) {
        user_entries[user_entry_count++] = &pd[index2];
    }


    pt = owned_table(pd[index2] & PAGE_ADDRESS_MASK_4K);

    if (pt == 0) {
        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    if ((pt[index1] & PAGE_ENTRY_PRESENT) != 0ULL) {
        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    pt[index1] =
        physical_address |
        PAGE_ENTRY_PRESENT |
        (
            flags & (
                VMM_FLAG_WRITABLE |
                VMM_FLAG_USER |
                VMM_FLAG_WRITE_THROUGH |
                VMM_FLAG_CACHE_DISABLE |
                VMM_FLAG_GLOBAL |
                VMM_FLAG_NO_EXECUTE
            )
        );


    while (user_entry_count > 0ULL) {
        --user_entry_count;
        *user_entries[user_entry_count] |= PAGE_ENTRY_USER;
    }


    if ((read_cr3() & PAGE_ADDRESS_MASK_4K) == root_table) {
        invalidate_page(virtual_address);
    }

    return 1;
}




int map_page(
    vaddr_t virtual_address,
    paddr_t physical_address,
    uint64_t flags
)
{
    return map_page_in_root(
        vmm.root_table,
        virtual_address,
        physical_address,
        flags
    );
}

int vmm_map_page_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address,
    paddr_t physical_address,
    uint64_t flags
)
{
    if (!vmm.initialized || root_table == PADDR_INVALID) {
        return 0;
    }

    /* Non-kernel roots may only receive user-accessible lower-half mappings. */
    if (root_table != vmm.root_table) {
        if (virtual_address >= 0x0000800000000000ULL ||
            (flags & VMM_FLAG_USER) == 0ULL) {
            return 0;
        }
    }

    return map_page_in_root(
        root_table,
        virtual_address,
        physical_address,
        flags
    );
}

paddr_t vmm_create_user_address_space(void)
{
    uint64_t *kernel_root;
    uint64_t *user_root;
    paddr_t root_physical;
    uint64_t index;

    if (!vmm.initialized) {
        return PADDR_INVALID;
    }

    root_physical = pmm_alloc_page();
    if (root_physical == PADDR_INVALID) {
        return PADDR_INVALID;
    }

    user_root = owned_table(root_physical);
    kernel_root = owned_table(vmm.root_table);

    if (user_root == 0 || kernel_root == 0) {
        (void)pmm_free_page(root_physical);
        return PADDR_INVALID;
    }

    clear_table(user_root);

    /*
     * Keep the lower canonical half empty for the process. The upper half is
     * shared with the kernel, but PML4 USER is forcibly clear so Ring 3 cannot
     * traverse into any kernel/HHDM/heap/MMIO mapping.
     */
    for (index = 256ULL; index < PAGE_TABLE_ENTRIES; ++index) {
        user_root[index] = kernel_root[index] & ~PAGE_ENTRY_USER;
    }

    ++vmm.page_table_pages;
    return root_physical;
}


static void free_user_table_tree(paddr_t table_physical, unsigned level)
{
    uint64_t *table = owned_table(table_physical);
    uint64_t index;

    if (table == 0) {
        return;
    }

    if (level > 1U) {
        for (index = 0ULL; index < PAGE_TABLE_ENTRIES; ++index) {
            const uint64_t entry = table[index];

            if ((entry & PAGE_ENTRY_PRESENT) == 0ULL ||
                entry_is_large(entry, level)) {
                continue;
            }

            free_user_table_tree(entry & PAGE_ADDRESS_MASK_4K, level - 1U);
        }
    }

    if (pmm_free_page(table_physical) && vmm.page_table_pages > 0ULL) {
        --vmm.page_table_pages;
    }
}

int vmm_destroy_user_address_space(paddr_t root_table)
{
    uint64_t *root;
    uint64_t index;

    if (!vmm.initialized || root_table == PADDR_INVALID ||
        root_table == vmm.root_table ||
        vmm_current_address_space() == root_table) {
        return 0;
    }

    root = owned_table(root_table);
    if (root == 0) {
        return 0;
    }

    /* Only lower-half structures belong exclusively to this user space. */
    for (index = 0ULL; index < 256ULL; ++index) {
        const uint64_t entry = root[index];

        if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
            continue;
        }

        if ((entry & PAGE_ENTRY_LARGE) != 0ULL) {
            continue;
        }

        free_user_table_tree(entry & PAGE_ADDRESS_MASK_4K, 3U);
        root[index] = 0ULL;
    }

    if (!pmm_free_page(root_table)) {
        return 0;
    }

    if (vmm.page_table_pages > 0ULL) {
        --vmm.page_table_pages;
    }

    return 1;
}

int vmm_activate_address_space(paddr_t root_table)
{
    if (!vmm.initialized || root_table == PADDR_INVALID ||
        !is_page_aligned(root_table) || owned_table(root_table) == 0) {
        return 0;
    }

    write_cr3(root_table);
    return (read_cr3() & PAGE_ADDRESS_MASK_4K) == root_table;
}

paddr_t vmm_current_address_space(void)
{
    if (!vmm.initialized) {
        return PADDR_INVALID;
    }

    return read_cr3() & PAGE_ADDRESS_MASK_4K;
}

paddr_t vmm_kernel_address_space(void)
{
    return vmm.initialized ? vmm.root_table : PADDR_INVALID;
}

paddr_t virt_to_phys(vaddr_t virtual_address)
{
    return virt_to_phys_in_root(vmm.root_table, virtual_address);
}

paddr_t vmm_virt_to_phys_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address
)
{
    if (!vmm.initialized || root_table == PADDR_INVALID) {
        return PADDR_INVALID;
    }

    return virt_to_phys_in_root(root_table, virtual_address);
}

static int unmap_page_in_root(paddr_t root_table, vaddr_t virtual_address)
{
    uint64_t *pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;
    paddr_t pdpt_physical;
    paddr_t pd_physical;
    paddr_t pt_physical;
    const uint64_t index4 = pml4_index(virtual_address);
    const uint64_t index3 = pdpt_index(virtual_address);
    const uint64_t index2 = pd_index(virtual_address);
    const uint64_t index1 = pt_index(virtual_address);

    if (!vmm.initialized || root_table == PADDR_INVALID ||
        !is_page_aligned(virtual_address) || !is_canonical(virtual_address)) {
        return 0;
    }

    pml4 = owned_table(root_table);
    if (pml4 == 0 || (pml4[index4] & PAGE_ENTRY_PRESENT) == 0ULL) return 0;
    pdpt_physical = pml4[index4] & PAGE_ADDRESS_MASK_4K;
    pdpt = owned_table(pdpt_physical);
    if (pdpt == 0 || (pdpt[index3] & PAGE_ENTRY_PRESENT) == 0ULL ||
        (pdpt[index3] & PAGE_ENTRY_LARGE) != 0ULL) return 0;
    pd_physical = pdpt[index3] & PAGE_ADDRESS_MASK_4K;
    pd = owned_table(pd_physical);
    if (pd == 0 || (pd[index2] & PAGE_ENTRY_PRESENT) == 0ULL ||
        (pd[index2] & PAGE_ENTRY_LARGE) != 0ULL) return 0;
    pt_physical = pd[index2] & PAGE_ADDRESS_MASK_4K;
    pt = owned_table(pt_physical);
    if (pt == 0 || (pt[index1] & PAGE_ENTRY_PRESENT) == 0ULL) return 0;

    pt[index1] = 0ULL;
    if (vmm_current_address_space() == root_table) invalidate_page(virtual_address);

    if (table_is_empty(pt)) {
        pd[index2] = 0ULL;
        if (pmm_free_page(pt_physical) && vmm.page_table_pages > 0ULL) --vmm.page_table_pages;
        if (table_is_empty(pd)) {
            pdpt[index3] = 0ULL;
            if (pmm_free_page(pd_physical) && vmm.page_table_pages > 0ULL) --vmm.page_table_pages;
            if (table_is_empty(pdpt)) {
                pml4[index4] = 0ULL;
                if (pmm_free_page(pdpt_physical) && vmm.page_table_pages > 0ULL) --vmm.page_table_pages;
            }
        }
    }
    return 1;
}

int unmap_page(vaddr_t virtual_address)
{
    return unmap_page_in_root(vmm.root_table, virtual_address);
}

int vmm_unmap_page_in_address_space(paddr_t root_table, vaddr_t virtual_address)
{
    if (root_table != vmm.root_table && virtual_address >= 0x0000800000000000ULL) return 0;
    return unmap_page_in_root(root_table, virtual_address);
}


static paddr_t virt_to_phys_in_root(paddr_t root_table, vaddr_t virtual_address)
{
    uint64_t *pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;

    uint64_t entry;


    if (!vmm.initialized || !is_canonical(virtual_address)) {
        return PADDR_INVALID;
    }


    pml4 = owned_table(root_table);

    if (pml4 == 0) {
        return PADDR_INVALID;
    }


    entry = pml4[pml4_index(virtual_address)];

    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return PADDR_INVALID;
    }


    pdpt = owned_table(entry & PAGE_ADDRESS_MASK_4K);

    if (pdpt == 0) {
        return PADDR_INVALID;
    }


    entry = pdpt[pdpt_index(virtual_address)];

    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return PADDR_INVALID;
    }


    if ((entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return
            (entry & PAGE_ADDRESS_MASK_1G) |
            (virtual_address & PAGE_OFFSET_MASK_1G);
    }


    pd = owned_table(entry & PAGE_ADDRESS_MASK_4K);

    if (pd == 0) {
        return PADDR_INVALID;
    }


    entry = pd[pd_index(virtual_address)];

    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return PADDR_INVALID;
    }


    if ((entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return
            (entry & PAGE_ADDRESS_MASK_2M) |
            (virtual_address & PAGE_OFFSET_MASK_2M);
    }


    pt = owned_table(entry & PAGE_ADDRESS_MASK_4K);

    if (pt == 0) {
        return PADDR_INVALID;
    }


    entry = pt[pt_index(virtual_address)];

    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return PADDR_INVALID;
    }


    return
        (entry & PAGE_ADDRESS_MASK_4K) |
        (virtual_address & PAGE_OFFSET_MASK_4K);
}


static int user_mapping_info(
    paddr_t root_table,
    vaddr_t virtual_address,
    int write_access,
    paddr_t *physical_out
)
{
    uint64_t *pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;
    uint64_t entry;

    if (!vmm.initialized || root_table == PADDR_INVALID ||
        virtual_address >= 0x0000800000000000ULL ||
        !is_canonical(virtual_address) || physical_out == 0) {
        return 0;
    }

    pml4 = owned_table(root_table);
    if (pml4 == 0) {
        return 0;
    }

    entry = pml4[pml4_index(virtual_address)];
    if ((entry & (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER)) !=
        (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER) ||
        (write_access && (entry & PAGE_ENTRY_WRITABLE) == 0ULL)) {
        return 0;
    }

    pdpt = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pdpt == 0) {
        return 0;
    }

    entry = pdpt[pdpt_index(virtual_address)];
    if ((entry & (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER)) !=
        (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER) ||
        (write_access && (entry & PAGE_ENTRY_WRITABLE) == 0ULL) ||
        (entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return 0;
    }

    pd = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pd == 0) {
        return 0;
    }

    entry = pd[pd_index(virtual_address)];
    if ((entry & (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER)) !=
        (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER) ||
        (write_access && (entry & PAGE_ENTRY_WRITABLE) == 0ULL) ||
        (entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return 0;
    }

    pt = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pt == 0) {
        return 0;
    }

    entry = pt[pt_index(virtual_address)];
    if ((entry & (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER)) !=
        (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER) ||
        (write_access && (entry & PAGE_ENTRY_WRITABLE) == 0ULL)) {
        return 0;
    }

    *physical_out =
        (entry & PAGE_ADDRESS_MASK_4K) |
        (virtual_address & PAGE_OFFSET_MASK_4K);
    return 1;
}

int vmm_user_range_accessible(
    paddr_t root_table,
    vaddr_t user_address,
    size_t length,
    int write_access
)
{
    size_t checked = 0u;

    if (length == 0u) {
        return 1;
    }

    if (user_address >= 0x0000800000000000ULL ||
        UINT64_MAX - user_address < (uint64_t)(length - 1u)) {
        return 0;
    }

    while (checked < length) {
        const vaddr_t address = user_address + (vaddr_t)checked;
        const size_t page_remaining =
            (size_t)(VMM_PAGE_SIZE - (address & PAGE_OFFSET_MASK_4K));
        const size_t chunk =
            (length - checked < page_remaining) ?
            (length - checked) : page_remaining;
        paddr_t physical;

        if (!user_mapping_info(root_table, address, write_access, &physical)) {
            return 0;
        }

        checked += chunk;
    }

    return 1;
}

int vmm_copy_from_user(
    paddr_t root_table,
    void *destination,
    vaddr_t user_source,
    size_t length
)
{
    uint8_t *out = (uint8_t *)destination;
    size_t copied = 0u;

    if ((length != 0u && destination == 0) ||
        !vmm_user_range_accessible(root_table, user_source, length, 0)) {
        return 0;
    }

    while (copied < length) {
        const vaddr_t address = user_source + (vaddr_t)copied;
        const size_t page_remaining =
            (size_t)(VMM_PAGE_SIZE - (address & PAGE_OFFSET_MASK_4K));
        const size_t chunk =
            (length - copied < page_remaining) ?
            (length - copied) : page_remaining;
        paddr_t physical;
        const uint8_t *source;
        size_t index;

        if (!user_mapping_info(root_table, address, 0, &physical)) {
            return 0;
        }

        source = (const uint8_t *)pmm_phys_to_hhdm(physical);
        if (source == 0) {
            return 0;
        }

        for (index = 0u; index < chunk; ++index) {
            out[copied + index] = source[index];
        }

        copied += chunk;
    }

    return 1;
}

int vmm_copy_to_user(
    paddr_t root_table,
    vaddr_t user_destination,
    const void *source,
    size_t length
)
{
    const uint8_t *in = (const uint8_t *)source;
    size_t copied = 0u;

    if ((length != 0u && source == 0) ||
        !vmm_user_range_accessible(root_table, user_destination, length, 1)) {
        return 0;
    }

    while (copied < length) {
        const vaddr_t address = user_destination + (vaddr_t)copied;
        const size_t page_remaining =
            (size_t)(VMM_PAGE_SIZE - (address & PAGE_OFFSET_MASK_4K));
        const size_t chunk =
            (length - copied < page_remaining) ?
            (length - copied) : page_remaining;
        paddr_t physical;
        uint8_t *destination;
        size_t index;

        if (!user_mapping_info(root_table, address, 1, &physical)) {
            return 0;
        }

        destination = (uint8_t *)pmm_phys_to_hhdm(physical);
        if (destination == 0) {
            return 0;
        }

        for (index = 0u; index < chunk; ++index) {
            destination[index] = in[copied + index];
        }

        copied += chunk;
    }

    return 1;
}


int vmm_mapping_flags_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address,
    uint64_t *flags_out
)
{
    uint64_t *pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;
    uint64_t entry;
    uint64_t flags = 0ULL;

    if (!vmm.initialized || root_table == PADDR_INVALID || flags_out == 0 ||
        !is_canonical(virtual_address)) {
        return 0;
    }

    pml4 = owned_table(root_table);
    if (pml4 == 0) {
        return 0;
    }

    entry = pml4[pml4_index(virtual_address)];
    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return 0;
    }
    if ((entry & PAGE_ENTRY_USER) != 0ULL) {
        flags |= VMM_FLAG_USER;
    }
    if ((entry & PAGE_ENTRY_WRITABLE) == 0ULL) {
        /* A read-only parent makes the whole path read-only. */
    }

    pdpt = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pdpt == 0) {
        return 0;
    }

    entry = pdpt[pdpt_index(virtual_address)];
    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL ||
        (entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return 0;
    }
    if ((entry & PAGE_ENTRY_USER) != 0ULL) {
        flags |= VMM_FLAG_USER;
    }

    pd = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pd == 0) {
        return 0;
    }

    entry = pd[pd_index(virtual_address)];
    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL ||
        (entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return 0;
    }
    if ((entry & PAGE_ENTRY_USER) != 0ULL) {
        flags |= VMM_FLAG_USER;
    }

    pt = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pt == 0) {
        return 0;
    }

    entry = pt[pt_index(virtual_address)];
    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return 0;
    }

    flags = entry & (
        VMM_FLAG_WRITABLE |
        VMM_FLAG_USER |
        VMM_FLAG_WRITE_THROUGH |
        VMM_FLAG_CACHE_DISABLE |
        VMM_FLAG_GLOBAL |
        VMM_FLAG_NO_EXECUTE
    );

    *flags_out = flags;
    return 1;
}

struct vmm_stats vmm_get_stats(void)
{
    struct vmm_stats stats;

    stats.root_table = vmm.root_table;
    stats.page_table_pages = vmm.page_table_pages;
    stats.hhdm_offset = vmm.hhdm_offset;

    return stats;
}
