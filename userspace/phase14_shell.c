#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <axiom/abi/errno.h>
#include <axiom/abi/fs.h>
#include <axiom/abi/graphics.h>
#include <axiom/abi/input.h>
#include <axiom/abi/ipc.h>
#include <axiom/abi/network.h>
#include <axiom/abi/process.h>
#include <axiom/abi/scheduler.h>
#include <axiom/abi/syscall.h>

#define SHELL_LINE_MAX 256u
#define SHELL_PATH_MAX 256u
#define SHELL_IO_CHUNK 256u

static char cwd[SHELL_PATH_MAX] = "/";
static char http_body[AXIOM_NET_HTTP_USER_MAX];
static int shell_exit_requested;

static long syscall0(long number)
{
    register long rax __asm__("rax") = number;
    __asm__ volatile("syscall" : "+a"(rax) : : "rcx", "r11", "memory");
    return rax;
}

static long syscall1(long number, long first)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = first;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi) : "rcx", "r11", "memory");
    return rax;
}

static long syscall2(long number, long first, long second)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = first;
    register long rsi __asm__("rsi") = second;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi) : "rcx", "r11", "memory");
    return rax;
}

static long syscall3(long number, long first, long second, long third)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = first;
    register long rsi __asm__("rsi") = second;
    register long rdx __asm__("rdx") = third;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi), "d"(rdx) : "rcx", "r11", "memory");
    return rax;
}

static long syscall5(
    long number,
    long first,
    long second,
    long third,
    long fourth,
    long fifth
)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = first;
    register long rsi __asm__("rsi") = second;
    register long rdx __asm__("rdx") = third;
    register long r10 __asm__("r10") = fourth;
    register long r8 __asm__("r8") = fifth;
    __asm__ volatile(
        "syscall"
        : "+a"(rax)
        : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory"
    );
    return rax;
}

static size_t text_length(const char *text)
{
    return strlen(text);
}

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

static void copy_text(char *destination, const char *source, size_t capacity)
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

static void print_n(const char *text, size_t count)
{
    if (count != 0u) (void)write(1, text, count);
}

static void print(const char *text)
{
    print_n(text, text_length(text));
}

static void print_u64(uint64_t value)
{
    char buffer[32];
    size_t used = 0u;
    if (value == 0u) {
        print("0");
        return;
    }
    while (value != 0u && used < sizeof(buffer)) {
        buffer[used++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (used != 0u) {
        --used;
        print_n(&buffer[used], 1u);
    }
}

static void print_i64(int64_t value)
{
    if (value < 0) {
        print("-");
        print_u64((uint64_t)(-(value + 1)) + 1u);
    } else {
        print_u64((uint64_t)value);
    }
}

static void print_hex_byte(uint8_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    char pair[2];
    pair[0] = digits[value >> 4];
    pair[1] = digits[value & 0x0Fu];
    print_n(pair, 2u);
}

static void print_ipv4(uint32_t address)
{
    print_u64((address >> 24) & 0xFFu); print(".");
    print_u64((address >> 16) & 0xFFu); print(".");
    print_u64((address >> 8) & 0xFFu); print(".");
    print_u64(address & 0xFFu);
}

static char *skip_spaces(char *text)
{
    while (*text == ' ' || *text == '\t') ++text;
    return text;
}

static char *next_token(char **cursor)
{
    char *start;
    char *position;
    if (cursor == 0 || *cursor == 0) return 0;
    position = skip_spaces(*cursor);
    if (*position == '\0') {
        *cursor = position;
        return 0;
    }
    start = position;
    while (*position != '\0' && *position != ' ' && *position != '\t') ++position;
    if (*position != '\0') *position++ = '\0';
    *cursor = position;
    return start;
}

static int normalize_path(const char *input, char *output, size_t capacity)
{
    char combined[SHELL_PATH_MAX * 2u];
    char *parts[32];
    size_t count = 0u;
    size_t length = 0u;
    char *cursor;

    if (input == 0 || output == 0 || capacity < 2u) return 0;

    if (input[0] == '/') {
        copy_text(combined, input, sizeof(combined));
    } else {
        size_t cwd_len = text_length(cwd);
        size_t input_len = text_length(input);
        if (cwd_len + input_len + 2u > sizeof(combined)) return 0;
        copy_text(combined, cwd, sizeof(combined));
        length = text_length(combined);
        if (length > 1u && combined[length - 1u] != '/') combined[length++] = '/';
        if (length == 1u && combined[0] == '/') { }
        for (size_t i = 0u; i < input_len; ++i) combined[length++] = input[i];
        combined[length] = '\0';
    }

    cursor = combined;
    while (*cursor != '\0') {
        char *start;
        while (*cursor == '/') ++cursor;
        if (*cursor == '\0') break;
        start = cursor;
        while (*cursor != '\0' && *cursor != '/') ++cursor;
        if (*cursor != '\0') *cursor++ = '\0';
        if (text_equal(start, ".")) continue;
        if (text_equal(start, "..")) {
            if (count != 0u) --count;
            continue;
        }
        if (count >= 32u) return 0;
        parts[count++] = start;
    }

    length = 0u;
    output[length++] = '/';
    for (size_t i = 0u; i < count; ++i) {
        const size_t part_len = text_length(parts[i]);
        if (length + part_len + 1u >= capacity) return 0;
        for (size_t j = 0u; j < part_len; ++j) output[length++] = parts[i][j];
        if (i + 1u < count) output[length++] = '/';
    }
    output[length] = '\0';
    return 1;
}

static long open_file(const char *path, long flags)
{
    return syscall2(AXIOM_SYS_OPEN, (long)(uintptr_t)path, flags);
}

static long close_file(long fd) { return syscall1(AXIOM_SYS_CLOSE, fd); }
static long stat_path(const char *path, struct axiom_stat *status)
{
    return syscall2(AXIOM_SYS_STAT, (long)(uintptr_t)path, (long)(uintptr_t)status);
}

static void print_error(const char *operation, long error)
{
    print(operation);
    print(": error ");
    print_i64(error);
    print("\n");
}

static int parse_u64(const char *text, uint64_t *value_out)
{
    uint64_t value = 0u;
    size_t index = 0u;

    if (text == 0 || value_out == 0 || text[0] == '\0') return 0;
    while (text[index] != '\0') {
        const unsigned digit = (unsigned)(text[index] - '0');
        if (digit > 9u || value > (UINT64_MAX - digit) / 10u) return 0;
        value = value * 10u + digit;
        ++index;
    }
    *value_out = value;
    return 1;
}

static const char *process_state_name(uint32_t state)
{
    switch (state) {
        case AXIOM_PROC_RUNNING: return "RUNNING";
        case AXIOM_PROC_READY: return "READY";
        case AXIOM_PROC_BLOCKED: return "BLOCKED";
        case AXIOM_PROC_SLEEPING: return "SLEEPING";
        case AXIOM_PROC_TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

static void command_ps(void)
{
    uint64_t index = 0u;
    long result;
    struct axiom_process_info info;

    print("PID  PPID  STATE       PRIV  PRI/EFF  AFF  TICKS  NAME\n");
    for (;;) {
        result = syscall2(
            AXIOM_SYS_PROCINFO,
            (long)index,
            (long)(uintptr_t)&info
        );
        if (result == 0) break;
        if (result < 0) {
            print_error("ps", result);
            return;
        }

        print_u64(info.pid); print("  ");
        if (info.ppid == UINT64_MAX) print("-"); else print_u64(info.ppid);
        print("  "); print(process_state_name(info.state)); print("  ");
        print(info.privilege == AXIOM_PRIV_USER ? "USER" : "KERN");
        print("  "); print_u64(info.priority); print("/");
        print_u64(info.effective_priority);
        print("  "); print_u64(info.affinity_mask);
        print("  "); print_u64(info.runtime_ticks); print("  ");
        print(info.name); print("\n");
        ++index;
    }
}

static int resolve_program_path(const char *command, char *path, size_t capacity)
{
    if (command == 0 || path == 0 || capacity == 0u) return 0;

    if (command[0] == '/' || command[0] == '.') {
        return normalize_path(command, path, capacity);
    }

    {
        static const char prefix[] = "/bin/";
        const size_t prefix_length = text_length(prefix);
        const size_t command_length = text_length(command);
        size_t index;

        if (prefix_length + command_length + 1u > capacity) return 0;
        copy_text(path, prefix, capacity);
        for (index = 0u; index < command_length; ++index) {
            path[prefix_length + index] = command[index];
        }
        path[prefix_length + command_length] = '\0';
    }
    return 1;
}

static void command_spawn_background(const char *program, const char *extra)
{
    char path[SHELL_PATH_MAX];
    long pid;

    if (program == 0) {
        print("spawn: missing program\n");
        return;
    }
    if (extra != 0 && *skip_spaces((char *)extra) != '\0') {
        print("spawn: program arguments are not supported yet\n");
        return;
    }
    if (!resolve_program_path(program, path, sizeof(path))) {
        print("spawn: program path too long\n");
        return;
    }

    pid = syscall1(AXIOM_SYS_SPAWN, (long)(uintptr_t)path);
    if (pid < 0) {
        print_error("spawn", pid);
        return;
    }
    print("[started pid "); print_u64((uint64_t)pid); print("]\n");
}

static void command_wait(const char *argument)
{
    uint64_t pid;
    int64_t status = 0;
    long result;

    if (!parse_u64(argument, &pid)) {
        print("wait: usage: wait <pid>\n");
        return;
    }
    result = syscall2(AXIOM_SYS_WAITPID, (long)pid, (long)(uintptr_t)&status);
    if (result < 0) {
        print_error("wait", result);
        return;
    }
    print("[process "); print_u64(pid); print(" exited "); print_i64(status); print("]\n");
}

static void command_kill(const char *argument)
{
    uint64_t pid;
    long result;

    if (!parse_u64(argument, &pid)) {
        print("kill: usage: kill <pid>\n");
        return;
    }
    result = syscall2(AXIOM_SYS_KILL, (long)pid, AXIOM_SIGTERM);
    if (result < 0) {
        print_error("kill", result);
        return;
    }
    print("sent SIGTERM to "); print_u64(pid); print("\n");
}

static void command_netinfo(void)
{
    struct axiom_net_info info;
    long result = syscall1(AXIOM_SYS_NETINFO, (long)(uintptr_t)&info);
    unsigned index;

    if (result < 0) { print_error("netinfo", result); return; }
    print("NIC: Intel E1000 (polled)\nMAC: ");
    for (index = 0u; index < 6u; ++index) {
        if (index != 0u) print(":");
        print_hex_byte(info.mac[index]);
    }
    print("\nIPv4: "); print_ipv4(info.address);
    print("\nNetmask: "); print_ipv4(info.netmask);
    print("\nGateway: "); print_ipv4(info.gateway);
    print("\nDNS: "); print_ipv4(info.dns_server);
    print("\nFrames TX/RX: "); print_u64(info.tx_frames); print("/"); print_u64(info.rx_frames);
    print("\nARP requests/replies: "); print_u64(info.arp_requests); print("/"); print_u64(info.arp_replies);
    print("\nIPv4 TX/RX: "); print_u64(info.ipv4_tx); print("/"); print_u64(info.ipv4_rx);
    print("\n");
}

static void command_ping(const char *target)
{
    struct axiom_ping_result ping;
    long result;
    if (target == 0) { print("ping: usage: ping <IPv4-or-host>\n"); return; }
    result = syscall2(
        AXIOM_SYS_PING,
        (long)(uintptr_t)target,
        (long)(uintptr_t)&ping
    );
    if (result < 0) { print_error("ping", result); return; }
    print("reply from "); print_ipv4(ping.address);
    print(": icmp_seq="); print_u64(ping.sequence);
    print(" polls="); print_u64(ping.poll_iterations); print("\n");
}

static void command_dns(const char *name)
{
    uint32_t address = 0u;
    long result;
    if (name == 0) { print("dns: usage: dns <hostname>\n"); return; }
    result = syscall2(
        AXIOM_SYS_DNS,
        (long)(uintptr_t)name,
        (long)(uintptr_t)&address
    );
    if (result < 0) { print_error("dns", result); return; }
    print(name); print(" -> "); print_ipv4(address); print("\n");
}

static void command_httpget(const char *host, const char *path)
{
    struct axiom_http_result result_info;
    long result;

    if (host == 0) {
        print("httpget: usage: httpget <host[:port]> [path]\n");
        return;
    }
    if (path == 0 || path[0] == '\0') path = "/";
    if (path[0] != '/') {
        print("httpget: path must begin with /\n");
        return;
    }

    result = syscall5(
        AXIOM_SYS_HTTPGET,
        (long)(uintptr_t)host,
        (long)(uintptr_t)path,
        (long)(uintptr_t)http_body,
        (long)sizeof(http_body),
        (long)(uintptr_t)&result_info
    );
    if (result < 0) { print_error("httpget", result); return; }

    print("HTTP "); print_u64(result_info.status_code);
    print(" from "); print_ipv4(result_info.address);
    print(":"); print_u64(result_info.port);
    print(" (body "); print_u64(result_info.body_bytes); print(" bytes");
    if (result_info.truncated != 0u) print(", truncated");
    print(")\n");
    if (result_info.body_bytes != 0u) {
        print_n(http_body, (size_t)result_info.body_bytes);
        if (http_body[result_info.body_bytes - 1u] != '\n') print("\n");
    }
}

static void command_schedstats(void)
{
    struct axiom_scheduler_stats stats;
    long result = syscall1(
        AXIOM_SYS_SCHEDSTATS,
        (long)(uintptr_t)&stats
    );

    if (result < 0) {
        print_error("schedstats", result);
        return;
    }

    print("Scheduler policy: ");
    print(stats.policy == AXIOM_SCHED_POLICY_PRIORITY_AGING ?
        "priority-aging" : "round-robin");
    print("\nTasks/runnable: "); print_u64(stats.task_count); print("/");
    print_u64(stats.runnable_tasks);
    print("\nContext switches/preemptions: ");
    print_u64(stats.context_switches); print("/"); print_u64(stats.preemptions);
    print("\nPriority preemptions/aging promotions: ");
    print_u64(stats.priority_preemptions); print("/"); print_u64(stats.aging_promotions);
    print("\nVoluntary yields: "); print_u64(stats.voluntary_yields);
    print("\nCurrent priority/effective: ");
    print_u64(stats.current_priority); print("/");
    print_u64(stats.current_effective_priority);
    print("\nCurrent affinity mask: "); print_u64(stats.current_affinity_mask);
    print("\n");
}

static void command_ipcstats(void)
{
    struct axiom_ipc_stats stats;
    long result = syscall1(AXIOM_SYS_IPCSTATS, (long)(uintptr_t)&stats);
    if (result < 0) {
        print_error("ipcstats", result);
        return;
    }
    print("IPC pipes created: "); print_u64(stats.pipes_created);
    print("\nPipe bytes written/read: "); print_u64(stats.pipe_bytes_written);
    print("/"); print_u64(stats.pipe_bytes_read);
    print("\nShared objects/attachments: "); print_u64(stats.shared_objects_created);
    print("/"); print_u64(stats.shared_attachments);
    print("\nMessage queues created: "); print_u64(stats.message_queues_created);
    print("\nMessages sent/received: "); print_u64(stats.messages_sent);
    print("/"); print_u64(stats.messages_received);
    print("\n");
}

static void command_gfxinfo(void)
{
    struct axiom_gfx_info info;
    struct axiom_gfx_stats stats;
    long result = syscall1(AXIOM_SYS_GFX_INFO, (long)(uintptr_t)&info);

    if (result < 0) {
        print_error("gfxinfo", result);
        return;
    }
    result = syscall1(AXIOM_SYS_GFX_STATS, (long)(uintptr_t)&stats);
    if (result < 0) {
        print_error("gfxstats", result);
        return;
    }

    print("Framebuffer: "); print_u64(info.width); print("x"); print_u64(info.height);
    print(" pitch="); print_u64(info.pitch); print(" bpp="); print_u64(info.bpp);
    print("\nFont: "); print_u64(info.font_width); print("x"); print_u64(info.font_height);
    print("\nGraphics calls clear/line/rect/bitmap/text: ");
    print_u64(stats.clears); print("/"); print_u64(stats.lines); print("/");
    print_u64(stats.rectangles); print("/"); print_u64(stats.bitmaps); print("/");
    print_u64(stats.text_calls);
    print("\nPixels/clipped: "); print_u64(stats.pixels); print("/");
    print_u64(stats.clipped_pixels); print("\n");
}

static void command_help(void)
{
    print("AxiomOS shell commands:\n");
    print("  help                 show this help\n");
    print("  clear                clear framebuffer terminal\n");
    print("  echo <text>          print text\n");
    print("  pwd                  print current directory\n");
    print("  cd <dir>             change shell directory\n");
    print("  ls [dir]             list a directory\n");
    print("  cat <file>           print a file\n");
    print("  stat <path>          show file type and size\n");
    print("  touch <file>         create a file\n");
    print("  write <file> <text>  replace file contents\n");
    print("  mkdir <dir>          create a directory\n");
    print("  kbdstats             show PS/2 keyboard counters\n");
    print("  id                   show Phase-20 uid/gid credentials\n");
    print("  ps                   show process table + priority/affinity\n");
    print("  schedstats           show Phase-21 scheduler statistics\n");
    print("  ipcstats             show Phase-22 IPC statistics\n");
    print("  gfxinfo              show Phase-23 framebuffer/graphics statistics\n");
    print("  spawn <program>      start a program without waiting\n");
    print("  wait <pid>           wait for one of this shell's children\n");
    print("  kill <pid>           terminate a userspace process (SIGTERM)\n");
    print("  netinfo              show E1000/IPv4 configuration and counters\n");
    print("  ping [host]          ICMP echo; no host runs Phase-24 /bin/ping\n");
    print("  dns <host>           resolve an IPv4 A record using UDP/DNS\n");
    print("  httpget [host] [p]   HTTP GET; no host runs Phase-24 /bin/httpget\n");
    print("  Phase-24 apps: ifconfig, dnslookup, httpd, /bin/ping, /bin/httpget\n");
    print("  sysinfo              Phase-25 /proc developer telemetry dashboard\n");
    print("  desktop              open the Phase-26 graphical desktop\n");
    print("  exit                 leave this shell (returns to desktop if launched there)\n");
    print("  /proc/*              live memory/process/IRQ/FD/net/scheduler/CPU/maps\n");
    print("  <program>            run /bin/<program> and wait\n");
    print("  /path/program        run an ELF by VFS path\n");
}

static void command_ls(const char *argument)
{
    char path[SHELL_PATH_MAX];
    struct axiom_dirent entry;
    uint64_t index = 0u;
    long result;

    if (!normalize_path(argument != 0 ? argument : ".", path, sizeof(path))) {
        print("ls: path too long\n");
        return;
    }

    for (;;) {
        result = syscall3(
            AXIOM_SYS_READDIR,
            (long)(uintptr_t)path,
            (long)index,
            (long)(uintptr_t)&entry
        );
        if (result == 0) break;
        if (result < 0) {
            print_error("ls", result);
            return;
        }
        print(entry.name);
        if (entry.type == AXIOM_DT_DIR) print("/");
        print("\n");
        ++index;
    }
}

static void command_cat(const char *argument)
{
    char path[SHELL_PATH_MAX];
    char buffer[SHELL_IO_CHUNK];
    long fd;
    long got;

    if (argument == 0) { print("cat: missing file\n"); return; }
    if (!normalize_path(argument, path, sizeof(path))) { print("cat: path too long\n"); return; }
    fd = open_file(path, AXIOM_O_RDONLY);
    if (fd < 0) { print_error("cat", fd); return; }
    for (;;) {
        got = syscall3(AXIOM_SYS_READ, fd, (long)(uintptr_t)buffer, sizeof(buffer));
        if (got < 0) { print_error("cat", got); break; }
        if (got == 0) break;
        print_n(buffer, (size_t)got);
    }
    (void)close_file(fd);
}

static void command_stat(const char *argument)
{
    char path[SHELL_PATH_MAX];
    struct axiom_stat status;
    long result;
    if (argument == 0) { print("stat: missing path\n"); return; }
    if (!normalize_path(argument, path, sizeof(path))) { print("stat: path too long\n"); return; }
    result = stat_path(path, &status);
    if (result < 0) { print_error("stat", result); return; }
    print("type: ");
    print(status.type == AXIOM_DT_DIR ? "directory" : "file");
    print("\nsize: ");
    print_u64(status.size);
    print(" bytes\nmode: ");
    print_u64(status.mode);
    print(" (permission bits)\n");
}

static void command_touch(const char *argument)
{
    char path[SHELL_PATH_MAX];
    long fd;
    if (argument == 0) { print("touch: missing file\n"); return; }
    if (!normalize_path(argument, path, sizeof(path))) { print("touch: path too long\n"); return; }
    fd = open_file(path, AXIOM_O_CREAT | AXIOM_O_RDWR);
    if (fd < 0) { print_error("touch", fd); return; }
    (void)close_file(fd);
}

static void command_write(const char *argument, const char *text)
{
    char path[SHELL_PATH_MAX];
    long fd;
    long result;
    if (argument == 0 || text == 0 || *text == '\0') {
        print("write: usage: write <file> <text>\n");
        return;
    }
    if (!normalize_path(argument, path, sizeof(path))) { print("write: path too long\n"); return; }
    fd = open_file(path, AXIOM_O_CREAT | AXIOM_O_WRONLY | AXIOM_O_TRUNC);
    if (fd < 0) { print_error("write", fd); return; }
    result = syscall3(AXIOM_SYS_WRITE, fd, (long)(uintptr_t)text, (long)text_length(text));
    if (result >= 0) {
        static const char newline = '\n';
        result = syscall3(AXIOM_SYS_WRITE, fd, (long)(uintptr_t)&newline, 1);
    }
    if (result < 0) print_error("write", result);
    (void)close_file(fd);
}

static void command_mkdir(const char *argument)
{
    char path[SHELL_PATH_MAX];
    long result;
    if (argument == 0) { print("mkdir: missing directory\n"); return; }
    if (!normalize_path(argument, path, sizeof(path))) { print("mkdir: path too long\n"); return; }
    result = syscall1(AXIOM_SYS_MKDIR, (long)(uintptr_t)path);
    if (result < 0) print_error("mkdir", result);
}

static void command_cd(const char *argument)
{
    char path[SHELL_PATH_MAX];
    struct axiom_stat status;
    long result;
    if (argument == 0) argument = "/";
    if (!normalize_path(argument, path, sizeof(path))) { print("cd: path too long\n"); return; }
    result = stat_path(path, &status);
    if (result < 0) { print_error("cd", result); return; }
    if (status.type != AXIOM_DT_DIR) { print("cd: not a directory\n"); return; }
    copy_text(cwd, path, sizeof(cwd));
}


static void command_id(void)
{
    print("uid=");
    print_u64((uint64_t)getuid());
    print(" gid=");
    print_u64((uint64_t)getgid());
    print("\n");
}

static void command_kbdstats(void)
{
    struct axiom_keyboard_stats stats;
    long result = syscall1(AXIOM_SYS_KBDSTATS, (long)(uintptr_t)&stats);
    if (result < 0) { print_error("kbdstats", result); return; }
    print("Keyboard IRQs/scancodes/chars/dropped: ");
    print_u64(stats.irq_count); print("/");
    print_u64(stats.scancode_count); print("/");
    print_u64(stats.character_count); print("/");
    print_u64(stats.dropped_characters); print("\n");
}

static void run_program(const char *command, const char *extra)
{
    char path[SHELL_PATH_MAX];
    int64_t status = 0;
    long pid;
    long waited;

    if (extra != 0 && *skip_spaces((char *)extra) != '\0') {
        print("shell: external program arguments are not supported yet\n");
        return;
    }

    if (!resolve_program_path(command, path, sizeof(path))) {
        print("shell: program path too long\n");
        return;
    }

    pid = syscall1(AXIOM_SYS_SPAWN, (long)(uintptr_t)path);
    if (pid < 0) {
        if (pid == -AXIOM_ENOENT) {
            print("shell: command not found: "); print(command); print("\n");
        } else {
            print_error("spawn", pid);
        }
        return;
    }

    waited = syscall2(AXIOM_SYS_WAITPID, pid, (long)(uintptr_t)&status);
    if (waited < 0) {
        print_error("waitpid", waited);
        return;
    }
    print("[process "); print_u64((uint64_t)pid); print(" exited "); print_i64(status); print("]\n");
}

static void execute_line(char *line)
{
    char *cursor = line;
    char *command = next_token(&cursor);
    char *first;
    char *rest;

    if (command == 0) return;
    first = next_token(&cursor);
    rest = skip_spaces(cursor);

    if (text_equal(command, "help")) command_help();
    else if (text_equal(command, "exit")) shell_exit_requested = 1;
    else if (text_equal(command, "clear")) (void)syscall0(AXIOM_SYS_CLEAR);
    else if (text_equal(command, "echo")) { if (first != 0) { print(first); if (*rest != '\0') { print(" "); print(rest); } } print("\n"); }
    else if (text_equal(command, "pwd")) { print(cwd); print("\n"); }
    else if (text_equal(command, "cd")) command_cd(first);
    else if (text_equal(command, "ls")) command_ls(first);
    else if (text_equal(command, "cat")) command_cat(first);
    else if (text_equal(command, "stat")) command_stat(first);
    else if (text_equal(command, "touch")) command_touch(first);
    else if (text_equal(command, "write")) command_write(first, rest);
    else if (text_equal(command, "mkdir")) command_mkdir(first);
    else if (text_equal(command, "kbdstats")) command_kbdstats();
    else if (text_equal(command, "id")) command_id();
    else if (text_equal(command, "ps")) command_ps();
    else if (text_equal(command, "schedstats")) command_schedstats();
    else if (text_equal(command, "ipcstats")) command_ipcstats();
    else if (text_equal(command, "gfxinfo")) command_gfxinfo();
    else if (text_equal(command, "spawn")) command_spawn_background(first, rest);
    else if (text_equal(command, "wait")) command_wait(first);
    else if (text_equal(command, "kill")) command_kill(first);
    else if (text_equal(command, "netinfo")) command_netinfo();
    else if (text_equal(command, "ping")) {
        if (first != 0) command_ping(first); else run_program("ping", 0);
    }
    else if (text_equal(command, "dns")) command_dns(first);
    else if (text_equal(command, "httpget")) {
        if (first != 0) command_httpget(first, *rest != '\0' ? rest : 0);
        else run_program("httpget", 0);
    }
    else run_program(command, first != 0 ? first : 0);
}

static size_t read_line(char *line, size_t capacity)
{
    size_t length = 0u;
    for (;;) {
        char character = '\0';
        long got = read(0, &character, 1u);
        if (got < 0) return 0u;
        if (got == 0) {
            (void)sleep(10u);
            continue;
        }
        if (character == '\n') {
            print("\n");
            line[length] = '\0';
            return length;
        }
        if (character == '\b') {
            if (length != 0u) {
                --length;
                line[length] = '\0';
                print("\b");
            }
            continue;
        }
        if (character >= 32 && character < 127 && length + 1u < capacity) {
            line[length++] = character;
            line[length] = '\0';
            print_n(&character, 1u);
        }
    }
}

int main(void)
{
    char line[SHELL_LINE_MAX];

    shell_exit_requested = 0;
    print("AxiomOS shell ready. Type 'help' for commands.\n");
    for (;;) {
        print("axiom> ");
        (void)read_line(line, sizeof(line));
        execute_line(line);
        if (shell_exit_requested) break;
    }
    print("Leaving AxiomOS shell.\n");
    return 0;
}
