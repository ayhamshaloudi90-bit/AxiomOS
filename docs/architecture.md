# Architecture at Phase 21

- Target: x86-64 SMP-capable machine, freestanding C plus x86-64 assembly.
- Limine v12.9.0 boots a higher-half ELF64 kernel and provides MP CPU discovery.
- Normal QEMU boots expose four virtual CPUs (`-smp 4`).
- CPU 0 is the bootstrap processor (BSP); secondary CPUs are APs.
- Each managed AP switches to the AxiomOS kernel CR3, a private 16 KiB kernel
  stack, and a private GDT/TSS/IST set before entering Phase-17 C code.
- The IDT is shared after BSP initialization; AP interrupts remain disabled in
  this foundational SMP phase.
- Serial output mirrors the framebuffer terminal/custom `kprintf()`.
- The BSP local APIC supplies the 100 Hz timer; the I/O APIC routes PS/2 IRQ1.
- PMM manages 4 KiB frames; VMM owns the kernel PML4 and isolated user roots.
- Kernel heap provides dynamic allocation.
- Scheduler keeps a five-tick APIC quantum but Phase 21 uses priority + aging:
  base priorities are 0..7, READY tasks age upward to prevent starvation, and
  equal effective priorities use circular round-robin tie-breaking.
- Per-task CPU-affinity masks are recorded, but normal scheduling remains
  deliberately BSP-only; APs still park after the Phase-17 SMP proof.
- Ring-3 tasks use private user memory/stacks and trusted kernel stacks.
- Phase 15 provides fork/exec/wait parent-child process management.
- Phase 16 provides spinlocks, FIFO wait queues, sleeping mutexes and counting
  semaphores.
- Phase 17 proves the spinlock across truly parallel CPUs using an exact shared
  counter test; APs park after that proof rather than running general tasks.
- Phase 18 adds an Intel E1000 PCI/MMIO/DMA NIC and an in-kernel
  Ethernet -> ARP -> IPv4 stack with ICMP, UDP/DNS, and a minimal TCP/HTTP
  client. Networking is polled and synchronous in this phase.
- `SYSCALL/SYSRETQ` validates user pointers at the kernel boundary.
- ELF64 loader maps ET_EXEC PT_LOAD segments with R/W/X permissions.
- VFS provides paths, mounts, vnodes and per-task file descriptors.
- RAMFS backs `/` and `/tmp`.
- Phase 13 discovers q35 AHCI through PCI, exposes a SATA block device, and
  mounts persistent `diskfs` at `/disk`.

## CPU/SMP topology

```text
                     +-----------------------------+
                     |        Shared kernel        |
                     | CR3 / kernel image / IDT    |
                     | spinlocks / shared memory   |
                     +---------------+-------------+
                                     |
          +--------------------------+--------------------------+
          |                          |                          |
          v                          v                          v
+-------------------+      +-------------------+      +-------------------+
| CPU 0 / BSP       |      | CPU 1 / AP        |      | CPU 2+ / AP       |
| private TSS/stack |      | private TSS/stack |      | private TSS/stack |
| APIC timer        |      | IF=0              |      | IF=0              |
| scheduler         |      | SMP proof -> HLT  |      | SMP proof -> HLT  |
| Ring-3 tasks      |      |                   |      |                   |
+-------------------+      +-------------------+      +-------------------+
```

This is intentionally **not** a fully SMP scheduler yet. Phase 21 adds affinity
metadata while still requiring CPU0 in runnable masks. Moving arbitrary tasks
between CPUs still requires per-CPU current-task state, run-queue locking or
per-CPU run queues, AP timers, reschedule IPIs, and TLB shootdowns.

## Storage stack

```text
Ring-3 ELF process
        |
        v
file syscalls
        |
        v
+-------------------------------+
|              VFS              |
+-------------------------------+
   |           |            |
   v           v            v
 RAMFS /    RAMFS /tmp   diskfs /disk
                              |
                              v
                       block_device API
                              |
                              v
                         AHCI driver
                              |
                              v
                     PCI / q35 ICH9 SATA
                              |
                              v
                    raw persistent image
```

AHCI currently uses a single command slot, polling and a one-sector DMA bounce
buffer. That intentionally favors correctness/visibility over throughput.


## Network stack

```text
Ring-3 axiomsh
      |
      v
netinfo / ping / dns / httpget
      |
      v
HTTP/1.0 -> TCP
DNS      -> UDP
ICMP echo
      \    |    /
          IPv4
            |
           ARP
            |
        Ethernet II
            |
   Intel E1000 82540EM
            |
   QEMU user networking
```

Phase 18 uses IPv4 only, a static QEMU guest topology, one synchronous TCP
connection, and plain HTTP. It deliberately does not claim DHCP, sockets, TCP
retransmission/congestion control, TLS/HTTPS, or IPv6 yet.

## Phase 22 IPC layer

A new `kernel/ipc/ipc.c` subsystem owns bounded pipe, shared-memory, and message
queue objects. The syscall layer validates/copies user buffers before invoking
it. Shared memory is the exception by design: the IPC layer maps a PMM frame
into participating Ring-3 page tables with USER|WRITABLE|NX permissions so
processes can exchange bytes directly while the kernel retains mapping and
lifetime control.

```text
Ring-3 program A                Ring-3 program B
      |                               |
      +---- validated IPC syscalls ---+
                      |
                 kernel/ipc
              /       |       \
           pipe    shared     msgq
          buffer     PMM      queue
                    page
              \       |       /
               process/VMM lifecycle
```

The scheduler calls the IPC lifecycle hooks during fork, exec, and task
teardown. Normal scheduling is still BSP-only, so Phase 22 does not claim
cross-CPU IPC concurrency yet.

## Phase 23 graphics stack

```text
Ring-3 program (/bin/gfxdemo)
        |
        v
libaxiom graphics API
        |
        v
validated gfx syscalls 45..52
        |
        v
kernel/graphics/framebuffer.c
   |        |         |
 pixels   bitmaps   boot bitmap font
   \        |         /
      Limine RGB32 framebuffer
                |
        QEMU/display device
```

The Phase-2 terminal and Phase-23 graphics library share the same framebuffer.
The terminal remains the kernel's logging/panic console. Ring-3 never receives
the raw framebuffer pointer; bitmap and text payloads cross the normal VMM
user-copy boundary. The renderer is deliberately software-only and establishes
the foundation for later mouse/window/compositor work without coupling those
features to the terminal implementation.


## Phase 24 networking applications

```text
Ring-3 programs
 ifconfig  ping  dnslookup  httpget  httpd
    |       |       |        |       |
    +-------+-------+--------+-------+
                    |
           validated network syscalls
                    |
        ICMP / DNS / HTTP client + server
                    |
          TCP active + passive open
                    |
              IPv4 -> ARP
                    |
             E1000 / QEMU
```

Phase 24 keeps the existing high-level networking ABI and adds only one server
primitive (`httpserve`). The passive TCP path accepts one listener/connection at
a time, which matches the stack's existing synchronous single-connection model.
The shell's Phase-18 builtins remain in place; the new `/bin` ELFs prove normal
isolated Ring-3 applications can consume the same networking capabilities.


## Phase 25 observability stack

```text
Ring-3 shell / /bin/sysinfo
            |
       open/read/readdir
            |
            VFS
            |
     /proc read-only procfs
      /   /   |   \   \
   PMM heap tasks IRQs net scheduler CPU VMM
```

`procfs` renders files on demand from live kernel structures. This deliberately
uses the existing VFS boundary instead of adding a broad privileged debug
syscall. `/proc/pagemap` reports the current process CR3 and its ELF, stack, and
anonymous mappings; `/proc/files` walks per-task descriptor tables; and
`/proc/interrupts` consumes counters maintained directly by the common IDT
dispatcher. The current system remains BSP-scheduled, so procfs rendering is
serialized by normal syscall execution.


## Phase 26 desktop stack

```text
                 /bin/desktop (Ring 3)
                      |
      +---------------+----------------+
      |               |                |
 graphics API      stdin/read      mouse state
      |               |                |
 gfx syscalls      keyboard        syscalls 54/55
      |               |                |
 framebuffer      PS/2 IRQ1        PS/2 IRQ12
      |                                |
      +---------- QEMU display/input --+

Desktop launch cards -> spawn/waitpid -> axiomsh/sysinfo/ifconfig/gfxdemo
```

The desktop is deliberately not a privileged kernel component. It is a normal
ELF process using the same protected Phase-23 graphics syscalls and Phase-15
process APIs as other programs. The boot shell remains the recovery/debug path.
A nested `axiomsh` can be launched from the desktop and its new `exit` command
returns control to the desktop process.

Phase 26 adds PS/2 second-port mouse support as an optional enhancement. The
mouse driver routes IRQ12 through the I/O APIC and exposes only decoded state;
direct device access remains in the kernel. Keyboard shortcuts provide a full
fallback path when no PS/2 mouse is detected.
