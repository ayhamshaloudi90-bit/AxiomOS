#include <stddef.h>
#include <stdint.h>

#include <axiom/kernel/panic.h>
#include <axiom/memory/heap.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>

#define HEAP_INITIAL_PAGES 4ULL
#define HEAP_MAX_PAGES (HEAP_MAX_SIZE / VMM_PAGE_SIZE)
#define HEAP_GUARD_BYTES 16ULL
#define HEAP_MIN_FREE_CAPACITY 32ULL

#define HEAP_BLOCK_FREE (1ULL << 0)

#define HEAP_HEADER_MAGIC 0xA610B10C4B484541ULL
#define HEAP_COOKIE_MAGIC 0xC001C0DE5EEDB10CULL
#define HEAP_GUARD_MAGIC  0xA6106A7DDEADC0DEULL


struct heap_block {
    uint64_t magic;
    uint64_t capacity;
    uint64_t requested_size;

    struct heap_block *prev;
    struct heap_block *next;

    uint64_t flags;
    uint64_t cookie;
    uint64_t cookie_inverse;
};


_Static_assert(
    sizeof(struct heap_block) == 64,
    "heap block header must remain 64 bytes"
);

_Static_assert(
    (sizeof(struct heap_block) % HEAP_ALIGNMENT) == 0,
    "heap block header alignment"
);


static struct {
    struct heap_block *first;

    uintptr_t mapped_end;
    uint64_t mapped_pages;

    uint64_t active_allocations;
    uint64_t total_allocations;
    uint64_t total_frees;
    uint64_t total_reallocations;
    uint64_t failed_allocations;

    uint64_t bytes_in_use;
    uint64_t peak_bytes_in_use;

    int initialized;
} heap;


static uintptr_t block_address(const struct heap_block *block)
{
    return (uintptr_t)block;
}


static uint8_t *block_payload(struct heap_block *block)
{
    return (uint8_t *)(void *)(block + 1);
}


static const uint8_t *block_payload_const(const struct heap_block *block)
{
    return (const uint8_t *)(const void *)(block + 1);
}


static uintptr_t block_end_address(const struct heap_block *block)
{
    return block_address(block) + sizeof(*block) + block->capacity;
}


static int block_is_free(const struct heap_block *block)
{
    return (block->flags & HEAP_BLOCK_FREE) != 0ULL;
}


static uint64_t block_magic_value(const struct heap_block *block)
{
    return HEAP_HEADER_MAGIC ^ (uint64_t)block_address(block);
}


static uint64_t block_cookie_value(const struct heap_block *block)
{
    return HEAP_COOKIE_MAGIC ^
           (uint64_t)block_address(block) ^
           block->capacity ^
           block->requested_size ^
           (uint64_t)(uintptr_t)block->prev ^
           (uint64_t)(uintptr_t)block->next ^
           block->flags;
}


static void refresh_block_integrity(struct heap_block *block)
{
    const uint64_t cookie = block_cookie_value(block);

    block->magic = block_magic_value(block);
    block->cookie = cookie;
    block->cookie_inverse = ~cookie;
}


static int block_header_valid(const struct heap_block *block)
{
    uint64_t expected_cookie;


    if (block == 0 || block->magic != block_magic_value(block)) {
        return 0;
    }


    expected_cookie = block_cookie_value(block);

    return block->cookie == expected_cookie &&
           block->cookie_inverse == ~expected_cookie;
}


static void write_u64_unaligned(uint8_t *destination, uint64_t value)
{
    uint64_t index;

    for (index = 0; index < 8ULL; ++index) {
        destination[index] = (uint8_t)(value >> (index * 8ULL));
    }
}


static uint64_t read_u64_unaligned(const uint8_t *source)
{
    uint64_t value;
    uint64_t index;

    value = 0ULL;

    for (index = 0; index < 8ULL; ++index) {
        value |= (uint64_t)source[index] << (index * 8ULL);
    }

    return value;
}


static uint64_t guard_value(const struct heap_block *block)
{
    return HEAP_GUARD_MAGIC ^
           (uint64_t)block_address(block) ^
           block->capacity ^
           block->requested_size;
}


static void write_guard(struct heap_block *block)
{
    uint8_t *guard;
    const uint64_t first = guard_value(block);

    guard = block_payload(block) + block->requested_size;

    write_u64_unaligned(guard, first);
    write_u64_unaligned(guard + 8ULL, ~first);
}


static int guard_valid(const struct heap_block *block)
{
    const uint8_t *guard;
    const uint64_t expected = guard_value(block);


    if (block_is_free(block) ||
        block->requested_size == 0ULL ||
        block->requested_size > block->capacity ||
        HEAP_GUARD_BYTES > block->capacity - block->requested_size) {

        return 0;
    }


    guard = block_payload_const(block) + block->requested_size;

    return read_u64_unaligned(guard) == expected &&
           read_u64_unaligned(guard + 8ULL) == ~expected;
}


static int add_overflow_u64(uint64_t left, uint64_t right, uint64_t *result)
{
    if (UINT64_MAX - left < right) {
        return 1;
    }

    *result = left + right;
    return 0;
}


static int required_capacity(size_t requested_size, uint64_t *capacity)
{
    uint64_t value;
    uint64_t remainder;


    if (requested_size == 0) {
        *capacity = 0ULL;
        return 1;
    }


    if (add_overflow_u64(
            (uint64_t)requested_size,
            HEAP_GUARD_BYTES,
            &value)) {

        return 0;
    }


    remainder = value & (HEAP_ALIGNMENT - 1ULL);

    if (remainder != 0ULL) {
        if (add_overflow_u64(
                value,
                HEAP_ALIGNMENT - remainder,
                &value)) {

            return 0;
        }
    }


    *capacity = value;
    return 1;
}


static void zero_page(paddr_t physical)
{
    uint8_t *page;
    uint64_t index;


    page = (uint8_t *)pmm_phys_to_hhdm(physical);

    if (page == 0) {
        kernel_panic("Heap could not access a newly allocated physical page");
    }


    for (index = 0; index < VMM_PAGE_SIZE; ++index) {
        page[index] = 0u;
    }
}


static void rollback_mapped_pages(uintptr_t start, uint64_t count)
{
    uint64_t index;


    for (index = 0; index < count; ++index) {
        const vaddr_t virtual_address =
            (vaddr_t)(start + index * VMM_PAGE_SIZE);

        const paddr_t physical = virt_to_phys(virtual_address);


        if (physical == PADDR_INVALID) {
            continue;
        }


        if (!unmap_page(virtual_address)) {
            kernel_panic("Heap mapping rollback could not unmap a page");
        }


        if (!pmm_free_page(physical & ~(PMM_PAGE_SIZE - 1ULL))) {
            kernel_panic("Heap mapping rollback could not free a page");
        }
    }
}


static struct heap_block *last_block(void)
{
    struct heap_block *block;


    block = heap.first;

    if (block == 0) {
        return 0;
    }


    while (block->next != 0) {
        block = block->next;
    }


    return block;
}


static int heap_validate_internal(int check_guards)
{
    struct heap_block *block;
    struct heap_block *previous;
    uintptr_t expected_address;
    uint64_t visited;


    if (!heap.initialized || heap.first == 0) {
        return 0;
    }


    block = heap.first;
    previous = 0;
    expected_address = (uintptr_t)HEAP_BASE_ADDRESS;
    visited = 0ULL;


    while (block != 0) {
        uintptr_t end;


        if (++visited > (HEAP_MAX_SIZE / sizeof(struct heap_block))) {
            return 0;
        }


        if (block_address(block) != expected_address ||
            (block_address(block) & (HEAP_ALIGNMENT - 1ULL)) != 0ULL ||
            block_address(block) < (uintptr_t)HEAP_BASE_ADDRESS ||
            block_address(block) + sizeof(*block) > heap.mapped_end ||
            !block_header_valid(block) ||
            block->prev != previous ||
            block->capacity == 0ULL ||
            (block->capacity & (HEAP_ALIGNMENT - 1ULL)) != 0ULL) {

            return 0;
        }


        end = block_end_address(block);

        if (end <= block_address(block) || end > heap.mapped_end) {
            return 0;
        }


        if (block_is_free(block)) {
            if (block->requested_size != 0ULL) {
                return 0;
            }
        } else {
            if (block->requested_size == 0ULL ||
                block->requested_size > block->capacity ||
                HEAP_GUARD_BYTES > block->capacity - block->requested_size) {

                return 0;
            }

            if (check_guards && !guard_valid(block)) {
                return 0;
            }
        }


        if (block->next != 0) {
            if ((uintptr_t)block->next != end) {
                return 0;
            }
        } else if (end != heap.mapped_end) {
            return 0;
        }


        expected_address = end;
        previous = block;
        block = block->next;
    }


    return expected_address == heap.mapped_end;
}


static void assert_heap_structure_valid(void)
{
    if (!heap_validate_internal(0)) {
        kernel_panic("Kernel heap metadata is corrupted");
    }
}


static void assert_heap_valid(void)
{
    if (!heap_validate_internal(1)) {
        kernel_panic("Kernel heap metadata or allocation guard is corrupted");
    }
}


static void initialise_free_block(
    struct heap_block *block,
    uint64_t capacity,
    struct heap_block *previous,
    struct heap_block *next
)
{
    block->capacity = capacity;
    block->requested_size = 0ULL;
    block->prev = previous;
    block->next = next;
    block->flags = HEAP_BLOCK_FREE;

    refresh_block_integrity(block);
}


static int map_heap_pages(uint64_t page_count)
{
    const uintptr_t start = heap.mapped_end;
    uint64_t mapped;


    if (page_count == 0ULL ||
        page_count > HEAP_MAX_PAGES - heap.mapped_pages) {

        return 0;
    }


    mapped = 0ULL;

    while (mapped < page_count) {
        const vaddr_t virtual_address =
            (vaddr_t)(start + mapped * VMM_PAGE_SIZE);

        paddr_t physical;


        if (virt_to_phys(virtual_address) != PADDR_INVALID) {
            rollback_mapped_pages(start, mapped);
            return 0;
        }


        physical = pmm_alloc_page();

        if (physical == PADDR_INVALID) {
            rollback_mapped_pages(start, mapped);
            return 0;
        }


        zero_page(physical);


        if (!map_page(
                virtual_address,
                physical,
                VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE)) {

            (void)pmm_free_page(physical);
            rollback_mapped_pages(start, mapped);
            return 0;
        }


        ++mapped;
    }


    heap.mapped_end += page_count * VMM_PAGE_SIZE;
    heap.mapped_pages += page_count;

    return 1;
}


static int grow_heap(uint64_t required)
{
    struct heap_block *tail;
    uint64_t additional_bytes;
    uint64_t pages;
    uintptr_t old_end;


    tail = last_block();


    if (tail != 0 && block_is_free(tail)) {
        if (tail->capacity >= required) {
            return 1;
        }

        additional_bytes = required - tail->capacity;
    } else {
        if (add_overflow_u64(
                required,
                (uint64_t)sizeof(struct heap_block),
                &additional_bytes)) {

            return 0;
        }
    }


    pages =
        (additional_bytes + VMM_PAGE_SIZE - 1ULL) /
        VMM_PAGE_SIZE;


    if (pages == 0ULL) {
        pages = 1ULL;
    }


    old_end = heap.mapped_end;


    if (!map_heap_pages(pages)) {
        return 0;
    }


    if (tail == 0) {
        heap.first = (struct heap_block *)(uintptr_t)HEAP_BASE_ADDRESS;

        initialise_free_block(
            heap.first,
            pages * VMM_PAGE_SIZE - sizeof(struct heap_block),
            0,
            0
        );

        return 1;
    }


    if (block_is_free(tail)) {
        tail->capacity += pages * VMM_PAGE_SIZE;
        refresh_block_integrity(tail);
        return 1;
    }


    {
        struct heap_block *new_tail =
            (struct heap_block *)(void *)old_end;

        initialise_free_block(
            new_tail,
            pages * VMM_PAGE_SIZE - sizeof(struct heap_block),
            tail,
            0
        );

        tail->next = new_tail;
        refresh_block_integrity(tail);
    }


    return 1;
}


static struct heap_block *find_fit(uint64_t required)
{
    struct heap_block *block;


    for (block = heap.first; block != 0; block = block->next) {
        if (block_is_free(block) && block->capacity >= required) {
            return block;
        }
    }


    return 0;
}


static struct heap_block *split_block(
    struct heap_block *block,
    uint64_t first_capacity
)
{
    const uint64_t original_capacity = block->capacity;
    struct heap_block *old_next;
    struct heap_block *remainder;


    if (original_capacity < first_capacity ||
        original_capacity - first_capacity <
            sizeof(struct heap_block) + HEAP_MIN_FREE_CAPACITY) {

        return 0;
    }


    old_next = block->next;

    remainder = (struct heap_block *)(void *)(
        block_address(block) +
        sizeof(struct heap_block) +
        first_capacity
    );


    initialise_free_block(
        remainder,
        original_capacity - first_capacity - sizeof(struct heap_block),
        block,
        old_next
    );


    block->capacity = first_capacity;
    block->next = remainder;
    refresh_block_integrity(block);


    if (old_next != 0) {
        old_next->prev = remainder;
        refresh_block_integrity(old_next);
    }


    return remainder;
}


static void absorb_next_block(struct heap_block *block)
{
    struct heap_block *next;
    struct heap_block *after;


    next = block->next;

    if (next == 0 || !block_is_free(next)) {
        return;
    }


    after = next->next;

    block->capacity += sizeof(struct heap_block) + next->capacity;
    block->next = after;
    refresh_block_integrity(block);


    if (after != 0) {
        after->prev = block;
        refresh_block_integrity(after);
    }


    next->magic = 0ULL;
    next->cookie = 0ULL;
    next->cookie_inverse = 0ULL;
}


static void mark_allocated(struct heap_block *block, size_t requested_size)
{
    block->requested_size = (uint64_t)requested_size;
    block->flags &= ~HEAP_BLOCK_FREE;
    refresh_block_integrity(block);
    write_guard(block);
}


static void mark_free(struct heap_block *block)
{
    uint64_t index;
    uint8_t *payload;


    payload = block_payload(block);

    for (index = 0; index < block->requested_size; ++index) {
        payload[index] = 0xDDu;
    }


    block->requested_size = 0ULL;
    block->flags |= HEAP_BLOCK_FREE;
    refresh_block_integrity(block);
}


static struct heap_block *find_block_by_payload(const void *pointer)
{
    struct heap_block *block;


    for (block = heap.first; block != 0; block = block->next) {
        if ((const void *)block_payload(block) == pointer) {
            return block;
        }
    }


    return 0;
}


int heap_init(void)
{
    struct vmm_stats paging;


    if (heap.initialized) {
        return 1;
    }


    paging = vmm_get_stats();

    if (paging.root_table == 0ULL || paging.root_table == PADDR_INVALID) {
        return 0;
    }


    if (virt_to_phys((vaddr_t)HEAP_BASE_ADDRESS) != PADDR_INVALID) {
        return 0;
    }


    heap.first = 0;
    heap.mapped_end = (uintptr_t)HEAP_BASE_ADDRESS;
    heap.mapped_pages = 0ULL;

    heap.active_allocations = 0ULL;
    heap.total_allocations = 0ULL;
    heap.total_frees = 0ULL;
    heap.total_reallocations = 0ULL;
    heap.failed_allocations = 0ULL;

    heap.bytes_in_use = 0ULL;
    heap.peak_bytes_in_use = 0ULL;


    if (!map_heap_pages(HEAP_INITIAL_PAGES)) {
        return 0;
    }


    heap.first = (struct heap_block *)(uintptr_t)HEAP_BASE_ADDRESS;

    initialise_free_block(
        heap.first,
        HEAP_INITIAL_PAGES * VMM_PAGE_SIZE - sizeof(struct heap_block),
        0,
        0
    );


    heap.initialized = 1;


    if (!heap_validate_internal(1)) {
        heap.initialized = 0;
        return 0;
    }


    return 1;
}


void *kmalloc(size_t size)
{
    uint64_t required;
    struct heap_block *block;


    if (!heap.initialized || size == 0) {
        return 0;
    }


    assert_heap_valid();


    if (!required_capacity(size, &required) || required > HEAP_MAX_SIZE) {
        ++heap.failed_allocations;
        return 0;
    }


    block = find_fit(required);

    if (block == 0) {
        if (!grow_heap(required)) {
            ++heap.failed_allocations;
            return 0;
        }

        block = find_fit(required);

        if (block == 0) {
            kernel_panic("Heap growth succeeded but no fitting block exists");
        }
    }


    (void)split_block(block, required);
    mark_allocated(block, size);

    ++heap.active_allocations;
    ++heap.total_allocations;
    heap.bytes_in_use += (uint64_t)size;

    if (heap.bytes_in_use > heap.peak_bytes_in_use) {
        heap.peak_bytes_in_use = heap.bytes_in_use;
    }


    assert_heap_valid();

    return block_payload(block);
}


void *kcalloc(size_t count, size_t size)
{
    size_t total;
    uint8_t *memory;
    size_t index;


    if (count == 0 || size == 0) {
        return 0;
    }


    if (count > SIZE_MAX / size) {
        ++heap.failed_allocations;
        return 0;
    }


    total = count * size;
    memory = (uint8_t *)kmalloc(total);

    if (memory == 0) {
        return 0;
    }


    for (index = 0; index < total; ++index) {
        memory[index] = 0u;
    }


    return memory;
}


void kfree(void *pointer)
{
    struct heap_block *block;
    uint64_t released;


    if (pointer == 0) {
        return;
    }


    if (!heap.initialized) {
        kernel_panic("kfree called before heap initialization");
    }


    assert_heap_structure_valid();

    block = find_block_by_payload(pointer);

    if (block == 0) {
        kernel_panic("Heap free received an invalid pointer");
    }


    if (block_is_free(block)) {
        kernel_panic("Heap double free detected");
    }


    if (!guard_valid(block)) {
        kernel_panic("Heap tail guard corrupted");
    }


    released = block->requested_size;
    mark_free(block);


    if (heap.active_allocations == 0ULL || heap.bytes_in_use < released) {
        kernel_panic("Heap allocation accounting underflow");
    }


    --heap.active_allocations;
    ++heap.total_frees;
    heap.bytes_in_use -= released;


    if (block->next != 0 && block_is_free(block->next)) {
        absorb_next_block(block);
    }


    if (block->prev != 0 && block_is_free(block->prev)) {
        block = block->prev;
        absorb_next_block(block);
    }


    assert_heap_valid();
}


void *krealloc(void *pointer, size_t new_size)
{
    struct heap_block *block;
    uint64_t required;
    uint64_t old_requested;


    if (pointer == 0) {
        return kmalloc(new_size);
    }


    if (new_size == 0) {
        kfree(pointer);
        return 0;
    }


    if (!heap.initialized) {
        return 0;
    }


    assert_heap_structure_valid();

    block = find_block_by_payload(pointer);

    if (block == 0 || block_is_free(block)) {
        kernel_panic("krealloc received an invalid pointer");
    }


    if (!guard_valid(block)) {
        kernel_panic("Heap tail guard corrupted before krealloc");
    }


    if (!required_capacity(new_size, &required) || required > HEAP_MAX_SIZE) {
        ++heap.failed_allocations;
        return 0;
    }


    old_requested = block->requested_size;
    ++heap.total_reallocations;


    if (required <= block->capacity) {
        struct heap_block *remainder;


        remainder = split_block(block, required);

        if (remainder != 0 &&
            remainder->next != 0 &&
            block_is_free(remainder->next)) {

            absorb_next_block(remainder);
        }


        block->requested_size = (uint64_t)new_size;
        refresh_block_integrity(block);
        write_guard(block);


        if ((uint64_t)new_size >= old_requested) {
            heap.bytes_in_use += (uint64_t)new_size - old_requested;
        } else {
            heap.bytes_in_use -= old_requested - (uint64_t)new_size;
        }


        if (heap.bytes_in_use > heap.peak_bytes_in_use) {
            heap.peak_bytes_in_use = heap.bytes_in_use;
        }


        assert_heap_valid();
        return pointer;
    }


    if (block->next != 0 && block_is_free(block->next)) {
        const uint64_t combined =
            block->capacity +
            sizeof(struct heap_block) +
            block->next->capacity;


        if (combined >= required) {
            struct heap_block *remainder;


            absorb_next_block(block);
            remainder = split_block(block, required);

            if (remainder != 0 &&
                remainder->next != 0 &&
                block_is_free(remainder->next)) {

                absorb_next_block(remainder);
            }


            block->requested_size = (uint64_t)new_size;
            refresh_block_integrity(block);
            write_guard(block);

            heap.bytes_in_use += (uint64_t)new_size - old_requested;

            if (heap.bytes_in_use > heap.peak_bytes_in_use) {
                heap.peak_bytes_in_use = heap.bytes_in_use;
            }


            assert_heap_valid();
            return pointer;
        }
    }


    {
        uint8_t *replacement;
        const uint8_t *old_bytes;
        size_t index;


        replacement = (uint8_t *)kmalloc(new_size);

        if (replacement == 0) {
            return 0;
        }


        old_bytes = (const uint8_t *)pointer;

        for (index = 0; index < old_requested; ++index) {
            replacement[index] = old_bytes[index];
        }


        kfree(pointer);
        return replacement;
    }
}


int heap_validate(void)
{
    if (!heap.initialized) {
        return 0;
    }

    return heap_validate_internal(1);
}


struct heap_stats heap_get_stats(void)
{
    struct heap_stats stats;
    struct heap_block *block;


    assert_heap_valid();


    stats.mapped_pages = heap.mapped_pages;
    stats.mapped_bytes = heap.mapped_pages * VMM_PAGE_SIZE;

    stats.active_allocations = heap.active_allocations;
    stats.total_allocations = heap.total_allocations;
    stats.total_frees = heap.total_frees;
    stats.total_reallocations = heap.total_reallocations;
    stats.failed_allocations = heap.failed_allocations;

    stats.bytes_in_use = heap.bytes_in_use;
    stats.peak_bytes_in_use = heap.peak_bytes_in_use;

    stats.block_count = 0ULL;
    stats.free_block_count = 0ULL;
    stats.free_bytes = 0ULL;
    stats.largest_free_block = 0ULL;


    for (block = heap.first; block != 0; block = block->next) {
        ++stats.block_count;

        if (block_is_free(block)) {
            ++stats.free_block_count;
            stats.free_bytes += block->capacity;

            if (block->capacity > stats.largest_free_block) {
                stats.largest_free_block = block->capacity;
            }
        }
    }


    return stats;
}
