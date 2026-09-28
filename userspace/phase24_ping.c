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
    struct axiom_net_info info;
    struct axiom_ping_result result;
    char target[32];

    if (axiom_netinfo(&info) < 0) {
        puts("ping: network unavailable");
        return 1;
    }
    snprintf(target, sizeof(target), "%u.%u.%u.%u",
        (unsigned)((info.gateway >> 24) & 0xFFu),
        (unsigned)((info.gateway >> 16) & 0xFFu),
        (unsigned)((info.gateway >> 8) & 0xFFu),
        (unsigned)(info.gateway & 0xFFu));

    printf("AxiomOS Phase 24 ping: %s\n", target);
    if (axiom_ping(target, &result) < 0) {
        puts("ping: request failed");
        return 2;
    }
    printf("reply from "); print_ipv4(result.address);
    printf(": icmp_seq=%u polls=%llu\n",
        (unsigned)result.sequence,
        (unsigned long long)result.poll_iterations);
    puts("Phase 24 ping application: OK");
    return 0;
}
