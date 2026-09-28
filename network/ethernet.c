#include <stddef.h>
#include <stdint.h>

#include <axiom/drivers/e1000.h>

#include "internal.h"

#define ETHERNET_HEADER_BYTES 14u

int ethernet_send(
    const uint8_t destination[6],
    uint16_t ether_type,
    const void *payload,
    size_t payload_length
)
{
    uint8_t frame[E1000_ETHERNET_FRAME_MAX];
    const struct net_config *config = net_config_internal();

    if (destination == 0 || payload == 0 || config == 0 ||
        payload_length + ETHERNET_HEADER_BYTES > sizeof(frame)) {
        return 0;
    }

    net_copy(&frame[0], destination, 6u);
    net_copy(&frame[6], config->mac, 6u);
    net_write_be16(&frame[12], ether_type);
    net_copy(&frame[14], payload, payload_length);
    return e1000_send(frame, payload_length + ETHERNET_HEADER_BYTES);
}

void ethernet_handle(const uint8_t *frame, size_t length)
{
    const struct net_config *config = net_config_internal();
    const uint16_t type = length >= ETHERNET_HEADER_BYTES ?
        net_read_be16(&frame[12]) : 0u;
    static const uint8_t broadcast[6] = {
        0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu
    };

    if (config == 0 || frame == 0 || length < ETHERNET_HEADER_BYTES) return;
    if (!net_bytes_equal(&frame[0], config->mac, 6u) &&
        !net_bytes_equal(&frame[0], broadcast, 6u)) {
        return;
    }

    if (type == ETHERTYPE_ARP) {
        arp_handle(&frame[ETHERNET_HEADER_BYTES], length - ETHERNET_HEADER_BYTES);
    } else if (type == ETHERTYPE_IPV4) {
        ipv4_handle(&frame[ETHERNET_HEADER_BYTES], length - ETHERNET_HEADER_BYTES);
    }
}
