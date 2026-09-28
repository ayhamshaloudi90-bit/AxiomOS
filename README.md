# AxiomOS

AxiomOS is a freestanding x86-64 hobby operating system...

## Features

- Memory management
- Ring-3 userspace
- Networking
- Graphics
- Desktop GUI

# AxiomOS

AxiomOS is a freestanding x86-64 hobby operating system built from scratch in C and assembly.

It began as a small kernel project and gradually evolved into a functional operating system with userspace programs, virtual memory, process management, persistent storage, networking, IPC, graphics, security features, developer tooling, and a graphical desktop environment.

> AxiomOS is an educational and hobby operating system and is not intended to replace production operating systems such as Linux or Windows.

---

## Features

### Kernel
- x86-64 higher-half kernel
- GDT, TSS and IDT
- Interrupt and exception handling
- APIC timer
- Physical memory manager
- Virtual memory manager
- Dynamic kernel heap

### Processes & Userspace
- Ring-3 userspace isolation
- ELF64 executable loading
- Custom syscall interface
- `fork`
- `exec`
- `waitpid`
- Process termination
- Priority scheduling
- Starvation-prevention aging
- CPU-affinity infrastructure

### Filesystems & Storage
- Virtual File System (VFS)
- RAM filesystem
- AHCI SATA driver
- Persistent disk filesystem
- Per-process file descriptors
- `/proc`-style pseudo-filesystem

### Networking
- Intel E1000 driver
- Ethernet
- ARP
- IPv4
- ICMP
- UDP
- TCP
- DNS
- HTTP client
- HTTP server

AxiomOS can act as both a network client and server.

Examples:

```text
axiom> ping
axiom> dnslookup
axiom> httpget
axiom> httpd

Inter-Process Communication
- Pipes
- Shared memory
- Message queues
Security
- Kernel/user memory isolation
- NX pages
- W^X enforcement
- Stack guard pages
- Kernel stack protection
- Syscall pointer validation
- File permissions
- UID/GID credentials
Graphics
- Linear framebuffer support
- Pixel drawing
- Line drawing
- Rectangle drawing
- Bitmap rendering
- Text rendering
- PS/2 mouse support
- Graphical desktop interface
Developer Tooling
AxiomOS provides a read-only /proc filesystem with runtime information such as:
/proc/meminfo
/proc/processes
/proc/interrupts
/proc/files
/proc/net
/proc/scheduler
/proc/cpuinfo
/proc/pagemap

It also includes:
axiom> sysinfo

AxiomOS Desktop
AxiomOS includes a graphical desktop environment built on its own framebuffer graphics system.
From the shell:
axiom> desktop

The desktop currently provides launchers for:
- Terminal
- System information
- Networking
- Graphics
The terminal remains fully accessible from the desktop.
Shell
AxiomOS includes its own Ring-3 interactive shell:
AxiomOS shell ready. Type 'help' for commands.
axiom>

Example commands include:
help
ps
ls
cat
sysinfo
ifconfig
ping
dnslookup
httpget
httpd
ipcdemo
gfxdemo
schedbench
desktop

Architecture
A simplified overview:
                    AxiomOS Userspace
 ┌─────────────────────────────────────────────────┐
 │ Desktop │ Shell │ sysinfo │ ping │ httpd │ ... │
 ├─────────────────────────────────────────────────┤
 │                   libaxiom                      │
 └───────────────────────┬─────────────────────────┘
                         │
                      Syscalls
                         │
 ┌───────────────────────▼─────────────────────────┐
 │                  AxiomOS Kernel                 │
 │                                                 │
 │ Scheduler │ VFS │ IPC │ Network │ Graphics    │
 │                                                 │
 │       Virtual / Physical Memory Management      │
 ├─────────────────────────────────────────────────┤
 │ AHCI │ E1000 │ PS/2 │ APIC │ Framebuffer      │
 └───────────────────────┬─────────────────────────┘
                         │
                      Hardware

Building AxiomOS
AxiomOS currently targets x86-64 and is primarily developed and tested using QEMU.
Main tools include:
- Clang
- LLD
- NASM
- GNU Make
- QEMU
- Limine
Build the operating system with:
make

Run it with:
make run

Testing
AxiomOS contains an automated regression suite covering the major subsystems developed throughout the project.
Run all tests with:
make test

Individual phases can also be tested, for example:
make test-phase21
make test-phase22
make test-phase23
make test-phase24
make test-phase25
make test-phase26

Testing covers areas including:
- CPU exceptions
- Physical memory
- Virtual memory
- Kernel heap
- Hardware interrupts
- Scheduling
- Ring-3 isolation
- Syscalls
- ELF loading
- Filesystems
- Persistent storage
- Processes
- Synchronization
- SMP
- Networking
- Security
- IPC
- Graphics
- Developer tooling
- Desktop functionality
Development Roadmap
Phase	Milestone
1	Bootable x86-64 kernel
2	Kernel terminal
3	CPU architecture & interrupts
4	Physical memory management
5	Virtual memory
6	Kernel heap
7	Timer & keyboard
8	Preemptive scheduler
9	Ring-3 userspace
10	Syscalls
11	ELF loader
12	Virtual filesystem
13	Persistent storage
14	Interactive shell
15	Process management
16	Synchronization
17	SMP
18	Networking
19	Userspace C library
20	Security hardening
21	Advanced scheduler
22	IPC
23	Graphical framebuffer
24	Networking applications
25	Developer tooling


The original Phase 0-25 roadmap is complete.
Post-roadmap development
Phase	Milestone
26	Graphical desktop environment


Development continues beyond the original roadmap.
Current Limitations
AxiomOS is a hobby operating system and still has many limitations compared with mature general-purpose operating systems.
Current limitations include:
- x86-64 only
- primarily tested under QEMU
- limited hardware-driver support
- no USB stack
- no Wi-Fi
- no audio subsystem
- no GPU acceleration
- no TLS/HTTPS
- no IPv6
- simplified TCP implementation
- no dynamic linker
- incomplete POSIX compatibility
- limited multicore process scheduling
- desktop does not yet provide a full window compositor
Project Structure
AxiomOS/
├── arch/        # x86-64 architecture-specific code
├── boot/        # Bootloader configuration
├── docs/        # Technical documentation
├── include/     # Public kernel and ABI headers
├── kernel/      # Core kernel subsystems
├── process/     # Scheduler and process management
├── tests/       # Automated regression tests
├── userspace/   # AxiomOS userspace programs and libc
├── Makefile
└── README.md

Project Status
AxiomOS has completed its original operating-system development roadmap and is now in post-roadmap development focused on improving usability, graphics, desktop functionality and system capabilities.
