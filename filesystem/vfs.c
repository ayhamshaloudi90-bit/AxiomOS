#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>
#include <axiom/filesystem/vfs.h>
#include <axiom/memory/heap.h>

struct vfs_mount_entry {
    char path[VFS_PATH_MAX + 1u];
    size_t path_length;
    struct vfs_filesystem *filesystem;
};

static struct vfs_mount_entry mounts[VFS_MAX_MOUNTS];
static size_t mount_count_value;
static struct vfs_stats stats;
static int initialized;

static size_t string_length(const char *text)
{
    size_t length = 0u;

    if (text == 0) {
        return 0u;
    }

    while (text[length] != '\0') {
        ++length;
    }

    return length;
}

static int strings_equal(const char *left, const char *right)
{
    size_t index = 0u;

    if (left == 0 || right == 0) {
        return 0;
    }

    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] != right[index]) {
            return 0;
        }
        ++index;
    }

    return left[index] == right[index];
}

static void string_copy(char *destination, const char *source, size_t capacity)
{
    size_t index = 0u;

    if (destination == 0 || capacity == 0u) {
        return;
    }

    if (source != 0) {
        while (index + 1u < capacity && source[index] != '\0') {
            destination[index] = source[index];
            ++index;
        }
    }

    destination[index] = '\0';
}

static int normalize_path(const char *path, char *output)
{
    char components[32][VFS_NAME_MAX + 1u];
    size_t component_count = 0u;
    size_t input_index = 0u;
    size_t output_index = 0u;

    if (path == 0 || output == 0 || path[0] != '/') {
        return -AXIOM_EINVAL;
    }

    while (path[input_index] != '\0') {
        char component[VFS_NAME_MAX + 1u];
        size_t length = 0u;

        while (path[input_index] == '/') {
            ++input_index;
        }

        if (path[input_index] == '\0') {
            break;
        }

        while (path[input_index] != '\0' && path[input_index] != '/') {
            if (length >= VFS_NAME_MAX) {
                return -AXIOM_ENAMETOOLONG;
            }

            component[length++] = path[input_index++];
        }
        component[length] = '\0';

        if (length == 1u && component[0] == '.') {
            continue;
        }

        if (length == 2u && component[0] == '.' && component[1] == '.') {
            if (component_count != 0u) {
                --component_count;
            }
            continue;
        }

        if (component_count >= 32u) {
            return -AXIOM_ENAMETOOLONG;
        }

        string_copy(
            components[component_count],
            component,
            sizeof(components[component_count])
        );
        ++component_count;
    }

    output[output_index++] = '/';

    for (size_t index = 0u; index < component_count; ++index) {
        const size_t length = string_length(components[index]);

        if (output_index + length + (index + 1u < component_count ? 1u : 0u) >
            VFS_PATH_MAX) {
            return -AXIOM_ENAMETOOLONG;
        }

        for (size_t character = 0u; character < length; ++character) {
            output[output_index++] = components[index][character];
        }

        if (index + 1u < component_count) {
            output[output_index++] = '/';
        }
    }

    output[output_index] = '\0';
    return 0;
}

static int mount_matches(
    const struct vfs_mount_entry *mount,
    const char *path
)
{
    size_t index;

    if (mount == 0 || path == 0) {
        return 0;
    }

    if (mount->path_length == 1u && mount->path[0] == '/') {
        return 1;
    }

    for (index = 0u; index < mount->path_length; ++index) {
        if (path[index] != mount->path[index]) {
            return 0;
        }
    }

    return path[mount->path_length] == '\0' ||
        path[mount->path_length] == '/';
}

static const struct vfs_mount_entry *best_mount(const char *normalized_path)
{
    const struct vfs_mount_entry *best = 0;
    size_t index;

    for (index = 0u; index < mount_count_value; ++index) {
        if (mount_matches(&mounts[index], normalized_path) &&
            (best == 0 || mounts[index].path_length > best->path_length)) {
            best = &mounts[index];
        }
    }

    return best;
}

static int lookup_normalized(
    const char *normalized_path,
    struct vfs_node **node_out
)
{
    const struct vfs_mount_entry *mount;
    struct vfs_node *current;
    const char *cursor;

    if (normalized_path == 0 || node_out == 0) {
        return -AXIOM_EINVAL;
    }

    mount = best_mount(normalized_path);
    if (mount == 0 || mount->filesystem == 0 ||
        mount->filesystem->root == 0) {
        return -AXIOM_ENOENT;
    }

    current = mount->filesystem->root;

    if (mount->path_length == 1u) {
        cursor = normalized_path + 1u;
    } else {
        cursor = normalized_path + mount->path_length;
        if (*cursor == '/') {
            ++cursor;
        }
    }

    while (*cursor != '\0') {
        char component[VFS_NAME_MAX + 1u];
        size_t length = 0u;
        struct vfs_node *next = 0;
        int result;

        while (cursor[length] != '\0' && cursor[length] != '/') {
            if (length >= VFS_NAME_MAX) {
                return -AXIOM_ENAMETOOLONG;
            }
            component[length] = cursor[length];
            ++length;
        }
        component[length] = '\0';

        if (current->type != VFS_NODE_DIRECTORY) {
            return -AXIOM_ENOTDIR;
        }
        if (current->ops == 0 || current->ops->lookup == 0) {
            return -AXIOM_ENOSYS;
        }

        result = current->ops->lookup(current, component, &next);
        if (result < 0) {
            return result;
        }
        if (next == 0) {
            return -AXIOM_ENOENT;
        }

        current = next;
        cursor += length;
        if (*cursor == '/') {
            ++cursor;
        }
    }

    *node_out = current;
    return 0;
}

static int split_parent(
    const char *normalized_path,
    char *parent_out,
    char *name_out
)
{
    size_t length;
    size_t slash;

    if (normalized_path == 0 || parent_out == 0 || name_out == 0 ||
        strings_equal(normalized_path, "/")) {
        return -AXIOM_EINVAL;
    }

    length = string_length(normalized_path);
    slash = length;

    while (slash > 0u && normalized_path[slash - 1u] != '/') {
        --slash;
    }

    if (slash == 0u || length - slash > VFS_NAME_MAX) {
        return -AXIOM_EINVAL;
    }

    string_copy(name_out, normalized_path + slash, VFS_NAME_MAX + 1u);

    if (slash == 1u) {
        parent_out[0] = '/';
        parent_out[1] = '\0';
    } else {
        size_t index;

        for (index = 0u; index + 1u < slash; ++index) {
            parent_out[index] = normalized_path[index];
        }
        parent_out[slash - 1u] = '\0';
    }

    return 0;
}

static int create_node_at_path(
    const char *normalized_path,
    enum vfs_node_type type,
    struct vfs_node **node_out
)
{
    char parent[VFS_PATH_MAX + 1u];
    char name[VFS_NAME_MAX + 1u];
    struct vfs_node *parent_node;
    struct vfs_node *created = 0;
    int result;

    result = split_parent(normalized_path, parent, name);
    if (result < 0) {
        return result;
    }

    result = lookup_normalized(parent, &parent_node);
    if (result < 0) {
        return result;
    }

    if (parent_node->type != VFS_NODE_DIRECTORY) {
        return -AXIOM_ENOTDIR;
    }
    if (parent_node->ops == 0 || parent_node->ops->create == 0) {
        return -AXIOM_EROFS;
    }

    result = parent_node->ops->create(parent_node, name, type, &created);
    if (result < 0) {
        return result;
    }

    if (node_out != 0) {
        *node_out = created;
    }
    return 0;
}

int vfs_init(void)
{
    size_t index;

    if (initialized) {
        return 0;
    }

    for (index = 0u; index < VFS_MAX_MOUNTS; ++index) {
        mounts[index].path[0] = '\0';
        mounts[index].path_length = 0u;
        mounts[index].filesystem = 0;
    }

    mount_count_value = 0u;
    stats.mount_count = 0u;
    stats.path_lookups = 0u;
    stats.opens = 0u;
    stats.closes = 0u;
    stats.reads = 0u;
    stats.writes = 0u;
    stats.seeks = 0u;
    stats.stats = 0u;
    stats.bytes_read = 0u;
    stats.bytes_written = 0u;
    initialized = 1;
    return 1;
}

int vfs_initialized(void)
{
    return initialized;
}

int vfs_mount(const char *path, struct vfs_filesystem *filesystem)
{
    char normalized[VFS_PATH_MAX + 1u];
    size_t index;
    int result;

    if (!initialized || filesystem == 0 || filesystem->root == 0 ||
        filesystem->name == 0) {
        return -AXIOM_EINVAL;
    }

    if (mount_count_value >= VFS_MAX_MOUNTS) {
        return -AXIOM_ENOSPC;
    }

    result = normalize_path(path, normalized);
    if (result < 0) {
        return result;
    }

    for (index = 0u; index < mount_count_value; ++index) {
        if (strings_equal(mounts[index].path, normalized)) {
            return -AXIOM_EBUSY;
        }
    }

    if (!strings_equal(normalized, "/")) {
        struct vfs_node *mountpoint = 0;

        result = lookup_normalized(normalized, &mountpoint);
        if (result < 0) {
            return result;
        }
        if (mountpoint->type != VFS_NODE_DIRECTORY) {
            return -AXIOM_ENOTDIR;
        }
    }

    string_copy(
        mounts[mount_count_value].path,
        normalized,
        sizeof(mounts[mount_count_value].path)
    );
    mounts[mount_count_value].path_length = string_length(normalized);
    mounts[mount_count_value].filesystem = filesystem;
    ++mount_count_value;
    stats.mount_count = mount_count_value;
    return 0;
}

int vfs_lookup(const char *path, struct vfs_node **node_out)
{
    char normalized[VFS_PATH_MAX + 1u];
    int result;

    if (!initialized || node_out == 0) {
        return -AXIOM_EINVAL;
    }

    ++stats.path_lookups;

    result = normalize_path(path, normalized);
    if (result < 0) {
        return result;
    }

    return lookup_normalized(normalized, node_out);
}

int vfs_mkdir(const char *path)
{
    char normalized[VFS_PATH_MAX + 1u];
    struct vfs_node *existing = 0;
    int result;

    if (!initialized) {
        return -AXIOM_ENOSYS;
    }

    result = normalize_path(path, normalized);
    if (result < 0) {
        return result;
    }

    result = lookup_normalized(normalized, &existing);
    if (result == 0) {
        return -AXIOM_EEXIST;
    }
    if (result != -AXIOM_ENOENT) {
        return result;
    }

    return create_node_at_path(normalized, VFS_NODE_DIRECTORY, 0);
}

int vfs_readdir(const char *path, size_t index, struct vfs_dirent *entry_out)
{
    struct vfs_node *directory;
    int result;

    if (!initialized || entry_out == 0) {
        return -AXIOM_EINVAL;
    }

    result = vfs_lookup(path, &directory);
    if (result < 0) {
        return result;
    }
    if (directory->type != VFS_NODE_DIRECTORY) {
        return -AXIOM_ENOTDIR;
    }
    if (directory->ops == 0 || directory->ops->readdir == 0) {
        return -AXIOM_ENOSYS;
    }

    return directory->ops->readdir(directory, index, entry_out);
}

int vfs_open(const char *path, uint32_t flags, struct vfs_file **file_out)
{
    char normalized[VFS_PATH_MAX + 1u];
    struct vfs_node *node = 0;
    struct vfs_file *file;
    const uint32_t access = flags & AXIOM_O_ACCMODE;
    int result;

    if (!initialized) {
        return -AXIOM_ENOSYS;
    }
    if (file_out == 0 || access > AXIOM_O_RDWR ||
        ((flags & AXIOM_O_TRUNC) != 0u && access == AXIOM_O_RDONLY)) {
        return -AXIOM_EINVAL;
    }

    result = normalize_path(path, normalized);
    if (result < 0) {
        return result;
    }

    ++stats.path_lookups;
    result = lookup_normalized(normalized, &node);

    if (result == -AXIOM_ENOENT && (flags & AXIOM_O_CREAT) != 0u) {
        result = create_node_at_path(normalized, VFS_NODE_FILE, &node);
    }
    if (result < 0) {
        return result;
    }
    if (node == 0) {
        return -AXIOM_ENOENT;
    }
    if (node->type == VFS_NODE_DIRECTORY) {
        return -AXIOM_EISDIR;
    }

    /* Phase 20: enforce the simple ownerless permission mask. */
    if ((access == AXIOM_O_RDONLY || access == AXIOM_O_RDWR) &&
        (node->mode & AXIOM_S_IRALL) == 0u) {
        return -AXIOM_EACCES;
    }
    if ((access == AXIOM_O_WRONLY || access == AXIOM_O_RDWR) &&
        (node->mode & AXIOM_S_IWALL) == 0u) {
        return -AXIOM_EACCES;
    }

    if ((flags & AXIOM_O_TRUNC) != 0u) {
        if (node->ops == 0 || node->ops->truncate == 0) {
            return -AXIOM_EROFS;
        }

        result = node->ops->truncate(node, 0u);
        if (result < 0) {
            return result;
        }
    }

    file = (struct vfs_file *)kmalloc(sizeof(*file));
    if (file == 0) {
        return -AXIOM_ENOMEM;
    }

    file->node = node;
    file->flags = flags;
    file->offset = (flags & AXIOM_O_APPEND) != 0u ? node->size : 0u;
    file->reference_count = 1u;
    *file_out = file;
    ++stats.opens;
    return 0;
}

int vfs_retain(struct vfs_file *file)
{
    if (file == 0 || file->reference_count == 0u ||
        file->reference_count == UINT32_MAX) {
        return -AXIOM_EBADF;
    }

    ++file->reference_count;
    return 0;
}

int vfs_close(struct vfs_file *file)
{
    if (file == 0 || file->reference_count == 0u) {
        return -AXIOM_EBADF;
    }

    --file->reference_count;
    if (file->reference_count == 0u) {
        kfree(file);
    }
    ++stats.closes;
    return 0;
}

int64_t vfs_read(struct vfs_file *file, void *buffer, size_t count)
{
    int64_t result;
    const uint32_t access = file != 0 ? file->flags & AXIOM_O_ACCMODE : 0u;

    if (file == 0 || file->node == 0) {
        return -AXIOM_EBADF;
    }
    if (count != 0u && buffer == 0) {
        return -AXIOM_EFAULT;
    }
    if (access == AXIOM_O_WRONLY) {
        return -AXIOM_EACCES;
    }
    if (file->node->ops == 0 || file->node->ops->read == 0) {
        return -AXIOM_EISDIR;
    }

    result = file->node->ops->read(file->node, file->offset, buffer, count);
    if (result > 0) {
        file->offset += (uint64_t)result;
        stats.bytes_read += (uint64_t)result;
    }
    ++stats.reads;
    return result;
}

int64_t vfs_write(struct vfs_file *file, const void *buffer, size_t count)
{
    int64_t result;
    const uint32_t access = file != 0 ? file->flags & AXIOM_O_ACCMODE : 0u;

    if (file == 0 || file->node == 0) {
        return -AXIOM_EBADF;
    }
    if (count != 0u && buffer == 0) {
        return -AXIOM_EFAULT;
    }
    if (access == AXIOM_O_RDONLY) {
        return -AXIOM_EACCES;
    }
    if (file->node->ops == 0 || file->node->ops->write == 0) {
        return -AXIOM_EISDIR;
    }

    if ((file->flags & AXIOM_O_APPEND) != 0u) {
        file->offset = file->node->size;
    }

    result = file->node->ops->write(file->node, file->offset, buffer, count);
    if (result > 0) {
        file->offset += (uint64_t)result;
        stats.bytes_written += (uint64_t)result;
    }
    ++stats.writes;
    return result;
}

int64_t vfs_seek(struct vfs_file *file, int64_t offset, int whence)
{
    int64_t base;
    int64_t next;

    if (file == 0 || file->node == 0) {
        return -AXIOM_EBADF;
    }

    if (file->node->size > (uint64_t)INT64_MAX ||
        file->offset > (uint64_t)INT64_MAX) {
        return -AXIOM_EINVAL;
    }

    switch (whence) {
        case AXIOM_SEEK_SET:
            base = 0;
            break;
        case AXIOM_SEEK_CUR:
            base = (int64_t)file->offset;
            break;
        case AXIOM_SEEK_END:
            base = (int64_t)file->node->size;
            break;
        default:
            return -AXIOM_EINVAL;
    }

    if ((offset > 0 && base > INT64_MAX - offset) ||
        (offset < 0 && base < INT64_MIN - offset)) {
        return -AXIOM_EINVAL;
    }

    next = base + offset;
    if (next < 0) {
        return -AXIOM_EINVAL;
    }

    file->offset = (uint64_t)next;
    ++stats.seeks;
    return next;
}

int vfs_stat(const char *path, struct axiom_stat *stat_out)
{
    struct vfs_node *node;
    int result;

    if (!initialized || stat_out == 0) {
        return -AXIOM_EINVAL;
    }

    result = vfs_lookup(path, &node);
    if (result < 0) {
        return result;
    }

    stat_out->size = node->size;
    stat_out->type = (uint32_t)node->type;
    stat_out->mode = node->mode;
    ++stats.stats;
    return 0;
}


int vfs_set_mode(const char *path, uint32_t mode)
{
    struct vfs_node *node;
    int result = vfs_lookup(path, &node);

    if (result < 0) return result;
    if (node == 0) return -AXIOM_ENOENT;
    node->mode = mode & 0777u;
    return 0;
}

int vfs_check_access(const char *path, uint32_t required_mode)
{
    struct vfs_node *node;
    int result = vfs_lookup(path, &node);

    if (result < 0) return result;
    if (node == 0) return -AXIOM_ENOENT;
    if ((node->mode & required_mode) == 0u) return -AXIOM_EACCES;
    return 0;
}

int vfs_write_file(const char *path, const void *data, size_t size)
{
    struct vfs_file *file = 0;
    int result;
    int64_t written;

    result = vfs_open(
        path,
        AXIOM_O_CREAT | AXIOM_O_RDWR | AXIOM_O_TRUNC,
        &file
    );
    if (result < 0) {
        return result;
    }

    written = vfs_write(file, data, size);
    (void)vfs_close(file);

    if (written < 0) {
        return (int)written;
    }
    return (size_t)written == size ? 0 : -AXIOM_EIO;
}

int vfs_read_all(const char *path, void **data_out, size_t *size_out)
{
    struct axiom_stat status;
    struct vfs_file *file = 0;
    uint8_t *buffer;
    size_t total = 0u;
    int result;

    if (data_out == 0 || size_out == 0) {
        return -AXIOM_EINVAL;
    }

    result = vfs_stat(path, &status);
    if (result < 0) {
        return result;
    }
    if (status.type != AXIOM_DT_FILE || status.size > (uint64_t)SIZE_MAX) {
        return -AXIOM_EINVAL;
    }

    buffer = (uint8_t *)kmalloc(status.size == 0u ? 1u : (size_t)status.size);
    if (buffer == 0) {
        return -AXIOM_ENOMEM;
    }

    result = vfs_open(path, AXIOM_O_RDONLY, &file);
    if (result < 0) {
        kfree(buffer);
        return result;
    }

    while (total < (size_t)status.size) {
        const int64_t got = vfs_read(
            file,
            buffer + total,
            (size_t)status.size - total
        );

        if (got < 0) {
            (void)vfs_close(file);
            kfree(buffer);
            return (int)got;
        }
        if (got == 0) {
            break;
        }
        total += (size_t)got;
    }

    (void)vfs_close(file);

    if (total != (size_t)status.size) {
        kfree(buffer);
        return -AXIOM_EIO;
    }

    *data_out = buffer;
    *size_out = total;
    return 0;
}

struct vfs_stats vfs_get_stats(void)
{
    return stats;
}
