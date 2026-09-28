#ifndef AXIOM_FILESYSTEM_DISKFS_H
#define AXIOM_FILESYSTEM_DISKFS_H

#include <stdint.h>

#include <axiom/drivers/block.h>
#include <axiom/filesystem/vfs.h>

struct diskfs_info {
    int freshly_formatted;
    uint64_t total_sectors;
    uint64_t data_start_sector;
    uint64_t next_free_sector;
    uint32_t file_count;
};

struct vfs_filesystem *diskfs_create(
    struct block_device *device,
    struct diskfs_info *info_out
);

struct diskfs_info diskfs_get_info(void);

#endif
