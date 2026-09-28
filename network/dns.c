#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>

#include "internal.h"

#define DNS_PORT 53u
#define DNS_PACKET_MAX 512u
#define DNS_TYPE_A 1u
#define DNS_CLASS_IN 1u

struct dns_pending_state {
    uint16_t transaction;
    uint16_t source_port;
    uint32_t answer;
    int waiting;
    int done;
    int error;
};

static struct dns_pending_state pending;
static uint16_t next_transaction = 0x1801u;

static int dns_skip_name(const uint8_t *packet, size_t length, size_t *offset)
{
    size_t position;
    unsigned labels = 0u;

    if (packet == 0 || offset == 0 || *offset >= length) return 0;
    position = *offset;
    while (position < length && labels++ < 128u) {
        const uint8_t size = packet[position];
        if ((size & 0xC0u) == 0xC0u) {
            if (position + 1u >= length) return 0;
            *offset = position + 2u;
            return 1;
        }
        if (size == 0u) {
            *offset = position + 1u;
            return 1;
        }
        if ((size & 0xC0u) != 0u || size > 63u ||
            position + 1u + size > length) {
            return 0;
        }
        position += 1u + size;
    }
    return 0;
}

static int dns_encode_name(
    const char *name,
    uint8_t *output,
    size_t capacity,
    size_t *length_out
)
{
    size_t input = 0u;
    size_t output_index = 0u;
    size_t total_length;

    if (name == 0 || output == 0 || length_out == 0) return 0;
    total_length = net_text_length(name);
    if (total_length == 0u || total_length > 253u) return 0;

    while (input < total_length) {
        size_t label_start = input;
        size_t label_length;

        while (input < total_length && name[input] != '.') ++input;
        label_length = input - label_start;
        if (label_length == 0u || label_length > 63u ||
            output_index + 1u + label_length + 1u > capacity) {
            return 0;
        }
        output[output_index++] = (uint8_t)label_length;
        net_copy(&output[output_index], &name[label_start], label_length);
        output_index += label_length;
        if (input < total_length && name[input] == '.') ++input;
    }

    output[output_index++] = 0u;
    *length_out = output_index;
    return 1;
}

void dns_handle_udp(
    uint32_t source,
    uint16_t source_port,
    uint16_t destination_port,
    const uint8_t *payload,
    size_t length
)
{
    const struct net_config *config = net_config_internal();
    uint16_t flags;
    uint16_t questions;
    uint16_t answers;
    size_t offset;
    unsigned index;

    if (!pending.waiting || config == 0 || payload == 0 || length < 12u ||
        source != config->dns_server || source_port != DNS_PORT ||
        destination_port != pending.source_port ||
        net_read_be16(&payload[0]) != pending.transaction) {
        return;
    }

    flags = net_read_be16(&payload[2]);
    if ((flags & 0x8000u) == 0u) return;
    if ((flags & 0x000Fu) != 0u) {
        pending.error = -AXIOM_ENOENT;
        pending.done = 1;
        return;
    }

    questions = net_read_be16(&payload[4]);
    answers = net_read_be16(&payload[6]);
    offset = 12u;

    for (index = 0u; index < questions; ++index) {
        if (!dns_skip_name(payload, length, &offset) || offset + 4u > length) {
            pending.error = -AXIOM_EIO;
            pending.done = 1;
            return;
        }
        offset += 4u;
    }

    for (index = 0u; index < answers; ++index) {
        uint16_t type;
        uint16_t class_code;
        uint16_t data_length;

        if (!dns_skip_name(payload, length, &offset) || offset + 10u > length) {
            pending.error = -AXIOM_EIO;
            pending.done = 1;
            return;
        }
        type = net_read_be16(&payload[offset]);
        class_code = net_read_be16(&payload[offset + 2u]);
        data_length = net_read_be16(&payload[offset + 8u]);
        offset += 10u;
        if (offset + data_length > length) {
            pending.error = -AXIOM_EIO;
            pending.done = 1;
            return;
        }

        if (type == DNS_TYPE_A && class_code == DNS_CLASS_IN && data_length == 4u) {
            pending.answer = net_read_be32(&payload[offset]);
            pending.error = 0;
            pending.done = 1;
            ++net_stats_mutable()->dns_answers;
            return;
        }
        offset += data_length;
    }

    pending.error = -AXIOM_ENOENT;
    pending.done = 1;
}

int dns_resolve(const char *name, uint32_t *address_out)
{
    uint8_t packet[DNS_PACKET_MAX];
    size_t name_length;
    size_t length;
    uint32_t poll;
    const struct net_config *config = net_config_internal();
    struct net_stats *stats = net_stats_mutable();
    int result;

    if (name == 0 || address_out == 0 || config == 0) return -AXIOM_EINVAL;
    net_clear(packet, sizeof(packet));

    pending.transaction = next_transaction++;
    if (next_transaction == 0u) next_transaction = 1u;
    pending.source_port = net_next_ephemeral_port();
    pending.answer = 0u;
    pending.waiting = 1;
    pending.done = 0;
    pending.error = 0;

    net_write_be16(&packet[0], pending.transaction);
    net_write_be16(&packet[2], 0x0100u); /* recursion desired */
    net_write_be16(&packet[4], 1u);

    if (!dns_encode_name(name, &packet[12], sizeof(packet) - 16u, &name_length)) {
        pending.waiting = 0;
        return -AXIOM_EINVAL;
    }
    length = 12u + name_length;
    net_write_be16(&packet[length], DNS_TYPE_A);
    net_write_be16(&packet[length + 2u], DNS_CLASS_IN);
    length += 4u;

    result = udp_send(
        config->dns_server,
        pending.source_port,
        DNS_PORT,
        packet,
        length
    );
    if (result < 0) {
        pending.waiting = 0;
        return result;
    }
    ++stats->dns_queries;

    for (poll = 0u; poll < NET_POLL_BUDGET; ++poll) {
        (void)net_poll(8u);
        if (pending.done) {
            pending.waiting = 0;
            if (pending.error < 0) return pending.error;
            *address_out = pending.answer;
            return 0;
        }
        __asm__ volatile ("pause");
    }

    pending.waiting = 0;
    return -AXIOM_ETIMEDOUT;
}
