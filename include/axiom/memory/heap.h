#ifndef AXIOM_MEMORY_HEAP_H
#define AXIOM_MEMORY_HEAP_H

#include <stddef.h>
#include <stdint.h>

#define HEAP_ALIGNMENT 16ULL
#define HEAP_BASE_ADDRESS 0xFFFFC00000000000ULL
#define HEAP_MAX_SIZE (64ULL * 1024ULL * 1024ULL)

struct heap_stats {
    uint64_t mapped_pages;
    uint64_t mapped_bytes;

    uint64_t active_allocations;
    uint64_t total_allocations;
    uint64_t total_frees;
    uint64_t total_reallocations;
    uint64_t failed_allocations;

    uint64_t bytes_in_use;
    uint64_t peak_bytes_in_use;

    uint64_t block_count;
    uint64_t free_block_count;
    uint64_t free_bytes;
    uint64_t largest_free_block;
};

/* Initialise the higher-half kernel heap. Returns 1 on success. */
int heap_init(void);

/* Freestanding kernel allocation API. */
void *kmalloc(size_t size);
void *kcalloc(size_t count, size_t size);
void *krealloc(void *pointer, size_t new_size);
void kfree(void *pointer);

/* Diagnostic helpers. */
int heap_validate(void);
struct heap_stats heap_get_stats(void);

/* Phase-6 destructive allocator self-test; leaves no live allocations. */
int phase6_heap_selftest(void);

#endif
