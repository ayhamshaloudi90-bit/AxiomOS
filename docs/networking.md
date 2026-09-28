# AxiomOS networking

The protocol stack is implemented as of Phase 18; Phase 24 adds standalone Ring-3 networking applications and passive-open HTTP serving.

## Current stack

```text
E1000 -> Ethernet -> ARP -> IPv4
                         |-> ICMP
                         |-> UDP -> DNS
                         `-> TCP -> HTTP/1.0
```

The implementation is under `drivers/net/` and `network/`. Public kernel-facing
interfaces are in `include/axiom/network/`, and the userspace ABI structures
are in `include/axiom/abi/network.h`.

Normal QEMU boots explicitly attach an Intel E1000 to QEMU user-mode networking.
AxiomOS currently uses the Phase-18 static QEMU topology `10.0.2.15/24`, gateway
`10.0.2.2`, and DNS `10.0.2.3`.

See [Phase 18](phase18.md) for protocol behavior, tests, and limitations.


## Phase 24 userspace applications

Phase 24 installs `/bin/ifconfig`, `/bin/ping`, `/bin/dnslookup`,
`/bin/httpget`, and `/bin/httpd`. The first four consume the existing high-level
network syscalls. `httpd` uses new syscall 53 (`httpserve`) and a minimal
passive-open TCP path to accept one inbound HTTP/1.0 request on port 8080.

Normal `make run` exposes guest port 8080 at host `127.0.0.1:18080`. The
Phase-24 automated test uses dynamic `guestfwd` and `hostfwd` ports so both
client and server traffic are deterministic. See [Phase 24](phase24.md).
