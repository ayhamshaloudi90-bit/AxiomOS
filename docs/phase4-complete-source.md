# Phase 4 complete new/modified source
Generated from the Phase-3 tree supplied by Ayham. Phase 4 is not accepted until the local QEMU tests pass.

## `include/axiom/boot/limine.h`
```c
#ifndef AXIOM_BOOT_LIMINE_H
#define AXIOM_BOOT_LIMINE_H

#include <stdint.h>


#define LIMINE_FRAMEBUFFER_RGB 1u

#define LIMINE_MEMMAP_USABLE                 0ULL
#define LIMINE_MEMMAP_RESERVED               1ULL
#define LIMINE_MEMMAP_ACPI_RECLAIMABLE       2ULL
#define LIMINE_MEMMAP_ACPI_NVS               3ULL
#define LIMINE_MEMMAP_BAD_MEMORY             4ULL
#define LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE 5ULL
#define LIMINE_MEMMAP_EXECUTABLE_AND_MODULES 6ULL
#define LIMINE_MEMMAP_FRAMEBUFFER            7ULL
#define LIMINE_MEMMAP_RESERVED_MAPPED        8ULL


struct limine_video_mode {
    uint64_t pitch;
    uint64_t width;
    uint64_t height;

    uint16_t bpp;

    uint8_t memory_model;

    uint8_t red_mask_size;
    uint8_t red_mask_shift;

    uint8_t green_mask_size;
    uint8_t green_mask_shift;

    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
};


struct limine_framebuffer {
    void *address;

    uint64_t width;
    uint64_t height;
    uint64_t pitch;

    uint16_t bpp;

    uint8_t memory_model;

    uint8_t red_mask_size;
    uint8_t red_mask_shift;

    uint8_t green_mask_size;
    uint8_t green_mask_shift;

    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;

    uint8_t unused[7];

    uint64_t edid_size;
    void *edid;

    uint64_t mode_count;
    struct limine_video_mode **modes;
};


struct limine_framebuffer_response {
    uint64_t revision;

    uint64_t framebuffer_count;

    struct limine_framebuffer **framebuffers;
};


struct limine_framebuffer_request {
    uint64_t id[4];

    uint64_t revision;

    struct limine_framebuffer_response *response;
};


struct limine_flanterm_fb_init_params {
    uint32_t *canvas;

    uint64_t canvas_size;

    uint32_t ansi_colours[8];
    uint32_t ansi_bright_colours[8];

    uint32_t default_bg;
    uint32_t default_fg;

    uint32_t default_bg_bright;
    uint32_t default_fg_bright;

    void *font;

    uint64_t font_width;
    uint64_t font_height;
    uint64_t font_spacing;

    uint64_t font_scale_x;
    uint64_t font_scale_y;

    uint64_t margin;
    uint64_t rotation;
};


struct limine_flanterm_fb_init_params_response {
    uint64_t revision;

    uint64_t entry_count;

    struct limine_flanterm_fb_init_params **entries;
};


struct limine_flanterm_fb_init_params_request {
    uint64_t id[4];

    uint64_t revision;

    struct limine_flanterm_fb_init_params_response *response;
};


struct limine_memmap_entry {
    uint64_t base;
    uint64_t length;
    uint64_t type;
};


struct limine_memmap_response {
    uint64_t revision;
    uint64_t entry_count;
    struct limine_memmap_entry **entries;
};


struct limine_memmap_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_memmap_response *response;
};


struct limine_hhdm_response {
    uint64_t revision;
    uint64_t offset;
};


struct limine_hhdm_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_hhdm_response *response;
};


int limine_base_revision_supported(void);


int limine_get_terminal_boot_info(
    struct limine_framebuffer **framebuffer,
    struct limine_flanterm_fb_init_params **font_params
);


int limine_get_memory_boot_info(
    struct limine_memmap_response **memory_map,
    uint64_t *hhdm_offset
);


#endif

```

## `arch/x86_64/boot/limine_requests.c`
```c
#include <stdint.h>

#include <axiom/boot/limine.h>


#define LIMINE_COMMON_MAGIC_0 0xc7b1dd30df4c8b88ULL
#define LIMINE_COMMON_MAGIC_1 0x0a82e883a194f07bULL


/*
 * Limine request-area start marker.
 */
__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[4] = {
    0xf6b8f4b39de7d1aeULL,
    0xfab91a6940fcb9cfULL,
    0x785c6ed015d3e316ULL,
    0x181e920a7852b9d9ULL
};


/*
 * AxiomOS currently targets Limine base revision 6.
 */
__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[3] = {
    0xf9562b2d5c95a6c8ULL,
    0x6a7b384944536bdcULL,
    6ULL
};


/*
 * Ask Limine for a graphical framebuffer.
 */
__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = {
        LIMINE_COMMON_MAGIC_0,
        LIMINE_COMMON_MAGIC_1,
        0x9d5827dcd881dd75ULL,
        0xa3148604f6fab11bULL
    },

    .revision = 0,

    .response = 0
};


/*
 * Ask Limine for the VGA-style bitmap font used by its framebuffer terminal.
 * AxiomOS only consumes the bitmap; rendering remains our responsibility.
 */
__attribute__((used, section(".limine_requests")))
static volatile struct limine_flanterm_fb_init_params_request
    flanterm_params_request = {
        .id = {
            LIMINE_COMMON_MAGIC_0,
            LIMINE_COMMON_MAGIC_1,
            0x3259399fe7c5f126ULL,
            0xe01c1c8c5db9d1a9ULL
        },

        .revision = 0,

        .response = 0
    };


/*
 * Phase 4: request the firmware/bootloader physical-memory map.
 */
__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = {
        LIMINE_COMMON_MAGIC_0,
        LIMINE_COMMON_MAGIC_1,
        0x67cf3d9d378a806fULL,
        0xe304acdfc50c3c62ULL
    },

    .revision = 0,

    .response = 0
};


/*
 * Phase 4: discover the Higher Half Direct Map offset. This lets the PMM
 * access RAM described by physical addresses while Limine's page tables are
 * still active.
 */
__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = {
        LIMINE_COMMON_MAGIC_0,
        LIMINE_COMMON_MAGIC_1,
        0x48dcf1cb8ad2b852ULL,
        0x63984e959a98244bULL
    },

    .revision = 0,

    .response = 0
};


/*
 * Limine request-area end marker.
 */
__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[2] = {
    0xadc0e0531bb10d03ULL,
    0x9572709f31764c62ULL
};


int limine_base_revision_supported(void)
{
    return limine_base_revision[2] == 0ULL;
}


int limine_get_terminal_boot_info(
    struct limine_framebuffer **framebuffer,
    struct limine_flanterm_fb_init_params **font_params
)
{
    struct limine_framebuffer_response *framebuffer_response;
    struct limine_flanterm_fb_init_params_response *font_response;


    if (framebuffer == 0 || font_params == 0) {
        return 0;
    }


    framebuffer_response = framebuffer_request.response;
    font_response = flanterm_params_request.response;


    if (framebuffer_response == 0 || font_response == 0) {
        return 0;
    }


    if (framebuffer_response->framebuffer_count == 0 ||
        framebuffer_response->framebuffers == 0) {

        return 0;
    }


    if (font_response->entry_count == 0 ||
        font_response->entries == 0) {

        return 0;
    }


    if (framebuffer_response->framebuffers[0] == 0 ||
        font_response->entries[0] == 0) {

        return 0;
    }


    *framebuffer = framebuffer_response->framebuffers[0];
    *font_params = font_response->entries[0];


    return 1;
}


int limine_get_memory_boot_info(
    struct limine_memmap_response **memory_map,
    uint64_t *hhdm_offset
)
{
    struct limine_memmap_response *map_response;
    struct limine_hhdm_response *hhdm_response;


    if (memory_map == 0 || hhdm_offset == 0) {
        return 0;
    }


    map_response = memmap_request.response;
    hhdm_response = hhdm_request.response;


    if (map_response == 0 || hhdm_response == 0) {
        return 0;
    }


    if (map_response->entry_count == 0 || map_response->entries == 0) {
        return 0;
    }


    *memory_map = map_response;
    *hhdm_offset = hhdm_response->offset;


    return 1;
}

```

## `include/axiom/memory/address.h`
```c
#ifndef AXIOM_MEMORY_ADDRESS_H
#define AXIOM_MEMORY_ADDRESS_H

#include <stdint.h>

/*
 * Keep physical and virtual addresses semantically distinct even though
 * x86-64 represents both with 64-bit integers.
 */
typedef uint64_t paddr_t;
typedef uint64_t vaddr_t;

#define PADDR_INVALID UINT64_MAX

#endif

```

## `include/axiom/memory/pmm.h`
```c
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

```

## `memory/pmm.c`
```c
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

```

## `memory/pmm_selftest.c`
```c
#include <stdint.h>

#include <axiom/memory/pmm.h>
#include <axiom/terminal/kprintf.h>


static int selftest_fail(const char *reason)
{
    kprintf("Phase 4 PMM allocation/free test: FAILED (%s)\n", reason);
    return 0;
}


int phase4_pmm_selftest(void)
{
    struct pmm_stats before;
    struct pmm_stats during;
    struct pmm_stats after;

    paddr_t first;
    paddr_t second;
    paddr_t third;

    volatile uint64_t *first_page;
    volatile uint64_t *second_page;
    volatile uint64_t *third_page;

    const uint64_t words_per_page = PMM_PAGE_SIZE / sizeof(uint64_t);


    before = pmm_get_stats();


    first = pmm_alloc_page();
    second = pmm_alloc_page();
    third = pmm_alloc_page();


    if (first == PADDR_INVALID ||
        second == PADDR_INVALID ||
        third == PADDR_INVALID) {

        return selftest_fail("allocation returned no page");
    }


    if ((first & (PMM_PAGE_SIZE - 1ULL)) != 0 ||
        (second & (PMM_PAGE_SIZE - 1ULL)) != 0 ||
        (third & (PMM_PAGE_SIZE - 1ULL)) != 0) {

        return selftest_fail("unaligned physical page");
    }


    if (first == second || first == third || second == third) {
        return selftest_fail("duplicate allocation");
    }


    during = pmm_get_stats();

    if (during.allocated_pages != before.allocated_pages + 3ULL ||
        during.free_pages + 3ULL != before.free_pages) {

        return selftest_fail("accounting after allocation");
    }


    first_page = (volatile uint64_t *)pmm_phys_to_hhdm(first);
    second_page = (volatile uint64_t *)pmm_phys_to_hhdm(second);
    third_page = (volatile uint64_t *)pmm_phys_to_hhdm(third);


    if (first_page == 0 || second_page == 0 || third_page == 0) {
        return selftest_fail("HHDM conversion");
    }


    first_page[0] = 0xA110C001A110C001ULL;
    first_page[words_per_page - 1ULL] = 0xA110C001FFFFFFFFULL;

    second_page[0] = 0xB220C002B220C002ULL;
    second_page[words_per_page - 1ULL] = 0xB220C002FFFFFFFFULL;

    third_page[0] = 0xC330C003C330C003ULL;
    third_page[words_per_page - 1ULL] = 0xC330C003FFFFFFFFULL;


    if (first_page[0] != 0xA110C001A110C001ULL ||
        first_page[words_per_page - 1ULL] != 0xA110C001FFFFFFFFULL ||
        second_page[0] != 0xB220C002B220C002ULL ||
        second_page[words_per_page - 1ULL] != 0xB220C002FFFFFFFFULL ||
        third_page[0] != 0xC330C003C330C003ULL ||
        third_page[words_per_page - 1ULL] != 0xC330C003FFFFFFFFULL) {

        return selftest_fail("page read/write verification");
    }


    if (pmm_free_page(first + 1ULL)) {
        return selftest_fail("accepted unaligned free");
    }


    if (!pmm_free_page(second) ||
        !pmm_free_page(first) ||
        !pmm_free_page(third)) {

        return selftest_fail("valid free rejected");
    }


    if (pmm_free_page(third)) {
        return selftest_fail("double free accepted");
    }


    after = pmm_get_stats();

    if (after.allocated_pages != before.allocated_pages ||
        after.free_pages != before.free_pages) {

        return selftest_fail("accounting after free");
    }


    kprintf("Phase 4 PMM allocation/free test: OK\n");
    return 1;
}

```

## `kernel/core/kernel.c`
```c
#include <stdint.h>

#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>

#include <axiom/boot/limine.h>
#include <axiom/drivers/serial.h>
#include <axiom/memory/pmm.h>

#include <axiom/terminal/kprintf.h>
#include <axiom/terminal/terminal.h>


void kernel_main(void)
{
    uint64_t line;
    uint64_t scroll_lines;
    struct pmm_stats memory_stats;


    /*
     * Phase-1 early debugging output remains available.
     */
    serial_init();


    if (!limine_base_revision_supported()) {
        serial_write_string(
            "AxiomOS boot error: "
            "unsupported Limine protocol revision.\n"
        );

        return;
    }


    /*
     * Keep this exact line so the Phase-1 regression test continues to work.
     */
    serial_write_string(
        "AxiomOS kernel booted successfully.\n"
    );


    /*
     * Initialise the Phase-2 framebuffer terminal.
     */
    if (!terminal_init()) {
        serial_write_string(
            "AxiomOS terminal error: "
            "framebuffer terminal unavailable.\n"
        );

        return;
    }


    /*
     * Exercise scrolling by printing slightly more than one screen of text.
     */
    scroll_lines =
        (uint64_t)terminal_rows()
        +
        3ULL;


    for (line = 1; line <= scroll_lines; ++line) {
        kprintf(
            "Scroll exercise line %llu\n",
            (unsigned long long)line
        );
    }


    terminal_clear();


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );


    kprintf(
        "AxiomOS Phase 2 terminal online.\n"
    );


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


    kprintf(
        "Terminal geometry: %llux%llu cells\n",
        (unsigned long long)terminal_columns(),
        (unsigned long long)terminal_rows()
    );


    kprintf(
        "Decimal test: %u\n",
        123456789u
    );


    kprintf(
        "Hex test: 0x%X\n",
        0xDEADBEEFu
    );


    kprintf(
        "64-bit test: %llu\n",
        18446744073709551615ULL
    );


    kprintf(
        "Kernel entry: %p\n",
        (void *)(uintptr_t)&kernel_main
    );


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );


    kprintf(
        "Color output: OK\n"
    );


    terminal_set_color(
        TERMINAL_COLOR_YELLOW,
        TERMINAL_COLOR_BLACK
    );


    kprintf(
        "Phase 2 terminal test complete.\n"
    );


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


    /*
     * Phase 3 CPU architecture initialization and regression tests.
     */
    gdt_init();
    interrupts_init();

    kprintf("GDT/TSS loaded. IDT: 256 gates installed.\n");
    kprintf("PIC remapped: IRQs masked; IF=0.\n");

    phase3_selftest();


    /*
     * Phase 4: discover usable physical RAM and initialise the 4 KiB page
     * allocator. Only LIMINE_MEMMAP_USABLE pages are managed at this stage.
     */
    if (!pmm_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        kprintf("Phase 4 PMM initialization: FAILED\n");
        return;
    }


    memory_stats = pmm_get_stats();


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );

    kprintf("AxiomOS Phase 4 physical memory manager online.\n");


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    kprintf(
        "PMM memory map entries: %llu\n",
        (unsigned long long)memory_stats.memory_map_entries
    );

    kprintf(
        "PMM total RAM-like memory: %llu MiB\n",
        (unsigned long long)(
            memory_stats.total_memory_bytes /
            (1024ULL * 1024ULL)
        )
    );

    kprintf(
        "PMM usable memory: %llu MiB (%llu pages)\n",
        (unsigned long long)(
            memory_stats.usable_memory_bytes /
            (1024ULL * 1024ULL)
        ),
        (unsigned long long)memory_stats.usable_pages
    );

    kprintf(
        "PMM metadata: %llu pages at physical 0x%llX\n",
        (unsigned long long)memory_stats.metadata_pages,
        (unsigned long long)memory_stats.metadata_base
    );

    kprintf(
        "PMM allocated pages: %llu\n",
        (unsigned long long)memory_stats.allocated_pages
    );

    kprintf(
        "PMM free pages: %llu\n",
        (unsigned long long)memory_stats.free_pages
    );


    if (!phase4_pmm_selftest()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        return;
    }


    memory_stats = pmm_get_stats();

    kprintf(
        "PMM post-test allocated pages: %llu\n",
        (unsigned long long)memory_stats.allocated_pages
    );

    kprintf(
        "PMM post-test free pages: %llu\n",
        (unsigned long long)memory_stats.free_pages
    );


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );

    kprintf("Phase 4 physical memory manager complete.\n");


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );
}

```

## `tests/phase4_pmm.py`
```python
#!/usr/bin/env python3
"""Boot the normal AxiomOS image and validate the Phase-4 physical allocator."""

from pathlib import Path
import re
import subprocess
import time


ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
ISO = BUILD / "AxiomOS.iso"
SERIAL = BUILD / "phase4-serial.log"
QEMU_LOG = BUILD / "phase4-qemu.log"


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def extract_u64(text, label):
    match = re.search(rf"^{re.escape(label)}: (\d+)$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


def main():
    subprocess.run(["make", "MODE=normal", "all"], cwd=ROOT, check=True)

    SERIAL.write_text("")

    command = [
        "qemu-system-x86_64",
        "-machine", "q35",
        "-accel", "tcg",
        "-m", "256M",
        "-cdrom", str(ISO),
        "-boot", "d",
        "-display", "none",
        "-serial", f"file:{SERIAL}",
        "-monitor", "none",
        "-no-reboot",
        "-no-shutdown",
    ]

    with QEMU_LOG.open("w") as output:
        process = subprocess.Popen(command, stdout=output, stderr=subprocess.STDOUT)

        try:
            deadline = time.monotonic() + 20
            marker = "Phase 4 physical memory manager complete."

            while marker not in SERIAL.read_text(errors="replace"):
                require(process.poll() is None, "QEMU exited before Phase 4 completed")
                require(time.monotonic() < deadline,
                        f"Phase 4 boot timed out; see {SERIAL}")
                time.sleep(0.05)
        finally:
            if process.poll() is None:
                process.terminate()

            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()

    text = SERIAL.read_text(errors="replace").replace("\r", "")

    for expected in [
        "AxiomOS kernel booted successfully.",
        "Phase 2 terminal test complete.",
        "Phase 3 CPU initialization complete.",
        "AxiomOS Phase 4 physical memory manager online.",
        "Phase 4 PMM allocation/free test: OK",
        "Phase 4 physical memory manager complete.",
    ]:
        require(expected in text, f"Missing output: {expected}")

    require("FAILED" not in text, "Kernel reported a Phase 4 failure")
    require("KERNEL PANIC" not in text, "Normal Phase 4 boot panicked")

    usable_match = re.search(
        r"^PMM usable memory: (\d+) MiB \((\d+) pages\)$",
        text,
        re.MULTILINE,
    )
    require(usable_match is not None, "Missing PMM usable-memory diagnostics")

    usable_mib = int(usable_match.group(1))
    usable_pages = int(usable_match.group(2))
    metadata_match = re.search(
        r"^PMM metadata: (\d+) pages at physical 0x([0-9A-F]+)$",
        text,
        re.MULTILINE,
    )
    require(metadata_match is not None, "Missing PMM metadata diagnostics")
    metadata_pages = int(metadata_match.group(1))
    metadata_base = int(metadata_match.group(2), 16)

    allocated = extract_u64(text, "PMM allocated pages")
    free = extract_u64(text, "PMM free pages")
    post_allocated = extract_u64(text, "PMM post-test allocated pages")
    post_free = extract_u64(text, "PMM post-test free pages")

    require(usable_pages > 0 and usable_mib > 0, "No usable RAM reported")
    require(metadata_pages > 0, "Allocator reserved no bitmap pages")
    require(metadata_base % 4096 == 0, "PMM metadata is not page aligned")
    require(allocated == metadata_pages,
            "Initial allocated count should equal allocator metadata pages")
    require(allocated + free == usable_pages,
            "Allocated + free pages does not equal usable pages")
    require(post_allocated == allocated and post_free == free,
            "Self-test did not restore allocator accounting")

    print("PASS: Phase 4 physical memory manager")
    print(f"  usable pages:    {usable_pages}")
    print(f"  metadata pages:  {metadata_pages}")
    print(f"  allocated pages: {allocated}")
    print(f"  free pages:      {free}")


if __name__ == "__main__":
    main()

```

## `Makefile`
```makefile
SHELL := /bin/bash


PROJECT := AxiomOS

MODE ?= normal
BUILD_DIR := build$(if $(filter-out normal,$(MODE)),-$(MODE))
VALID_MODES := normal divide invalid gp double_fault
ifeq ($(filter $(MODE),$(VALID_MODES)),)
$(error Invalid MODE: $(MODE))
endif
OBJ_DIR := $(BUILD_DIR)/obj
ISO_ROOT := $(BUILD_DIR)/iso_root

KERNEL_ELF := $(BUILD_DIR)/$(PROJECT).elf
ISO_IMAGE := $(BUILD_DIR)/$(PROJECT).iso


CC := clang
LD := ld.lld
ASM := nasm

QEMU := qemu-system-x86_64
GDB := gdb

READELF := llvm-readelf
OBJDUMP := llvm-objdump


LIMINE_VERSION := v12.9.0
LIMINE_DIR := third_party/limine
LIMINE_ARCHIVE := $(BUILD_DIR)/limine-binary.tar.gz

LIMINE_URL := \
	https://github.com/Limine-Bootloader/Limine/releases/download/$(LIMINE_VERSION)/limine-binary.tar.gz


CFLAGS := \
	--target=x86_64-unknown-none-elf \
	-std=gnu11 \
	-Wall \
	-Wextra \
	-Werror \
	-O2 \
	-g \
	-ffreestanding \
	-fno-stack-protector \
	-fno-stack-check \
	-fno-pic \
	-fno-pie \
	-fno-lto \
	-ffunction-sections \
	-fdata-sections \
	-m64 \
	-march=x86-64 \
	-mabi=sysv \
	-mno-red-zone \
	-mno-80387 \
	-mno-mmx \
	-mno-sse \
	-mno-sse2 \
	-mcmodel=kernel


CPPFLAGS := \
	-Iinclude

ifneq ($(MODE),normal)
CPPFLAGS += -DAXIOM_TEST_$(shell echo $(MODE) | tr a-z A-Z)
endif


ASMFLAGS := \
	-f elf64 \
	-g \
	-F dwarf \
	-Wall


LDFLAGS := \
	-m elf_x86_64 \
	-nostdlib \
	-static \
	-z max-page-size=0x1000 \
	-z noexecstack \
	--gc-sections \
	-T linker/x86_64.ld


C_SOURCES := \
	arch/x86_64/boot/limine_requests.c \
	drivers/serial/serial.c \
	kernel/terminal/terminal.c \
	kernel/terminal/kprintf.c \
	kernel/core/kernel.c \
	kernel/core/panic.c \
	memory/pmm.c \
	memory/pmm_selftest.c \
	arch/x86_64/cpu/gdt.c \
	arch/x86_64/cpu/selftest.c \
	arch/x86_64/interrupts/idt.c \
	arch/x86_64/interrupts/pic.c


ASM_SOURCES := \
	arch/x86_64/boot/entry.asm \
	arch/x86_64/cpu/gdt_load.asm \
	arch/x86_64/cpu/register_probe.asm \
	arch/x86_64/interrupts/isr_stubs.asm


C_OBJECTS := \
	$(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SOURCES))


ASM_OBJECTS := \
	$(patsubst %.asm,$(OBJ_DIR)/%.o,$(ASM_SOURCES))


OBJECTS := \
	$(ASM_OBJECTS) \
	$(C_OBJECTS)


.DEFAULT_GOAL := all


.PHONY: \
	all \
	deps \
	run \
	debug \
	test \
	inspect \
	clean \
	distclean \
	help


all: $(ISO_IMAGE)


deps: $(LIMINE_DIR)/limine


$(LIMINE_DIR)/limine:
	@echo "Fetching Limine $(LIMINE_VERSION)..."

	mkdir -p $(BUILD_DIR)

	curl \
		-fL \
		$(LIMINE_URL) \
		-o $(LIMINE_ARCHIVE)

	rm -rf $(LIMINE_DIR)

	mkdir -p $(LIMINE_DIR)

	tar \
		-xzf $(LIMINE_ARCHIVE) \
		-C $(LIMINE_DIR) \
		--strip-components=1

	$(MAKE) -C $(LIMINE_DIR)


$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)

	$(CC) \
		$(CFLAGS) \
		$(CPPFLAGS) \
		-MMD -MP \
		-c $< \
		-o $@


$(OBJ_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)

	$(ASM) \
		$(ASMFLAGS) \
		$< \
		-o $@


$(KERNEL_ELF): $(OBJECTS) linker/x86_64.ld
	@mkdir -p $(BUILD_DIR)

	$(LD) \
		$(LDFLAGS) \
		$(OBJECTS) \
		-o $(KERNEL_ELF)

	@echo
	@echo "Built kernel:"
	@echo "  $(KERNEL_ELF)"


$(ISO_IMAGE): \
	$(KERNEL_ELF) \
	boot/limine/limine.conf \
	$(LIMINE_DIR)/limine

	rm -rf $(ISO_ROOT)

	mkdir -p $(ISO_ROOT)/boot/limine
	mkdir -p $(ISO_ROOT)/EFI/BOOT


	cp \
		$(KERNEL_ELF) \
		$(ISO_ROOT)/boot/$(PROJECT).elf


	cp \
		boot/limine/limine.conf \
		$(ISO_ROOT)/boot/limine/


	cp \
		$(LIMINE_DIR)/limine-bios.sys \
		$(LIMINE_DIR)/limine-bios-cd.bin \
		$(LIMINE_DIR)/limine-uefi-cd.bin \
		$(ISO_ROOT)/boot/limine/


	cp \
		$(LIMINE_DIR)/BOOTX64.EFI \
		$(ISO_ROOT)/EFI/BOOT/


	xorriso \
		-as mkisofs \
		-R \
		-r \
		-J \
		-b boot/limine/limine-bios-cd.bin \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		-hfsplus \
		-apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part \
		--efi-boot-image \
		--protective-msdos-label \
		$(ISO_ROOT) \
		-o $(ISO_IMAGE)


	$(LIMINE_DIR)/limine \
		bios-install \
		$(ISO_IMAGE)


	rm -rf $(ISO_ROOT)


	@echo
	@echo "Built bootable image:"
	@echo "  $(ISO_IMAGE)"


run: $(ISO_IMAGE)
	$(QEMU) \
		-machine q35 \
		-m 256M \
		-cdrom $(ISO_IMAGE) \
		-boot d \
		-serial stdio \
		-monitor none \
		-no-reboot \
		-no-shutdown


debug: $(ISO_IMAGE)
	$(QEMU) \
		-machine q35 \
		-m 256M \
		-cdrom $(ISO_IMAGE) \
		-boot d \
		-serial stdio \
		-monitor none \
		-no-reboot \
		-no-shutdown \
		-S \
		-gdb tcp::1234


test: $(ISO_IMAGE)
	bash tests/phase1_boot.sh $(ISO_IMAGE)
	bash tests/phase2_terminal.sh $(ISO_IMAGE)
	$(MAKE) test-phase3
	$(MAKE) test-phase4


inspect: $(KERNEL_ELF)
	@echo "========== ELF HEADER =========="
	$(READELF) -h $(KERNEL_ELF)

	@echo
	@echo "========== PROGRAM HEADERS =========="
	$(READELF) -l $(KERNEL_ELF)

	@echo
	@echo "========== DISASSEMBLY =========="
	$(OBJDUMP) -d $(KERNEL_ELF)


clean:
	rm -rf $(BUILD_DIR)


distclean: clean
	rm -rf $(LIMINE_DIR)


help:
	@echo "AxiomOS Phase 4 targets:"
	@echo
	@echo "  make          Build kernel and bootable ISO"
	@echo "  make run      Boot AxiomOS using QEMU"
	@echo "  make debug    Boot QEMU paused for GDB"
	@echo "  make test     Run Phase 1 through Phase 4 tests"
	@echo "  make inspect  Inspect generated ELF64 kernel"
	@echo "  make clean    Remove generated build files"
	@echo "  make distclean Remove build files and Limine"

.PHONY: test-phase3 test-phase4

test-phase3:
	python3 tests/phase3_cpu.py

test-phase4:
	python3 tests/phase4_pmm.py

# Track C header changes too; each fault mode has its own object directory.
-include $(C_OBJECTS:.o=.d)

```

## `docs/memory.md`
```markdown
# AxiomOS memory management — Phase 4

Phase 4 introduces the first AxiomOS-owned dynamic resource manager: a physical
memory manager (PMM) for 4 KiB page frames. Virtual address-space ownership is
still deferred to Phase 5; Limine's page tables and HHDM remain active.

## Boot information

AxiomOS requests two additional Limine features:

- Memory map: describes physical address ranges and their ownership/type.
- HHDM: gives the runtime virtual offset used to access mapped physical RAM.

Only `LIMINE_MEMMAP_USABLE` pages are allocatable in Phase 4. Bootloader-
reclaimable pages are intentionally *not* reclaimed yet because Limine response
structures and the current bootloader page tables still live there.

## Address types

`paddr_t` and `vaddr_t` are distinct typedefs even though both are represented
by 64-bit integers on x86-64. `pmm_alloc_page()` returns a physical address.
`pmm_phys_to_hhdm()` is the explicit conversion used when the kernel needs a
virtual pointer to an allocated physical page while Limine's mappings remain
active.

## Allocator design

The PMM uses two one-bit-per-page bitmaps:

- usable bitmap: 1 means Limine marked that physical page usable;
- used bitmap: 1 means the page is unavailable or currently allocated.

All pages start unavailable. Limine-usable pages are then marked usable/free.
The bitmap storage itself is placed in the first usable region large enough to
contain it and is immediately marked allocated so it can never be handed out.

Two bitmaps cost 2 bits per represented 4 KiB page. For 256 MiB of low physical
memory that is about 16 KiB of raw bitmap data before page rounding.

## Why bitmap instead of a free list

A free list has very cheap O(1) pop/push operations and can store linkage inside
free pages, but it does not by itself distinguish a valid allocated page from a
reserved page or a page that has already been freed. The bitmap makes those
checks direct and prepares us for page-state bookkeeping needed by the VMM.

Allocation is currently a first-free scan with a rolling hint. This is simple
and correct, not yet optimized for very large machines.

## Accounting

The PMM tracks:

- memory-map entry count;
- RAM-like memory reported by relevant Limine map types;
- Limine-usable memory;
- total usable page count;
- allocator metadata pages;
- allocated pages;
- free pages.

Allocator metadata counts as allocated memory. Therefore the invariant after
initialization is:

`allocated_pages + free_pages == usable_pages`

## Phase-4 self-test

The boot self-test allocates three distinct pages, checks 4 KiB alignment,
converts them through the HHDM, writes and reads signatures at both ends of each
page, rejects an unaligned free, frees all three pages, rejects a double free,
and verifies that allocation counters return to their baseline values.

Phase 5 will use this allocator to obtain physical pages for AxiomOS-owned page
tables. Bootloader-reclaimable memory will only be considered for reclamation
after boot data and page-table ownership have been handled safely.

```

## `docs/architecture.md`
```markdown
# Architecture at Phase 4

- Target: x86-64, one bootstrap CPU, ring 0, freestanding C/NASM.
- Toolchain: Clang x86_64-unknown-none-elf, LLD, NASM, Make, QEMU.
- Limine v12.9.0 loads an ELF64 higher-half kernel at 0xffffffff80000000.
- Entry disables IRQs, clears DF, chooses our 64 KiB stack, and calls C.
- Serial first, then framebuffer terminal and custom kprintf.
- GDT: null, kernel code, kernel data, two-slot 64-bit TSS descriptor.
- TSS: RSP0 points to bootstrap stack; IST1/2/3 have separate 16 KiB arrays.
- IDT: all 256 vectors are present ring-0 interrupt gates (0x8E).
- #DF uses IST1; NMI uses IST2; #MC uses IST3. All others use IST0.
- Stubs normalize error codes and save GPRs; C dispatches; IRETQ restores.
- #BP and software INT 0x80 return. Other exceptions panic and halt.
- PIC remapped to 0x20/0x28 and fully masked. Device IRQ delivery remains off.
- Limine memory map and HHDM are requested for Phase 4.
- PMM page size: 4 KiB. Only LIMINE_MEMMAP_USABLE RAM is allocatable.
- PMM uses usable + used bitmaps stored in reserved usable physical pages.
- pmm_alloc_page returns paddr_t; HHDM conversion is explicit.
- Limine page tables remain active; AxiomOS does not own virtual memory yet.
- No heap, scheduler, ring 3, filesystem or networking exists yet.

Do not reclaim bootloader-reclaimable pages in Phase 4. Limine protocol response
objects and the current page tables can still live there. Phase 5 must establish
safe AxiomOS-owned mappings before that policy changes.

```

## `docs/project-state.md`
```markdown
# AxiomOS handoff — Phase 4 implementation pending local acceptance

User: Ayham. Linux development container: `distrobox enter axiom-dev` on Steam
Deck. Run from the directory containing Makefile. Phases 0–3 were working
before this update; never restart from earlier phases.

Original roadmap: Phase 0 through Phase 25, 26 phases including setup.
Current task: Phase 4 physical memory manager. Do not begin Phase 5 until the
local `make test` Phase-4 boot test succeeds.

Phase 3 retained APIs: gdt_init(), interrupts_init(), irq_register(),
interrupt_dispatch(), exception_panic(), phase3_selftest(). PIC remains masked
and IF remains 0.

Phase 4 adds:

- Limine memory-map and HHDM requests.
- paddr_t / vaddr_t address types.
- 4 KiB bitmap physical page allocator.
- pmm_init(), pmm_alloc_page(), pmm_free_page(), pmm_phys_to_hhdm().
- pmm_get_stats() accounting.
- phase4_pmm_selftest().
- tests/phase4_pmm.py automated QEMU validation.

Only LIMINE_MEMMAP_USABLE pages are managed. Bootloader-reclaimable memory is
not freed because Limine response data/current page tables can still occupy it.
The bitmap metadata is stored inside usable RAM and permanently reserved.

Current paging is still Limine-owned. Phase 5 is the first phase that should
create and switch to AxiomOS-owned page tables.

```

## `docs/validation.md`
```markdown
# Phase 3 validation — 2026-09-12

Verified with Ubuntu Clang/LLD 18.1.3, NASM 2.16.01, QEMU 8.2.2,
q35 machine, TCG emulation, 256 MiB RAM, BIOS boot through Limine v12.9.0.

## Results

- Clean build: PASS with -Wall -Wextra -Werror; no compiler/assembler warnings.
- `make test`: PASS (Phase 1, Phase 2, and all five Phase 3 boot cases).
- Normal interrupt return: PASS for all 15 general registers, RSP and RFLAGS,
  including restoring DF after the handler clears it for C.
- Software vector 0x80: returned successfully.
- #DE: correct vector/error, RIP equals divide-instruction symbol; RAX=123,
  RCX=0 and RDX=0 preserved.
- #UD: correct vector/error and fault-instruction RIP.
- #GP: correct vector 13, CPU error code 0x28, and fault-instruction RIP.
- #DF: correct vector 8/error 0, frame verified within the IST1 stack.
- Normal boot and divide-panic screenshots inspected: text visible, no clipping.

`test-results/` contains raw serial logs. `screenshots/` contains real QEMU
framebuffer captures, not illustrative mockups. Re-run `make test` locally.
A repeated #DF run was used to retain its log after the execution environment
initially failed to persist that output file; it passed again.

## Measured normal ELF sections

`llvm-size build/AxiomOS.elf` reported:

| Section category | Bytes |
|---|---:|
| text | 13,816 |
| data | 176 |
| bss | 119,200 |
| total | 133,192 |

BSS includes the 64 KiB bootstrap stack and three 16 KiB emergency stacks;
these are memory allocations, not equivalent on-disk executable bytes.

## Limits of verification

No boot on physical hardware or UEFI was performed. NMI/machine-check injection
and real device-generated PIC IRQ delivery were not exercised. IRQs remain
masked and IF=0 by design. The GDB walkthrough is provided for your machine;
GDB was not run in this validation environment. The source remains single-core
ring-0 code, with no stack guard pages or dynamic memory allocator.

Phase 4 source has now been added, but this validation record still covers the
accepted Phase-3 baseline only. Phase 4 must be accepted separately by running
`make test` locally and confirming `PASS: Phase 4 physical memory manager`.

```

## `README.md`
```markdown
# AxiomOS

A freestanding x86-64 hobby kernel in C and NASM, built with Clang/LLD and
booted by Limine v12.9.0 under QEMU. Higher-half ELF64; no host libc.

Phases 0–3 are complete. Phase 4 source adds Limine memory-map/HHDM discovery,
a 4 KiB bitmap physical page allocator, accounting, invalid/double-free checks,
and a destructive allocate/write/read/free self-test. Phase 4 is accepted only
after the local `make test` run passes.

```bash
make
make run
make test
make test-phase4
make MODE=divide run
make MODE=invalid debug
```

Python 3 is required for the Phase 3/4 QEMU tests. `make run` stays the normal
build; `MODE=divide`, `invalid`, `gp` and `double_fault` use separate build
directories. Hardware IRQs remain masked and IF remains clear.

The PMM manages only Limine `USABLE` RAM. Virtual memory is still Limine-owned;
Phase 5 will build AxiomOS page tables.

[Architecture](docs/architecture.md) | [Memory](docs/memory.md) |
[Project state](docs/project-state.md) | [Original roadmap](docs/roadmap.md)

```
