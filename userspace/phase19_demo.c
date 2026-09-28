#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <axiom/syscalls.h>
#include <axiom/abi/fs.h>

static int check(int condition, const char *name)
{
    if (!condition) {
        printf("Phase 19 %s: FAILED\n", name);
        return 0;
    }
    printf("Phase 19 %s: OK\n", name);
    return 1;
}

int main(void)
{
    char format[128];
    char moved[16] = "abcdef";
    char filebuf[64];
    const char payload[] = "libaxiom file round-trip";
    struct axiom_stat st;
    struct axiom_net_info net;
    int fd;
    long child;
    long status = -1;
    uint8_t *heap;
    uint8_t *grown;
    uint8_t *zeros;
    size_t i;

    printf("AxiomOS Phase 19 userspace libc demo.\n");

    snprintf(format, sizeof(format), "printf %s %d %u %x %p", "works", -19, 19u, 0x19u, (void *)(uintptr_t)0x4019u);
    if (!check(strcmp(format, "printf works -19 19 19 0x4019") == 0, "stdio formatting")) return 91;

    memmove(moved + 2, moved, 6u);
    if (!check(strlen("axiom") == 5u && strcmp("abc", "abc") == 0 &&
               strncmp("abcdef", "abcxyz", 3u) == 0 &&
               memcmp(moved, "ababcdef", 8u) == 0,
               "string/memory routines")) return 92;

    if (!check(isalpha('A') && isdigit('7') && isspace(' ') &&
               toupper('q') == 'Q' && tolower('Z') == 'z' &&
               atoi("-123") == -123 && strtol("0x2A", 0, 0) == 42,
               "ctype/numeric conversion")) return 93;

    heap = (uint8_t *)malloc(6000u);
    zeros = (uint8_t *)calloc(128u, 1u);
    if (heap == 0 || zeros == 0 || ((uintptr_t)heap & 15u) != 0u || ((uintptr_t)zeros & 15u) != 0u) return 94;
    for (i = 0u; i < 6000u; ++i) heap[i] = (uint8_t)(i ^ 0x5Au);
    for (i = 0u; i < 128u; ++i) if (zeros[i] != 0u) return 95;
    grown = (uint8_t *)realloc(heap, 12000u);
    if (grown == 0) return 96;
    for (i = 0u; i < 6000u; ++i) if (grown[i] != (uint8_t)(i ^ 0x5Au)) return 97;
    free(zeros);
    free(grown);
    heap = (uint8_t *)malloc(256u);
    if (!check(heap != 0, "malloc/calloc/realloc/free")) return 98;
    memset(heap, 0x3Cu, 256u);

    child = fork();
    if (child < 0) return 99;
    if (child == 0) {
        heap[0] = 0x99u;
        _exit(19);
    }
    if (waitpid(child, &status) != child || status != 19 || heap[0] != 0x3Cu) return 100;
    if (!check(1, "malloc-backed fork isolation")) return 101;

    status = -1;
    child = fork();
    if (child < 0) return 112;
    if (child == 0) {
        if (exec("/bin/phase11-demo") < 0) _exit(113);
        _exit(114);
    }
    if (waitpid(child, &status) != child || status != 11 || heap[0] != 0x3Cu) return 115;
    if (!check(1, "mmap-backed exec cleanup")) return 116;
    free(heap);

    fd = open("/tmp/phase19-libc.txt", O_CREAT | O_RDWR | O_TRUNC);
    if (fd < 0) return 102;
    if (write(fd, payload, sizeof(payload) - 1u) != (long)(sizeof(payload) - 1u)) return 103;
    if (lseek(fd, 0, AXIOM_SEEK_SET) != 0) return 104;
    memset(filebuf, 0, sizeof(filebuf));
    if (read(fd, filebuf, sizeof(payload) - 1u) != (long)(sizeof(payload) - 1u)) return 105;
    if (close(fd) < 0 || stat("/tmp/phase19-libc.txt", &st) < 0) return 106;
    if (!check(strcmp(filebuf, payload) == 0 && st.size == sizeof(payload) - 1u, "unistd/file wrappers")) return 107;

    if (getpid() <= 0 || getppid() < 0 || yield() < 0 || sleep(1u) < 0) return 108;
    if (!check(1, "process wrappers")) return 109;

    if (axiom_netinfo(&net) < 0 || net.address == 0u) return 110;
    if (!check(1, "network syscall wrapper")) return 111;

    printf("Phase 19 libc demo complete: pid=%ld IPv4=%u.%u.%u.%u\n",
           getpid(),
           (net.address >> 24) & 0xFFu,
           (net.address >> 16) & 0xFFu,
           (net.address >> 8) & 0xFFu,
           net.address & 0xFFu);
    return 19;
}
