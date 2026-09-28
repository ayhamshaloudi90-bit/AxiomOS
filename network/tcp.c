#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>
#include <axiom/terminal/kprintf.h>

#include "internal.h"

#define TCP_HEADER_BYTES 20u
#define TCP_FLAG_FIN 0x01u
#define TCP_FLAG_SYN 0x02u
#define TCP_FLAG_RST 0x04u
#define TCP_FLAG_PSH 0x08u
#define TCP_FLAG_ACK 0x10u

#define TCP_STATE_CLOSED 0u
#define TCP_STATE_SYN_SENT 1u
#define TCP_STATE_ESTABLISHED 2u
#define TCP_STATE_DONE 3u
#define TCP_STATE_ERROR 4u
#define TCP_STATE_LISTEN 5u
#define TCP_STATE_SYN_RECEIVED 6u

#define TCP_SERVER_REQUEST_MAX 1024u

struct tcp_connection {
    uint32_t remote_address;
    uint16_t remote_port;
    uint16_t local_port;
    uint32_t send_next;
    uint32_t receive_next;
    uint8_t state;
    uint8_t *receive_buffer;
    size_t receive_capacity;
    size_t receive_length;
    int truncated;
    int error;
};

static struct tcp_connection connection;
static uint8_t server_request_storage[TCP_SERVER_REQUEST_MAX];
static uint32_t initial_sequence = 0x18000000u;

static int send_segment(uint8_t flags, const void *payload, size_t payload_length)
{
    uint8_t segment[NET_MTU - 20u];
    const struct net_config *config = net_config_internal();
    struct net_stats *stats = net_stats_mutable();
    const size_t length = TCP_HEADER_BYTES + payload_length;
    uint32_t sequence;
    uint16_t checksum;
    int result;

    if (config == 0 || length > sizeof(segment)) return -AXIOM_EMSGSIZE;

    sequence = connection.send_next;
    net_clear(segment, TCP_HEADER_BYTES);
    net_write_be16(&segment[0], connection.local_port);
    net_write_be16(&segment[2], connection.remote_port);
    net_write_be32(&segment[4], sequence);
    net_write_be32(&segment[8], connection.receive_next);
    segment[12] = (uint8_t)(5u << 4);
    segment[13] = flags;
    net_write_be16(&segment[14], 65535u);
    if (payload_length != 0u) net_copy(&segment[20], payload, payload_length);

    checksum = net_transport_checksum(
        config->address,
        connection.remote_address,
        IPV4_PROTOCOL_TCP,
        segment,
        length
    );
    net_write_be16(&segment[16], checksum);

    result = ipv4_send(
        IPV4_PROTOCOL_TCP,
        connection.remote_address,
        segment,
        length
    );
    if (result < 0) return result;

    ++stats->tcp_tx;
    if ((flags & TCP_FLAG_SYN) != 0u) ++connection.send_next;
    if ((flags & TCP_FLAG_FIN) != 0u) ++connection.send_next;
    connection.send_next += (uint32_t)payload_length;
    return 0;
}


static int ascii_equal_ignore_case(uint8_t left, char right)
{
    if (left >= (uint8_t)'A' && left <= (uint8_t)'Z') {
        left = (uint8_t)(left - (uint8_t)'A' + (uint8_t)'a');
    }
    if (right >= 'A' && right <= 'Z') {
        right = (char)(right - 'A' + 'a');
    }
    return left == (uint8_t)right;
}

static int header_name_matches(
    const uint8_t *line,
    size_t line_length,
    const char *name
)
{
    size_t index = 0u;
    if (line == 0 || name == 0) return 0;
    while (name[index] != '\0') {
        if (index >= line_length ||
            !ascii_equal_ignore_case(line[index], name[index])) {
            return 0;
        }
        ++index;
    }
    return index < line_length && line[index] == (uint8_t)':';
}

/*
 * Return 1 once a complete HTTP response is already buffered.  Phase 18
 * deliberately supports responses framed by Content-Length; responses without
 * an explicit length still complete when the peer sends FIN.
 */
static int http_response_complete(const uint8_t *buffer, size_t length)
{
    size_t header_end = SIZE_MAX;
    size_t index;
    size_t content_length = 0u;
    int has_content_length = 0;

    if (buffer == 0 || length < 4u) return 0;

    for (index = 0u; index + 3u < length; ++index) {
        if (buffer[index] == (uint8_t)'\r' &&
            buffer[index + 1u] == (uint8_t)'\n' &&
            buffer[index + 2u] == (uint8_t)'\r' &&
            buffer[index + 3u] == (uint8_t)'\n') {
            header_end = index + 4u;
            break;
        }
    }
    if (header_end == SIZE_MAX) return 0;

    index = 0u;
    while (index + 1u < header_end) {
        size_t line_start = index;
        size_t line_end;
        size_t value;

        while (index + 1u < header_end &&
               !(buffer[index] == (uint8_t)'\r' &&
                 buffer[index + 1u] == (uint8_t)'\n')) {
            ++index;
        }
        line_end = index;
        index += 2u;

        if (line_end == line_start) break;
        if (!header_name_matches(
                &buffer[line_start],
                line_end - line_start,
                "Content-Length"
            )) {
            continue;
        }

        value = line_start;
        while (value < line_end && buffer[value] != (uint8_t)':') ++value;
        if (value >= line_end) continue;
        ++value;
        while (value < line_end &&
               (buffer[value] == (uint8_t)' ' || buffer[value] == (uint8_t)'\t')) {
            ++value;
        }
        if (value >= line_end) continue;

        content_length = 0u;
        has_content_length = 1;
        while (value < line_end) {
            const uint8_t digit = buffer[value++];
            if (digit < (uint8_t)'0' || digit > (uint8_t)'9') {
                has_content_length = 0;
                break;
            }
            if (content_length > (SIZE_MAX - 9u) / 10u) return 0;
            content_length = content_length * 10u + (size_t)(digit - (uint8_t)'0');
        }
        break;
    }

    return has_content_length &&
        content_length <= SIZE_MAX - header_end &&
        length >= header_end + content_length;
}

static int http_request_complete(const uint8_t *buffer, size_t length)
{
    size_t index;
    if (buffer == 0 || length < 4u) return 0;
    for (index = 0u; index + 3u < length; ++index) {
        if (buffer[index] == (uint8_t)'\r' &&
            buffer[index + 1u] == (uint8_t)'\n' &&
            buffer[index + 2u] == (uint8_t)'\r' &&
            buffer[index + 3u] == (uint8_t)'\n') {
            return 1;
        }
    }
    return 0;
}

static int wait_for_server_request(void)
{
    uint32_t poll;
    for (poll = 0u; poll < NET_POLL_BUDGET; ++poll) {
        (void)net_poll(16u);
        if (connection.state == TCP_STATE_ERROR) {
            return connection.error != 0 ? connection.error : -AXIOM_EIO;
        }
        if (connection.state == TCP_STATE_DONE) return -AXIOM_EIO;
        if (connection.state == TCP_STATE_ESTABLISHED &&
            http_request_complete(connection.receive_buffer, connection.receive_length)) {
            return 0;
        }
        __asm__ volatile ("pause");
    }
    return -AXIOM_ETIMEDOUT;
}

static int wait_for_http_completion(void)
{
    uint32_t poll;

    for (poll = 0u; poll < NET_POLL_BUDGET; ++poll) {
        (void)net_poll(16u);
        if (connection.state == TCP_STATE_DONE) return 0;
        if (connection.state == TCP_STATE_ERROR) {
            return connection.error != 0 ? connection.error : -AXIOM_EIO;
        }
        if (connection.state == TCP_STATE_ESTABLISHED &&
            http_response_complete(
                connection.receive_buffer,
                connection.receive_length
            )) {
            /*
             * The HTTP message is complete.  Do not depend on a translated
             * guestfwd/backend close being surfaced as a prompt peer FIN;
             * actively close our side and return the complete response.
             */
            (void)send_segment(TCP_FLAG_FIN | TCP_FLAG_ACK, 0, 0u);
            connection.state = TCP_STATE_DONE;
            return 0;
        }
        __asm__ volatile ("pause");
    }
    return -AXIOM_ETIMEDOUT;
}

static int wait_for_state(uint8_t wanted)
{
    uint32_t poll;

    for (poll = 0u; poll < NET_POLL_BUDGET; ++poll) {
        (void)net_poll(16u);
        if (connection.state == wanted) return 0;
        if (connection.state == TCP_STATE_ERROR) {
            return connection.error != 0 ? connection.error : -AXIOM_EIO;
        }
        __asm__ volatile ("pause");
    }
    return -AXIOM_ETIMEDOUT;
}

void tcp_handle(
    uint32_t source,
    uint32_t destination,
    const uint8_t *segment,
    size_t length
)
{
    const struct net_config *config = net_config_internal();
    struct net_stats *stats = net_stats_mutable();
    uint16_t source_port;
    uint16_t destination_port;
    size_t header_length;
    uint32_t sequence;
    uint32_t acknowledgment;
    uint8_t flags;
    const uint8_t *payload;
    size_t payload_length;

    if (segment == 0 || config == 0 || length < TCP_HEADER_BYTES ||
        connection.state == TCP_STATE_CLOSED || connection.state == TCP_STATE_DONE) {
        return;
    }

    source_port = net_read_be16(&segment[0]);
    destination_port = net_read_be16(&segment[2]);
    header_length = (size_t)(segment[12] >> 4) * 4u;
    if (header_length < TCP_HEADER_BYTES || header_length > length) return;
    if (net_transport_checksum(
            source,
            destination,
            IPV4_PROTOCOL_TCP,
            segment,
            length
        ) != 0u) {
        return;
    }

    sequence = net_read_be32(&segment[4]);
    acknowledgment = net_read_be32(&segment[8]);
    flags = segment[13];
    payload = &segment[header_length];
    payload_length = length - header_length;
    ++stats->tcp_rx;

    if (connection.state == TCP_STATE_LISTEN) {
        if (destination_port != connection.local_port ||
            (flags & TCP_FLAG_SYN) == 0u ||
            (flags & TCP_FLAG_ACK) != 0u) {
            return;
        }
        connection.remote_address = source;
        connection.remote_port = source_port;
        connection.receive_next = sequence + 1u;
        connection.state = TCP_STATE_SYN_RECEIVED;
        if (send_segment(TCP_FLAG_SYN | TCP_FLAG_ACK, 0, 0u) < 0) {
            connection.error = -AXIOM_EIO;
            connection.state = TCP_STATE_ERROR;
        }
        return;
    }

    if (source != connection.remote_address ||
        source_port != connection.remote_port ||
        destination_port != connection.local_port) {
        return;
    }

    if ((flags & TCP_FLAG_RST) != 0u) {
        connection.error = -AXIOM_ECONNREFUSED;
        connection.state = TCP_STATE_ERROR;
        return;
    }

    if (connection.state == TCP_STATE_SYN_SENT) {
        if ((flags & (TCP_FLAG_SYN | TCP_FLAG_ACK)) ==
                (TCP_FLAG_SYN | TCP_FLAG_ACK) &&
            acknowledgment == connection.send_next) {
            connection.receive_next = sequence + 1u;
            connection.state = TCP_STATE_ESTABLISHED;
            (void)send_segment(TCP_FLAG_ACK, 0, 0u);
        }
        return;
    }

    if (connection.state == TCP_STATE_SYN_RECEIVED) {
        if ((flags & TCP_FLAG_ACK) != 0u &&
            acknowledgment == connection.send_next) {
            connection.state = TCP_STATE_ESTABLISHED;
            /* The first HTTP bytes may be piggybacked on the final ACK. */
        } else {
            return;
        }
    }

    if (connection.state != TCP_STATE_ESTABLISHED) return;

    /* Ignore/ack duplicate or out-of-order data; Phase 24 still has no reassembly. */
    if ((payload_length != 0u || (flags & TCP_FLAG_FIN) != 0u) &&
        sequence != connection.receive_next) {
        (void)send_segment(TCP_FLAG_ACK, 0, 0u);
        return;
    }

    if (payload_length != 0u) {
        const size_t space = connection.receive_capacity > connection.receive_length ?
            connection.receive_capacity - connection.receive_length : 0u;
        const size_t copied = payload_length < space ? payload_length : space;

        if (copied != 0u) {
            net_copy(
                connection.receive_buffer + connection.receive_length,
                payload,
                copied
            );
            connection.receive_length += copied;
        }
        if (copied < payload_length) connection.truncated = 1;
        connection.receive_next += (uint32_t)payload_length;
        (void)send_segment(TCP_FLAG_ACK, 0, 0u);
    }

    if ((flags & TCP_FLAG_FIN) != 0u) {
        ++connection.receive_next;
        (void)send_segment(TCP_FLAG_ACK, 0, 0u);
        connection.state = TCP_STATE_DONE;
    }
}

int tcp_exchange_http(
    uint32_t address,
    uint16_t port,
    const void *request,
    size_t request_length,
    uint8_t *response,
    size_t response_capacity,
    size_t *response_length_out,
    int *truncated_out
)
{
    int result;

    if (address == 0u || port == 0u || request == 0 || request_length == 0u ||
        response == 0 || response_capacity == 0u ||
        response_length_out == 0 || truncated_out == 0) {
        return -AXIOM_EINVAL;
    }
    if (request_length + TCP_HEADER_BYTES > NET_MTU - 20u) {
        return -AXIOM_EMSGSIZE;
    }

    net_clear(&connection, sizeof(connection));
    connection.remote_address = address;
    connection.remote_port = port;
    connection.local_port = net_next_ephemeral_port();
    connection.send_next = initial_sequence;
    initial_sequence += 0x10000u;
    if (initial_sequence < 0x18000000u) initial_sequence = 0x18000000u;
    connection.receive_buffer = response;
    connection.receive_capacity = response_capacity;
    connection.state = TCP_STATE_SYN_SENT;

    result = send_segment(TCP_FLAG_SYN, 0, 0u);
    if (result < 0) {
        connection.state = TCP_STATE_CLOSED;
        return result;
    }

    result = wait_for_state(TCP_STATE_ESTABLISHED);
    if (result < 0) {
        if (result == -AXIOM_ETIMEDOUT) {
            const struct net_stats *stats = net_stats_mutable();
            kprintf(
                "Phase 18 TCP timeout: SYN handshake (tx/rx=%llu/%llu)\n",
                (unsigned long long)stats->tcp_tx,
                (unsigned long long)stats->tcp_rx
            );
        }
        connection.state = TCP_STATE_CLOSED;
        return result;
    }

    result = send_segment(TCP_FLAG_ACK | TCP_FLAG_PSH, request, request_length);
    if (result < 0) {
        connection.state = TCP_STATE_CLOSED;
        return result;
    }

    result = wait_for_http_completion();
    if (result < 0) {
        if (result == -AXIOM_ETIMEDOUT) {
            const struct net_stats *stats = net_stats_mutable();
            kprintf(
                "Phase 18 TCP timeout: HTTP response (tx/rx=%llu/%llu bytes=%llu)\n",
                (unsigned long long)stats->tcp_tx,
                (unsigned long long)stats->tcp_rx,
                (unsigned long long)connection.receive_length
            );
        }
        connection.state = TCP_STATE_CLOSED;
        return result;
    }

    *response_length_out = connection.receive_length;
    *truncated_out = connection.truncated;
    connection.state = TCP_STATE_CLOSED;
    return 0;
}

int tcp_serve_http_once(
    uint16_t port,
    const void *response,
    size_t response_length,
    struct axiom_http_server_result *result_out
)
{
    int result;

    if (port == 0u || response == 0 || response_length == 0u ||
        result_out == 0 || response_length + TCP_HEADER_BYTES > NET_MTU - 20u) {
        return -AXIOM_EINVAL;
    }

    net_clear(&connection, sizeof(connection));
    net_clear(server_request_storage, sizeof(server_request_storage));
    connection.local_port = port;
    connection.send_next = initial_sequence;
    initial_sequence += 0x10000u;
    if (initial_sequence < 0x18000000u) initial_sequence = 0x18000000u;
    connection.receive_buffer = server_request_storage;
    connection.receive_capacity = sizeof(server_request_storage);
    connection.state = TCP_STATE_LISTEN;

    result = wait_for_server_request();
    if (result < 0) {
        connection.state = TCP_STATE_CLOSED;
        return result;
    }

    result = send_segment(TCP_FLAG_ACK | TCP_FLAG_PSH, response, response_length);
    if (result < 0) {
        connection.state = TCP_STATE_CLOSED;
        return result;
    }
    result = send_segment(TCP_FLAG_FIN | TCP_FLAG_ACK, 0, 0u);
    if (result < 0) {
        connection.state = TCP_STATE_CLOSED;
        return result;
    }

    result_out->client_address = connection.remote_address;
    result_out->client_port = connection.remote_port;
    result_out->listen_port = port;
    result_out->request_bytes = connection.receive_length;
    result_out->response_bytes = response_length;
    connection.state = TCP_STATE_CLOSED;
    return 0;
}

