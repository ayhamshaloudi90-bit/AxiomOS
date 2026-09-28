# Phase 18 TCP/HTTP runtime fix

The first Phase 18 package passed E1000, Ethernet, ARP, IPv4, ICMP, UDP and DNS
on the Steam Deck/QEMU acceptance test, but the deterministic `httpget`
connection returned `-110` (`ETIMEDOUT`).

The original TCP helper waited for `TCP_STATE_DONE`, which meant it required a
peer FIN before returning the response. That is unnecessarily strict for an
HTTP client and makes correctness depend on when QEMU/libslirp propagates the
close of the host-side `guestfwd` backend.

This fix makes `tcp_exchange_http()` recognize a complete HTTP message once:

1. the HTTP header terminator (`\r\n\r\n`) has arrived,
2. a valid `Content-Length` header is present, and
3. all declared body bytes have arrived.

At that point AxiomOS actively sends `FIN|ACK` and returns the complete response.
Responses without `Content-Length` still use peer FIN framing, which preserves
the intentionally small Phase 18 HTTP/1.0 design.

The acceptance HTTP server now also explicitly sends `Connection: close` and
sets `close_connection = True`.

If TCP still times out, the kernel now prints a stage-specific diagnostic:

- `Phase 18 TCP timeout: SYN handshake ...`
- `Phase 18 TCP timeout: HTTP response ...`

The latter also reports the number of HTTP bytes already buffered. This keeps a
future transport problem distinguishable from an HTTP-close/framing problem.

No lower networking subsystem was changed by this fix.
