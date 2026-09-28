#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

static volatile uint64_t fork_marker = 0x1111111111111111ULL;

int main(void)
{
    const long parent_pid = getpid();
    long child;
    long status = -1;

    puts("Phase 15 fork demo starting.");
    child = fork();
    if (child < 0) { puts("Phase 15 fork: FAILED"); return 80; }
    if (child == 0) {
        if (getppid() != parent_pid) _exit(81);
        puts("Phase 15 child: fork returned 0; exec follows.");
        if (exec("/bin/phase11-demo") < 0) _exit(82);
        _exit(83);
    }
    if (waitpid(child, &status) != child || status != 11) { puts("Phase 15 fork/exec/wait: FAILED"); return 84; }
    puts("Phase 15 fork/exec/wait: OK");

    status = -1;
    child = fork();
    if (child < 0) { puts("Phase 15 second fork: FAILED"); return 85; }
    if (child == 0) { fork_marker = 0x2222222222222222ULL; (void)sleep(150); _exit(42); }
    if (waitpid(child, &status) != child || status != 42) { puts("Phase 15 blocked wait/sleep: FAILED"); return 86; }
    puts("Phase 15 blocked wait/sleep: OK");
    if (fork_marker != 0x1111111111111111ULL) { puts("Phase 15 fork memory isolation: FAILED"); return 87; }
    puts("Phase 15 fork memory isolation: OK");
    puts("Phase 15 process management demo complete.");
    return 15;
}
