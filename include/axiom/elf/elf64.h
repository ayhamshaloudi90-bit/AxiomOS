#ifndef AXIOM_ELF_ELF64_H
#define AXIOM_ELF_ELF64_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/memory/address.h>

#define ELF64_MAX_LOAD_PAGES 32u

struct elf64_loaded_page {
    vaddr_t virtual_address;
    paddr_t physical_address;
    uint64_t vmm_flags;
};

struct elf64_load_result {
    vaddr_t entry;
    uint16_t program_headers;
    uint16_t load_segments;
    size_t page_count;
    uint64_t file_bytes;
    uint64_t memory_bytes;
    struct elf64_loaded_page pages[ELF64_MAX_LOAD_PAGES];
};

/*
 * Parse an x86-64 little-endian ET_EXEC image already resident in kernel
 * memory and map each PT_LOAD segment into an existing user address space.
 * All mapped pages are recorded in result so task teardown can release them.
 */
int elf64_load_executable(
    paddr_t address_space,
    const void *image,
    size_t image_size,
    struct elf64_load_result *result
);

const char *elf64_last_error(void);

#endif
