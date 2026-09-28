#include <stddef.h>

#include <axiom/filesystem/vfs.h>
#include <axiom/process/task.h>

const char *task_state_name(enum task_state state)
{
    switch (state) {
        case TASK_RUNNING:
            return "RUNNING";
        case TASK_READY:
            return "READY";
        case TASK_BLOCKED:
            return "BLOCKED";
        case TASK_SLEEPING:
            return "SLEEPING";
        case TASK_TERMINATED:
            return "TERMINATED";
        default:
            return "UNKNOWN";
    }
}

const char *task_privilege_name(enum task_privilege privilege)
{
    return privilege == TASK_PRIVILEGE_USER ? "Ring 3" : "Ring 0";
}

int task_install_file(struct task *task, struct vfs_file *file)
{
    size_t index;

    if (task == 0 || file == 0) {
        return -1;
    }

    for (index = 3u; index < TASK_MAX_FILES; ++index) {
        if (task->file_descriptors[index] == 0) {
            task->file_descriptors[index] = file;
            return (int)index;
        }
    }

    return -1;
}

struct vfs_file *task_file(struct task *task, int fd)
{
    if (task == 0 || fd < 3 || (size_t)fd >= TASK_MAX_FILES) {
        return 0;
    }

    return task->file_descriptors[fd];
}

struct vfs_file *task_take_file(struct task *task, int fd)
{
    struct vfs_file *file;

    if (task == 0 || fd < 3 || (size_t)fd >= TASK_MAX_FILES) {
        return 0;
    }

    file = task->file_descriptors[fd];
    task->file_descriptors[fd] = 0;
    return file;
}

void task_close_all_files(struct task *task)
{
    size_t index;

    if (task == 0) {
        return;
    }

    for (index = 3u; index < TASK_MAX_FILES; ++index) {
        if (task->file_descriptors[index] != 0) {
            (void)vfs_close(task->file_descriptors[index]);
            task->file_descriptors[index] = 0;
        }
    }
}

int task_share_files(struct task *destination, const struct task *source)
{
    size_t index;

    if (destination == 0 || source == 0) {
        return 0;
    }

    for (index = 3u; index < TASK_MAX_FILES; ++index) {
        struct vfs_file *file = source->file_descriptors[index];

        if (file == 0) {
            continue;
        }

        if (vfs_retain(file) < 0) {
            task_close_all_files(destination);
            return 0;
        }
        destination->file_descriptors[index] = file;
    }

    return 1;
}
