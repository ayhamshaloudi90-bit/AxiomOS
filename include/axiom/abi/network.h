#ifndef AXIOM_ABI_NETWORK_H
#define AXIOM_ABI_NETWORK_H

#define AXIOM_NET_HOST_MAX 96u
#define AXIOM_NET_PATH_MAX 192u
#define AXIOM_NET_HTTP_USER_MAX 4096u
#define AXIOM_NET_HTTP_SERVER_BODY_MAX 1024u
#define AXIOM_NET_HTTP_SERVER_PORT 8080u

#ifndef __ASSEMBLER__
#include <stdint.h>

struct axiom_net_info {
    uint8_t mac[6];
    uint8_t reserved0[2];
    uint32_t address;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns_server;
    uint64_t tx_frames;
    uint64_t rx_frames;
    uint64_t arp_requests;
    uint64_t arp_replies;
    uint64_t ipv4_tx;
    uint64_t ipv4_rx;
};

struct axiom_ping_result {
    uint32_t address;
    uint32_t sequence;
    uint64_t poll_iterations;
};

struct axiom_http_result {
    uint32_t address;
    uint16_t port;
    uint16_t status_code;
    uint64_t body_bytes;
    uint32_t truncated;
    uint32_t reserved0;
};

struct axiom_http_server_result {
    uint32_t client_address;
    uint16_t client_port;
    uint16_t listen_port;
    uint64_t request_bytes;
    uint64_t response_bytes;
};
#endif

#endif
