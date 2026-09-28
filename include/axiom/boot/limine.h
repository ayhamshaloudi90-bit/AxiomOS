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




struct limine_mp_info;
typedef void (*limine_goto_address)(struct limine_mp_info *);

struct limine_mp_info {
    uint32_t processor_id;
    uint32_t lapic_id;
    uint64_t reserved;
    limine_goto_address goto_address;
    uint64_t extra_argument;
};

struct limine_mp_response {
    uint64_t revision;
    uint32_t flags;
    uint32_t bsp_lapic_id;
    uint64_t cpu_count;
    struct limine_mp_info **cpus;
};

struct limine_mp_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_mp_response *response;
    uint64_t flags;
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




struct limine_uuid {
    uint32_t a;
    uint16_t b;
    uint16_t c;
    uint8_t d[8];
};


struct limine_file {
    uint64_t revision;
    void *address;
    uint64_t size;
    char *path;
    char *string;
    uint32_t media_type;
    uint32_t unused;
    uint8_t tftp_ipv4[4];
    uint32_t tftp_port;
    uint32_t partition_index;
    uint32_t mbr_disk_id;
    struct limine_uuid gpt_disk_uuid;
    struct limine_uuid gpt_part_uuid;
    struct limine_uuid part_uuid;
};


struct limine_module_response {
    uint64_t revision;
    uint64_t module_count;
    struct limine_file **modules;
};


struct limine_module_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_module_response *response;
    uint64_t internal_module_count;
    void *internal_modules;
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


struct limine_mp_response *limine_get_mp_response(void);


int limine_get_module(
    const char *module_string,
    const void **address,
    uint64_t *size,
    const char **path
);


#endif
