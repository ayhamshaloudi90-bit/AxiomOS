#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>
#include <axiom/abi/fs.h>
#include <axiom/arch/apic.h>
#include <axiom/arch/interrupts.h>
#include <axiom/arch/smp.h>
#include <axiom/drivers/e1000.h>
#include <axiom/drivers/keyboard.h>
#include <axiom/drivers/timer.h>
#include <axiom/filesystem/procfs.h>
#include <axiom/filesystem/vfs.h>
#include <axiom/memory/heap.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>
#include <axiom/network/net.h>
#include <axiom/process/scheduler.h>

#define PROCFS_FILE_COUNT 8u
#define PROCFS_RENDER_CAPACITY 16384u
#define PROCFS_FILE_MODE 0444u
#define PROCFS_DIRECTORY_MODE 0555u
#define PROCFS_STAT_SIZE PROCFS_RENDER_CAPACITY

enum procfs_kind {
    PROCFS_MEMINFO = 0,
    PROCFS_PROCESSES,
    PROCFS_INTERRUPTS,
    PROCFS_FILES,
    PROCFS_NET,
    PROCFS_SCHEDULER,
    PROCFS_CPUINFO,
    PROCFS_PAGEMAP,
};

struct procfs_node {
    struct vfs_node vfs;
    enum procfs_kind kind;
};

struct proc_writer {
    char *buffer;
    size_t capacity;
    size_t length;
};

static int procfs_lookup(
    struct vfs_node *directory,
    const char *name,
    struct vfs_node **node_out
);
static int64_t procfs_read(
    struct vfs_node *node,
    uint64_t offset,
    void *buffer,
    size_t count
);
static int procfs_readdir(
    struct vfs_node *directory,
    size_t index,
    struct vfs_dirent *entry_out
);

static const struct vfs_node_ops procfs_ops = {
    .lookup = procfs_lookup,
    .create = 0,
    .read = procfs_read,
    .write = 0,
    .truncate = 0,
    .readdir = procfs_readdir,
};

static struct procfs_node proc_nodes[PROCFS_FILE_COUNT];
static struct vfs_node proc_root;
static struct vfs_filesystem proc_filesystem;
static char render_buffer[PROCFS_RENDER_CAPACITY];
static size_t render_length;
static enum procfs_kind render_kind;
static int render_valid;
static int initialized;

static const char *const proc_names[PROCFS_FILE_COUNT] = {
    "meminfo",
    "processes",
    "interrupts",
    "files",
    "net",
    "scheduler",
    "cpuinfo",
    "pagemap",
};

static int text_equal(const char *left, const char *right)
{
    size_t index = 0u;
    if (left == 0 || right == 0) return 0;
    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] != right[index]) return 0;
        ++index;
    }
    return left[index] == right[index];
}

static void bytes_copy(void *destination, const void *source, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    size_t index;
    for (index = 0u; index < count; ++index) out[index] = in[index];
}

static void copy_name(char *destination, const char *source, size_t capacity)
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

static void writer_char(struct proc_writer *writer, char character)
{
    if (writer == 0 || writer->capacity == 0u) return;
    if (writer->length + 1u < writer->capacity) {
        writer->buffer[writer->length++] = character;
        writer->buffer[writer->length] = '\0';
    }
}

static void writer_text(struct proc_writer *writer, const char *text)
{
    size_t index = 0u;
    if (writer == 0 || text == 0) return;
    while (text[index] != '\0') writer_char(writer, text[index++]);
}

static void writer_u64(struct proc_writer *writer, uint64_t value)
{
    char digits[32];
    size_t used = 0u;
    if (value == 0u) {
        writer_char(writer, '0');
        return;
    }
    while (value != 0u && used < sizeof(digits)) {
        digits[used++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (used != 0u) writer_char(writer, digits[--used]);
}

static void writer_hex64(struct proc_writer *writer, uint64_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    int shift;
    int started = 0;
    writer_text(writer, "0x");
    for (shift = 60; shift >= 0; shift -= 4) {
        const unsigned digit = (unsigned)((value >> (unsigned)shift) & 0xFULL);
        if (digit != 0u || started || shift == 0) {
            writer_char(writer, hex[digit]);
            started = 1;
        }
    }
}

static void writer_ipv4(struct proc_writer *writer, uint32_t address)
{
    writer_u64(writer, (address >> 24) & 0xFFu); writer_char(writer, '.');
    writer_u64(writer, (address >> 16) & 0xFFu); writer_char(writer, '.');
    writer_u64(writer, (address >> 8) & 0xFFu); writer_char(writer, '.');
    writer_u64(writer, address & 0xFFu);
}

static void writer_mac(struct proc_writer *writer, const uint8_t mac[6])
{
    static const char hex[] = "0123456789ABCDEF";
    size_t index;
    for (index = 0u; index < 6u; ++index) {
        writer_char(writer, hex[mac[index] >> 4]);
        writer_char(writer, hex[mac[index] & 0x0Fu]);
        if (index + 1u < 6u) writer_char(writer, ':');
    }
}

static void writer_key_u64(
    struct proc_writer *writer,
    const char *key,
    uint64_t value
)
{
    writer_text(writer, key);
    writer_text(writer, ": ");
    writer_u64(writer, value);
    writer_char(writer, '\n');
}

static void render_meminfo(struct proc_writer *writer)
{
    const struct pmm_stats pmm = pmm_get_stats();
    const struct heap_stats heap = heap_get_stats();
    const struct vmm_stats vmm = vmm_get_stats();

    writer_text(writer, "AxiomOS Phase 25 memory telemetry\n");
    writer_key_u64(writer, "MemTotalBytes", pmm.total_memory_bytes);
    writer_key_u64(writer, "MemUsableBytes", pmm.usable_memory_bytes);
    writer_key_u64(writer, "PhysicalPages", pmm.usable_pages);
    writer_key_u64(writer, "PhysicalAllocatedPages", pmm.allocated_pages);
    writer_key_u64(writer, "PhysicalFreePages", pmm.free_pages);
    writer_key_u64(writer, "PMMMetadataPages", pmm.metadata_pages);
    writer_key_u64(writer, "HeapMappedBytes", heap.mapped_bytes);
    writer_key_u64(writer, "HeapBytesInUse", heap.bytes_in_use);
    writer_key_u64(writer, "HeapPeakBytesInUse", heap.peak_bytes_in_use);
    writer_key_u64(writer, "HeapActiveAllocations", heap.active_allocations);
    writer_key_u64(writer, "HeapFreeBytes", heap.free_bytes);
    writer_key_u64(writer, "HeapLargestFreeBlock", heap.largest_free_block);
    writer_key_u64(writer, "PageTablePages", vmm.page_table_pages);
    writer_text(writer, "KernelCR3: "); writer_hex64(writer, vmm.root_table); writer_char(writer, '\n');
    writer_text(writer, "HHDMOffset: "); writer_hex64(writer, vmm.hhdm_offset); writer_char(writer, '\n');
}

static void render_processes(struct proc_writer *writer)
{
    size_t index;
    writer_text(writer, "PID PPID STATE PRIV PRI EFF AFF RUNTIME READY NAME\n");
    for (index = 0u; index < scheduler_task_count(); ++index) {
        const struct task *task = scheduler_task_at(index);
        if (task == 0) continue;
        writer_u64(writer, task->id); writer_char(writer, ' ');
        if (task->parent_id == UINT64_MAX) writer_char(writer, '-');
        else writer_u64(writer, task->parent_id);
        writer_char(writer, ' '); writer_text(writer, task_state_name(task->state));
        writer_char(writer, ' '); writer_text(writer, task_privilege_name(task->privilege));
        writer_char(writer, ' '); writer_u64(writer, task->priority);
        writer_char(writer, ' '); writer_u64(writer, task->effective_priority);
        writer_char(writer, ' '); writer_hex64(writer, task->affinity_mask);
        writer_char(writer, ' '); writer_u64(writer, task->runtime_ticks);
        writer_char(writer, ' '); writer_u64(writer, task->accumulated_ready_ticks);
        writer_char(writer, ' '); writer_text(writer, task->name != 0 ? task->name : "?");
        writer_char(writer, '\n');
    }
}

static void render_interrupts(struct proc_writer *writer)
{
    uint16_t vector;
    writer_text(writer, "AxiomOS interrupt counters\n");
    writer_key_u64(writer, "Total", interrupt_total_count());
    writer_key_u64(writer, "TimerVector32", interrupt_count(32u));
    writer_key_u64(writer, "KeyboardVector33", interrupt_count(33u));
    writer_text(writer, "NonZeroVectors:\n");
    for (vector = 0u; vector < 256u; ++vector) {
        const uint64_t count = interrupt_count((uint8_t)vector);
        if (count == 0u) continue;
        writer_text(writer, "  vector "); writer_u64(writer, vector);
        writer_text(writer, ": "); writer_u64(writer, count); writer_char(writer, '\n');
    }
}

static void render_files(struct proc_writer *writer)
{
    size_t task_index;
    writer_text(writer, "PID FD FLAGS OFFSET REFS NODE\n");
    for (task_index = 0u; task_index < scheduler_task_count(); ++task_index) {
        const struct task *task = scheduler_task_at(task_index);
        size_t fd;
        if (task == 0) continue;
        for (fd = 0u; fd < TASK_MAX_FILES; ++fd) {
            const struct vfs_file *file = task->file_descriptors[fd];
            if (file == 0 || file->node == 0) continue;
            writer_u64(writer, task->id); writer_char(writer, ' ');
            writer_u64(writer, fd); writer_char(writer, ' ');
            writer_hex64(writer, file->flags); writer_char(writer, ' ');
            writer_u64(writer, file->offset); writer_char(writer, ' ');
            writer_u64(writer, file->reference_count); writer_char(writer, ' ');
            writer_text(writer, file->node->name != 0 ? file->node->name : "?");
            writer_char(writer, '\n');
        }
    }
}

static void render_net(struct proc_writer *writer)
{
    const struct net_stats net = net_get_stats();
    const struct e1000_stats nic = e1000_get_stats();
    const struct net_config *config = net_get_config();

    writer_text(writer, "AxiomOS network telemetry\n");
    writer_key_u64(writer, "Available", (uint64_t)net_available());
    if (config != 0) {
        writer_text(writer, "MAC: "); writer_mac(writer, config->mac); writer_char(writer, '\n');
        writer_text(writer, "IPv4: "); writer_ipv4(writer, config->address); writer_char(writer, '\n');
        writer_text(writer, "Netmask: "); writer_ipv4(writer, config->netmask); writer_char(writer, '\n');
        writer_text(writer, "Gateway: "); writer_ipv4(writer, config->gateway); writer_char(writer, '\n');
        writer_text(writer, "DNS: "); writer_ipv4(writer, config->dns_server); writer_char(writer, '\n');
    }
    writer_key_u64(writer, "NICFramesTX", nic.tx_frames);
    writer_key_u64(writer, "NICFramesRX", nic.rx_frames);
    writer_key_u64(writer, "NICBytesTX", nic.tx_bytes);
    writer_key_u64(writer, "NICBytesRX", nic.rx_bytes);
    writer_key_u64(writer, "NICTransmitErrors", nic.tx_errors);
    writer_key_u64(writer, "NICReceiveDropped", nic.rx_dropped);
    writer_key_u64(writer, "ARPRequests", net.arp_requests);
    writer_key_u64(writer, "ARPReplies", net.arp_replies);
    writer_key_u64(writer, "IPv4TX", net.ipv4_tx);
    writer_key_u64(writer, "IPv4RX", net.ipv4_rx);
    writer_key_u64(writer, "ICMPTX", net.icmp_tx);
    writer_key_u64(writer, "ICMPRX", net.icmp_rx);
    writer_key_u64(writer, "UDPTX", net.udp_tx);
    writer_key_u64(writer, "UDPRX", net.udp_rx);
    writer_key_u64(writer, "TCPTX", net.tcp_tx);
    writer_key_u64(writer, "TCPRX", net.tcp_rx);
    writer_key_u64(writer, "DNSQueries", net.dns_queries);
    writer_key_u64(writer, "DNSAnswers", net.dns_answers);
    writer_key_u64(writer, "HTTPRequests", net.http_requests);
    writer_key_u64(writer, "HTTPResponses", net.http_responses);
}

static void render_scheduler(struct proc_writer *writer)
{
    const struct scheduler_stats stats = scheduler_get_stats();
    writer_text(writer, "AxiomOS scheduler telemetry\n");
    writer_key_u64(writer, "Policy", stats.policy);
    writer_key_u64(writer, "TaskCount", stats.task_count);
    writer_key_u64(writer, "RunnableTasks", stats.runnable_tasks);
    writer_key_u64(writer, "ContextSwitches", stats.context_switches);
    writer_key_u64(writer, "Preemptions", stats.preemptions);
    writer_key_u64(writer, "PriorityPreemptions", stats.priority_preemptions);
    writer_key_u64(writer, "SchedulingTicks", stats.scheduling_ticks);
    writer_key_u64(writer, "AgingPromotions", stats.aging_promotions);
    writer_key_u64(writer, "VoluntaryYields", stats.voluntary_yields);
    writer_key_u64(writer, "UserFaultTerminations", stats.user_fault_terminations);
    writer_key_u64(writer, "CurrentTask", stats.current_task_id);
    writer_key_u64(writer, "CurrentPriority", stats.current_priority);
    writer_key_u64(writer, "CurrentEffectivePriority", stats.current_effective_priority);
    writer_text(writer, "CurrentAffinity: "); writer_hex64(writer, stats.current_affinity_mask); writer_char(writer, '\n');
    writer_key_u64(writer, "TimerTicks", timer_ticks());
    writer_key_u64(writer, "TimerHz", timer_frequency());
}

static void render_cpuinfo(struct proc_writer *writer)
{
    uint32_t eax = 0u;
    uint32_t ebx = 0u;
    uint32_t ecx = 0u;
    uint32_t edx = 0u;
    char vendor[13];
    const struct smp_stats *smp = smp_get_stats();
    uint64_t index;

    __asm__ volatile ("cpuid" : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx));
    ((uint32_t *)(void *)vendor)[0] = ebx;
    ((uint32_t *)(void *)vendor)[1] = edx;
    ((uint32_t *)(void *)vendor)[2] = ecx;
    vendor[12] = '\0';

    writer_text(writer, "AxiomOS CPU telemetry\nVendor: "); writer_text(writer, vendor); writer_char(writer, '\n');
    writer_key_u64(writer, "MaxCPUIDLeaf", eax);
    writer_key_u64(writer, "LocalAPICID", apic_local_id());
    writer_key_u64(writer, "APICActive", (uint64_t)apic_active());
    if (smp != 0) {
        writer_key_u64(writer, "CPUsDetected", smp->detected_cpus);
        writer_key_u64(writer, "CPUsManaged", smp->managed_cpus);
        writer_key_u64(writer, "CPUsOnline", smp->online_cpus);
        writer_text(writer, "ParticipantMask: "); writer_hex64(writer, smp->participant_mask); writer_char(writer, '\n');
        for (index = 0u; index < smp->managed_cpus; ++index) {
            const struct smp_cpu *cpu = smp_cpu_at(index);
            if (cpu == 0) continue;
            writer_text(writer, "cpu"); writer_u64(writer, index);
            writer_text(writer, " processor="); writer_u64(writer, cpu->processor_id);
            writer_text(writer, " lapic="); writer_u64(writer, cpu->lapic_id);
            writer_text(writer, " role="); writer_text(writer, cpu->role == SMP_CPU_BSP ? "BSP" : "AP");
            writer_text(writer, " online="); writer_u64(writer, cpu->online);
            writer_char(writer, '\n');
        }
    }
}

static void write_mapping(
    struct proc_writer *writer,
    const struct task *task,
    const char *label,
    vaddr_t virtual_address
)
{
    paddr_t physical;
    uint64_t flags = 0u;
    if (task == 0 || task->address_space == PADDR_INVALID) return;
    physical = vmm_virt_to_phys_in_address_space(task->address_space, virtual_address);
    if (physical == PADDR_INVALID) return;
    (void)vmm_mapping_flags_in_address_space(task->address_space, virtual_address, &flags);
    writer_text(writer, label); writer_char(writer, ' ');
    writer_hex64(writer, virtual_address); writer_text(writer, " -> ");
    writer_hex64(writer, physical); writer_text(writer, " flags="); writer_hex64(writer, flags);
    writer_char(writer, '\n');
}

static void render_pagemap(struct proc_writer *writer)
{
    const struct task *task = scheduler_current_task();
    size_t index;
    writer_text(writer, "AxiomOS current-process page mappings\n");
    if (task == 0) {
        writer_text(writer, "No current task\n");
        return;
    }
    writer_text(writer, "PID: "); writer_u64(writer, task->id); writer_char(writer, '\n');
    writer_text(writer, "Name: "); writer_text(writer, task->name != 0 ? task->name : "?"); writer_char(writer, '\n');
    writer_text(writer, "CR3: "); writer_hex64(writer, task->address_space); writer_char(writer, '\n');

    write_mapping(writer, task, "entry", task->user_entry);
    for (index = 0u; index < task->user_elf_page_count; ++index) {
        write_mapping(writer, task, "elf", task->user_elf_virtual_pages[index]);
    }
    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        const vaddr_t address = TASK_USER_STACK_TOP -
            (vaddr_t)((TASK_USER_STACK_PAGES - index) * VMM_PAGE_SIZE);
        write_mapping(writer, task, "stack", address);
    }
    for (index = 0u; index < task->user_mmap_page_count; ++index) {
        write_mapping(writer, task, "mmap", task->user_mmap_virtual_pages[index]);
    }
    writer_text(writer, "guard "); writer_hex64(writer, TASK_USER_STACK_GUARD_BASE);
    writer_text(writer, " unmapped\n");
}

static size_t procfs_render(enum procfs_kind kind)
{
    struct proc_writer writer = {
        .buffer = render_buffer,
        .capacity = sizeof(render_buffer),
        .length = 0u,
    };
    render_buffer[0] = '\0';

    switch (kind) {
        case PROCFS_MEMINFO: render_meminfo(&writer); break;
        case PROCFS_PROCESSES: render_processes(&writer); break;
        case PROCFS_INTERRUPTS: render_interrupts(&writer); break;
        case PROCFS_FILES: render_files(&writer); break;
        case PROCFS_NET: render_net(&writer); break;
        case PROCFS_SCHEDULER: render_scheduler(&writer); break;
        case PROCFS_CPUINFO: render_cpuinfo(&writer); break;
        case PROCFS_PAGEMAP: render_pagemap(&writer); break;
        default: break;
    }
    return writer.length;
}

static int procfs_lookup(
    struct vfs_node *directory,
    const char *name,
    struct vfs_node **node_out
)
{
    size_t index;
    if (directory != &proc_root || name == 0 || node_out == 0) return -AXIOM_EINVAL;
    for (index = 0u; index < PROCFS_FILE_COUNT; ++index) {
        if (text_equal(name, proc_names[index])) {
            *node_out = &proc_nodes[index].vfs;
            return 0;
        }
    }
    *node_out = 0;
    return -AXIOM_ENOENT;
}

static int64_t procfs_read(
    struct vfs_node *node,
    uint64_t offset,
    void *buffer,
    size_t count
)
{
    struct procfs_node *proc_node;
    size_t length;
    size_t available;
    size_t copied;

    if (node == 0 || (count != 0u && buffer == 0)) return -AXIOM_EFAULT;
    proc_node = (struct procfs_node *)node->private_data;
    if (proc_node == 0) return -AXIOM_EIO;

    if (!render_valid || offset == 0u || render_kind != proc_node->kind) {
        render_length = procfs_render(proc_node->kind);
        render_kind = proc_node->kind;
        render_valid = 1;
    }
    length = render_length;
    if (offset >= (uint64_t)length || count == 0u) return 0;
    available = length - (size_t)offset;
    copied = count < available ? count : available;
    bytes_copy(buffer, render_buffer + (size_t)offset, copied);
    return (int64_t)copied;
}

static int procfs_readdir(
    struct vfs_node *directory,
    size_t index,
    struct vfs_dirent *entry_out
)
{
    if (directory != &proc_root || entry_out == 0) return -AXIOM_EINVAL;
    if (index >= PROCFS_FILE_COUNT) return 0;
    copy_name(entry_out->name, proc_names[index], sizeof(entry_out->name));
    entry_out->type = AXIOM_DT_FILE;
    return 1;
}

struct vfs_filesystem *procfs_create(void)
{
    size_t index;
    if (initialized) return &proc_filesystem;

    proc_root.name = "proc";
    proc_root.type = VFS_NODE_DIRECTORY;
    proc_root.mode = PROCFS_DIRECTORY_MODE;
    proc_root.size = 0u;
    proc_root.ops = &procfs_ops;
    proc_root.private_data = 0;

    for (index = 0u; index < PROCFS_FILE_COUNT; ++index) {
        proc_nodes[index].kind = (enum procfs_kind)index;
        proc_nodes[index].vfs.name = proc_names[index];
        proc_nodes[index].vfs.type = VFS_NODE_FILE;
        proc_nodes[index].vfs.mode = PROCFS_FILE_MODE;
        proc_nodes[index].vfs.size = PROCFS_STAT_SIZE;
        proc_nodes[index].vfs.ops = &procfs_ops;
        proc_nodes[index].vfs.private_data = &proc_nodes[index];
    }

    proc_filesystem.name = "procfs";
    proc_filesystem.root = &proc_root;
    proc_filesystem.private_data = 0;
    render_length = 0u;
    render_kind = PROCFS_MEMINFO;
    render_valid = 0;
    initialized = 1;
    return &proc_filesystem;
}

int procfs_selftest(void)
{
    static const char *const required[] = {
        "/proc/meminfo",
        "/proc/processes",
        "/proc/interrupts",
        "/proc/files",
        "/proc/net",
        "/proc/scheduler",
        "/proc/cpuinfo",
        "/proc/pagemap",
    };
    size_t index;
    for (index = 0u; index < sizeof(required) / sizeof(required[0]); ++index) {
        struct vfs_file *file = 0;
        char probe[32];
        int result = vfs_open(required[index], AXIOM_O_RDONLY, &file);
        if (result < 0 || file == 0) return 0;
        if (vfs_read(file, probe, sizeof(probe)) <= 0) {
            (void)vfs_close(file);
            return 0;
        }
        if (vfs_write(file, probe, 1u) != -AXIOM_EACCES) {
            (void)vfs_close(file);
            return 0;
        }
        (void)vfs_close(file);
    }
    return 1;
}
