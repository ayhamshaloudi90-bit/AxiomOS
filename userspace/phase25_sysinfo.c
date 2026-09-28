#include <stddef.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

static const char *const proc_files[] = {
    "/proc/cpuinfo",
    "/proc/meminfo",
    "/proc/processes",
    "/proc/scheduler",
    "/proc/interrupts",
    "/proc/files",
    "/proc/net",
    "/proc/pagemap",
};

static int dump_file(const char *path)
{
    char buffer[256];
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("sysinfo: cannot open %s (%d)\n", path, fd);
        return 0;
    }

    printf("\n===== %s =====\n", path);
    for (;;) {
        const long got = read(fd, buffer, sizeof(buffer));
        if (got < 0) {
            printf("sysinfo: read failed for %s (%ld)\n", path, got);
            close(fd);
            return 0;
        }
        if (got == 0) break;
        if (write(1, buffer, (size_t)got) != got) {
            close(fd);
            return 0;
        }
    }
    close(fd);
    return 1;
}

int main(void)
{
    size_t index;

    printf("AxiomOS Phase 25 developer tooling dashboard.\n");
    for (index = 0u; index < sizeof(proc_files) / sizeof(proc_files[0]); ++index) {
        if (!dump_file(proc_files[index])) return 25;
    }
    printf("\nPhase 25 /proc observability: OK\n");
    printf("Phase 25 developer tooling demo complete.\n");
    return 0;
}
