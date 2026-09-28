#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/syscall.h>
#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/drivers/keyboard.h>
#include <axiom/drivers/mouse.h>
#include <axiom/drivers/timer.h>
#include <axiom/filesystem/vfs.h>
#include <axiom/graphics/framebuffer.h>
#include <axiom/kernel/panic.h>
#include <axiom/ipc/ipc.h>
#include <axiom/kernel/syscall.h>
#include <axiom/memory/heap.h>
#include <axiom/memory/vmm.h>
#include <axiom/network/net.h>
#include <axiom/process/scheduler.h>
#include <axiom/terminal/terminal.h>

#define IA32_EFER  0xC0000080u
#define IA32_STAR  0xC0000081u
#define IA32_LSTAR 0xC0000082u
#define IA32_FMASK 0xC0000084u

#define EFER_SCE (1ULL << 0)
#define RFLAGS_TF (1ULL << 8)
#define RFLAGS_IF (1ULL << 9)
#define RFLAGS_DF (1ULL << 10)

#define SYSCALL_WRITE_MAX 4096u
#define SYSCALL_READ_MAX  4096u
#define SYSCALL_SLEEP_MAX_MS 60000ULL
#define SYSCALL_HTTP_MAX AXIOM_NET_HTTP_USER_MAX

static uint8_t syscall_http_buffer[AXIOM_NET_HTTP_USER_MAX];

/* Assembly reads this immediately after SYSCALL while IF is masked. */
uintptr_t syscall_kernel_stack_top;

static struct syscall_stats stats;
static int initialized;

extern void syscall_entry(void);

static uint64_t rdmsr(uint32_t msr)
{
    uint32_t low;
    uint32_t high;

    __asm__ volatile (
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(msr)
    );

    return ((uint64_t)high << 32) | low;
}

static void wrmsr(uint32_t msr, uint64_t value)
{
    __asm__ volatile (
        "wrmsr"
        :
        : "c"(msr),
          "a"((uint32_t)value),
          "d"((uint32_t)(value >> 32))
        : "memory"
    );
}

static int cpu_supports_syscall(void)
{
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;

    eax = 0x80000000u;
    __asm__ volatile (
        "cpuid"
        : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
    );

    if (eax < 0x80000001u) {
        return 0;
    }

    eax = 0x80000001u;
    __asm__ volatile (
        "cpuid"
        : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
    );

    return (edx & (1u << 11)) != 0u;
}

static int64_t syscall_error(int error)
{
    return -(int64_t)error;
}

static struct task *current_user_task(void)
{
    struct task *task = scheduler_current_task_mutable();

    if (task == 0 || task->privilege != TASK_PRIVILEGE_USER ||
        task->address_space == PADDR_INVALID) {
        return 0;
    }

    return task;
}

static int copy_user_path(
    const struct task *task,
    vaddr_t user_path,
    char *destination,
    size_t capacity
)
{
    size_t index;

    if (task == 0 || destination == 0 || capacity < 2u) {
        return -AXIOM_EINVAL;
    }

    for (index = 0u; index + 1u < capacity; ++index) {
        if (!vmm_copy_from_user(
                task->address_space,
                &destination[index],
                user_path + (vaddr_t)index,
                1u
            )) {
            return -AXIOM_EFAULT;
        }

        if (destination[index] == '\0') {
            return 0;
        }
    }

    destination[capacity - 1u] = '\0';
    return -AXIOM_ENAMETOOLONG;
}

static int64_t sys_write(uint64_t fd, vaddr_t buffer, uint64_t count)
{
    struct task *task = current_user_task();
    struct vfs_file *file = 0;
    uint64_t transferred = 0u;
    uint8_t local[128];

    ++stats.write_calls;

    if (count > SYSCALL_WRITE_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }
    if (count == 0u) {
        return 0;
    }
    if (task == 0 ||
        !vmm_user_range_accessible(task->address_space, buffer, (size_t)count, 0)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    if (fd != 1u && fd != 2u) {
        if (fd > (uint64_t)INT32_MAX) {
            return syscall_error(AXIOM_EBADF);
        }
        file = task_file(task, (int)fd);
        if (file == 0) {
            return syscall_error(AXIOM_EBADF);
        }
    }

    while (transferred < count) {
        const size_t chunk =
            (count - transferred < sizeof(local)) ?
            (size_t)(count - transferred) : sizeof(local);

        if (!vmm_copy_from_user(
                task->address_space,
                local,
                buffer + transferred,
                chunk
            )) {
            ++stats.rejected_pointers;
            return transferred != 0u ?
                (int64_t)transferred : syscall_error(AXIOM_EFAULT);
        }

        if (file == 0) {
            size_t index;

            for (index = 0u; index < chunk; ++index) {
                terminal_putchar((char)local[index]);
            }
            transferred += chunk;
        } else {
            const int64_t written = vfs_write(file, local, chunk);

            if (written < 0) {
                return transferred != 0u ? (int64_t)transferred : written;
            }
            if (written == 0) {
                break;
            }
            transferred += (uint64_t)written;
            if ((size_t)written < chunk) {
                break;
            }
        }
    }

    stats.bytes_written += transferred;
    return (int64_t)transferred;
}

static int64_t sys_read(uint64_t fd, vaddr_t buffer, uint64_t count)
{
    struct task *task = current_user_task();
    struct vfs_file *file = 0;
    uint64_t transferred = 0u;
    uint8_t local[128];

    ++stats.read_calls;

    if (count > SYSCALL_READ_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }
    if (count == 0u) {
        return 0;
    }
    if (task == 0 ||
        !vmm_user_range_accessible(task->address_space, buffer, (size_t)count, 1)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    if (fd != 0u) {
        if (fd > (uint64_t)INT32_MAX) {
            return syscall_error(AXIOM_EBADF);
        }
        file = task_file(task, (int)fd);
        if (file == 0) {
            return syscall_error(AXIOM_EBADF);
        }
    }

    if (file == 0) {
        while (transferred < count) {
            char character;

            if (!keyboard_read_char(&character)) {
                break;
            }

            if (!vmm_copy_to_user(
                    task->address_space,
                    buffer + transferred,
                    &character,
                    1u
                )) {
                ++stats.rejected_pointers;
                return transferred != 0u ?
                    (int64_t)transferred : syscall_error(AXIOM_EFAULT);
            }
            ++transferred;
        }
    } else {
        while (transferred < count) {
            const size_t chunk =
                (count - transferred < sizeof(local)) ?
                (size_t)(count - transferred) : sizeof(local);
            const int64_t got = vfs_read(file, local, chunk);

            if (got < 0) {
                return transferred != 0u ? (int64_t)transferred : got;
            }
            if (got == 0) {
                break;
            }

            if (!vmm_copy_to_user(
                    task->address_space,
                    buffer + transferred,
                    local,
                    (size_t)got
                )) {
                ++stats.rejected_pointers;
                return transferred != 0u ?
                    (int64_t)transferred : syscall_error(AXIOM_EFAULT);
            }

            transferred += (uint64_t)got;
            if ((size_t)got < chunk) {
                break;
            }
        }
    }

    stats.bytes_read += transferred;
    return (int64_t)transferred;
}

static int64_t sys_open(vaddr_t user_path, uint64_t flags)
{
    struct task *task = current_user_task();
    struct vfs_file *file = 0;
    char path[VFS_PATH_MAX + 1u];
    int copied;
    int result;
    int fd;

    ++stats.open_calls;

    if (!vfs_initialized()) {
        ++stats.unimplemented_calls;
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0 || flags > UINT32_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    result = vfs_open(path, (uint32_t)flags, &file);
    if (result < 0) {
        return result;
    }

    fd = task_install_file(task, file);
    if (fd < 0) {
        (void)vfs_close(file);
        return syscall_error(AXIOM_EMFILE);
    }

    return fd;
}

static int64_t sys_close(uint64_t fd)
{
    struct task *task = current_user_task();
    struct vfs_file *file;

    ++stats.close_calls;

    if (!vfs_initialized()) {
        ++stats.unimplemented_calls;
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0 || fd > (uint64_t)INT32_MAX) {
        return syscall_error(AXIOM_EBADF);
    }

    file = task_take_file(task, (int)fd);
    if (file == 0) {
        return syscall_error(AXIOM_EBADF);
    }

    return vfs_close(file);
}

static int64_t sys_fork(struct syscall_frame *frame)
{
    uint64_t child_id = 0u;

    ++stats.fork_calls;

    if (frame == 0 || current_user_task() == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    if (!scheduler_fork_current(frame, &child_id)) {
        return syscall_error(AXIOM_ENOMEM);
    }

    return (int64_t)child_id;
}

static int64_t sys_exec(vaddr_t user_path, struct syscall_frame *frame)
{
    struct task *task = current_user_task();
    char path[VFS_PATH_MAX + 1u];
    void *image = 0;
    size_t image_size = 0u;
    vaddr_t entry;
    vaddr_t stack;
    int copied;
    int result;

    ++stats.exec_calls;

    if (!vfs_initialized()) {
        ++stats.unimplemented_calls;
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0 || frame == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    result = vfs_check_access(path, AXIOM_S_IXALL);
    if (result < 0) {
        return result;
    }

    result = vfs_read_all(path, &image, &image_size);
    if (result < 0) {
        return result;
    }

    if (!scheduler_exec_current_elf(
            image,
            image_size,
            &entry,
            &stack
        )) {
        kfree(image);
        return syscall_error(AXIOM_EINVAL);
    }
    kfree(image);

    /* exec() starts the new image with a clean user register set. */
    frame->r15 = 0u;
    frame->r14 = 0u;
    frame->r13 = 0u;
    frame->r12 = 0u;
    frame->r10 = 0u;
    frame->r9 = 0u;
    frame->r8 = 0u;
    frame->rbp = 0u;
    frame->rdi = 0u;
    frame->rsi = 0u;
    frame->rdx = 0u;
    frame->rbx = 0u;
    frame->user_rip = entry;
    frame->user_rflags = 0x202ULL;
    frame->user_rsp = stack;

    ++stats.exec_successes;
    return 0;
}

static int64_t sys_lseek(uint64_t fd, int64_t offset, uint64_t whence)
{
    struct task *task = current_user_task();
    struct vfs_file *file;

    ++stats.lseek_calls;

    if (!vfs_initialized()) {
        ++stats.unimplemented_calls;
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0 || fd > (uint64_t)INT32_MAX || whence > INT32_MAX) {
        return syscall_error(AXIOM_EBADF);
    }

    file = task_file(task, (int)fd);
    if (file == 0) {
        return syscall_error(AXIOM_EBADF);
    }

    return vfs_seek(file, offset, (int)whence);
}

static int64_t sys_stat(vaddr_t user_path, vaddr_t user_stat)
{
    struct task *task = current_user_task();
    char path[VFS_PATH_MAX + 1u];
    struct axiom_stat status;
    int copied;
    int result;

    ++stats.stat_calls;

    if (!vfs_initialized()) {
        ++stats.unimplemented_calls;
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    if (!vmm_user_range_accessible(
            task->address_space,
            user_stat,
            sizeof(status),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    result = vfs_stat(path, &status);
    if (result < 0) {
        return result;
    }

    if (!vmm_copy_to_user(
            task->address_space,
            user_stat,
            &status,
            sizeof(status)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    return 0;
}


static int64_t sys_readdir(vaddr_t user_path, uint64_t index, vaddr_t user_entry)
{
    struct task *task = current_user_task();
    char path[VFS_PATH_MAX + 1u];
    struct vfs_dirent entry;
    struct axiom_dirent user_value;
    int copied;
    int result;

    ++stats.readdir_calls;

    if (!vfs_initialized()) {
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0 || index > (uint64_t)SIZE_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    if (!vmm_user_range_accessible(
            task->address_space,
            user_entry,
            sizeof(user_value),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    result = vfs_readdir(path, (size_t)index, &entry);
    if (result <= 0) {
        return result;
    }

    for (size_t i = 0u; i < sizeof(user_value.name); ++i) {
        user_value.name[i] = entry.name[i];
        if (entry.name[i] == '\0') {
            for (++i; i < sizeof(user_value.name); ++i) {
                user_value.name[i] = '\0';
            }
            break;
        }
    }
    user_value.type = entry.type;

    if (!vmm_copy_to_user(
            task->address_space,
            user_entry,
            &user_value,
            sizeof(user_value)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    return 1;
}

static int64_t sys_mkdir(vaddr_t user_path)
{
    struct task *task = current_user_task();
    char path[VFS_PATH_MAX + 1u];
    int copied;

    ++stats.mkdir_calls;

    if (!vfs_initialized()) {
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    return vfs_mkdir(path);
}

static int64_t sys_spawn(vaddr_t user_path)
{
    struct task *parent = current_user_task();
    struct task *child;
    char path[VFS_PATH_MAX + 1u];
    void *image = 0;
    size_t image_size = 0u;
    uint64_t child_id;
    int copied;
    int result;

    ++stats.spawn_calls;

    if (!vfs_initialized()) {
        return syscall_error(AXIOM_ENOSYS);
    }
    if (parent == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(parent, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    result = vfs_check_access(path, AXIOM_S_IXALL);
    if (result < 0) {
        return result;
    }

    result = vfs_read_all(path, &image, &image_size);
    if (result < 0) {
        return result;
    }

    if (!user_task_create_elf("spawned-elf", image, image_size, &child_id)) {
        kfree(image);
        return syscall_error(AXIOM_EINVAL);
    }
    kfree(image);

    child = scheduler_task_by_id_mutable(child_id);
    if (child == 0) {
        return syscall_error(AXIOM_EINVAL);
    }
    child->parent_id = parent->id;
    return (int64_t)child_id;
}

static int64_t sys_waitpid(uint64_t pid, vaddr_t user_status)
{
    struct task *parent = current_user_task();
    int64_t status;

    ++stats.waitpid_calls;

    if (parent == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    if (user_status != 0u &&
        !vmm_user_range_accessible(
            parent->address_space,
            user_status,
            sizeof(status),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    if (!scheduler_wait_current_child(pid, &status)) {
        return syscall_error(AXIOM_ECHILD);
    }

    if (user_status != 0u &&
        !vmm_copy_to_user(
            parent->address_space,
            user_status,
            &status,
            sizeof(status)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    return (int64_t)pid;
}

static int64_t sys_clear(void)
{
    ++stats.clear_calls;
    terminal_clear();
    return 0;
}

static int64_t sys_kbdstats(vaddr_t user_stats)
{
    struct task *task = current_user_task();
    const struct keyboard_stats source = keyboard_get_stats();
    struct axiom_keyboard_stats destination;

    ++stats.kbdstats_calls;

    if (task == 0 ||
        !vmm_user_range_accessible(
            task->address_space,
            user_stats,
            sizeof(destination),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    destination.irq_count = source.irq_count;
    destination.scancode_count = source.scancode_count;
    destination.character_count = source.character_count;
    destination.dropped_characters = source.dropped_characters;

    if (!vmm_copy_to_user(
            task->address_space,
            user_stats,
            &destination,
            sizeof(destination)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    return 0;
}

static int64_t sys_mousestate(vaddr_t user_state)
{
    struct task *task = current_user_task();
    const struct mouse_state source = mouse_get_state();
    struct axiom_mouse_state destination;

    if (task == 0 ||
        !vmm_user_range_accessible(
            task->address_space,
            user_state,
            sizeof(destination),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    destination.x = source.x;
    destination.y = source.y;
    destination.irq_count = source.irq_count;
    destination.packet_count = source.packet_count;
    destination.buttons = source.buttons;
    destination.available = source.available;
    for (size_t index = 0u; index < sizeof(destination.reserved); ++index) {
        destination.reserved[index] = 0u;
    }

    if (!vmm_copy_to_user(
            task->address_space,
            user_state,
            &destination,
            sizeof(destination)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    return 0;
}


static int64_t sys_procinfo(uint64_t index, vaddr_t user_info)
{
    struct task *caller = current_user_task();
    const struct task *task;
    struct axiom_process_info info;
    size_t character;

    ++stats.procinfo_calls;

    if (caller == 0 || index > (uint64_t)SIZE_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }
    if (!vmm_user_range_accessible(
            caller->address_space,
            user_info,
            sizeof(info),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    task = scheduler_task_at((size_t)index);
    if (task == 0) {
        return 0;
    }

    info.pid = task->id;
    info.ppid = task->parent_id;
    info.runtime_ticks = task->runtime_ticks;
    info.exit_status = task->exit_code;
    info.state = (uint32_t)task->state;
    info.privilege = (uint32_t)task->privilege;
    info.termination_signal = task->termination_signal;
    info.priority = task->priority;
    info.effective_priority = task->effective_priority;
    info.reserved = 0u;
    info.affinity_mask = task->affinity_mask;
    info.ready_ticks = task->accumulated_ready_ticks;

    for (character = 0u; character + 1u < sizeof(info.name); ++character) {
        if (task->name == 0 || task->name[character] == '\0') {
            break;
        }
        info.name[character] = task->name[character];
    }
    info.name[character++] = '\0';
    while (character < sizeof(info.name)) {
        info.name[character++] = '\0';
    }

    if (!vmm_copy_to_user(
            caller->address_space,
            user_info,
            &info,
            sizeof(info)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    return 1;
}

static int64_t sys_kill(uint64_t pid, uint64_t signal_number)
{
    struct task *caller = current_user_task();
    const struct task *target;

    ++stats.kill_calls;

    if (caller == 0 || signal_number != AXIOM_SIGTERM) {
        return syscall_error(AXIOM_EINVAL);
    }

    target = scheduler_task_by_id(pid);
    if (target == 0 || target->state == TASK_TERMINATED) {
        return syscall_error(AXIOM_ESRCH);
    }
    if (target->privilege != TASK_PRIVILEGE_USER) {
        return syscall_error(AXIOM_EPERM);
    }

    if (!scheduler_terminate_task(pid, (uint32_t)signal_number)) {
        return syscall_error(AXIOM_ESRCH);
    }
    return 0;
}

static int64_t sys_getppid(void)
{
    const struct task *task = current_user_task();

    ++stats.getppid_calls;
    if (task == 0) {
        return syscall_error(AXIOM_EINVAL);
    }
    return task->parent_id == UINT64_MAX ? 0 : (int64_t)task->parent_id;
}

static int64_t sys_getuid(void)
{
    struct task *task = current_user_task();
    return task != 0 ? (int64_t)task->uid : syscall_error(AXIOM_EPERM);
}

static int64_t sys_getgid(void)
{
    struct task *task = current_user_task();
    return task != 0 ? (int64_t)task->gid : syscall_error(AXIOM_EPERM);
}

static int64_t sys_getpriority(void)
{
    ++stats.getpriority_calls;
    if (current_user_task() == 0) {
        return syscall_error(AXIOM_EPERM);
    }
    return (int64_t)scheduler_get_current_priority();
}

static int64_t sys_setpriority(uint64_t priority)
{
    ++stats.setpriority_calls;
    if (current_user_task() == 0 || priority > AXIOM_SCHED_PRIORITY_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }
    return scheduler_set_current_priority((uint32_t)priority) ? 0 :
        syscall_error(AXIOM_EINVAL);
}

static int64_t sys_getaffinity(void)
{
    ++stats.getaffinity_calls;
    if (current_user_task() == 0) {
        return syscall_error(AXIOM_EPERM);
    }
    return (int64_t)scheduler_get_current_affinity();
}

static int64_t sys_setaffinity(uint64_t affinity_mask)
{
    ++stats.setaffinity_calls;
    if (current_user_task() == 0) {
        return syscall_error(AXIOM_EPERM);
    }
    return scheduler_set_current_affinity(affinity_mask) ? 0 :
        syscall_error(AXIOM_EINVAL);
}

static int64_t sys_schedstats(vaddr_t user_stats)
{
    struct task *task = current_user_task();
    const struct scheduler_stats kernel_stats = scheduler_get_stats();
    struct axiom_scheduler_stats info;

    ++stats.schedstats_calls;
    if (task == 0) {
        return syscall_error(AXIOM_EPERM);
    }
    if (!vmm_user_range_accessible(
            task->address_space,
            user_stats,
            sizeof(info),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    info.task_count = kernel_stats.task_count;
    info.runnable_tasks = kernel_stats.runnable_tasks;
    info.context_switches = kernel_stats.context_switches;
    info.preemptions = kernel_stats.preemptions;
    info.priority_preemptions = kernel_stats.priority_preemptions;
    info.scheduling_ticks = kernel_stats.scheduling_ticks;
    info.aging_promotions = kernel_stats.aging_promotions;
    info.voluntary_yields = kernel_stats.voluntary_yields;
    info.current_task_id = kernel_stats.current_task_id;
    info.current_affinity_mask = kernel_stats.current_affinity_mask;
    info.policy = kernel_stats.policy;
    info.current_priority = kernel_stats.current_priority;
    info.current_effective_priority = kernel_stats.current_effective_priority;
    info.reserved = 0u;

    if (!vmm_copy_to_user(
            task->address_space,
            user_stats,
            &info,
            sizeof(info)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return 0;
}


static int64_t sys_netinfo(vaddr_t user_info)
{
    struct task *task = current_user_task();
    struct axiom_net_info info;

    ++stats.netinfo_calls;
    if (task == 0) return syscall_error(AXIOM_EINVAL);
    if (!net_available()) return syscall_error(AXIOM_ENETDOWN);
    if (!vmm_user_range_accessible(
            task->address_space,
            user_info,
            sizeof(info),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    net_get_abi_info(&info);
    if (!vmm_copy_to_user(task->address_space, user_info, &info, sizeof(info))) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return 0;
}

static int64_t sys_ping(vaddr_t user_target, vaddr_t user_result)
{
    struct task *task = current_user_task();
    struct axiom_ping_result result_info;
    char target[AXIOM_NET_HOST_MAX];
    int copied;
    int result;

    ++stats.ping_calls;
    if (task == 0) return syscall_error(AXIOM_EINVAL);
    if (!net_available()) return syscall_error(AXIOM_ENETDOWN);

    copied = copy_user_path(task, user_target, target, sizeof(target));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) ++stats.rejected_pointers;
        return copied;
    }
    if (!vmm_user_range_accessible(
            task->address_space,
            user_result,
            sizeof(result_info),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    result = net_ping(target, &result_info);
    if (result < 0) return result;
    if (!vmm_copy_to_user(
            task->address_space,
            user_result,
            &result_info,
            sizeof(result_info)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return 0;
}

static int64_t sys_dns(vaddr_t user_name, vaddr_t user_address)
{
    struct task *task = current_user_task();
    char name[AXIOM_NET_HOST_MAX];
    uint32_t address;
    int copied;
    int result;

    ++stats.dns_calls;
    if (task == 0) return syscall_error(AXIOM_EINVAL);
    if (!net_available()) return syscall_error(AXIOM_ENETDOWN);

    copied = copy_user_path(task, user_name, name, sizeof(name));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) ++stats.rejected_pointers;
        return copied;
    }
    if (!vmm_user_range_accessible(task->address_space, user_address, sizeof(address), 1)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    result = net_resolve(name, &address);
    if (result < 0) return result;
    if (!vmm_copy_to_user(task->address_space, user_address, &address, sizeof(address))) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return 0;
}

static int64_t sys_httpget(
    vaddr_t user_host,
    vaddr_t user_path,
    vaddr_t user_buffer,
    uint64_t capacity,
    vaddr_t user_result
)
{
    struct task *task = current_user_task();
    struct axiom_http_result result_info;
    char host[AXIOM_NET_HOST_MAX];
    char path[AXIOM_NET_PATH_MAX];
    int copied;
    int result;

    ++stats.httpget_calls;
    if (task == 0 || capacity == 0u || capacity > SYSCALL_HTTP_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }
    if (!net_available()) return syscall_error(AXIOM_ENETDOWN);

    copied = copy_user_path(task, user_host, host, sizeof(host));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) ++stats.rejected_pointers;
        return copied;
    }
    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) ++stats.rejected_pointers;
        return copied;
    }

    if (!vmm_user_range_accessible(
            task->address_space,
            user_buffer,
            (size_t)capacity,
            1
        ) ||
        !vmm_user_range_accessible(
            task->address_space,
            user_result,
            sizeof(result_info),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    result = net_http_get(
        host,
        path,
        syscall_http_buffer,
        (size_t)capacity,
        &result_info
    );
    if (result < 0) return result;

    if (result_info.body_bytes != 0u &&
        !vmm_copy_to_user(
            task->address_space,
            user_buffer,
            syscall_http_buffer,
            (size_t)result_info.body_bytes
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    if (!vmm_copy_to_user(
            task->address_space,
            user_result,
            &result_info,
            sizeof(result_info)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return 0;
}

static int64_t sys_httpserve(
    uint64_t port,
    vaddr_t user_body,
    uint64_t body_length,
    vaddr_t user_result
)
{
    struct task *task = current_user_task();
    struct axiom_http_server_result result_info;
    uint8_t body[AXIOM_NET_HTTP_SERVER_BODY_MAX];
    int result;

    ++stats.httpserve_calls;
    if (task == 0 || port == 0u || port > UINT16_MAX || body_length == 0u ||
        body_length > sizeof(body)) {
        return syscall_error(AXIOM_EINVAL);
    }
    if (!net_available()) return syscall_error(AXIOM_ENETDOWN);
    if (!vmm_user_range_accessible(
            task->address_space,
            user_body,
            (size_t)body_length,
            0
        ) ||
        !vmm_user_range_accessible(
            task->address_space,
            user_result,
            sizeof(result_info),
            1
        ) ||
        !vmm_copy_from_user(
            task->address_space,
            body,
            user_body,
            (size_t)body_length
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    result = net_http_serve_once(
        (uint16_t)port,
        body,
        (size_t)body_length,
        &result_info
    );
    if (result < 0) return result;
    if (!vmm_copy_to_user(
            task->address_space,
            user_result,
            &result_info,
            sizeof(result_info)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return 0;
}

static int64_t sys_sleep(uint64_t milliseconds)
{
    uint64_t ticks;
    const uint64_t frequency = timer_frequency();

    ++stats.sleep_calls;

    if (milliseconds > SYSCALL_SLEEP_MAX_MS || frequency == 0u) {
        return syscall_error(AXIOM_EINVAL);
    }

    if (milliseconds == 0u) {
        return 0;
    }

    if (milliseconds > (UINT64_MAX - 999ULL) / frequency) {
        return syscall_error(AXIOM_EINVAL);
    }

    ticks = (milliseconds * frequency + 999ULL) / 1000ULL;
    if (ticks == 0u) {
        ticks = 1u;
    }

    return scheduler_sleep_current(ticks) ? 0 : syscall_error(AXIOM_EINVAL);
}


static int64_t sys_pipe_create(void)
{
    uint32_t pipe_id = 0u;
    const int result = ipc_pipe_create(&pipe_id);
    return result < 0 ? (int64_t)result : (int64_t)pipe_id;
}

static int64_t sys_pipe_write(uint64_t pipe_id, vaddr_t user_buffer, uint64_t count)
{
    struct task *task = current_user_task();
    uint8_t local[AXIOM_IPC_PIPE_IO_MAX];

    if (task == 0 || pipe_id > UINT32_MAX || count == 0u ||
        count > AXIOM_IPC_PIPE_IO_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }
    if (!vmm_user_range_accessible(task->address_space, user_buffer, (size_t)count, 0) ||
        !vmm_copy_from_user(task->address_space, local, user_buffer, (size_t)count)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return ipc_pipe_write((uint32_t)pipe_id, local, (size_t)count);
}

static int64_t sys_pipe_read(uint64_t pipe_id, vaddr_t user_buffer, uint64_t count)
{
    struct task *task = current_user_task();
    uint8_t local[AXIOM_IPC_PIPE_IO_MAX];
    int64_t result;

    if (task == 0 || pipe_id > UINT32_MAX || count == 0u ||
        count > AXIOM_IPC_PIPE_IO_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }
    if (!vmm_user_range_accessible(task->address_space, user_buffer, (size_t)count, 1)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    result = ipc_pipe_read((uint32_t)pipe_id, local, (size_t)count);
    if (result <= 0) return result;
    if (!vmm_copy_to_user(task->address_space, user_buffer, local, (size_t)result)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return result;
}

static int64_t sys_pipe_close(uint64_t pipe_id)
{
    if (pipe_id > UINT32_MAX) return syscall_error(AXIOM_EINVAL);
    return (int64_t)ipc_pipe_close((uint32_t)pipe_id);
}

static int64_t sys_shm_attach(uint64_t key)
{
    struct task *task = current_user_task();
    vaddr_t address = 0u;
    int result;
    if (task == 0 || key == 0u) return syscall_error(AXIOM_EINVAL);
    result = ipc_shm_attach(task->id, task->address_space, key, &address);
    return result < 0 ? (int64_t)result : (int64_t)address;
}

static int64_t sys_shm_detach(vaddr_t address)
{
    struct task *task = current_user_task();
    if (task == 0 || (address & (VMM_PAGE_SIZE - 1u)) != 0u) {
        return syscall_error(AXIOM_EINVAL);
    }
    return (int64_t)ipc_shm_detach(task->id, task->address_space, address);
}

static int64_t sys_msgq_open(uint64_t key)
{
    uint32_t queue_id = 0u;
    const int result = ipc_msgq_open(key, &queue_id);
    return result < 0 ? (int64_t)result : (int64_t)queue_id;
}

static int64_t sys_msgq_send(uint64_t queue_id, vaddr_t user_message, uint64_t length)
{
    struct task *task = current_user_task();
    uint8_t local[AXIOM_IPC_MESSAGE_MAX];
    if (task == 0 || queue_id > UINT32_MAX || length == 0u ||
        length > AXIOM_IPC_MESSAGE_MAX) {
        return syscall_error(length > AXIOM_IPC_MESSAGE_MAX ? AXIOM_EMSGSIZE : AXIOM_EINVAL);
    }
    if (!vmm_user_range_accessible(task->address_space, user_message, (size_t)length, 0) ||
        !vmm_copy_from_user(task->address_space, local, user_message, (size_t)length)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return ipc_msgq_send((uint32_t)queue_id, local, (size_t)length);
}

static int64_t sys_msgq_receive(uint64_t queue_id, vaddr_t user_buffer, uint64_t capacity)
{
    struct task *task = current_user_task();
    uint8_t local[AXIOM_IPC_MESSAGE_MAX];
    int64_t result;
    if (task == 0 || queue_id > UINT32_MAX || capacity == 0u ||
        capacity > AXIOM_IPC_MESSAGE_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }
    if (!vmm_user_range_accessible(task->address_space, user_buffer, (size_t)capacity, 1)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    result = ipc_msgq_receive((uint32_t)queue_id, local, (size_t)capacity);
    if (result <= 0) return result;
    if (!vmm_copy_to_user(task->address_space, user_buffer, local, (size_t)result)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return result;
}

static int64_t sys_msgq_close(uint64_t queue_id)
{
    if (queue_id > UINT32_MAX) return syscall_error(AXIOM_EINVAL);
    return (int64_t)ipc_msgq_close((uint32_t)queue_id);
}

static int64_t sys_ipcstats(vaddr_t user_stats)
{
    struct task *task = current_user_task();
    const struct axiom_ipc_stats info = ipc_get_stats();
    if (task == 0 || !vmm_user_range_accessible(
            task->address_space, user_stats, sizeof(info), 1)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    if (!vmm_copy_to_user(task->address_space, user_stats, &info, sizeof(info))) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return 0;
}

static int64_t sys_gfx_info(vaddr_t user_info)
{
    struct task *task = current_user_task();
    struct axiom_gfx_info info;

    if (!graphics_initialized()) return syscall_error(AXIOM_ENODEV);
    if (task == 0 || !vmm_user_range_accessible(
            task->address_space, user_info, sizeof(info), 1)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    graphics_get_info(&info);
    if (!vmm_copy_to_user(task->address_space, user_info, &info, sizeof(info))) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return 0;
}

static int64_t sys_gfx_clear(uint64_t rgb)
{
    if (!graphics_initialized()) return syscall_error(AXIOM_ENODEV);
    graphics_clear((uint32_t)rgb);
    return 0;
}

static int64_t sys_gfx_pixel(uint64_t x, uint64_t y, uint64_t rgb)
{
    if (!graphics_initialized()) return syscall_error(AXIOM_ENODEV);
    if (x > UINT32_MAX || y > UINT32_MAX) return syscall_error(AXIOM_EINVAL);
    draw_pixel((uint32_t)x, (uint32_t)y, (uint32_t)rgb);
    return 0;
}

static int64_t sys_gfx_line(
    uint64_t x0, uint64_t y0, uint64_t x1, uint64_t y1, uint64_t rgb)
{
    struct axiom_gfx_info info;

    if (!graphics_initialized()) return syscall_error(AXIOM_ENODEV);
    graphics_get_info(&info);
    if (x0 >= info.width || y0 >= info.height ||
        x1 >= info.width || y1 >= info.height) return syscall_error(AXIOM_EINVAL);
    draw_line((uint32_t)x0, (uint32_t)y0, (uint32_t)x1, (uint32_t)y1, (uint32_t)rgb);
    return 0;
}

static int64_t sys_gfx_rect(
    uint64_t x, uint64_t y, uint64_t width, uint64_t height, uint64_t rgb)
{
    struct axiom_gfx_info info;

    if (!graphics_initialized()) return syscall_error(AXIOM_ENODEV);
    graphics_get_info(&info);
    if (x >= info.width || y >= info.height || width == 0u || height == 0u ||
        width > info.width || height > info.height ||
        width * height > (uint64_t)info.width * info.height) {
        return syscall_error(AXIOM_EINVAL);
    }
    draw_rectangle((uint32_t)x, (uint32_t)y, (uint32_t)width, (uint32_t)height, (uint32_t)rgb);
    return 0;
}

static int64_t sys_gfx_bitmap(vaddr_t user_request)
{
    struct task *task = current_user_task();
    struct axiom_gfx_bitmap_request request;
    uint32_t *pixels;
    size_t pixel_count;
    size_t byte_count;

    if (!graphics_initialized()) return syscall_error(AXIOM_ENODEV);
    if (task == 0 || !vmm_user_range_accessible(
            task->address_space, user_request, sizeof(request), 0) ||
        !vmm_copy_from_user(task->address_space, &request, user_request, sizeof(request))) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    if (request.width == 0u || request.height == 0u ||
        request.width > AXIOM_GFX_BITMAP_MAX_PIXELS ||
        request.height > AXIOM_GFX_BITMAP_MAX_PIXELS ||
        (uint64_t)request.width * request.height > AXIOM_GFX_BITMAP_MAX_PIXELS) {
        return syscall_error(AXIOM_EINVAL);
    }
    pixel_count = (size_t)request.width * (size_t)request.height;
    byte_count = pixel_count * sizeof(uint32_t);
    if (!vmm_user_range_accessible(
            task->address_space, (vaddr_t)request.pixels, byte_count, 0)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    pixels = (uint32_t *)kmalloc(byte_count);
    if (pixels == 0) return syscall_error(AXIOM_ENOMEM);
    if (!vmm_copy_from_user(
            task->address_space, pixels, (vaddr_t)request.pixels, byte_count)) {
        kfree(pixels);
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    draw_bitmap(request.x, request.y, request.width, request.height, pixels);
    kfree(pixels);
    return 0;
}

static int64_t sys_gfx_text(uint64_t x, uint64_t y, uint64_t rgb, vaddr_t user_text)
{
    struct task *task = current_user_task();
    char text[AXIOM_GFX_TEXT_MAX + 1u];
    size_t index;

    if (!graphics_initialized()) return syscall_error(AXIOM_ENODEV);
    if (x > UINT32_MAX || y > UINT32_MAX || task == 0) return syscall_error(AXIOM_EINVAL);
    for (index = 0u; index < AXIOM_GFX_TEXT_MAX; ++index) {
        if (!vmm_copy_from_user(
                task->address_space, &text[index], user_text + index, 1u)) {
            ++stats.rejected_pointers;
            return syscall_error(AXIOM_EFAULT);
        }
        if (text[index] == '\0') {
            draw_text((uint32_t)x, (uint32_t)y, (uint32_t)rgb, text);
            return 0;
        }
    }
    text[AXIOM_GFX_TEXT_MAX] = '\0';
    return syscall_error(AXIOM_ENAMETOOLONG);
}

static int64_t sys_gfx_stats(vaddr_t user_stats)
{
    struct task *task = current_user_task();
    const struct axiom_gfx_stats info = graphics_get_stats();

    if (!graphics_initialized()) return syscall_error(AXIOM_ENODEV);
    if (task == 0 || !vmm_user_range_accessible(
            task->address_space, user_stats, sizeof(info), 1)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    if (!vmm_copy_to_user(task->address_space, user_stats, &info, sizeof(info))) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return 0;
}


static int64_t sys_gfx_cursor(uint64_t x, uint64_t y, uint64_t visible)
{
    struct axiom_gfx_info info;

    if (!graphics_initialized()) return syscall_error(AXIOM_ENODEV);
    graphics_get_info(&info);
    if (visible != 0u && (x >= info.width || y >= info.height)) {
        return syscall_error(AXIOM_EINVAL);
    }
    graphics_cursor_set((uint32_t)x, (uint32_t)y, visible != 0u);
    return 0;
}

static int64_t sys_mmap(uint64_t length)
{
    vaddr_t address = 0u;

    ++stats.mmap_calls;
    if (length == 0u || length > (uint64_t)TASK_USER_MMAP_MAX_PAGES * VMM_PAGE_SIZE) {
        return syscall_error(AXIOM_EINVAL);
    }
    if (!scheduler_mmap_current((size_t)length, &address)) {
        return syscall_error(AXIOM_ENOMEM);
    }
    return (int64_t)address;
}

static int64_t sys_getpid(void)
{
    const struct task *task = current_user_task();

    ++stats.getpid_calls;
    return task != 0 ? (int64_t)task->id : syscall_error(AXIOM_EINVAL);
}

static int64_t sys_yield(void)
{
    ++stats.yield_calls;
    return scheduler_yield_current() ? 0 : syscall_error(AXIOM_EINVAL);
}

int syscall_init(void)
{
    uint64_t efer;
    uint64_t star;

    if (initialized || interrupts_enabled() || !cpu_supports_syscall()) {
        return 0;
    }

    /*
     * SYSCALL: CS=0x08, SS=0x10.
     * SYSRET:  STAR.user+16 => 0x23 user code, STAR.user+8 => 0x1B user data.
     */
    star = ((uint64_t)0x10u << 48) | ((uint64_t)GDT_KERNEL_CODE << 32);

    efer = rdmsr(IA32_EFER);
    wrmsr(IA32_EFER, efer | EFER_SCE);
    wrmsr(IA32_STAR, star);
    wrmsr(IA32_LSTAR, (uint64_t)(uintptr_t)&syscall_entry);
    wrmsr(IA32_FMASK, RFLAGS_TF | RFLAGS_IF | RFLAGS_DF);

    syscall_kernel_stack_top = gdt_kernel_stack();

    stats.total_calls = 0u;
    stats.write_calls = 0u;
    stats.read_calls = 0u;
    stats.exit_calls = 0u;
    stats.sleep_calls = 0u;
    stats.getpid_calls = 0u;
    stats.yield_calls = 0u;
    stats.open_calls = 0u;
    stats.close_calls = 0u;
    stats.fork_calls = 0u;
    stats.exec_calls = 0u;
    stats.exec_successes = 0u;
    stats.lseek_calls = 0u;
    stats.stat_calls = 0u;
    stats.readdir_calls = 0u;
    stats.mkdir_calls = 0u;
    stats.spawn_calls = 0u;
    stats.waitpid_calls = 0u;
    stats.clear_calls = 0u;
    stats.kbdstats_calls = 0u;
    stats.procinfo_calls = 0u;
    stats.kill_calls = 0u;
    stats.getppid_calls = 0u;
    stats.netinfo_calls = 0u;
    stats.ping_calls = 0u;
    stats.dns_calls = 0u;
    stats.httpget_calls = 0u;
    stats.mmap_calls = 0u;
    stats.getpriority_calls = 0u;
    stats.setpriority_calls = 0u;
    stats.getaffinity_calls = 0u;
    stats.setaffinity_calls = 0u;
    stats.schedstats_calls = 0u;
    stats.rejected_pointers = 0u;
    stats.unimplemented_calls = 0u;
    stats.bytes_written = 0u;
    stats.bytes_read = 0u;
    initialized = 1;
    return 1;
}

int syscall_initialized(void)
{
    return initialized;
}

void syscall_set_kernel_stack(uintptr_t stack_top)
{
    syscall_kernel_stack_top = stack_top;
}

void syscall_dispatch(struct syscall_frame *frame)
{
    int64_t result;

    if (!initialized || frame == 0) {
        return;
    }

    ++stats.total_calls;

    if (current_user_task() == 0) {
        kernel_panic("SYSCALL entered without a Ring-3 current task");
    }

    /* Never let SYSRETQ carry a malicious non-canonical user stack to CPL3. */
    if (frame->user_rip >= 0x0000800000000000ULL ||
        frame->user_rsp >= 0x0000800000000000ULL) {
        ++stats.rejected_pointers;
        task_exit_current(-(int64_t)AXIOM_EFAULT);
    }

    switch (frame->rax) {
        case AXIOM_SYS_WRITE:
            result = sys_write(frame->rdi, (vaddr_t)frame->rsi, frame->rdx);
            break;

        case AXIOM_SYS_READ:
            result = sys_read(frame->rdi, (vaddr_t)frame->rsi, frame->rdx);
            break;

        case AXIOM_SYS_EXIT:
            ++stats.exit_calls;
            task_exit_current((int64_t)frame->rdi);

        case AXIOM_SYS_SLEEP:
            result = sys_sleep(frame->rdi);
            break;

        case AXIOM_SYS_GETPID:
            result = sys_getpid();
            break;

        case AXIOM_SYS_YIELD:
            result = sys_yield();
            break;

        case AXIOM_SYS_OPEN:
            result = sys_open((vaddr_t)frame->rdi, frame->rsi);
            break;

        case AXIOM_SYS_CLOSE:
            result = sys_close(frame->rdi);
            break;

        case AXIOM_SYS_FORK:
            result = sys_fork(frame);
            break;

        case AXIOM_SYS_EXEC:
            result = sys_exec((vaddr_t)frame->rdi, frame);
            break;

        case AXIOM_SYS_LSEEK:
            result = sys_lseek(
                frame->rdi,
                (int64_t)frame->rsi,
                frame->rdx
            );
            break;

        case AXIOM_SYS_STAT:
            result = sys_stat((vaddr_t)frame->rdi, (vaddr_t)frame->rsi);
            break;

        case AXIOM_SYS_READDIR:
            result = sys_readdir(
                (vaddr_t)frame->rdi,
                frame->rsi,
                (vaddr_t)frame->rdx
            );
            break;

        case AXIOM_SYS_MKDIR:
            result = sys_mkdir((vaddr_t)frame->rdi);
            break;

        case AXIOM_SYS_SPAWN:
            result = sys_spawn((vaddr_t)frame->rdi);
            break;

        case AXIOM_SYS_WAITPID:
            result = sys_waitpid(frame->rdi, (vaddr_t)frame->rsi);
            break;

        case AXIOM_SYS_CLEAR:
            result = sys_clear();
            break;

        case AXIOM_SYS_KBDSTATS:
            result = sys_kbdstats((vaddr_t)frame->rdi);
            break;

        case AXIOM_SYS_PROCINFO:
            result = sys_procinfo(frame->rdi, (vaddr_t)frame->rsi);
            break;

        case AXIOM_SYS_KILL:
            result = sys_kill(frame->rdi, frame->rsi);
            break;

        case AXIOM_SYS_GETPPID:
            result = sys_getppid();
            break;

        case AXIOM_SYS_NETINFO:
            result = sys_netinfo((vaddr_t)frame->rdi);
            break;

        case AXIOM_SYS_PING:
            result = sys_ping((vaddr_t)frame->rdi, (vaddr_t)frame->rsi);
            break;

        case AXIOM_SYS_DNS:
            result = sys_dns((vaddr_t)frame->rdi, (vaddr_t)frame->rsi);
            break;

        case AXIOM_SYS_HTTPGET:
            result = sys_httpget(
                (vaddr_t)frame->rdi,
                (vaddr_t)frame->rsi,
                (vaddr_t)frame->rdx,
                frame->r10,
                (vaddr_t)frame->r8
            );
            break;

        case AXIOM_SYS_HTTPSERVE:
            result = sys_httpserve(
                frame->rdi,
                (vaddr_t)frame->rsi,
                frame->rdx,
                (vaddr_t)frame->r10
            );
            break;

        case AXIOM_SYS_MMAP:
            result = sys_mmap(frame->rdi);
            break;

        case AXIOM_SYS_GETUID:
            result = sys_getuid();
            break;

        case AXIOM_SYS_GETGID:
            result = sys_getgid();
            break;

        case AXIOM_SYS_GETPRIORITY:
            result = sys_getpriority();
            break;

        case AXIOM_SYS_SETPRIORITY:
            result = sys_setpriority(frame->rdi);
            break;

        case AXIOM_SYS_GETAFFINITY:
            result = sys_getaffinity();
            break;

        case AXIOM_SYS_SETAFFINITY:
            result = sys_setaffinity(frame->rdi);
            break;

        case AXIOM_SYS_SCHEDSTATS:
            result = sys_schedstats((vaddr_t)frame->rdi);
            break;

        case AXIOM_SYS_PIPE_CREATE:
            result = sys_pipe_create();
            break;
        case AXIOM_SYS_PIPE_WRITE:
            result = sys_pipe_write(frame->rdi, (vaddr_t)frame->rsi, frame->rdx);
            break;
        case AXIOM_SYS_PIPE_READ:
            result = sys_pipe_read(frame->rdi, (vaddr_t)frame->rsi, frame->rdx);
            break;
        case AXIOM_SYS_PIPE_CLOSE:
            result = sys_pipe_close(frame->rdi);
            break;
        case AXIOM_SYS_SHM_ATTACH:
            result = sys_shm_attach(frame->rdi);
            break;
        case AXIOM_SYS_SHM_DETACH:
            result = sys_shm_detach((vaddr_t)frame->rdi);
            break;
        case AXIOM_SYS_MSGQ_OPEN:
            result = sys_msgq_open(frame->rdi);
            break;
        case AXIOM_SYS_MSGQ_SEND:
            result = sys_msgq_send(frame->rdi, (vaddr_t)frame->rsi, frame->rdx);
            break;
        case AXIOM_SYS_MSGQ_RECV:
            result = sys_msgq_receive(frame->rdi, (vaddr_t)frame->rsi, frame->rdx);
            break;
        case AXIOM_SYS_MSGQ_CLOSE:
            result = sys_msgq_close(frame->rdi);
            break;
        case AXIOM_SYS_IPCSTATS:
            result = sys_ipcstats((vaddr_t)frame->rdi);
            break;

        case AXIOM_SYS_GFX_INFO:
            result = sys_gfx_info((vaddr_t)frame->rdi);
            break;
        case AXIOM_SYS_GFX_CLEAR:
            result = sys_gfx_clear(frame->rdi);
            break;
        case AXIOM_SYS_GFX_PIXEL:
            result = sys_gfx_pixel(frame->rdi, frame->rsi, frame->rdx);
            break;
        case AXIOM_SYS_GFX_LINE:
            result = sys_gfx_line(frame->rdi, frame->rsi, frame->rdx, frame->r10, frame->r8);
            break;
        case AXIOM_SYS_GFX_RECT:
            result = sys_gfx_rect(frame->rdi, frame->rsi, frame->rdx, frame->r10, frame->r8);
            break;
        case AXIOM_SYS_GFX_BITMAP:
            result = sys_gfx_bitmap((vaddr_t)frame->rdi);
            break;
        case AXIOM_SYS_GFX_TEXT:
            result = sys_gfx_text(frame->rdi, frame->rsi, frame->rdx, (vaddr_t)frame->r10);
            break;
        case AXIOM_SYS_GFX_STATS:
            result = sys_gfx_stats((vaddr_t)frame->rdi);
            break;

        case AXIOM_SYS_MOUSESTATE:
            result = sys_mousestate((vaddr_t)frame->rdi);
            break;
        case AXIOM_SYS_GFX_CURSOR:
            result = sys_gfx_cursor(frame->rdi, frame->rsi, frame->rdx);
            break;

        default:
            ++stats.unimplemented_calls;
            result = syscall_error(AXIOM_ENOSYS);
            break;
    }

    frame->rax = (uint64_t)result;
}

struct syscall_stats syscall_get_stats(void)
{
    return stats;
}
