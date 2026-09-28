#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>
#include <axiom/filesystem/ramfs.h>
#include <axiom/memory/heap.h>

#define RAMFS_NAME_CAPACITY 32u
#define RAMFS_FILE_MODE 0644u
#define RAMFS_DIRECTORY_MODE 0755u

struct ramfs_node {
    struct vfs_node vfs;
    char name[VFS_NAME_MAX + 1u];
    struct ramfs_node *parent;
    struct ramfs_node *first_child;
    struct ramfs_node *next_sibling;
    uint8_t *data;
    size_t capacity;
};

struct ramfs_instance {
    struct vfs_filesystem filesystem;
    char name[RAMFS_NAME_CAPACITY];
    struct ramfs_node *root;
};

static int ramfs_lookup(
    struct vfs_node *directory,
    const char *name,
    struct vfs_node **node_out
);
static int ramfs_create_node(
    struct vfs_node *directory,
    const char *name,
    enum vfs_node_type type,
    struct vfs_node **node_out
);
static int64_t ramfs_read(
    struct vfs_node *node,
    uint64_t offset,
    void *buffer,
    size_t count
);
static int64_t ramfs_write(
    struct vfs_node *node,
    uint64_t offset,
    const void *buffer,
    size_t count
);
static int ramfs_truncate(struct vfs_node *node, uint64_t size);
static int ramfs_readdir(
    struct vfs_node *directory,
    size_t index,
    struct vfs_dirent *entry_out
);

static const struct vfs_node_ops ramfs_ops = {
    .lookup = ramfs_lookup,
    .create = ramfs_create_node,
    .read = ramfs_read,
    .write = ramfs_write,
    .truncate = ramfs_truncate,
    .readdir = ramfs_readdir,
};

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

static void bytes_clear(void *memory, size_t count)
{
    uint8_t *bytes = (uint8_t *)memory;
    size_t index;

    for (index = 0u; index < count; ++index) {
        bytes[index] = 0u;
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

static struct ramfs_node *ramfs_from_vfs(struct vfs_node *node)
{
    return node != 0 ? (struct ramfs_node *)node->private_data : 0;
}

static struct ramfs_node *allocate_node(
    const char *name,
    enum vfs_node_type type,
    struct ramfs_node *parent
)
{
    struct ramfs_node *node;

    if (name == 0 || string_length(name) > VFS_NAME_MAX) {
        return 0;
    }

    node = (struct ramfs_node *)kmalloc(sizeof(*node));
    if (node == 0) {
        return 0;
    }

    string_copy(node->name, name, sizeof(node->name));
    node->parent = parent;
    node->first_child = 0;
    node->next_sibling = 0;
    node->data = 0;
    node->capacity = 0u;

    node->vfs.name = node->name;
    node->vfs.type = type;
    node->vfs.mode = type == VFS_NODE_DIRECTORY ?
        RAMFS_DIRECTORY_MODE : RAMFS_FILE_MODE;
    node->vfs.size = 0u;
    node->vfs.ops = &ramfs_ops;
    node->vfs.private_data = node;
    return node;
}

static int ramfs_lookup(
    struct vfs_node *directory,
    const char *name,
    struct vfs_node **node_out
)
{
    struct ramfs_node *parent;
    struct ramfs_node *child;

    if (directory == 0 || name == 0 || node_out == 0) {
        return -AXIOM_EINVAL;
    }
    if (directory->type != VFS_NODE_DIRECTORY) {
        return -AXIOM_ENOTDIR;
    }

    parent = ramfs_from_vfs(directory);
    if (parent == 0) {
        return -AXIOM_EIO;
    }

    for (child = parent->first_child; child != 0; child = child->next_sibling) {
        if (strings_equal(child->name, name)) {
            *node_out = &child->vfs;
            return 0;
        }
    }

    *node_out = 0;
    return -AXIOM_ENOENT;
}

static int ramfs_create_node(
    struct vfs_node *directory,
    const char *name,
    enum vfs_node_type type,
    struct vfs_node **node_out
)
{
    struct ramfs_node *parent;
    struct ramfs_node *node;
    struct vfs_node *existing = 0;

    if (directory == 0 || name == 0 || name[0] == '\0' ||
        string_length(name) > VFS_NAME_MAX ||
        (type != VFS_NODE_FILE && type != VFS_NODE_DIRECTORY)) {
        return -AXIOM_EINVAL;
    }
    if (directory->type != VFS_NODE_DIRECTORY) {
        return -AXIOM_ENOTDIR;
    }

    if (ramfs_lookup(directory, name, &existing) == 0) {
        return -AXIOM_EEXIST;
    }

    parent = ramfs_from_vfs(directory);
    if (parent == 0) {
        return -AXIOM_EIO;
    }

    node = allocate_node(name, type, parent);
    if (node == 0) {
        return -AXIOM_ENOMEM;
    }

    node->next_sibling = parent->first_child;
    parent->first_child = node;

    if (node_out != 0) {
        *node_out = &node->vfs;
    }
    return 0;
}

static int64_t ramfs_read(
    struct vfs_node *node,
    uint64_t offset,
    void *buffer,
    size_t count
)
{
    struct ramfs_node *ram_node;
    uint64_t available;
    size_t transfer;

    if (node == 0 || (count != 0u && buffer == 0)) {
        return -AXIOM_EINVAL;
    }
    if (node->type != VFS_NODE_FILE) {
        return -AXIOM_EISDIR;
    }
    if (offset >= node->size || count == 0u) {
        return 0;
    }

    ram_node = ramfs_from_vfs(node);
    if (ram_node == 0) {
        return -AXIOM_EIO;
    }

    available = node->size - offset;
    transfer = available < (uint64_t)count ? (size_t)available : count;

    if (transfer != 0u) {
        if (ram_node->data == 0) {
            return -AXIOM_EIO;
        }
        bytes_copy(buffer, ram_node->data + (size_t)offset, transfer);
    }

    return (int64_t)transfer;
}

static int ensure_capacity(struct ramfs_node *node, size_t required)
{
    size_t new_capacity;
    uint8_t *new_data;

    if (required <= node->capacity) {
        return 0;
    }

    new_capacity = node->capacity == 0u ? 64u : node->capacity;
    while (new_capacity < required) {
        if (new_capacity > SIZE_MAX / 2u) {
            new_capacity = required;
            break;
        }
        new_capacity *= 2u;
    }

    new_data = (uint8_t *)krealloc(node->data, new_capacity);
    if (new_data == 0) {
        return -AXIOM_ENOMEM;
    }

    bytes_clear(new_data + node->capacity, new_capacity - node->capacity);
    node->data = new_data;
    node->capacity = new_capacity;
    return 0;
}

static int64_t ramfs_write(
    struct vfs_node *node,
    uint64_t offset,
    const void *buffer,
    size_t count
)
{
    struct ramfs_node *ram_node;
    uint64_t end_u64;
    size_t end;
    int result;

    if (node == 0 || (count != 0u && buffer == 0)) {
        return -AXIOM_EINVAL;
    }
    if (node->type != VFS_NODE_FILE) {
        return -AXIOM_EISDIR;
    }
    if (count == 0u) {
        return 0;
    }
    if (offset > (uint64_t)SIZE_MAX || UINT64_MAX - offset < (uint64_t)count) {
        return -AXIOM_ENOSPC;
    }

    end_u64 = offset + (uint64_t)count;
    if (end_u64 > (uint64_t)SIZE_MAX) {
        return -AXIOM_ENOSPC;
    }
    end = (size_t)end_u64;

    ram_node = ramfs_from_vfs(node);
    if (ram_node == 0) {
        return -AXIOM_EIO;
    }

    result = ensure_capacity(ram_node, end);
    if (result < 0) {
        return result;
    }

    if (offset > node->size) {
        bytes_clear(
            ram_node->data + (size_t)node->size,
            (size_t)(offset - node->size)
        );
    }

    bytes_copy(ram_node->data + (size_t)offset, buffer, count);
    if (end_u64 > node->size) {
        node->size = end_u64;
    }
    return (int64_t)count;
}

static int ramfs_truncate(struct vfs_node *node, uint64_t size)
{
    struct ramfs_node *ram_node;
    int result;

    if (node == 0 || node->type != VFS_NODE_FILE || size > (uint64_t)SIZE_MAX) {
        return -AXIOM_EINVAL;
    }

    ram_node = ramfs_from_vfs(node);
    if (ram_node == 0) {
        return -AXIOM_EIO;
    }

    result = ensure_capacity(ram_node, (size_t)size);
    if (result < 0) {
        return result;
    }

    if (size > node->size && ram_node->data != 0) {
        bytes_clear(
            ram_node->data + (size_t)node->size,
            (size_t)(size - node->size)
        );
    }

    node->size = size;
    return 0;
}

static int ramfs_readdir(
    struct vfs_node *directory,
    size_t index,
    struct vfs_dirent *entry_out
)
{
    struct ramfs_node *parent;
    struct ramfs_node *child;
    size_t current = 0u;

    if (directory == 0 || entry_out == 0) {
        return -AXIOM_EINVAL;
    }
    if (directory->type != VFS_NODE_DIRECTORY) {
        return -AXIOM_ENOTDIR;
    }

    parent = ramfs_from_vfs(directory);
    if (parent == 0) {
        return -AXIOM_EIO;
    }

    for (child = parent->first_child; child != 0; child = child->next_sibling) {
        if (current == index) {
            string_copy(entry_out->name, child->name, sizeof(entry_out->name));
            entry_out->type = (uint32_t)child->vfs.type;
            return 1;
        }
        ++current;
    }

    return 0;
}

struct vfs_filesystem *ramfs_create(const char *name)
{
    struct ramfs_instance *instance;
    struct ramfs_node *root;

    instance = (struct ramfs_instance *)kmalloc(sizeof(*instance));
    if (instance == 0) {
        return 0;
    }

    root = allocate_node("/", VFS_NODE_DIRECTORY, 0);
    if (root == 0) {
        kfree(instance);
        return 0;
    }

    string_copy(
        instance->name,
        name != 0 ? name : "ramfs",
        sizeof(instance->name)
    );
    instance->root = root;
    instance->filesystem.name = instance->name;
    instance->filesystem.root = &root->vfs;
    instance->filesystem.private_data = instance;
    return &instance->filesystem;
}
