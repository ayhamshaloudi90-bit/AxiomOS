#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>

#include "internal.h"

#define IPV4_HEADER_BYTES 20u

int ipv4_send(
    uint8_t protocol,
    uint32_t destination,
    const void *payload,
    size_t payload_length
)
{
    uint8_t packet[NET_MTU];
    uint8_t destination_mac[6];
    const struct net_config *config = net_config_internal();
    struct net_stats *stats = net_stats_mutable();
    const size_t total_length = IPV4_HEADER_BYTES + payload_length;
    int result;

    if (config == 0 || payload == 0 || total_length > sizeof(packet) ||
        total_length > UINT16_MAX) {
        return -AXIOM_EINVAL;
    }

    result = arp_resolve(destination, destination_mac);
    if (result < 0) return result;

    net_clear(packet, IPV4_HEADER_BYTES);
    packet[0] = 0x45u;
    packet[1] = 0u;
    net_write_be16(&packet[2], (uint16_t)total_length);
    net_write_be16(&packet[4], net_next_ipv4_id());
    net_write_be16(&packet[6], 0x4000u); /* Don't Fragment. */
    packet[8] = 64u;
    packet[9] = protocol;
    net_write_be32(&packet[12], config->address);
    net_write_be32(&packet[16], destination);
    net_write_be16(&packet[10], net_checksum(packet, IPV4_HEADER_BYTES));
    net_copy(&packet[IPV4_HEADER_BYTES], payload, payload_length);

    if (!ethernet_send(destination_mac, ETHERTYPE_IPV4, packet, total_length)) {
        return -AXIOM_EIO;
    }
    ++stats->ipv4_tx;
    return 0;
}

void ipv4_handle(const uint8_t *packet, size_t length)
{
    const struct net_config *config = net_config_internal();
    struct net_stats *stats = net_stats_mutable();
    size_t header_length;
    uint16_t total_length;
    uint16_t fragment;
    uint32_t source;
    uint32_t destination;
    const uint8_t *payload;
    size_t payload_length;

    if (packet == 0 || config == 0 || length < IPV4_HEADER_BYTES) return;
    if ((packet[0] >> 4) != 4u) return;
    header_length = (size_t)(packet[0] & 0x0Fu) * 4u;
    if (header_length < IPV4_HEADER_BYTES || header_length > length) return;

    total_length = net_read_be16(&packet[2]);
    if (total_length < header_length || total_length > length) return;
    if (net_checksum(packet, header_length) != 0u) return;

    fragment = net_read_be16(&packet[6]);
    if ((fragment & 0x3FFFu) != 0u) return;

    source = net_read_be32(&packet[12]);
    destination = net_read_be32(&packet[16]);
    if (destination != config->address && destination != 0xFFFFFFFFu) return;

    payload = &packet[header_length];
    payload_length = (size_t)total_length - header_length;
    ++stats->ipv4_rx;

    if (packet[9] == IPV4_PROTOCOL_ICMP) {
        icmp_handle(source, destination, payload, payload_length);
    } else if (packet[9] == IPV4_PROTOCOL_UDP) {
        udp_handle(source, destination, payload, payload_length);
    } else if (packet[9] == IPV4_PROTOCOL_TCP) {
        tcp_handle(source, destination, payload, payload_length);
    }
}
