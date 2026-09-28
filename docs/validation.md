# Validation

AxiomOS uses phase-specific QEMU regression tests plus a full cumulative suite.

Current acceptance commands:

```bash
make clean
make
make test-phase25
make test
```

`make test` runs Phase 1 through Phase 25. Destructive fault/corruption cases
are built in separate output directories so they do not overwrite the normal
kernel image.

Phase 12 additionally validates that:

- `phase12_demo.elf` is a standalone ELF64 `ET_EXEC` with three PT_LOAD segments;
- rootfs and `/tmp` are separate mounts;
- `/bin`, `/etc` and `/tmp` resolve as expected;
- ordinary process file descriptors begin at 3;
- userspace can read `/etc/motd`;
- userspace can create/write/seek/read/stat/close `/tmp/phase12.txt`;
- the kernel reads the file back and verifies its bytes;
- `exec("/bin/phase11-demo")` resolves through the VFS and runs the ELF target;
- the kernel reaches the existing interactive keyboard loop afterward without a panic.

## Phase 14 static/package validation

Before packaging Phase 14, the kernel C sources were compiled with the normal
freestanding flags and every existing deliberate fault/corruption define:
`divide`, `invalid`, `gp`, `double_fault`, `page_fault`, `heap_double_free`, and
`heap_guard`. The userspace shell C and assembly entry were compiled and linked
as ELF64 `ET_EXEC`, x86-64, entry `0x400000`, with three `PT_LOAD` segments.
All Python tests were syntax-checked. Runtime QEMU acceptance remains the local
`make test-phase14` / `make test` step.

## Phase 15

```bash
make test-phase15
```

The test boots QEMU with a disk, drives `/bin/axiomsh` via PS/2 input, executes
`phase15-demo` to verify fork/exec/blocking-wait/private-memory semantics, uses
`ps`, launches a sleeping process in the background, kills it with SIGTERM,
reaps status 143, and then launches another ELF to prove the task slot is
reusable.

## Phase 16

```bash
make test-phase16
```

The test boots QEMU with the ordinary Phase-16 kernel and validates four
synchronization behaviors: a deterministic unprotected lost-update race, an
exact spinlock-protected counter, a sleeping mutex that actually blocks and
wakes waiters, and a counting semaphore whose measured concurrency peak is
exactly its configured limit of two. It then types `echo phase16-ok` into the
Ring-3 shell to verify that the kernel returns to normal userspace operation.

Static package validation additionally compiles every kernel C translation unit
with `-Wall -Wextra -Werror` under normal mode and all seven existing fault /
corruption modes, syntax-checks every Python test, and compiles all existing GAS
and userspace C sources without host-libc dependencies.


## Phase 17

`make test-phase17` boots QEMU with `-smp 4`, requires at least four detected/managed/online CPUs, verifies that every released AP enters AxiomOS code, checks the per-CPU participant mask, and requires the exact locked shared-counter total (`online CPUs × 5,000`). It also requires the normal Ring-3 shell to launch after APs park. Older phase tests may still boot one CPU; Phase 17 discovery remains active but its multicore proof reports SKIPPED on those boots rather than breaking historical regression tests.


## Phase 18

```bash
make test-phase18
```

The Phase-18 test boots QEMU with an explicit Intel E1000 and QEMU user-mode
networking. It drives the Ring-3 shell through virtual PS/2 input and checks:

- PCI/MMIO/DMA E1000 initialization;
- the static Phase-18 guest IPv4 configuration;
- Ethernet + ARP + IPv4 by reaching the QEMU router;
- ICMP Echo Request/Reply with `ping 10.0.2.2`;
- UDP + DNS A-record resolution through the QEMU DNS proxy;
- a TCP three-way handshake and ordered stream receive;
- HTTP/1.0 GET against a deterministic host-side test server exposed with
  QEMU `guestfwd`;
- return to a usable Ring-3 shell without a kernel panic.

The DNS subtest needs ordinary host DNS/Internet connectivity. Static package
validation compiles all 47 kernel C translation units under normal mode and
all seven deliberate fault/corruption modes, builds the GAS/userspace sources,
syntax-checks the Python tests, and audits the combined kernel objects for
unexpected host-libc/runtime dependencies. Actual NIC/TCP runtime acceptance
remains the local QEMU test above.


## Phase 19

`make test-phase19` boots QEMU with the normal SMP/E1000 environment, waits for the Ring-3 shell, runs `/bin/phase19-demo`, and validates libc formatting, strings/memory, ctype/conversion, dynamic allocation, anonymous-mapping fork isolation, exec cleanup of anonymous mappings, VFS wrappers, process wrappers, and networking wrappers. `make test` includes this target after Phase 18.


## Phase 20

`make test-phase20` boots the normal 4-vCPU/E1000 QEMU environment, verifies the Phase-20 boot policy, runs `id`, then executes `/bin/phase20-demo`. The demo requires successful UID/GID reporting, executable permission enforcement, `-EFAULT` for a kernel-half syscall pointer, Ring-3 kernel isolation, NX heap enforcement, a fault on the explicit user stack guard page, and rejection of a deliberately RWE ELF PT_LOAD segment. `make test` includes this target after Phase 19.


## Phase 21

```bash
make test-phase21
```

The Phase-21 test boots the normal four-vCPU QEMU configuration and requires the
kernel to switch from the historical early-boot round-robin policy to
priority+aging. It verifies the priority range/default, starvation-prevention
aging, BSP affinity-mask policy, and the deterministic scheduler comparison.
It then drives the Ring-3 shell, runs `schedstats`, executes `/bin/schedbench`,
and checks the priority, affinity, and statistics syscall APIs.

The deterministic benchmark compares 96 decisions over three priorities.
Round-robin must select `32/32/32`; the current priority-aging model produces
`3/7/86`, demonstrating preference for the high-priority task while still
selecting every task. This benchmark is a policy comparison, not a wall-clock
performance claim.

Static package validation compiles every kernel C translation unit with the
project's freestanding `-Wall -Wextra -Werror` flags, syntax-checks the Phase-21
Python test, and compiles/links the new userspace scheduler benchmark. Runtime
QEMU acceptance remains the local `make test-phase21` / `make test` step.

## Phase 22

```bash
make test-phase22
```

The Phase-22 test boots the normal four-vCPU/E1000 configuration, verifies the
kernel IPC initialization markers, then drives the Ring-3 shell and executes
`/bin/ipcdemo`. Acceptance requires:

- a child process writing and its parent reading through a kernel pipe;
- `/bin/ipcdemo` and the independent `/bin/ipc-peer` ELF observing the same
  physical shared-memory page;
- the peer sending a bounded message and the parent receiving the exact message;
- IPC statistics reporting real pipe, shared-memory, and queue activity;
- return to the shell without a kernel panic.

Static package validation compiles every kernel C translation unit with the
project's freestanding `-Wall -Wextra -Werror` flags, builds and links the two
new Phase-22 userspace ELFs plus the updated shell/libc, and syntax-checks all
Python regression tests. This packaging environment has no NASM/QEMU, so final
runtime acceptance remains the local `make test-phase22` / `make test` step.

## Phase 23

`make test-phase23` boots the normal four-vCPU configuration, verifies graphics
initialization and framebuffer read/write/clipping self-tests, drives the Ring-3
shell, queries `gfxinfo`, executes `/bin/gfxdemo`, and requires success markers
for pixel, line, rectangle, bitmap, text, and the userspace graphics library.
The demo then restores the framebuffer terminal and returns normally to the
shell.

Static package validation compiles all 49 kernel C translation units with the
project's freestanding `-Wall -Wextra -Werror` flags; builds and links every
userspace ELF from Phases 11–23 with the updated `libaxiom.a`; and syntax-checks
the complete Python/shell test suite. Runtime QEMU acceptance remains the local
`make test-phase23` / `make test` step.


## Phase 24

```bash
make test-phase24
```

The Phase-24 test boots the normal four-vCPU E1000 configuration with both
QEMU forwarding directions enabled. It executes the five standalone Ring-3
networking programs and requires:

- `/bin/ifconfig` to report `10.0.2.15`, netmask/gateway/DNS, and counters;
- `/bin/ping` to receive an ICMP reply from the QEMU gateway;
- `/bin/dnslookup` to resolve an IPv4 A record through QEMU DNS;
- `/bin/httpget` to retrieve a deterministic body from a host HTTP server
  exposed to the guest by `guestfwd`;
- `/bin/httpd` to accept a host-originated TCP connection through `hostfwd`,
  return HTTP/1.0 200, and serve the expected body;
- return to the Ring-3 shell with no kernel panic.

Static package validation compiles all 49 kernel C translation units with the
normal freestanding `-Wall -Wextra -Werror` flags; compiles/links the full libc,
updated shell, all existing C userspace programs, and the five new networking
ELFs; compiles the GAS sources; and syntax-checks the full Python/shell test
suite. Runtime QEMU acceptance remains the local `make test-phase24` /
`make test` step.


## Phase 25

```bash
make test-phase25
```

The Phase-25 test boots the normal four-vCPU/E1000 configuration, requires the
read-only `/proc` mount and all eight telemetry files, runs `/bin/sysinfo`, and
verifies live memory/process/page-map, interrupt/open-file/network, scheduler,
and CPU data. It also attempts to write `/proc/meminfo` and requires `-EACCES`.

Static validation compiles all 50 kernel C translation units with the normal
freestanding `-Wall -Wextra -Werror` flags; links all existing userspace ELFs
plus `/bin/sysinfo`; and syntax-checks the complete Python/shell test suite.
Runtime acceptance remains the local `make test-phase25` / `make test` step.


## Phase 26

```bash
make test-phase26
```

The Phase-26 test keeps the accepted shell boot path, launches `/bin/desktop`,
verifies its startup/control markers, opens a nested Terminal using the `T`
shortcut, runs an ordinary shell command, executes the new `exit` built-in to
return to the graphical desktop, then quits the desktop with `Q` and proves the
original shell is still interactive. Mouse initialization is allowed to be
either online or unavailable because keyboard fallback is a deliberate part of
the design.

Static validation additionally compiles the new PS/2 mouse driver, syscall
plumbing, desktop ELF, updated shell, and every kernel C translation unit with
`-Wall -Wextra -Werror`; syntax-checks the Phase-26 Python regression; and keeps
Phase 26 in the cumulative `make test` target. Runtime framebuffer/input
acceptance remains the local QEMU step.
