#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>
#include <axiom/drivers/e1000.h>
#include <axiom/network/net.h>

#include "internal.h"

static struct net_config config;
static struct net_stats stats;
static uint16_t ipv4_id = 1u;
static uint16_t ephemeral_port = 49152u;
static int initialized;
static int available;

uint16_t net_read_be16(const uint8_t *bytes)
{
    return (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
}

uint32_t net_read_be32(const uint8_t *bytes)
{
    return ((uint32_t)bytes[0] << 24) |
        ((uint32_t)bytes[1] << 16) |
        ((uint32_t)bytes[2] << 8) |
        (uint32_t)bytes[3];
}

void net_write_be16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)(value >> 8);
    bytes[1] = (uint8_t)value;
}

void net_write_be32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)(value >> 24);
    bytes[1] = (uint8_t)(value >> 16);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)value;
}

void net_copy(void *destination, const void *source, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    size_t index;
    for (index = 0u; index < count; ++index) out[index] = in[index];
}

void net_clear(void *destination, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    size_t index;
    for (index = 0u; index < count; ++index) out[index] = 0u;
}

int net_bytes_equal(const void *left, const void *right, size_t count)
{
    const uint8_t *lhs = (const uint8_t *)left;
    const uint8_t *rhs = (const uint8_t *)right;
    size_t index;
    for (index = 0u; index < count; ++index) {
        if (lhs[index] != rhs[index]) return 0;
    }
    return 1;
}

size_t net_text_length(const char *text)
{
    size_t length = 0u;
    if (text == 0) return 0u;
    while (text[length] != '\0') ++length;
    return length;
}

int net_text_equal(const char *left, const char *right)
{
    size_t index = 0u;
    if (left == 0 || right == 0) return 0;
    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] != right[index]) return 0;
        ++index;
    }
    return left[index] == right[index];
}

static uint32_t checksum_accumulate(uint32_t sum, const uint8_t *bytes, size_t length)
{
    while (length >= 2u) {
        sum += (uint32_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
        bytes += 2;
        length -= 2u;
    }
    if (length != 0u) sum += (uint32_t)((uint16_t)bytes[0] << 8);
    return sum;
}

static uint16_t checksum_finish(uint32_t sum)
{
    while ((sum >> 16) != 0u) sum = (sum & 0xFFFFu) + (sum >> 16);
    return (uint16_t)~sum;
}

uint16_t net_checksum(const void *data, size_t length)
{
    return checksum_finish(checksum_accumulate(0u, (const uint8_t *)data, length));
}

uint16_t net_transport_checksum(
    uint32_t source,
    uint32_t destination,
    uint8_t protocol,
    const void *segment,
    size_t length
)
{
    uint8_t pseudo[12];
    uint32_t sum;

    net_write_be32(&pseudo[0], source);
    net_write_be32(&pseudo[4], destination);
    pseudo[8] = 0u;
    pseudo[9] = protocol;
    net_write_be16(&pseudo[10], (uint16_t)length);

    sum = checksum_accumulate(0u, pseudo, sizeof(pseudo));
    sum = checksum_accumulate(sum, (const uint8_t *)segment, length);
    return checksum_finish(sum);
}

const struct net_config *net_config_internal(void)
{
    return &config;
}

struct net_stats *net_stats_mutable(void)
{
    return &stats;
}

uint16_t net_next_ipv4_id(void)
{
    const uint16_t result = ipv4_id++;
    if (ipv4_id == 0u) ipv4_id = 1u;
    return result;
}

uint16_t net_next_ephemeral_port(void)
{
    const uint16_t result = ephemeral_port++;
    if (ephemeral_port < 49152u || ephemeral_port == 0u) ephemeral_port = 49152u;
    return result;
}

unsigned net_poll(unsigned frame_budget)
{
    uint8_t frame[E1000_ETHERNET_FRAME_MAX];
    unsigned processed = 0u;

    if (!available) return 0u;
    while (processed < frame_budget) {
        size_t length = 0u;
        if (!e1000_receive(frame, sizeof(frame), &length)) break;
        ethernet_handle(frame, length);
        ++processed;
    }
    return processed;
}

int net_init(void)
{
    if (initialized) return available;
    initialized = 1;

    if (!e1000_init()) return 0;

    e1000_mac(config.mac);
    /*
     * Phase 18 uses QEMU user networking's documented default IPv4 topology.
     * DHCP is intentionally deferred; the NIC and protocol stack are the focus
     * of this phase and the address remains a normal configurable net_config.
     */
    config.address = NET_IPV4(10, 0, 2, 15);
    config.netmask = NET_IPV4(255, 255, 255, 0);
    config.gateway = NET_IPV4(10, 0, 2, 2);
    config.dns_server = NET_IPV4(10, 0, 2, 3);
    available = 1;
    return 1;
}

int net_available(void)
{
    return available;
}

const struct net_config *net_get_config(void)
{
    return available ? &config : 0;
}

struct net_stats net_get_stats(void)
{
    return stats;
}

int net_parse_ipv4(const char *text, uint32_t *address_out)
{
    uint32_t parts[4];
    size_t part = 0u;
    size_t index = 0u;

    if (text == 0 || address_out == 0 || text[0] == '\0') return 0;
    parts[0] = parts[1] = parts[2] = parts[3] = 0u;

    while (part < 4u) {
        unsigned digits = 0u;
        uint32_t value = 0u;

        while (text[index] >= '0' && text[index] <= '9') {
            value = value * 10u + (uint32_t)(text[index] - '0');
            if (value > 255u || ++digits > 3u) return 0;
            ++index;
        }
        if (digits == 0u) return 0;
        parts[part++] = value;

        if (part == 4u) {
            if (text[index] != '\0') return 0;
        } else {
            if (text[index] != '.') return 0;
            ++index;
        }
    }

    *address_out = NET_IPV4(parts[0], parts[1], parts[2], parts[3]);
    return 1;
}

int net_resolve(const char *name, uint32_t *address_out)
{
    if (!available) return -AXIOM_ENETDOWN;
    if (name == 0 || address_out == 0) return -AXIOM_EINVAL;
    if (net_parse_ipv4(name, address_out)) return 0;
    return dns_resolve(name, address_out);
}

int net_ping(const char *target, struct axiom_ping_result *result_out)
{
    uint32_t address;
    int result;

    if (!available) return -AXIOM_ENETDOWN;
    if (target == 0 || result_out == 0) return -AXIOM_EINVAL;
    result = net_resolve(target, &address);
    if (result < 0) return result;
    return icmp_ping(address, result_out);
}

int net_http_get(
    const char *host_spec,
    const char *path,
    uint8_t *body,
    size_t capacity,
    struct axiom_http_result *result_out
)
{
    if (!available) return -AXIOM_ENETDOWN;
    return http_get(host_spec, path, body, capacity, result_out);
}

int net_http_serve_once(
    uint16_t port,
    const uint8_t *body,
    size_t body_length,
    struct axiom_http_server_result *result_out
)
{
    if (!available) return -AXIOM_ENETDOWN;
    return http_serve_once(port, body, body_length, result_out);
}

void net_get_abi_info(struct axiom_net_info *info_out)
{
    struct e1000_stats driver;
    unsigned index;

    if (info_out == 0) return;
    net_clear(info_out, sizeof(*info_out));
    if (!available) return;

    for (index = 0u; index < 6u; ++index) info_out->mac[index] = config.mac[index];
    info_out->address = config.address;
    info_out->netmask = config.netmask;
    info_out->gateway = config.gateway;
    info_out->dns_server = config.dns_server;
    driver = e1000_get_stats();
    info_out->tx_frames = driver.tx_frames;
    info_out->rx_frames = driver.rx_frames;
    info_out->arp_requests = stats.arp_requests;
    info_out->arp_replies = stats.arp_replies;
    info_out->ipv4_tx = stats.ipv4_tx;
    info_out->ipv4_rx = stats.ipv4_rx;
}
