#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>

#include "internal.h"

#define HTTP_RESPONSE_MAX 8192u
#define HTTP_REQUEST_MAX 768u

static uint8_t response_storage[HTTP_RESPONSE_MAX];

static int append_text(char *buffer, size_t capacity, size_t *used, const char *text)
{
    size_t index;
    size_t length;
    if (buffer == 0 || used == 0 || text == 0) return 0;
    length = net_text_length(text);
    if (*used + length >= capacity) return 0;
    for (index = 0u; index < length; ++index) buffer[(*used)++] = text[index];
    buffer[*used] = '\0';
    return 1;
}

static int parse_port(const char *text, uint16_t *port_out)
{
    uint32_t value = 0u;
    size_t index = 0u;
    if (text == 0 || port_out == 0 || text[0] == '\0') return 0;
    while (text[index] != '\0') {
        if (text[index] < '0' || text[index] > '9') return 0;
        value = value * 10u + (uint32_t)(text[index] - '0');
        if (value > 65535u) return 0;
        ++index;
    }
    if (value == 0u) return 0;
    *port_out = (uint16_t)value;
    return 1;
}

static int split_host_port(
    const char *host_spec,
    char *host,
    size_t host_capacity,
    uint16_t *port_out
)
{
    size_t length;
    size_t colon = SIZE_MAX;
    size_t index;

    if (host_spec == 0 || host == 0 || host_capacity < 2u || port_out == 0) return 0;
    length = net_text_length(host_spec);
    if (length == 0u || length >= host_capacity) return 0;

    for (index = 0u; index < length; ++index) {
        if (host_spec[index] == ':') colon = index;
    }

    *port_out = 80u;
    if (colon != SIZE_MAX) {
        if (colon == 0u || colon + 1u >= length || colon >= host_capacity) return 0;
        for (index = 0u; index < colon; ++index) host[index] = host_spec[index];
        host[colon] = '\0';
        if (!parse_port(&host_spec[colon + 1u], port_out)) return 0;
        return 1;
    }

    for (index = 0u; index < length; ++index) host[index] = host_spec[index];
    host[length] = '\0';
    return 1;
}

static int parse_status(const uint8_t *response, size_t length, uint16_t *status_out)
{
    size_t index;
    if (response == 0 || status_out == 0 || length < 12u) return 0;
    if (response[0] != 'H' || response[1] != 'T' || response[2] != 'T' ||
        response[3] != 'P' || response[4] != '/') {
        return 0;
    }
    for (index = 5u; index + 3u < length && index < 16u; ++index) {
        if (response[index] == ' ' &&
            response[index + 1u] >= '0' && response[index + 1u] <= '9' &&
            response[index + 2u] >= '0' && response[index + 2u] <= '9' &&
            response[index + 3u] >= '0' && response[index + 3u] <= '9') {
            *status_out = (uint16_t)(
                (response[index + 1u] - '0') * 100u +
                (response[index + 2u] - '0') * 10u +
                (response[index + 3u] - '0')
            );
            return 1;
        }
    }
    return 0;
}

static int find_body(const uint8_t *response, size_t length, size_t *offset_out)
{
    size_t index;
    if (response == 0 || offset_out == 0) return 0;
    for (index = 0u; index + 3u < length; ++index) {
        if (response[index] == '\r' && response[index + 1u] == '\n' &&
            response[index + 2u] == '\r' && response[index + 3u] == '\n') {
            *offset_out = index + 4u;
            return 1;
        }
    }
    return 0;
}

int http_get(
    const char *host_spec,
    const char *path,
    uint8_t *body,
    size_t capacity,
    struct axiom_http_result *result_out
)
{
    char host[AXIOM_NET_HOST_MAX];
    char request[HTTP_REQUEST_MAX];
    uint16_t port;
    uint32_t address;
    size_t request_length = 0u;
    size_t response_length = 0u;
    size_t body_offset;
    size_t body_length;
    size_t copied;
    uint16_t status;
    int truncated = 0;
    int result;

    if (host_spec == 0 || path == 0 || body == 0 || capacity == 0u ||
        result_out == 0) {
        return -AXIOM_EINVAL;
    }
    if (!split_host_port(host_spec, host, sizeof(host), &port)) return -AXIOM_EINVAL;
    if (path[0] != '/') return -AXIOM_EINVAL;

    result = net_resolve(host, &address);
    if (result < 0) return result;

    request[0] = '\0';
    if (!append_text(request, sizeof(request), &request_length, "GET ") ||
        !append_text(request, sizeof(request), &request_length, path) ||
        !append_text(request, sizeof(request), &request_length, " HTTP/1.0\r\nHost: ") ||
        !append_text(request, sizeof(request), &request_length, host_spec) ||
        !append_text(request, sizeof(request), &request_length,
                     "\r\nUser-Agent: AxiomOS/0.18\r\nConnection: close\r\n\r\n")) {
        return -AXIOM_ENAMETOOLONG;
    }

    net_clear(response_storage, sizeof(response_storage));
    result = tcp_exchange_http(
        address,
        port,
        request,
        request_length,
        response_storage,
        sizeof(response_storage),
        &response_length,
        &truncated
    );
    if (result < 0) return result;

    if (!parse_status(response_storage, response_length, &status) ||
        !find_body(response_storage, response_length, &body_offset) ||
        body_offset > response_length) {
        return -AXIOM_EIO;
    }

    body_length = response_length - body_offset;
    copied = body_length < capacity ? body_length : capacity;
    net_copy(body, &response_storage[body_offset], copied);

    result_out->address = address;
    result_out->port = port;
    result_out->status_code = status;
    result_out->body_bytes = copied;
    result_out->truncated = (uint32_t)(truncated || copied < body_length);
    result_out->reserved0 = 0u;
    ++net_stats_mutable()->http_requests;
    return 0;
}

int http_serve_once(
    uint16_t port,
    const uint8_t *body,
    size_t body_length,
    struct axiom_http_server_result *result_out
)
{
    static const char header[] =
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "Server: AxiomOS/0.24\r\n"
        "\r\n";
    uint8_t response[128u + AXIOM_NET_HTTP_SERVER_BODY_MAX];
    const size_t header_length = sizeof(header) - 1u;
    int result;

    if (port == 0u || body == 0 || body_length == 0u ||
        body_length > AXIOM_NET_HTTP_SERVER_BODY_MAX || result_out == 0) {
        return -AXIOM_EINVAL;
    }
    if (header_length + body_length > sizeof(response)) {
        return -AXIOM_EMSGSIZE;
    }

    net_copy(response, header, header_length);
    net_copy(response + header_length, body, body_length);
    result = tcp_serve_http_once(
        port,
        response,
        header_length + body_length,
        result_out
    );
    if (result == 0) {
        ++net_stats_mutable()->http_responses;
    }
    return result;
}
