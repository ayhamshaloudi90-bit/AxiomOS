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
 * Phase 17: ask Limine to bootstrap and park every processor. Keep flags at
 * zero so x86-64 remains in xAPIC mode; the Phase-7 APIC driver deliberately
 * uses the MMIO xAPIC interface rather than x2APIC MSRs.
 */
__attribute__((used, section(".limine_requests")))
static volatile struct limine_mp_request mp_request = {
    .id = {
        LIMINE_COMMON_MAGIC_0,
        LIMINE_COMMON_MAGIC_1,
        0x95a67b819a1b857eULL,
        0xa0b61b723b6a73e0ULL
    },
    .revision = 0,
    .response = 0,
    .flags = 0
};


/*
 * Phase 11: receive standalone ELF executables as Limine modules.
 */
__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request module_request = {
    .id = {
        LIMINE_COMMON_MAGIC_0,
        LIMINE_COMMON_MAGIC_1,
        0x3e7e279702be32afULL,
        0xca1c4f3bd1280ceeULL
    },

    .revision = 0,
    .response = 0,
    .internal_module_count = 0,
    .internal_modules = 0
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


struct limine_mp_response *limine_get_mp_response(void)
{
    return mp_request.response;
}


static int strings_equal(const char *left, const char *right)
{
    if (left == 0 || right == 0) {
        return 0;
    }

    while (*left != '\0' && *right != '\0') {
        if (*left != *right) {
            return 0;
        }

        ++left;
        ++right;
    }

    return *left == *right;
}


int limine_get_module(
    const char *module_string,
    const void **address,
    uint64_t *size,
    const char **path
)
{
    struct limine_module_response *response;
    uint64_t index;

    if (module_string == 0 || address == 0 || size == 0 || path == 0) {
        return 0;
    }

    response = module_request.response;
    if (response == 0 || response->module_count == 0u ||
        response->modules == 0) {
        return 0;
    }

    for (index = 0u; index < response->module_count; ++index) {
        struct limine_file *file = response->modules[index];

        if (file != 0 && file->address != 0 && file->size != 0u &&
            strings_equal(file->string, module_string)) {
            *address = file->address;
            *size = file->size;
            *path = file->path != 0 ? file->path : "<unnamed-module>";
            return 1;
        }
    }

    return 0;
}
