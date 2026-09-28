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
    long r = axiom_netinfo(&info);
    if (r < 0) {
        printf("ifconfig: error %ld\n", r);
        return 1;
    }

    printf("AxiomOS Phase 24 ifconfig\n");
    printf("eth0  MAC %X:%X:%X:%X:%X:%X\n",
        info.mac[0], info.mac[1], info.mac[2],
        info.mac[3], info.mac[4], info.mac[5]);
    printf("      inet "); print_ipv4(info.address);
    printf("  netmask "); print_ipv4(info.netmask); printf("\n");
    printf("      gateway "); print_ipv4(info.gateway);
    printf("  dns "); print_ipv4(info.dns_server); printf("\n");
    printf("      frames tx/rx %llu/%llu  ipv4 tx/rx %llu/%llu\n",
        (unsigned long long)info.tx_frames,
        (unsigned long long)info.rx_frames,
        (unsigned long long)info.ipv4_tx,
        (unsigned long long)info.ipv4_rx);
    printf("Phase 24 ifconfig application: OK\n");
    return 0;
}
