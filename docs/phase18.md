# Phase 18 — Networking

Phase 18 gives AxiomOS its first real network stack and its first ability to
exchange packets with systems outside the guest.

## Architecture

```text
Ring-3 axiomsh
   |
   | netinfo / ping / dns / httpget syscalls
   v
network services
   |
   +-- HTTP/1.0
   |     `-- TCP
   +-- DNS
   |     `-- UDP
   +-- ICMP echo
   `-- IPv4
          `-- ARP routing/neighbor resolution
                 `-- Ethernet
                        `-- Intel E1000 82540EM PCI NIC
                               `-- QEMU user-mode networking
```

AxiomOS implements these protocol components itself. It does not import a host
TCP/IP stack or a userspace networking library.

## NIC choice

Phase 18 uses QEMU's emulated Intel 82540EM E1000. Compared with starting with
VirtIO-net, E1000 exposes the classic pieces we want to learn directly:

- PCI discovery;
- MMIO registers;
- DMA receive/transmit descriptor rings;
- physical DMA buffer addresses;
- Ethernet frames.

The driver is intentionally polled. NIC interrupts and asynchronous socket I/O
are future improvements.

## QEMU network configuration

Normal `make run` attaches:

```text
-netdev user,id=net0,ipv6=off
-device e1000,netdev=net0,mac=52:54:00:12:34:56
```

Phase 18 uses QEMU user-networking's stable IPv4 topology:

```text
AxiomOS     10.0.2.15/24
router      10.0.2.2
dns         10.0.2.3
```

The address is static in this phase. DHCP is not claimed yet.

## Implemented protocols

### Ethernet

Creates and parses Ethernet II frames, supports unicast to the local MAC and
broadcast reception, and dispatches EtherTypes 0x0806 (ARP) and 0x0800 (IPv4).

### ARP

Maintains a small neighbor cache, sends ARP requests, learns replies, responds
to requests for AxiomOS's own address, and resolves either the destination host
or the default gateway depending on the IPv4 subnet.

### IPv4

Creates/validates IPv4 headers and Internet checksums and dispatches ICMP, UDP,
and TCP. Phase 18 deliberately rejects fragmented packets instead of pretending
fragment reassembly exists.

### ICMP

Implements Echo Request/Echo Reply. The shell's `ping` sends one request. The
QEMU router at `10.0.2.2` is the deterministic test peer.

### UDP + DNS

UDP includes pseudo-header checksums. DNS sends recursive A-record questions to
`10.0.2.3` and understands compressed DNS names well enough to walk questions
and answers and return an IPv4 A record.

### TCP

Implements one synchronous active connection at a time:

- SYN;
- SYN/ACK;
- ACK;
- ordered data send/receive;
- checksum verification;
- ACK generation;
- RST handling;
- FIN reception/acknowledgment.

There is no retransmission engine, congestion control, out-of-order reassembly,
select/poll API, or general socket API yet.

### HTTP

`httpget` performs an HTTP/1.0 GET over TCP. It parses the response status line
and header/body boundary and returns up to 4096 body bytes to userspace.

HTTPS/TLS is **not** implemented. `https://` sites are not claimed to work.

## Shell commands

```text
axiom> netinfo
axiom> ping 10.0.2.2
axiom> dns example.com
axiom> httpget example.com /
```

`httpget` also supports a non-default port for development/testing:

```text
axiom> httpget 10.0.2.2:8080 /
```

## New syscalls

```text
23  netinfo
24  ping
25  dns
26  httpget
```

They deliberately expose useful Phase-18 operations rather than pretending a
full POSIX sockets API exists. A socket-style userspace API can be added after
the userspace libc and asynchronous I/O foundations mature.

## Acceptance test

```bash
make test-phase18
```

The test boots QEMU with four CPUs and an explicit E1000. It verifies:

1. E1000 PCI/MMIO/DMA initialization;
2. `netinfo` reports `10.0.2.15`;
3. ARP + IPv4 + ICMP by pinging `10.0.2.2`;
4. UDP + DNS by resolving `example.com` through QEMU's DNS proxy;
5. TCP + HTTP against a deterministic local HTTP server reached through
   QEMU `guestfwd` at `10.0.2.100:80`;
6. the Ring-3 shell remains usable afterward.

The DNS portion requires ordinary host Internet/DNS connectivity.

Then run the complete regression suite:

```bash
make test
```

## Important limitations

- IPv4 only;
- static QEMU user-network configuration, no DHCP yet;
- E1000 only;
- polling NIC, no NIC interrupt handler;
- synchronous network syscalls temporarily monopolize the BSP while waiting;
- ARP cache is small and has no wall-clock expiration;
- no IP fragmentation/reassembly;
- one TCP connection at a time;
- no TCP retransmission, congestion control, receive reordering or sockets;
- DNS A records only;
- HTTP/1.0 GET only;
- no TLS/HTTPS;
- no IPv6.

Those boundaries are intentional. Phase 18 proves the complete vertical path
from a real NIC DMA ring through TCP/HTTP without claiming production-network
semantics we have not built.
