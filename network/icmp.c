#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>

#include "internal.h"

#define ICMP_ECHO_REPLY 0u
#define ICMP_ECHO_REQUEST 8u
#define ICMP_HEADER_BYTES 8u
#define ICMP_IDENTIFIER 0xA118u

struct ping_state {
    uint32_t address;
    uint16_t identifier;
    uint16_t sequence;
    int waiting;
    int received;
};

static struct ping_state ping_state;
static uint16_t next_sequence = 1u;

static int send_echo(
    uint32_t destination,
    uint8_t type,
    uint16_t identifier,
    uint16_t sequence,
    const uint8_t *data,
    size_t data_length
)
{
    uint8_t packet[64];
    struct net_stats *stats = net_stats_mutable();
    const size_t length = ICMP_HEADER_BYTES + data_length;
    int result;

    if (length > sizeof(packet)) return -AXIOM_EMSGSIZE;
    net_clear(packet, length);
    packet[0] = type;
    packet[1] = 0u;
    net_write_be16(&packet[4], identifier);
    net_write_be16(&packet[6], sequence);
    if (data_length != 0u) net_copy(&packet[8], data, data_length);
    net_write_be16(&packet[2], net_checksum(packet, length));

    result = ipv4_send(IPV4_PROTOCOL_ICMP, destination, packet, length);
    if (result == 0) ++stats->icmp_tx;
    return result;
}

void icmp_handle(
    uint32_t source,
    uint32_t destination,
    const uint8_t *packet,
    size_t length
)
{
    struct net_stats *stats = net_stats_mutable();
    uint16_t identifier;
    uint16_t sequence;
    (void)destination;

    if (packet == 0 || length < ICMP_HEADER_BYTES ||
        net_checksum(packet, length) != 0u) {
        return;
    }

    ++stats->icmp_rx;
    identifier = net_read_be16(&packet[4]);
    sequence = net_read_be16(&packet[6]);

    if (packet[0] == ICMP_ECHO_REPLY && ping_state.waiting &&
        source == ping_state.address &&
        identifier == ping_state.identifier &&
        sequence == ping_state.sequence) {
        ping_state.received = 1;
        return;
    }

    if (packet[0] == ICMP_ECHO_REQUEST) {
        (void)send_echo(
            source,
            ICMP_ECHO_REPLY,
            identifier,
            sequence,
            &packet[8],
            length - ICMP_HEADER_BYTES
        );
    }
}

int icmp_ping(uint32_t address, struct axiom_ping_result *result_out)
{
    static const uint8_t payload[] = {
        'A','x','i','o','m','O','S','-','P','h','a','s','e','1','8'
    };
    uint32_t poll;
    int result;

    if (result_out == 0 || address == 0u) return -AXIOM_EINVAL;

    ping_state.address = address;
    ping_state.identifier = ICMP_IDENTIFIER;
    ping_state.sequence = next_sequence++;
    if (next_sequence == 0u) next_sequence = 1u;
    ping_state.waiting = 1;
    ping_state.received = 0;

    result = send_echo(
        address,
        ICMP_ECHO_REQUEST,
        ping_state.identifier,
        ping_state.sequence,
        payload,
        sizeof(payload)
    );
    if (result < 0) {
        ping_state.waiting = 0;
        return result;
    }

    for (poll = 0u; poll < NET_POLL_BUDGET; ++poll) {
        (void)net_poll(8u);
        if (ping_state.received) {
            result_out->address = address;
            result_out->sequence = ping_state.sequence;
            result_out->poll_iterations = poll + 1u;
            ping_state.waiting = 0;
            return 0;
        }
        __asm__ volatile ("pause");
    }

    ping_state.waiting = 0;
    return -AXIOM_ETIMEDOUT;
}
