#ifndef AXIOM_FILESYSTEM_VFS_H
#define AXIOM_FILESYSTEM_VFS_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/fs.h>

#define VFS_NAME_MAX 63u
#define VFS_PATH_MAX 255u
#define VFS_MAX_MOUNTS 8u

enum vfs_node_type {
    VFS_NODE_FILE = AXIOM_DT_FILE,
    VFS_NODE_DIRECTORY = AXIOM_DT_DIR,
};

struct vfs_node;
struct vfs_filesystem;

struct vfs_dirent {
    char name[VFS_NAME_MAX + 1u];
    uint32_t type;
};

struct vfs_node_ops {
    int (*lookup)(
        struct vfs_node *directory,
        const char *name,
        struct vfs_node **node_out
    );
    int (*create)(
        struct vfs_node *directory,
        const char *name,
        enum vfs_node_type type,
        struct vfs_node **node_out
    );
    int64_t (*read)(
        struct vfs_node *node,
        uint64_t offset,
        void *buffer,
        size_t count
    );
    int64_t (*write)(
        struct vfs_node *node,
        uint64_t offset,
        const void *buffer,
        size_t count
    );
    int (*truncate)(struct vfs_node *node, uint64_t size);
    int (*readdir)(
        struct vfs_node *directory,
        size_t index,
        struct vfs_dirent *entry_out
    );
};

struct vfs_node {
    const char *name;
    enum vfs_node_type type;
    uint32_t mode;
    uint64_t size;
    const struct vfs_node_ops *ops;
    void *private_data;
};

struct vfs_filesystem {
    const char *name;
    struct vfs_node *root;
    void *private_data;
};

struct vfs_file {
    struct vfs_node *node;
    uint64_t offset;
    uint32_t flags;
    uint32_t reference_count;
};

struct vfs_stats {
    uint64_t mount_count;
    uint64_t path_lookups;
    uint64_t opens;
    uint64_t closes;
    uint64_t reads;
    uint64_t writes;
    uint64_t seeks;
    uint64_t stats;
    uint64_t bytes_read;
    uint64_t bytes_written;
};

int vfs_init(void);
int vfs_initialized(void);

int vfs_mount(const char *path, struct vfs_filesystem *filesystem);
int vfs_lookup(const char *path, struct vfs_node **node_out);
int vfs_mkdir(const char *path);
int vfs_readdir(const char *path, size_t index, struct vfs_dirent *entry_out);

int vfs_open(const char *path, uint32_t flags, struct vfs_file **file_out);
int vfs_close(struct vfs_file *file);
int vfs_retain(struct vfs_file *file);
int64_t vfs_read(struct vfs_file *file, void *buffer, size_t count);
int64_t vfs_write(struct vfs_file *file, const void *buffer, size_t count);
int64_t vfs_seek(struct vfs_file *file, int64_t offset, int whence);
int vfs_stat(const char *path, struct axiom_stat *stat_out);
int vfs_set_mode(const char *path, uint32_t mode);
int vfs_check_access(const char *path, uint32_t required_mode);

/* Kernel convenience helpers used by the ELF loader/bootstrap tests. */
int vfs_write_file(const char *path, const void *data, size_t size);
int vfs_read_all(const char *path, void **data_out, size_t *size_out);

struct vfs_stats vfs_get_stats(void);

#endif
