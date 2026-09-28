#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <axiom/syscalls.h>

int main(void)
{
    static const char body[] = "AXIOMOS_PHASE24_HTTP_SERVER_OK\n";
    struct axiom_http_server_result result;
    long r;

    printf("AxiomOS Phase 24 httpd listening on port %u for one request\n",
        (unsigned)AXIOM_NET_HTTP_SERVER_PORT);
    r = axiom_httpserve(
        AXIOM_NET_HTTP_SERVER_PORT,
        body,
        strlen(body),
        &result
    );
    if (r < 0) {
        printf("httpd: error %ld\n", r);
        return 1;
    }
    printf("served client %u.%u.%u.%u:%u request=%llu response=%llu\n",
        (unsigned)((result.client_address >> 24) & 0xFFu),
        (unsigned)((result.client_address >> 16) & 0xFFu),
        (unsigned)((result.client_address >> 8) & 0xFFu),
        (unsigned)(result.client_address & 0xFFu),
        (unsigned)result.client_port,
        (unsigned long long)result.request_bytes,
        (unsigned long long)result.response_bytes);
    puts("Phase 24 HTTP server application: OK");
    return 0;
}
