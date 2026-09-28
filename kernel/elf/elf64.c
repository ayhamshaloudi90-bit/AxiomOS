#include <stddef.h>
#include <stdint.h>

#include <axiom/elf/elf64.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>

#define EI_NIDENT 16u
#define EI_CLASS   4u
#define EI_DATA    5u
#define EI_VERSION 6u

#define ELFCLASS64 2u
#define ELFDATA2LSB 1u
#define EV_CURRENT 1u
#define ET_EXEC 2u
#define EM_X86_64 62u
#define PT_LOAD 1u

#define PF_X 0x1u
#define PF_W 0x2u
#define PF_R 0x4u

#define USER_MIN_ADDRESS 0x0000000000010000ULL
#define USER_MAX_ADDRESS 0x00007FFFFFFFFFFFULL

struct elf64_ehdr {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed));

struct elf64_phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed));

static const char *last_error = "no error";

static void bytes_clear(void *destination, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    size_t index;

    for (index = 0u; index < count; ++index) {
        out[index] = 0u;
    }
}

static void bytes_copy(void *destination, const void *source, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    size_t index;

    for (index = 0u; index < count; ++index) {
        out[index] = in[index];
    }
}

static int add_overflow_u64(uint64_t a, uint64_t b, uint64_t *result)
{
    if (UINT64_MAX - a < b) {
        return 1;
    }

    *result = a + b;
    return 0;
}

static uint64_t align_down(uint64_t value)
{
    return value & ~(VMM_PAGE_SIZE - 1ULL);
}

static int align_up(uint64_t value, uint64_t *result)
{
    if (value > UINT64_MAX - (VMM_PAGE_SIZE - 1ULL)) {
        return 0;
    }

    *result = (value + VMM_PAGE_SIZE - 1ULL) & ~(VMM_PAGE_SIZE - 1ULL);
    return 1;
}

static int power_of_two(uint64_t value)
{
    return value != 0ULL && (value & (value - 1ULL)) == 0ULL;
}

static int range_inside_image(uint64_t offset, uint64_t length, size_t image_size)
{
    uint64_t end;

    if (add_overflow_u64(offset, length, &end)) {
        return 0;
    }

    return end <= (uint64_t)image_size;
}

static int page_already_planned(
    const struct elf64_load_result *result,
    vaddr_t virtual_address
)
{
    size_t index;

    for (index = 0u; index < result->page_count; ++index) {
        if (result->pages[index].virtual_address == virtual_address) {
            return 1;
        }
    }

    return 0;
}

static const struct elf64_phdr *program_header_at(
    const uint8_t *image,
    const struct elf64_ehdr *header,
    uint16_t index
)
{
    return (const struct elf64_phdr *)(const void *)(
        image + header->e_phoff + (uint64_t)index * header->e_phentsize
    );
}

static int validate_header(
    const uint8_t *image,
    size_t image_size,
    const struct elf64_ehdr **header_out
)
{
    const struct elf64_ehdr *header;
    uint64_t ph_size;

    if (image == 0 || image_size < sizeof(struct elf64_ehdr)) {
        last_error = "ELF image is smaller than the ELF64 header";
        return 0;
    }

    header = (const struct elf64_ehdr *)(const void *)image;

    if (header->e_ident[0] != 0x7Fu ||
        header->e_ident[1] != 'E' ||
        header->e_ident[2] != 'L' ||
        header->e_ident[3] != 'F') {
        last_error = "ELF magic is invalid";
        return 0;
    }

    if (header->e_ident[EI_CLASS] != ELFCLASS64 ||
        header->e_ident[EI_DATA] != ELFDATA2LSB ||
        header->e_ident[EI_VERSION] != EV_CURRENT) {
        last_error = "ELF is not 64-bit little-endian version 1";
        return 0;
    }

    if (header->e_type != ET_EXEC || header->e_machine != EM_X86_64 ||
        header->e_version != EV_CURRENT) {
        last_error = "ELF is not an x86-64 ET_EXEC image";
        return 0;
    }

    if (header->e_ehsize != sizeof(struct elf64_ehdr) ||
        header->e_phentsize != sizeof(struct elf64_phdr) ||
        header->e_phnum == 0u || header->e_phnum > 64u) {
        last_error = "ELF program-header table shape is unsupported";
        return 0;
    }

    ph_size = (uint64_t)header->e_phentsize * header->e_phnum;
    if (!range_inside_image(header->e_phoff, ph_size, image_size)) {
        last_error = "ELF program-header table is outside the file";
        return 0;
    }

    if (header->e_entry < USER_MIN_ADDRESS ||
        header->e_entry > USER_MAX_ADDRESS) {
        last_error = "ELF entry point is outside the user canonical range";
        return 0;
    }

    *header_out = header;
    return 1;
}

static int validate_load_segments(
    const uint8_t *image,
    size_t image_size,
    const struct elf64_ehdr *header,
    struct elf64_load_result *result
)
{
    uint16_t index;
    int entry_in_executable_segment = 0;

    (void)image;

    result->program_headers = header->e_phnum;
    result->load_segments = 0u;
    result->page_count = 0u;
    result->file_bytes = 0u;
    result->memory_bytes = 0u;
    result->entry = header->e_entry;

    for (index = 0u; index < header->e_phnum; ++index) {
        const struct elf64_phdr *program =
            program_header_at(image, header, index);
        uint64_t segment_end;
        uint64_t page_end;
        uint64_t page;

        if (program->p_type != PT_LOAD) {
            continue;
        }

        if (program->p_memsz == 0ULL) {
            continue;
        }

        if (program->p_filesz > program->p_memsz ||
            !range_inside_image(program->p_offset, program->p_filesz, image_size)) {
            last_error = "ELF PT_LOAD file range is invalid";
            return 0;
        }

        /* Phase 20: enforce W^X. Writable executable pages are rejected. */
        if ((program->p_flags & PF_W) != 0u &&
            (program->p_flags & PF_X) != 0u) {
            last_error = "ELF PT_LOAD violates W^X (writable + executable)";
            return 0;
        }

        if (add_overflow_u64(program->p_vaddr, program->p_memsz, &segment_end) ||
            program->p_vaddr < USER_MIN_ADDRESS ||
            segment_end == 0ULL || segment_end - 1ULL > USER_MAX_ADDRESS) {
            last_error = "ELF PT_LOAD virtual range is invalid";
            return 0;
        }

        if (program->p_align > 1ULL) {
            if (!power_of_two(program->p_align) ||
                (program->p_vaddr & (program->p_align - 1ULL)) !=
                (program->p_offset & (program->p_align - 1ULL))) {
                last_error = "ELF PT_LOAD alignment is invalid";
                return 0;
            }
        }

        if (!align_up(segment_end, &page_end)) {
            last_error = "ELF PT_LOAD page range overflowed";
            return 0;
        }

        for (page = align_down(program->p_vaddr);
             page < page_end;
             page += VMM_PAGE_SIZE) {
            if (page_already_planned(result, page)) {
                last_error = "ELF PT_LOAD segments overlap the same page";
                return 0;
            }

            if (result->page_count >= ELF64_MAX_LOAD_PAGES) {
                last_error = "ELF needs more load pages than Phase 11 allows";
                return 0;
            }

            result->pages[result->page_count].virtual_address = page;
            result->pages[result->page_count].physical_address = PADDR_INVALID;
            result->pages[result->page_count].vmm_flags = 0ULL;
            ++result->page_count;
        }

        if ((program->p_flags & PF_X) != 0u &&
            header->e_entry >= program->p_vaddr &&
            header->e_entry < segment_end) {
            entry_in_executable_segment = 1;
        }

        ++result->load_segments;
        result->file_bytes += program->p_filesz;
        result->memory_bytes += program->p_memsz;
    }

    if (result->load_segments == 0u) {
        last_error = "ELF has no PT_LOAD segments";
        return 0;
    }

    if (!entry_in_executable_segment) {
        last_error = "ELF entry point is not inside an executable PT_LOAD";
        return 0;
    }

    return 1;
}

static struct elf64_loaded_page *result_page(
    struct elf64_load_result *result,
    vaddr_t virtual_address
)
{
    size_t index;

    for (index = 0u; index < result->page_count; ++index) {
        if (result->pages[index].virtual_address == virtual_address) {
            return &result->pages[index];
        }
    }

    return 0;
}

static void release_allocated_pages(struct elf64_load_result *result)
{
    size_t index;

    for (index = 0u; index < result->page_count; ++index) {
        if (result->pages[index].physical_address != PADDR_INVALID) {
            (void)pmm_free_page(result->pages[index].physical_address);
            result->pages[index].physical_address = PADDR_INVALID;
        }
    }
}

int elf64_load_executable(
    paddr_t address_space,
    const void *image_pointer,
    size_t image_size,
    struct elf64_load_result *result
)
{
    const uint8_t *image = (const uint8_t *)image_pointer;
    const struct elf64_ehdr *header;
    uint16_t index;

    if (result == 0 || address_space == PADDR_INVALID) {
        last_error = "ELF loader received invalid arguments";
        return 0;
    }

    bytes_clear(result, sizeof(*result));
    last_error = "no error";

    if (!validate_header(image, image_size, &header) ||
        !validate_load_segments(image, image_size, header, result)) {
        return 0;
    }

    for (index = 0u; index < header->e_phnum; ++index) {
        const struct elf64_phdr *program =
            program_header_at(image, header, index);
        uint64_t segment_end;
        uint64_t page_end;
        uint64_t page;
        uint64_t mapping_flags;

        if (program->p_type != PT_LOAD || program->p_memsz == 0ULL) {
            continue;
        }

        segment_end = program->p_vaddr + program->p_memsz;
        (void)align_up(segment_end, &page_end);

        mapping_flags = VMM_FLAG_USER;
        if ((program->p_flags & PF_W) != 0u) {
            mapping_flags |= VMM_FLAG_WRITABLE;
        }
        if ((program->p_flags & PF_X) == 0u) {
            mapping_flags |= VMM_FLAG_NO_EXECUTE;
        }

        for (page = align_down(program->p_vaddr);
             page < page_end;
             page += VMM_PAGE_SIZE) {
            struct elf64_loaded_page *loaded = result_page(result, page);
            paddr_t physical;
            uint8_t *destination;
            uint64_t file_start;
            uint64_t file_end;
            uint64_t page_start_in_segment;
            uint64_t page_end_in_segment;

            if (loaded == 0 || loaded->physical_address != PADDR_INVALID) {
                last_error = "ELF loader internal page-plan mismatch";
                release_allocated_pages(result);
                return 0;
            }

            physical = pmm_alloc_page();
            if (physical == PADDR_INVALID) {
                last_error = "ELF loader ran out of physical pages";
                release_allocated_pages(result);
                return 0;
            }

            destination = (uint8_t *)pmm_phys_to_hhdm(physical);
            if (destination == 0) {
                (void)pmm_free_page(physical);
                last_error = "ELF loader could not access a physical page";
                release_allocated_pages(result);
                return 0;
            }

            bytes_clear(destination, VMM_PAGE_SIZE);
            loaded->physical_address = physical;
            loaded->vmm_flags = mapping_flags;

            page_start_in_segment =
                page > program->p_vaddr ? page : program->p_vaddr;
            page_end_in_segment =
                page + VMM_PAGE_SIZE < program->p_vaddr + program->p_filesz ?
                page + VMM_PAGE_SIZE : program->p_vaddr + program->p_filesz;

            if (page_end_in_segment > page_start_in_segment) {
                file_start = program->p_offset +
                    (page_start_in_segment - program->p_vaddr);
                file_end = program->p_offset +
                    (page_end_in_segment - program->p_vaddr);

                bytes_copy(
                    destination + (page_start_in_segment - page),
                    image + file_start,
                    (size_t)(file_end - file_start)
                );
            }

            if (!vmm_map_page_in_address_space(
                    address_space,
                    page,
                    physical,
                    mapping_flags
                )) {
                last_error = "ELF loader could not map a PT_LOAD page";
                release_allocated_pages(result);
                return 0;
            }
        }
    }

    return 1;
}

const char *elf64_last_error(void)
{
    return last_error;
}
