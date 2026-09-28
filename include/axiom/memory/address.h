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
