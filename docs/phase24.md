# Phase 24 — Networking applications

## Goal

Move the Phase-18 networking stack from shell-only demonstrations into reusable
Ring-3 applications, and prove that AxiomOS can act as both an HTTP client and a
small HTTP server.

Phase 24 installs these executable programs:

```text
/bin/ifconfig
/bin/ping
/bin/dnslookup
/bin/httpget
/bin/httpd
```

The existing Phase-18 shell builtins remain available so historical tests and
argument-driven commands such as `ping <host>` keep working.

## Applications

### `ifconfig`

Reports the E1000 MAC address, static IPv4 address, netmask, gateway, DNS server,
and basic frame/IP counters through the existing `netinfo` syscall.

### `ping`

Performs a userspace ICMP reachability test. Because AxiomOS does not yet pass
`argc/argv` into newly spawned ELF programs, `/bin/ping` uses the configured
QEMU gateway as its deterministic default target. The shell builtin still
supports arbitrary `ping <host>` targets.

### `dnslookup`

Resolves `example.com` through the configured DNS server using the existing
UDP/DNS stack and prints the returned IPv4 A record.

### `httpget`

Uses the existing HTTP/1.0 client syscall against the deterministic Phase-24
test peer at `10.0.2.100/phase24`. The shell builtin remains the general
host/path interface.

### `httpd`

Listens on TCP port 8080, accepts one connection, serves a fixed text/plain
HTTP/1.0 response, and exits. One-shot behavior is intentional: it proves the
passive-open/server path without pretending the current single-connection TCP
implementation is a production socket stack.

Normal `make run` forwards host `127.0.0.1:18080` to guest port 8080, so a
manual server demonstration is:

```text
axiom> httpd
```

and on the host:

```bash
curl http://127.0.0.1:18080/
```

## Passive TCP extension

Phase 18 only initiated outbound TCP connections. Phase 24 adds a bounded
single-listener state path:

```text
LISTEN -> SYN_RECEIVED -> ESTABLISHED -> response -> FIN -> CLOSED
```

The implementation still uses one global synchronous TCP connection object and
polls the E1000 receive ring. It validates the incoming TCP checksum, matches the
listening port, tracks peer sequence/acknowledgment numbers, buffers at most 1024
request bytes, and considers an HTTP request ready after `\r\n\r\n`.

This is enough for a real inbound HTTP request through QEMU `hostfwd`, while
keeping Phase 18's deliberately small TCP scope intact.

## New syscall

Phase 24 adds syscall 53:

```text
53 httpserve
```

`httpserve(port, body, length, result)` copies a bounded response body from
validated Ring-3 memory, blocks synchronously until one client request arrives,
serves HTTP/1.0 status 200, and copies peer/request statistics back to userspace.
The body is capped at 1024 bytes and the generated response fits in one TCP
segment.

`libaxiom.a` exposes:

```c
axiom_httpserve()
```

## Testing

```bash
make test-phase24
```

The automated test boots QEMU with:

- E1000 user networking;
- `guestfwd` to a deterministic host HTTP server for `/bin/httpget`;
- `hostfwd` from a temporary host port to guest port 8080 for `/bin/httpd`.

It then runs all five Ring-3 programs and verifies both outbound and inbound
HTTP traffic.

Expected success markers include:

```text
AxiomOS Phase 24 networking applications online.
Phase 24 ifconfig application: OK
Phase 24 ping application: OK
Phase 24 dnslookup application: OK
Phase 24 httpget application: OK
Phase 24 HTTP server application: OK
```

Afterward run the full regression suite:

```bash
make test
```

## Deliberate limits

- IPv4 only; no IPv6 or DHCP.
- E1000 only, still polled rather than interrupt-driven.
- No POSIX/BSD sockets ABI yet.
- One active TCP connection/listener at a time.
- No TCP retransmission, congestion control, receive reordering, or general
  multi-segment server responses.
- HTTP is plaintext HTTP/1.0 only; no TLS/HTTPS.
- `/bin/ping`, `/bin/dnslookup`, and `/bin/httpget` use deterministic defaults
  because process `argv` support has not been added yet; the interactive shell
  retains argument-aware Phase-18 builtins.
- `httpd` is deliberately one-shot rather than a daemon.

Those limits are explicit so Phase 24 demonstrates real network applications
without claiming a mature sockets/network-service environment.
