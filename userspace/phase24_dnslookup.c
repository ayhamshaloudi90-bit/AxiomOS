#include <stdint.h>
#include <stdio.h>
#include <axiom/syscalls.h>

static void print_ipv4(uint32_t a)
{
    printf("%u.%u.%u.%u",
        (unsigned)((a >> 24) & 0xFFu),
        (unsigned)((a >> 16) & 0xFFu),
        (unsigned)((a >> 8) & 0xFFu),
        (unsigned)(a & 0xFFu));
}

int main(void)
{
    uint32_t address = 0u;
    static const char host[] = "example.com";
    long r;

    printf("AxiomOS Phase 24 dnslookup: %s\n", host);
    r = axiom_dns(host, &address);
    if (r < 0) {
        printf("dnslookup: error %ld\n", r);
        return 1;
    }
    printf("%s -> ", host);
    print_ipv4(address);
    printf("\nPhase 24 dnslookup application: OK\n");
    return 0;
}
