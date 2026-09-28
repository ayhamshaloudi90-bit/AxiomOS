#include <stddef.h>
#include <stdint.h>

#include <axiom/boot/limine.h>
#include <axiom/memory/pmm.h>


static struct {
    uint8_t *usable_bitmap;
    uint8_t *used_bitmap;

    uint64_t bitmap_bytes;
    uint64_t page_count;
    uint64_t hhdm_offset;
    uint64_t scan_hint;

    uint64_t metadata_first_page;
    uint64_t metadata_page_count;

    struct pmm_stats stats;

    int initialized;
} pmm;


static int add_overflow_u64(uint64_t left, uint64_t right, uint64_t *result)
{
    if (UINT64_MAX - left < right) {
        return 1;
    }

    *result = left + right;
    return 0;
}


static int mul_overflow_u64(uint64_t left, uint64_t right, uint64_t *result)
{
    if (left != 0 && right > UINT64_MAX / left) {
        return 1;
    }

    *result = left * right;
    return 0;
}


static int type_is_ram_backed(uint64_t type)
{
    return type == LIMINE_MEMMAP_USABLE ||
           type == LIMINE_MEMMAP_ACPI_RECLAIMABLE ||
           type == LIMINE_MEMMAP_ACPI_NVS ||
           type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE ||
           type == LIMINE_MEMMAP_EXECUTABLE_AND_MODULES ||
           type == LIMINE_MEMMAP_RESERVED_MAPPED;
}


static uint64_t bitmap_byte_index(uint64_t page_index)
{
    return page_index >> 3;
}


static uint8_t bitmap_mask(uint64_t page_index)
{
    return (uint8_t)(1u << (page_index & 7u));
}


static int bitmap_test(const uint8_t *bitmap, uint64_t page_index)
{
    return (bitmap[bitmap_byte_index(page_index)] & bitmap_mask(page_index)) != 0u;
}


static void bitmap_set(uint8_t *bitmap, uint64_t page_index)
{
    bitmap[bitmap_byte_index(page_index)] |= bitmap_mask(page_index);
}


static void bitmap_clear(uint8_t *bitmap, uint64_t page_index)
{
    bitmap[bitmap_byte_index(page_index)] &= (uint8_t)~bitmap_mask(page_index);
}


static uint64_t compute_total_ram_bytes(const struct limine_memmap_response *memory_map)
{
    uint64_t total;
    uint64_t current_start;
    uint64_t current_end;
    uint64_t index;
    int have_range;


    total = 0;
    current_start = 0;
    current_end = 0;
    have_range = 0;


    for (index = 0; index < memory_map->entry_count; ++index) {
        const struct limine_memmap_entry *entry;
        uint64_t end;


        entry = memory_map->entries[index];

        if (entry == 0 || entry->length == 0 || !type_is_ram_backed(entry->type)) {
            continue;
        }

        if (add_overflow_u64(entry->base, entry->length, &end)) {
            return 0;
        }

        if (!have_range) {
            current_start = entry->base;
            current_end = end;
            have_range = 1;
            continue;
        }

        if (entry->base <= current_end) {
            if (end > current_end) {
                current_end = end;
            }
            continue;
        }

        if (UINT64_MAX - total < current_end - current_start) {
            return 0;
        }

        total += current_end - current_start;
        current_start = entry->base;
        current_end = end;
    }


    if (have_range) {
        if (UINT64_MAX - total < current_end - current_start) {
            return 0;
        }

        total += current_end - current_start;
    }


    return total;
}


static uint64_t find_free_page(uint64_t begin, uint64_t end)
{
    uint64_t page_index;


    for (page_index = begin; page_index < end; ++page_index) {
        if (bitmap_test(pmm.usable_bitmap, page_index) &&
            !bitmap_test(pmm.used_bitmap, page_index)) {

            return page_index;
        }
    }


    return UINT64_MAX;
}


int pmm_init(void)
{
    struct limine_memmap_response *memory_map;
    uint64_t hhdm_offset;
    uint64_t highest_usable_end;
    uint64_t usable_pages;
    uint64_t usable_bytes;
    uint64_t bitmap_bytes;
    uint64_t metadata_bytes;
    uint64_t metadata_storage_bytes;
    uint64_t metadata_pages;
    uint64_t metadata_base;
    uint64_t index;


    if (pmm.initialized) {
        return 1;
    }


    if (!limine_get_memory_boot_info(&memory_map, &hhdm_offset)) {
        return 0;
    }


    highest_usable_end = 0;
    usable_pages = 0;
    usable_bytes = 0;


    for (index = 0; index < memory_map->entry_count; ++index) {
        const struct limine_memmap_entry *entry;
        uint64_t end;


        entry = memory_map->entries[index];

        if (entry == 0 || entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        /* Limine guarantees this for usable entries; verify the invariant. */
        if ((entry->base & (PMM_PAGE_SIZE - 1ULL)) != 0 ||
            (entry->length & (PMM_PAGE_SIZE - 1ULL)) != 0) {

            return 0;
        }

        if (add_overflow_u64(entry->base, entry->length, &end)) {
            return 0;
        }

        if (end > highest_usable_end) {
            highest_usable_end = end;
        }

        if (UINT64_MAX - usable_bytes < entry->length) {
            return 0;
        }

        usable_bytes += entry->length;

        if (UINT64_MAX - usable_pages < entry->length / PMM_PAGE_SIZE) {
            return 0;
        }

        usable_pages += entry->length / PMM_PAGE_SIZE;
    }


    if (highest_usable_end == 0 || usable_pages == 0) {
        return 0;
    }


    pmm.page_count = highest_usable_end / PMM_PAGE_SIZE;

    if (pmm.page_count > UINT64_MAX - 7ULL) {
        return 0;
    }

    bitmap_bytes = (pmm.page_count + 7ULL) / 8ULL;

    if (mul_overflow_u64(bitmap_bytes, 2ULL, &metadata_bytes)) {
        return 0;
    }

    if (metadata_bytes > UINT64_MAX - (PMM_PAGE_SIZE - 1ULL)) {
        return 0;
    }

    metadata_pages =
        (metadata_bytes + PMM_PAGE_SIZE - 1ULL) /
        PMM_PAGE_SIZE;

    if (metadata_pages == 0 || metadata_pages >= usable_pages) {
        return 0;
    }

    if (mul_overflow_u64(metadata_pages, PMM_PAGE_SIZE, &metadata_storage_bytes)) {
        return 0;
    }


    metadata_base = PADDR_INVALID;

    for (index = 0; index < memory_map->entry_count; ++index) {
        const struct limine_memmap_entry *entry;


        entry = memory_map->entries[index];

        if (entry != 0 &&
            entry->type == LIMINE_MEMMAP_USABLE &&
            entry->length >= metadata_storage_bytes) {

            metadata_base = entry->base;
            break;
        }
    }


    if (metadata_base == PADDR_INVALID ||
        UINT64_MAX - hhdm_offset < metadata_base) {

        return 0;
    }


    pmm.hhdm_offset = hhdm_offset;
    pmm.bitmap_bytes = bitmap_bytes;
    pmm.metadata_first_page = metadata_base / PMM_PAGE_SIZE;
    pmm.metadata_page_count = metadata_pages;

    pmm.usable_bitmap = (uint8_t *)(uintptr_t)(hhdm_offset + metadata_base);
    pmm.used_bitmap = pmm.usable_bitmap + bitmap_bytes;


    /*
     * Usability starts false everywhere. Used starts true everywhere.
     * That makes holes/reserved regions unallocatable by default.
     */
    for (index = 0; index < bitmap_bytes; ++index) {
        pmm.usable_bitmap[index] = 0x00u;
        pmm.used_bitmap[index] = 0xFFu;
    }


    /* Mark every Limine USABLE page as allocatable and initially free. */
    for (index = 0; index < memory_map->entry_count; ++index) {
        const struct limine_memmap_entry *entry;
        uint64_t first_page;
        uint64_t pages;
        uint64_t page;


        entry = memory_map->entries[index];

        if (entry == 0 || entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        first_page = entry->base / PMM_PAGE_SIZE;
        pages = entry->length / PMM_PAGE_SIZE;

        for (page = 0; page < pages; ++page) {
            uint64_t page_index;


            page_index = first_page + page;
            bitmap_set(pmm.usable_bitmap, page_index);
            bitmap_clear(pmm.used_bitmap, page_index);
        }
    }


    /* The bitmaps themselves occupy physical pages and may never be freed. */
    for (index = 0; index < metadata_pages; ++index) {
        bitmap_set(pmm.used_bitmap, pmm.metadata_first_page + index);
    }


    pmm.scan_hint = 0;

    pmm.stats.memory_map_entries = memory_map->entry_count;
    pmm.stats.total_memory_bytes = compute_total_ram_bytes(memory_map);
    pmm.stats.usable_memory_bytes = usable_bytes;
    pmm.stats.usable_pages = usable_pages;
    pmm.stats.metadata_pages = metadata_pages;
    pmm.stats.allocated_pages = metadata_pages;
    pmm.stats.free_pages = usable_pages - metadata_pages;
    pmm.stats.metadata_base = metadata_base;
    pmm.stats.highest_usable_address = highest_usable_end;


    if (pmm.stats.total_memory_bytes == 0) {
        return 0;
    }


    pmm.initialized = 1;
    return 1;
}


paddr_t pmm_alloc_page(void)
{
    uint64_t page_index;


    if (!pmm.initialized || pmm.stats.free_pages == 0) {
        return PADDR_INVALID;
    }


    page_index = find_free_page(pmm.scan_hint, pmm.page_count);

    if (page_index == UINT64_MAX && pmm.scan_hint != 0) {
        page_index = find_free_page(0, pmm.scan_hint);
    }


    if (page_index == UINT64_MAX) {
        return PADDR_INVALID;
    }


    bitmap_set(pmm.used_bitmap, page_index);

    --pmm.stats.free_pages;
    ++pmm.stats.allocated_pages;

    pmm.scan_hint = page_index + 1ULL;

    if (pmm.scan_hint >= pmm.page_count) {
        pmm.scan_hint = 0;
    }


    return page_index * PMM_PAGE_SIZE;
}


int pmm_free_page(paddr_t page)
{
    uint64_t page_index;
    uint64_t metadata_end_page;


    if (!pmm.initialized || page == PADDR_INVALID) {
        return 0;
    }


    if ((page & (PMM_PAGE_SIZE - 1ULL)) != 0) {
        return 0;
    }


    page_index = page / PMM_PAGE_SIZE;

    if (page_index >= pmm.page_count ||
        !bitmap_test(pmm.usable_bitmap, page_index)) {

        return 0;
    }


    metadata_end_page =
        pmm.metadata_first_page +
        pmm.metadata_page_count;


    if (page_index >= pmm.metadata_first_page &&
        page_index < metadata_end_page) {

        return 0;
    }


    /* A clear used bit means this page is already free. */
    if (!bitmap_test(pmm.used_bitmap, page_index)) {
        return 0;
    }


    if (pmm.stats.allocated_pages <= pmm.stats.metadata_pages) {
        return 0;
    }


    bitmap_clear(pmm.used_bitmap, page_index);

    --pmm.stats.allocated_pages;
    ++pmm.stats.free_pages;


    if (page_index < pmm.scan_hint) {
        pmm.scan_hint = page_index;
    }


    return 1;
}


void *pmm_phys_to_hhdm(paddr_t address)
{
    uint64_t page_index;


    if (!pmm.initialized || address == PADDR_INVALID) {
        return 0;
    }


    page_index = address / PMM_PAGE_SIZE;

    if (page_index >= pmm.page_count ||
        !bitmap_test(pmm.usable_bitmap, page_index) ||
        UINT64_MAX - pmm.hhdm_offset < address) {

        return 0;
    }


    return (void *)(uintptr_t)(pmm.hhdm_offset + address);
}


struct pmm_stats pmm_get_stats(void)
{
    return pmm.stats;
}
