#ifndef AXIOM_NETWORK_NET_H
#define AXIOM_NETWORK_NET_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/network.h>

#define NET_MTU 1500u
#define NET_IPV4(a, b, c, d) \
    (((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | \
     ((uint32_t)(c) << 8) | (uint32_t)(d))

struct net_config {
    uint8_t mac[6];
    uint32_t address;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns_server;
};

struct net_stats {
    uint64_t arp_requests;
    uint64_t arp_replies;
    uint64_t ipv4_tx;
    uint64_t ipv4_rx;
    uint64_t icmp_tx;
    uint64_t icmp_rx;
    uint64_t udp_tx;
    uint64_t udp_rx;
    uint64_t tcp_tx;
    uint64_t tcp_rx;
    uint64_t dns_queries;
    uint64_t dns_answers;
    uint64_t http_requests;
    uint64_t http_responses;
};

/* Initialise Phase-18 networking. No NIC is a non-fatal unavailable result. */
int net_init(void);
int net_available(void);
const struct net_config *net_get_config(void);
struct net_stats net_get_stats(void);

int net_parse_ipv4(const char *text, uint32_t *address_out);
int net_resolve(const char *name, uint32_t *address_out);
int net_ping(const char *target, struct axiom_ping_result *result_out);
int net_http_get(
    const char *host_spec,
    const char *path,
    uint8_t *body,
    size_t capacity,
    struct axiom_http_result *result_out
);
int net_http_serve_once(
    uint16_t port,
    const uint8_t *body,
    size_t body_length,
    struct axiom_http_server_result *result_out
);

void net_get_abi_info(struct axiom_net_info *info_out);

#endif
