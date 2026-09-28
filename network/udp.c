#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>

#include "internal.h"

#define UDP_HEADER_BYTES 8u

int udp_send(
    uint32_t destination,
    uint16_t source_port,
    uint16_t destination_port,
    const void *payload,
    size_t payload_length
)
{
    uint8_t segment[NET_MTU - 20u];
    const struct net_config *config = net_config_internal();
    struct net_stats *stats = net_stats_mutable();
    const size_t length = UDP_HEADER_BYTES + payload_length;
    uint16_t checksum;
    int result;

    if (config == 0 || payload == 0 || length > sizeof(segment) ||
        length > UINT16_MAX) {
        return -AXIOM_EMSGSIZE;
    }

    net_clear(segment, UDP_HEADER_BYTES);
    net_write_be16(&segment[0], source_port);
    net_write_be16(&segment[2], destination_port);
    net_write_be16(&segment[4], (uint16_t)length);
    net_copy(&segment[UDP_HEADER_BYTES], payload, payload_length);

    checksum = net_transport_checksum(
        config->address, destination, IPV4_PROTOCOL_UDP, segment, length
    );
    if (checksum == 0u) checksum = 0xFFFFu;
    net_write_be16(&segment[6], checksum);

    result = ipv4_send(IPV4_PROTOCOL_UDP, destination, segment, length);
    if (result == 0) ++stats->udp_tx;
    return result;
}

void udp_handle(
    uint32_t source,
    uint32_t destination,
    const uint8_t *segment,
    size_t length
)
{
    struct net_stats *stats = net_stats_mutable();
    uint16_t source_port;
    uint16_t destination_port;
    uint16_t udp_length;
    uint16_t checksum;

    if (segment == 0 || length < UDP_HEADER_BYTES) return;
    source_port = net_read_be16(&segment[0]);
    destination_port = net_read_be16(&segment[2]);
    udp_length = net_read_be16(&segment[4]);
    checksum = net_read_be16(&segment[6]);
    if (udp_length < UDP_HEADER_BYTES || udp_length > length) return;

    if (checksum != 0u &&
        net_transport_checksum(
            source,
            destination,
            IPV4_PROTOCOL_UDP,
            segment,
            udp_length
        ) != 0u) {
        return;
    }

    ++stats->udp_rx;
    dns_handle_udp(
        source,
        source_port,
        destination_port,
        &segment[UDP_HEADER_BYTES],
        (size_t)udp_length - UDP_HEADER_BYTES
    );
}
