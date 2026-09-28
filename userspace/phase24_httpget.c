#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <axiom/syscalls.h>

int main(void)
{
    static const char host[] = "10.0.2.100";
    static const char path[] = "/phase24";
    char body[512];
    struct axiom_http_result result;
    long r;

    puts("AxiomOS Phase 24 httpget");
    r = axiom_httpget(host, path, body, sizeof(body), &result);
    if (r < 0) {
        printf("httpget: error %ld\n", r);
        return 1;
    }
    printf("HTTP %u from %u.%u.%u.%u:%u (%llu bytes%s)\n",
        (unsigned)result.status_code,
        (unsigned)((result.address >> 24) & 0xFFu),
        (unsigned)((result.address >> 16) & 0xFFu),
        (unsigned)((result.address >> 8) & 0xFFu),
        (unsigned)(result.address & 0xFFu),
        (unsigned)result.port,
        (unsigned long long)result.body_bytes,
        result.truncated ? ", truncated" : "");
    if (result.body_bytes != 0u) {
        (void)write(1, body, (size_t)result.body_bytes);
        if (body[result.body_bytes - 1u] != '\n') putchar('\n');
    }
    puts("Phase 24 httpget application: OK");
    return result.status_code == 200u ? 0 : 2;
}
