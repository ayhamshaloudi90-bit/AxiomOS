#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <axiom/syscalls.h>
#include <axiom/abi/errno.h>
#include <axiom/abi/fs.h>
#include <axiom/abi/user_layout.h>

static int check(int condition, const char *name)
{
    if (!condition) {
        printf("Phase 20 %s: FAILED\n", name);
        return 0;
    }
    printf("Phase 20 %s: OK\n", name);
    return 1;
}

static int expect_fault_from_kernel_read(void)
{
    long child = fork();
    long status = -1;
    if (child < 0) return 0;
    if (child == 0) {
        volatile uint64_t value = *(volatile uint64_t *)(uintptr_t)0xFFFFFFFF80000000ULL;
        (void)value;
        _exit(1);
    }
    return waitpid(child, &status) == child && status == 142;
}

static int expect_fault_from_heap_execute(void)
{
    long child = fork();
    long status = -1;
    if (child < 0) return 0;
    if (child == 0) {
        unsigned char *code = (unsigned char *)malloc(16u);
        void (*fn)(void);
        if (code == 0) _exit(2);
        code[0] = 0xC3u; /* ret */
        fn = (void (*)(void))(uintptr_t)code;
        fn();
        _exit(3);
    }
    return waitpid(child, &status) == child && status == 142;
}

static int expect_fault_from_stack_guard(void)
{
    long child = fork();
    long status = -1;
    if (child < 0) return 0;
    if (child == 0) {
        volatile uint64_t *guard = (volatile uint64_t *)(uintptr_t)AXIOM_USER_STACK_GUARD_BASE;
        *guard = 0x20u;
        _exit(4);
    }
    return waitpid(child, &status) == child && status == 142;
}

int main(void)
{
    struct axiom_stat st;
    int fd;
    long result;
    const char payload[] = "not an executable";

    printf("AxiomOS Phase 20 security demo.\n");

    if (!check(getuid() == 1000 && getgid() == 1000, "userspace UID/GID credentials")) return 120;

    if (stat("/bin/phase19-demo", &st) < 0) return 121;
    if (!check((st.mode & AXIOM_S_IXALL) != 0u && (st.mode & AXIOM_S_IWALL) == 0u,
               "system executable permissions")) return 122;

    fd = open("/tmp/phase20-data", O_CREAT | O_RDWR | O_TRUNC);
    if (fd < 0) return 123;
    if (write(fd, payload, sizeof(payload) - 1u) != (long)(sizeof(payload) - 1u)) return 124;
    if (close(fd) < 0 || stat("/tmp/phase20-data", &st) < 0) return 125;
    result = axiom_spawn("/tmp/phase20-data");
    if (!check((st.mode & AXIOM_S_IXALL) == 0u && result == -AXIOM_EACCES,
               "non-executable file blocked")) return 126;

    result = write(1, (const void *)(uintptr_t)0xFFFFFFFF80000000ULL, 8u);
    if (!check(result == -AXIOM_EFAULT, "invalid syscall pointer rejected")) return 127;

    if (!check(expect_fault_from_kernel_read(), "Ring-3 kernel isolation")) return 128;
    if (!check(expect_fault_from_heap_execute(), "NX heap enforcement")) return 129;
    if (!check(expect_fault_from_stack_guard(), "user stack guard page")) return 130;

    result = axiom_spawn("/bin/phase20-wx");
    if (!check(result == -AXIOM_EINVAL, "ELF W^X rejection")) return 131;

    printf("Phase 20 security demo complete: uid=%ld gid=%ld\n", getuid(), getgid());
    return 20;
}
