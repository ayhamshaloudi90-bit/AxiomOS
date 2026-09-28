#include <stddef.h>
#include <stdint.h>

#include <axiom/drivers/ahci.h>
#include <axiom/drivers/block.h>

static struct block_device *primary_device;
static struct block_stats stats;

int block_init(void)
{
    if (primary_device != 0) {
        return 1;
    }

    primary_device = ahci_probe();
    return primary_device != 0;
}

struct block_device *block_primary(void)
{
    return primary_device;
}

int block_read(
    struct block_device *device,
    uint64_t lba,
    uint32_t sectors,
    void *buffer
)
{
    int result;

    if (device == 0 || device->read == 0 || buffer == 0 || sectors == 0u ||
        lba >= device->sector_count ||
        (uint64_t)sectors > device->sector_count - lba) {
        ++stats.errors;
        return 0;
    }

    result = device->read(device, lba, sectors, buffer);
    ++stats.read_commands;
    if (result) {
        stats.sectors_read += sectors;
    } else {
        ++stats.errors;
    }
    return result;
}

int block_write(
    struct block_device *device,
    uint64_t lba,
    uint32_t sectors,
    const void *buffer
)
{
    int result;

    if (device == 0 || device->write == 0 || buffer == 0 || sectors == 0u ||
        lba >= device->sector_count ||
        (uint64_t)sectors > device->sector_count - lba) {
        ++stats.errors;
        return 0;
    }

    result = device->write(device, lba, sectors, buffer);
    ++stats.write_commands;
    if (result) {
        stats.sectors_written += sectors;
    } else {
        ++stats.errors;
    }
    return result;
}

int block_flush(struct block_device *device)
{
    int result;

    if (device == 0 || device->flush == 0) {
        ++stats.errors;
        return 0;
    }

    result = device->flush(device);
    ++stats.flush_commands;
    if (!result) {
        ++stats.errors;
    }
    return result;
}

struct block_stats block_get_stats(void)
{
    return stats;
}
