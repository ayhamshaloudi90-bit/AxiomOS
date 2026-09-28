#ifndef AXIOM_DRIVERS_BLOCK_H
#define AXIOM_DRIVERS_BLOCK_H

#include <stddef.h>
#include <stdint.h>

#define BLOCK_SECTOR_SIZE 512u

struct block_device;

typedef int (*block_read_fn)(struct block_device *, uint64_t, uint32_t, void *);
typedef int (*block_write_fn)(struct block_device *, uint64_t, uint32_t, const void *);
typedef int (*block_flush_fn)(struct block_device *);

struct block_device {
    const char *name;
    uint32_t sector_size;
    uint64_t sector_count;
    block_read_fn read;
    block_write_fn write;
    block_flush_fn flush;
    void *private_data;
};

struct block_stats {
    uint64_t read_commands;
    uint64_t write_commands;
    uint64_t sectors_read;
    uint64_t sectors_written;
    uint64_t flush_commands;
    uint64_t errors;
};

int block_init(void);
struct block_device *block_primary(void);
int block_read(struct block_device *device, uint64_t lba, uint32_t sectors, void *buffer);
int block_write(struct block_device *device, uint64_t lba, uint32_t sectors, const void *buffer);
int block_flush(struct block_device *device);
struct block_stats block_get_stats(void);

#endif
