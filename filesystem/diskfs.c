#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>
#include <axiom/drivers/block.h>
#include <axiom/filesystem/diskfs.h>
#include <axiom/memory/heap.h>

#define DISKFS_MAGIC 0x3153464D4F495841ULL /* "AXIOMFS1" little-endian */
#define DISKFS_VERSION 1u
#define DISKFS_DIRECTORY_START 1u
#define DISKFS_DIRECTORY_SECTORS 8u
#define DISKFS_DATA_START 16u
#define DISKFS_MAX_FILES 64u
#define DISKFS_NAME_MAX 47u
#define DISKFS_FILE_MODE 0644u
#define DISKFS_ROOT_MODE 0755u

struct diskfs_superblock {
    uint64_t magic;
    uint32_t version;
    uint32_t sector_size;
    uint64_t total_sectors;
    uint64_t directory_start;
    uint32_t directory_sectors;
    uint32_t max_files;
    uint64_t data_start;
    uint64_t next_free_sector;
    uint8_t reserved[456];
};

struct diskfs_dir_entry {
    uint8_t used;
    uint8_t type;
    uint16_t reserved0;
    uint32_t start_sector;
    uint32_t sector_count;
    uint32_t size;
    char name[48];
};

_Static_assert(sizeof(struct diskfs_superblock) == BLOCK_SECTOR_SIZE,
               "diskfs superblock must be one sector");
_Static_assert(sizeof(struct diskfs_dir_entry) == 64u,
               "diskfs directory entry must be 64 bytes");
_Static_assert(DISKFS_MAX_FILES * sizeof(struct diskfs_dir_entry) ==
               DISKFS_DIRECTORY_SECTORS * BLOCK_SECTOR_SIZE,
               "diskfs directory table layout");

struct diskfs_instance;

struct diskfs_node {
    struct vfs_node vfs;
    struct diskfs_instance *filesystem;
    uint32_t entry_index;
    char name[48];
};

struct diskfs_instance {
    struct vfs_filesystem vfs;
    struct block_device *device;
    struct diskfs_superblock superblock;
    struct diskfs_dir_entry entries[DISKFS_MAX_FILES];
    struct diskfs_node nodes[DISKFS_MAX_FILES];
    struct diskfs_node root;
    struct diskfs_info info;
};

static struct diskfs_instance *active_instance;

static int diskfs_lookup(struct vfs_node *, const char *, struct vfs_node **);
static int diskfs_create_node(struct vfs_node *, const char *, enum vfs_node_type, struct vfs_node **);
static int64_t diskfs_read(struct vfs_node *, uint64_t, void *, size_t);
static int64_t diskfs_write(struct vfs_node *, uint64_t, const void *, size_t);
static int diskfs_truncate(struct vfs_node *, uint64_t);
static int diskfs_readdir(struct vfs_node *, size_t, struct vfs_dirent *);

static const struct vfs_node_ops diskfs_ops = {
    .lookup = diskfs_lookup,
    .create = diskfs_create_node,
    .read = diskfs_read,
    .write = diskfs_write,
    .truncate = diskfs_truncate,
    .readdir = diskfs_readdir,
};

static void bytes_clear(void *memory, size_t count)
{
    uint8_t *bytes = (uint8_t *)memory;
    size_t index;
    for (index = 0u; index < count; ++index) bytes[index] = 0u;
}

static void bytes_copy(void *destination, const void *source, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    size_t index;
    for (index = 0u; index < count; ++index) out[index] = in[index];
}

static size_t string_length(const char *text)
{
    size_t length = 0u;
    if (text == 0) return 0u;
    while (text[length] != '\0') ++length;
    return length;
}

static int strings_equal(const char *left, const char *right)
{
    size_t index = 0u;
    if (left == 0 || right == 0) return 0;
    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] != right[index]) return 0;
        ++index;
    }
    return left[index] == right[index];
}

static void string_copy(char *destination, const char *source, size_t capacity)
{
    size_t index = 0u;
    if (destination == 0 || capacity == 0u) return;
    if (source != 0) {
        while (index + 1u < capacity && source[index] != '\0') {
            destination[index] = source[index];
            ++index;
        }
    }
    destination[index] = '\0';
}

static struct diskfs_node *node_from_vfs(struct vfs_node *node)
{
    return node != 0 ? (struct diskfs_node *)node->private_data : 0;
}

static int persist_superblock(struct diskfs_instance *fs)
{
    return block_write(fs->device, 0u, 1u, &fs->superblock) &&
           block_flush(fs->device);
}

static int persist_directory_sector(struct diskfs_instance *fs, uint32_t entry_index)
{
    uint8_t sector[BLOCK_SECTOR_SIZE];
    const uint32_t entries_per_sector = BLOCK_SECTOR_SIZE / sizeof(struct diskfs_dir_entry);
    const uint32_t sector_index = entry_index / entries_per_sector;
    const uint32_t first_entry = sector_index * entries_per_sector;

    if (sector_index >= DISKFS_DIRECTORY_SECTORS) return 0;
    bytes_copy(sector, &fs->entries[first_entry], sizeof(sector));
    return block_write(
        fs->device,
        fs->superblock.directory_start + sector_index,
        1u,
        sector
    ) && block_flush(fs->device);
}

static void refresh_node(struct diskfs_instance *fs, uint32_t index)
{
    struct diskfs_node *node = &fs->nodes[index];
    const struct diskfs_dir_entry *entry = &fs->entries[index];

    node->filesystem = fs;
    node->entry_index = index;
    string_copy(node->name, entry->name, sizeof(node->name));
    node->vfs.name = node->name;
    node->vfs.type = VFS_NODE_FILE;
    node->vfs.mode = DISKFS_FILE_MODE;
    node->vfs.size = entry->size;
    node->vfs.ops = &diskfs_ops;
    node->vfs.private_data = node;
}

static int format_filesystem(struct diskfs_instance *fs)
{
    uint8_t zero[BLOCK_SECTOR_SIZE];
    uint32_t sector;

    if (fs->device->sector_count <= DISKFS_DATA_START + 1u ||
        fs->device->sector_count > UINT32_MAX) {
        return 0;
    }

    bytes_clear(&fs->superblock, sizeof(fs->superblock));
    bytes_clear(fs->entries, sizeof(fs->entries));
    bytes_clear(zero, sizeof(zero));

    fs->superblock.magic = DISKFS_MAGIC;
    fs->superblock.version = DISKFS_VERSION;
    fs->superblock.sector_size = BLOCK_SECTOR_SIZE;
    fs->superblock.total_sectors = fs->device->sector_count;
    fs->superblock.directory_start = DISKFS_DIRECTORY_START;
    fs->superblock.directory_sectors = DISKFS_DIRECTORY_SECTORS;
    fs->superblock.max_files = DISKFS_MAX_FILES;
    fs->superblock.data_start = DISKFS_DATA_START;
    fs->superblock.next_free_sector = DISKFS_DATA_START;

    if (!block_write(fs->device, 0u, 1u, &fs->superblock)) return 0;
    for (sector = 0u; sector < DISKFS_DIRECTORY_SECTORS; ++sector) {
        if (!block_write(
                fs->device,
                DISKFS_DIRECTORY_START + sector,
                1u,
                zero
            )) return 0;
    }
    return block_flush(fs->device);
}

static int load_directory(struct diskfs_instance *fs)
{
    uint32_t sector;
    uint8_t buffer[BLOCK_SECTOR_SIZE];

    for (sector = 0u; sector < DISKFS_DIRECTORY_SECTORS; ++sector) {
        if (!block_read(
                fs->device,
                fs->superblock.directory_start + sector,
                1u,
                buffer
            )) return 0;
        bytes_copy(
            (uint8_t *)fs->entries + (size_t)sector * BLOCK_SECTOR_SIZE,
            buffer,
            BLOCK_SECTOR_SIZE
        );
    }
    return 1;
}

static int superblock_valid(
    const struct diskfs_superblock *superblock,
    const struct block_device *device
)
{
    return superblock->magic == DISKFS_MAGIC &&
        superblock->version == DISKFS_VERSION &&
        superblock->sector_size == BLOCK_SECTOR_SIZE &&
        superblock->total_sectors == device->sector_count &&
        superblock->directory_start == DISKFS_DIRECTORY_START &&
        superblock->directory_sectors == DISKFS_DIRECTORY_SECTORS &&
        superblock->max_files == DISKFS_MAX_FILES &&
        superblock->data_start == DISKFS_DATA_START &&
        superblock->next_free_sector >= DISKFS_DATA_START &&
        superblock->next_free_sector <= device->sector_count;
}

static int ensure_extent(struct diskfs_node *node, uint64_t required_size)
{
    struct diskfs_instance *fs = node->filesystem;
    struct diskfs_dir_entry *entry = &fs->entries[node->entry_index];
    uint64_t required_sectors64 = (required_size + BLOCK_SECTOR_SIZE - 1u) /
        BLOCK_SECTOR_SIZE;
    uint32_t required_sectors;
    uint32_t new_start;
    uint8_t zero[BLOCK_SECTOR_SIZE];
    uint8_t copy[BLOCK_SECTOR_SIZE];
    uint32_t sector;

    if (required_sectors64 <= entry->sector_count) return 1;
    if (required_sectors64 > UINT32_MAX) return 0;
    required_sectors = (uint32_t)required_sectors64;

    if (fs->superblock.next_free_sector > fs->device->sector_count ||
        (uint64_t)required_sectors >
            fs->device->sector_count - fs->superblock.next_free_sector) {
        return 0;
    }

    new_start = (uint32_t)fs->superblock.next_free_sector;
    bytes_clear(zero, sizeof(zero));

    for (sector = 0u; sector < required_sectors; ++sector) {
        if (!block_write(fs->device, (uint64_t)new_start + sector, 1u, zero)) {
            return 0;
        }
    }

    for (sector = 0u; sector < entry->sector_count; ++sector) {
        if (!block_read(fs->device, (uint64_t)entry->start_sector + sector, 1u, copy) ||
            !block_write(fs->device, (uint64_t)new_start + sector, 1u, copy)) {
            return 0;
        }
    }

    entry->start_sector = new_start;
    entry->sector_count = required_sectors;
    fs->superblock.next_free_sector += required_sectors;
    fs->info.next_free_sector = fs->superblock.next_free_sector;

    return persist_superblock(fs) &&
        persist_directory_sector(fs, node->entry_index);
}

static int diskfs_lookup(
    struct vfs_node *directory,
    const char *name,
    struct vfs_node **node_out
)
{
    struct diskfs_node *root;
    struct diskfs_instance *fs;
    uint32_t index;

    if (directory == 0 || name == 0 || node_out == 0) return -AXIOM_EINVAL;
    if (directory->type != VFS_NODE_DIRECTORY) return -AXIOM_ENOTDIR;
    root = node_from_vfs(directory);
    if (root == 0 || root->filesystem == 0) return -AXIOM_EIO;
    fs = root->filesystem;

    for (index = 0u; index < DISKFS_MAX_FILES; ++index) {
        if (fs->entries[index].used && strings_equal(fs->entries[index].name, name)) {
            refresh_node(fs, index);
            *node_out = &fs->nodes[index].vfs;
            return 0;
        }
    }
    *node_out = 0;
    return -AXIOM_ENOENT;
}

static int diskfs_create_node(
    struct vfs_node *directory,
    const char *name,
    enum vfs_node_type type,
    struct vfs_node **node_out
)
{
    struct diskfs_node *root;
    struct diskfs_instance *fs;
    uint32_t index;
    struct vfs_node *existing = 0;

    if (directory == 0 || name == 0 || name[0] == '\0' ||
        string_length(name) > DISKFS_NAME_MAX) return -AXIOM_EINVAL;
    if (type != VFS_NODE_FILE) return -AXIOM_EROFS;
    if (diskfs_lookup(directory, name, &existing) == 0) return -AXIOM_EEXIST;

    root = node_from_vfs(directory);
    if (root == 0 || root->filesystem == 0) return -AXIOM_EIO;
    fs = root->filesystem;

    for (index = 0u; index < DISKFS_MAX_FILES; ++index) {
        if (!fs->entries[index].used) {
            struct diskfs_dir_entry *entry = &fs->entries[index];
            bytes_clear(entry, sizeof(*entry));
            entry->used = 1u;
            entry->type = AXIOM_DT_FILE;
            string_copy(entry->name, name, sizeof(entry->name));
            if (!persist_directory_sector(fs, index)) {
                bytes_clear(entry, sizeof(*entry));
                return -AXIOM_EIO;
            }
            ++fs->info.file_count;
            refresh_node(fs, index);
            if (node_out != 0) *node_out = &fs->nodes[index].vfs;
            return 0;
        }
    }
    return -AXIOM_ENOSPC;
}

static int64_t diskfs_read(
    struct vfs_node *vfs_node,
    uint64_t offset,
    void *buffer,
    size_t count
)
{
    struct diskfs_node *node = node_from_vfs(vfs_node);
    struct diskfs_dir_entry *entry;
    uint8_t sector_data[BLOCK_SECTOR_SIZE];
    uint8_t *out = (uint8_t *)buffer;
    uint64_t remaining;
    size_t copied = 0u;

    if (node == 0 || node->filesystem == 0 ||
        (count != 0u && buffer == 0)) return -AXIOM_EINVAL;
    entry = &node->filesystem->entries[node->entry_index];
    if (offset >= entry->size || count == 0u) return 0;

    remaining = entry->size - offset;
    if (remaining > (uint64_t)count) remaining = count;

    while ((uint64_t)copied < remaining) {
        const uint64_t position = offset + copied;
        const uint32_t sector_offset = (uint32_t)(position % BLOCK_SECTOR_SIZE);
        size_t chunk = BLOCK_SECTOR_SIZE - sector_offset;
        if ((uint64_t)chunk > remaining - copied) chunk = (size_t)(remaining - copied);

        if (!block_read(
                node->filesystem->device,
                (uint64_t)entry->start_sector + position / BLOCK_SECTOR_SIZE,
                1u,
                sector_data
            )) return -AXIOM_EIO;
        bytes_copy(out + copied, sector_data + sector_offset, chunk);
        copied += chunk;
    }
    return (int64_t)copied;
}

static int64_t diskfs_write(
    struct vfs_node *vfs_node,
    uint64_t offset,
    const void *buffer,
    size_t count
)
{
    struct diskfs_node *node = node_from_vfs(vfs_node);
    struct diskfs_instance *fs;
    struct diskfs_dir_entry *entry;
    const uint8_t *in = (const uint8_t *)buffer;
    uint8_t sector_data[BLOCK_SECTOR_SIZE];
    uint64_t end;
    size_t copied = 0u;

    if (node == 0 || node->filesystem == 0 ||
        (count != 0u && buffer == 0)) return -AXIOM_EINVAL;
    if (count == 0u) return 0;
    if (UINT64_MAX - offset < (uint64_t)count) return -AXIOM_ENOSPC;
    end = offset + count;
    if (end > UINT32_MAX) return -AXIOM_ENOSPC;

    fs = node->filesystem;
    entry = &fs->entries[node->entry_index];
    if (!ensure_extent(node, end)) return -AXIOM_ENOSPC;

    while (copied < count) {
        const uint64_t position = offset + copied;
        const uint32_t sector_offset = (uint32_t)(position % BLOCK_SECTOR_SIZE);
        size_t chunk = BLOCK_SECTOR_SIZE - sector_offset;
        const uint64_t lba = (uint64_t)entry->start_sector + position / BLOCK_SECTOR_SIZE;
        if (chunk > count - copied) chunk = count - copied;

        if (sector_offset != 0u || chunk != BLOCK_SECTOR_SIZE) {
            if (!block_read(fs->device, lba, 1u, sector_data)) return -AXIOM_EIO;
        } else {
            bytes_clear(sector_data, sizeof(sector_data));
        }
        bytes_copy(sector_data + sector_offset, in + copied, chunk);
        if (!block_write(fs->device, lba, 1u, sector_data)) return -AXIOM_EIO;
        copied += chunk;
    }

    if (end > entry->size) entry->size = (uint32_t)end;
    refresh_node(fs, node->entry_index);
    if (!persist_directory_sector(fs, node->entry_index) || !block_flush(fs->device)) {
        return -AXIOM_EIO;
    }
    return (int64_t)count;
}

static int diskfs_truncate(struct vfs_node *vfs_node, uint64_t size)
{
    struct diskfs_node *node = node_from_vfs(vfs_node);
    struct diskfs_instance *fs;
    struct diskfs_dir_entry *entry;

    if (node == 0 || node->filesystem == 0 || size > UINT32_MAX) return -AXIOM_EINVAL;
    fs = node->filesystem;
    entry = &fs->entries[node->entry_index];

    if (size > entry->size && !ensure_extent(node, size)) return -AXIOM_ENOSPC;
    entry->size = (uint32_t)size;
    refresh_node(fs, node->entry_index);
    return persist_directory_sector(fs, node->entry_index) ? 0 : -AXIOM_EIO;
}

static int diskfs_readdir(
    struct vfs_node *directory,
    size_t requested,
    struct vfs_dirent *entry_out
)
{
    struct diskfs_node *root = node_from_vfs(directory);
    struct diskfs_instance *fs;
    uint32_t index;
    size_t current = 0u;

    if (root == 0 || root->filesystem == 0 || entry_out == 0) return -AXIOM_EINVAL;
    fs = root->filesystem;

    for (index = 0u; index < DISKFS_MAX_FILES; ++index) {
        if (!fs->entries[index].used) continue;
        if (current++ == requested) {
            string_copy(entry_out->name, fs->entries[index].name, sizeof(entry_out->name));
            entry_out->type = AXIOM_DT_FILE;
            return 1;
        }
    }
    return 0;
}

struct vfs_filesystem *diskfs_create(
    struct block_device *device,
    struct diskfs_info *info_out
)
{
    struct diskfs_instance *fs;
    uint32_t index;
    uint32_t file_count = 0u;

    if (device == 0 || device->sector_size != BLOCK_SECTOR_SIZE) return 0;

    fs = (struct diskfs_instance *)kmalloc(sizeof(*fs));
    if (fs == 0) return 0;
    bytes_clear(fs, sizeof(*fs));
    fs->device = device;

    if (!block_read(device, 0u, 1u, &fs->superblock)) return 0;
    if (!superblock_valid(&fs->superblock, device)) {
        if (!format_filesystem(fs)) return 0;
        fs->info.freshly_formatted = 1;
    }
    if (!load_directory(fs)) return 0;

    for (index = 0u; index < DISKFS_MAX_FILES; ++index) {
        if (fs->entries[index].used) {
            ++file_count;
            refresh_node(fs, index);
        }
    }

    fs->root.filesystem = fs;
    fs->root.entry_index = UINT32_MAX;
    string_copy(fs->root.name, "/", sizeof(fs->root.name));
    fs->root.vfs.name = fs->root.name;
    fs->root.vfs.type = VFS_NODE_DIRECTORY;
    fs->root.vfs.mode = DISKFS_ROOT_MODE;
    fs->root.vfs.size = file_count;
    fs->root.vfs.ops = &diskfs_ops;
    fs->root.vfs.private_data = &fs->root;

    fs->vfs.name = "diskfs";
    fs->vfs.root = &fs->root.vfs;
    fs->vfs.private_data = fs;

    fs->info.total_sectors = device->sector_count;
    fs->info.data_start_sector = fs->superblock.data_start;
    fs->info.next_free_sector = fs->superblock.next_free_sector;
    fs->info.file_count = file_count;
    active_instance = fs;

    if (info_out != 0) *info_out = fs->info;
    return &fs->vfs;
}

struct diskfs_info diskfs_get_info(void)
{
    struct diskfs_info empty = {0};
    return active_instance != 0 ? active_instance->info : empty;
}
