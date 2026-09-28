#ifndef AXIOM_NETWORK_INTERNAL_H
#define AXIOM_NETWORK_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/network/net.h>

#define ETHERTYPE_IPV4 0x0800u
#define ETHERTYPE_ARP  0x0806u

#define IPV4_PROTOCOL_ICMP 1u
#define IPV4_PROTOCOL_TCP  6u
#define IPV4_PROTOCOL_UDP  17u

#define NET_POLL_BUDGET 12000000u
#define NET_ARP_ATTEMPTS 3u

uint16_t net_read_be16(const uint8_t *bytes);
uint32_t net_read_be32(const uint8_t *bytes);
void net_write_be16(uint8_t *bytes, uint16_t value);
void net_write_be32(uint8_t *bytes, uint32_t value);
uint16_t net_checksum(const void *data, size_t length);
uint16_t net_transport_checksum(
    uint32_t source,
    uint32_t destination,
    uint8_t protocol,
    const void *segment,
    size_t length
);
void net_copy(void *destination, const void *source, size_t count);
void net_clear(void *destination, size_t count);
int net_bytes_equal(const void *left, const void *right, size_t count);
size_t net_text_length(const char *text);
int net_text_equal(const char *left, const char *right);

const struct net_config *net_config_internal(void);
struct net_stats *net_stats_mutable(void);
uint16_t net_next_ipv4_id(void);
uint16_t net_next_ephemeral_port(void);

int ethernet_send(
    const uint8_t destination[6],
    uint16_t ether_type,
    const void *payload,
    size_t payload_length
);
void ethernet_handle(const uint8_t *frame, size_t length);

int arp_resolve(uint32_t address, uint8_t mac_out[6]);
void arp_handle(const uint8_t *packet, size_t length);

int ipv4_send(
    uint8_t protocol,
    uint32_t destination,
    const void *payload,
    size_t payload_length
);
void ipv4_handle(const uint8_t *packet, size_t length);

void icmp_handle(uint32_t source, uint32_t destination, const uint8_t *packet, size_t length);
int icmp_ping(uint32_t address, struct axiom_ping_result *result_out);

int udp_send(
    uint32_t destination,
    uint16_t source_port,
    uint16_t destination_port,
    const void *payload,
    size_t payload_length
);
void udp_handle(uint32_t source, uint32_t destination, const uint8_t *segment, size_t length);

void dns_handle_udp(
    uint32_t source,
    uint16_t source_port,
    uint16_t destination_port,
    const uint8_t *payload,
    size_t length
);
int dns_resolve(const char *name, uint32_t *address_out);

void tcp_handle(uint32_t source, uint32_t destination, const uint8_t *segment, size_t length);
int tcp_exchange_http(
    uint32_t address,
    uint16_t port,
    const void *request,
    size_t request_length,
    uint8_t *response,
    size_t response_capacity,
    size_t *response_length_out,
    int *truncated_out
);
int tcp_serve_http_once(
    uint16_t port,
    const void *response,
    size_t response_length,
    struct axiom_http_server_result *result_out
);

int http_get(
    const char *host_spec,
    const char *path,
    uint8_t *body,
    size_t capacity,
    struct axiom_http_result *result_out
);
int http_serve_once(
    uint16_t port,
    const uint8_t *body,
    size_t body_length,
    struct axiom_http_server_result *result_out
);

/* Poll one or more received frames. Returns the number processed. */
unsigned net_poll(unsigned frame_budget);

#endif
