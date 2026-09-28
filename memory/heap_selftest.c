#include <stddef.h>
#include <stdint.h>

#include <axiom/memory/heap.h>
#include <axiom/terminal/kprintf.h>


static int selftest_fail(const char *reason)
{
    kprintf("Phase 6 heap self-test: FAILED (%s)\n", reason);
    return 0;
}


static int pointer_is_aligned(const void *pointer)
{
    return ((uintptr_t)pointer & (HEAP_ALIGNMENT - 1ULL)) == 0ULL;
}


int phase6_heap_selftest(void)
{
    uint8_t *small;
    uint8_t *medium;
    uint8_t *reuse;
    uint8_t *large;
    uint64_t *zeroed;

    uint8_t *resized;
    uint8_t *grown;
    uint8_t *shrunk;

    uint8_t *first;
    uint8_t *second;
    uint8_t *third;
    uint8_t *merged;

    struct heap_stats midpoint;
    struct heap_stats after;
    size_t index;


    if (!heap_validate()) {
        return selftest_fail("initial heap validation failed");
    }


    /* Basic allocation, alignment, data integrity, and heap growth. */
    small = (uint8_t *)kmalloc(24);
    medium = (uint8_t *)kmalloc(1000);
    large = (uint8_t *)kmalloc(20000);
    zeroed = (uint64_t *)kcalloc(128, sizeof(uint64_t));


    if (small == 0 || medium == 0 || large == 0 || zeroed == 0) {
        return selftest_fail("basic allocation failed");
    }


    if (!pointer_is_aligned(small) ||
        !pointer_is_aligned(medium) ||
        !pointer_is_aligned(large) ||
        !pointer_is_aligned(zeroed)) {

        return selftest_fail("allocation alignment is not 16 bytes");
    }


    if (small == medium || small == large || medium == large) {
        return selftest_fail("distinct allocations overlapped");
    }


    for (index = 0; index < 24; ++index) {
        small[index] = (uint8_t)(0xA0u + (uint8_t)index);
    }


    for (index = 0; index < 1000; ++index) {
        medium[index] = (uint8_t)(index ^ 0x5Au);
    }


    for (index = 0; index < 20000; ++index) {
        large[index] = (uint8_t)(index * 13u + 7u);
    }


    for (index = 0; index < 128; ++index) {
        if (zeroed[index] != 0ULL) {
            return selftest_fail("kcalloc did not zero memory");
        }
    }


    for (index = 0; index < 24; ++index) {
        if (small[index] != (uint8_t)(0xA0u + (uint8_t)index)) {
            return selftest_fail("small allocation data changed");
        }
    }


    for (index = 0; index < 1000; ++index) {
        if (medium[index] != (uint8_t)(index ^ 0x5Au)) {
            return selftest_fail("medium allocation data changed");
        }
    }


    for (index = 0; index < 20000; ++index) {
        if (large[index] != (uint8_t)(index * 13u + 7u)) {
            return selftest_fail("large allocation data changed");
        }
    }


    /* Overflow must fail rather than wrap into a tiny allocation. */
    if (kcalloc(SIZE_MAX, 2) != 0) {
        return selftest_fail("kcalloc overflow was accepted");
    }


    /* First-fit reuse: freeing 1000 bytes should service a 512-byte request. */
    kfree(medium);
    reuse = (uint8_t *)kmalloc(512);

    if (reuse == 0 || reuse != medium) {
        return selftest_fail("first-fit allocator did not reuse a free block");
    }


    for (index = 0; index < 512; ++index) {
        reuse[index] = 0x3Cu;
    }


    /* Reallocation must preserve the old payload while growing and shrinking. */
    resized = (uint8_t *)kmalloc(64);

    if (resized == 0) {
        return selftest_fail("realloc setup allocation failed");
    }


    for (index = 0; index < 64; ++index) {
        resized[index] = (uint8_t)(0xD0u + (uint8_t)index);
    }


    grown = (uint8_t *)krealloc(resized, 4096);

    if (grown == 0) {
        return selftest_fail("krealloc growth failed");
    }


    for (index = 0; index < 64; ++index) {
        if (grown[index] != (uint8_t)(0xD0u + (uint8_t)index)) {
            return selftest_fail("krealloc growth did not preserve data");
        }
    }


    shrunk = (uint8_t *)krealloc(grown, 32);

    if (shrunk == 0) {
        return selftest_fail("krealloc shrink failed");
    }


    for (index = 0; index < 32; ++index) {
        if (shrunk[index] != (uint8_t)(0xD0u + (uint8_t)index)) {
            return selftest_fail("krealloc shrink did not preserve data");
        }
    }


    kfree(small);
    kfree(reuse);
    kfree(large);
    kfree(zeroed);
    kfree(shrunk);


    if (!heap_validate()) {
        return selftest_fail("heap failed after basic free/realloc tests");
    }


    midpoint = heap_get_stats();

    if (midpoint.active_allocations != 0ULL ||
        midpoint.bytes_in_use != 0ULL ||
        midpoint.free_block_count != 1ULL) {

        return selftest_fail("basic allocations did not coalesce back to one block");
    }


    /*
     * Deterministic coalescing test. With a single free block at this point,
     * first and second are adjacent. Freeing both must produce enough room for
     * a 500-byte request at first's original address.
     */
    first = (uint8_t *)kmalloc(256);
    second = (uint8_t *)kmalloc(256);
    third = (uint8_t *)kmalloc(256);

    if (first == 0 || second == 0 || third == 0) {
        return selftest_fail("coalescing setup allocation failed");
    }


    kfree(first);
    kfree(second);

    merged = (uint8_t *)kmalloc(500);

    if (merged == 0 || merged != first) {
        return selftest_fail("adjacent free blocks were not coalesced");
    }


    kfree(merged);
    kfree(third);


    if (!heap_validate()) {
        return selftest_fail("final heap validation failed");
    }


    after = heap_get_stats();


    if (after.active_allocations != 0ULL || after.bytes_in_use != 0ULL) {
        return selftest_fail("live allocations remained after self-test");
    }


    if (after.total_allocations != after.total_frees) {
        return selftest_fail("allocation/free counters are unbalanced");
    }


    if (after.mapped_pages <= 4ULL) {
        return selftest_fail("large allocation did not grow the heap");
    }


    if (after.free_block_count != 1ULL ||
        after.largest_free_block != after.free_bytes) {

        return selftest_fail("free blocks did not fully coalesce");
    }


    kprintf("Phase 6 heap self-test: OK\n");
    return 1;
}
