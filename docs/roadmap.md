You are a senior operating-systems engineer and low-level systems programmer. I want you to help me design and build a completely new hobby operating system from scratch as a serious Computer Engineering portfolio project.

This must NOT be a fake operating system, a Linux distribution, a wrapper around an existing kernel, or a superficial terminal application. We are building an actual bootable kernel with memory management, processes, scheduling, system calls, userspace, a filesystem, device drivers, and eventually networking.

The goal is for this project to become resume-worthy and technically defensible in an interview.

## Core technical choices

Use:

- Architecture: x86-64
- Primary language: C
- Low-level code: x86-64 Assembly where necessary
- Compiler: GCC or Clang cross-compiler
- Build system: Make
- Emulator: QEMU
- Debugger: GDB
- Executable format: ELF64
- Version control: Git
- Development environment: Linux

Avoid C++ unless there is a compelling reason.

The kernel should eventually be freestanding and must not depend on libc or the host operating system.

For the first working versions, using a mature boot protocol such as Limine is acceptable so we can focus on kernel development. Later, add an optional phase where we write our own bootloader to better understand the complete boot process.

## Important working style

Do NOT attempt to generate the entire operating system in one response.

We will build this incrementally.

For every phase:

1. Explain what subsystem we are about to build.
2. Explain how it works conceptually.
3. Show the project directory structure.
4. Give me the complete code for every new or modified file.
5. Explain important code line-by-line where useful.
6. Explain how the subsystem interacts with previously implemented components.
7. Give exact Linux commands to compile it.
8. Give exact commands to run it in QEMU.
9. Tell me exactly what output or behavior I should expect.
10. Give debugging instructions if it fails.
11. Create at least one test for the new functionality.
12. Do not move to the next major phase until the current subsystem can be tested successfully.

Whenever there are multiple architectural choices, explain the tradeoffs before selecting one.

Do not silently hide complexity behind libraries.

I want to understand the underlying operating-system concepts.

## Code quality requirements

All code should:

- be modular
- have clear headers and source files
- avoid giant source files
- use descriptive names
- contain useful comments
- avoid unnecessary global state
- have proper error handling where possible
- compile without warnings
- use explicit integer widths such as uint64\_t
- distinguish physical and virtual addresses clearly
- document important data structures
- follow a consistent coding style

Use assertions/panics for kernel invariants where appropriate.

Design APIs as if the kernel may grow considerably later.

## Project name

Until I give you another name, call the operating system:

AxiomOS

Use:

```text
AxiomOS/
├── kernel/
├── arch/
├── drivers/
├── memory/
├── process/
├── filesystem/
├── networking/
├── userspace/
├── libc/
├── include/
├── tools/
├── tests/
├── docs/
├── Makefile
└── README.md

```

Modify the structure when technically appropriate.

## Development roadmap

Build AxiomOS through the following phases.

### PHASE 0 — Development environment

Set up:

- cross compiler
- binutils
- QEMU
- GDB
- Make
- NASM or GNU assembler
- Limine
- linker script

Create the minimum repository structure.

Explain why a cross compiler is preferable for kernel development.

Create:

```bash
make
make run
make debug
make clean

```

commands.

---

### PHASE 1 — Bootable 64-bit kernel

Create a kernel that:

- boots in QEMU
- enters x86-64 long mode
- initializes a stack
- enters `kernel_main()`
- prints:

```text
AxiomOS kernel booted successfully.

```

Do not simply call BIOS printing routines from C.

Implement an early kernel output system.

At the end of this phase, I should have an actual bootable kernel.

---

### PHASE 2 — Kernel terminal

Create a terminal subsystem supporting:

- framebuffer or VGA output
- characters
- strings
- decimal integers
- hexadecimal integers
- newline
- scrolling
- colors
- clear screen

Provide something similar to:

```c
kprintf("Physical memory: %llu MB\n", memory_mb);

```

Do NOT simply import libc `printf`.

Implement our own minimal formatter.

---

### PHASE 3 — CPU architecture initialization

Implement and explain:

- GDT
- IDT
- interrupt descriptor entries
- CPU exceptions
- exception handlers
- interrupt handlers
- register-state structures
- PIC/APIC fundamentals

When a divide-by-zero or invalid opcode occurs, AxiomOS should print something such as:

```text
KERNEL PANIC

Exception: Divide by Zero
RIP: 0xFFFFFFFF80001234
RSP: 0xFFFFFFFF802FF000
RFLAGS: ...

```

---

### PHASE 4 — Physical memory manager

Parse the bootloader memory map.

Implement a physical page allocator using 4 KiB pages.

Possible approaches include:

- bitmap allocator
- free-list allocator

Explain the tradeoff and choose one.

Implement interfaces such as:

```c
void* pmm_alloc_page(void);
void pmm_free_page(void* page);

```

Track:

- total memory
- usable memory
- allocated memory
- free memory

Provide diagnostic output.

---

### PHASE 5 — Virtual memory

Implement x86-64 paging.

Teach me:

- PML4
- PDPT
- page directories
- page tables
- page table entries
- virtual addresses
- physical addresses
- TLB
- page faults

Implement:

```c
map_page()
unmap_page()
virt_to_phys()

```

Create a higher-half kernel design if appropriate.

Add a page-fault handler.

---

### PHASE 6 — Kernel heap

Build a dynamic memory allocator.

Start with something understandable, then improve it.

Implement:

```c
kmalloc()
kcalloc()
krealloc()
kfree()

```

Add corruption checks if practical.

Test allocation and freeing extensively.

Explain fragmentation.

---

### PHASE 7 — Timer and keyboard drivers

Implement:

- programmable/APIC timer
- keyboard interrupt handling
- keyboard scancode decoding
- input buffering

Allow me to type characters into AxiomOS.

---

### PHASE 8 — Multitasking

Implement processes or kernel threads.

Create:

```c
struct task

```

containing at minimum:

- task ID
- state
- CPU registers
- stack
- address space
- scheduling information

Implement context switching.

Start with round-robin scheduling.

States should include something like:

```text
RUNNING
READY
BLOCKED
SLEEPING
TERMINATED

```

Demonstrate multiple tasks executing apparently concurrently.

---

### PHASE 9 — Userspace

Separate Ring 0 kernel code from Ring 3 applications.

Implement:

- user-mode execution
- separate user stacks
- separate address spaces
- privilege transitions
- protection between processes

Create the first userspace program:

```text
Hello from AxiomOS userspace!

```

A userspace process must not be able to directly access kernel memory.

---

### PHASE 10 — System calls

Create a syscall interface.

Potential syscalls:

```c
SYS_write
SYS_read
SYS_exit
SYS_sleep
SYS_fork
SYS_exec
SYS_open
SYS_close
SYS_getpid
SYS_yield
SYS_mmap

```

Use the appropriate x86-64 mechanism.

Create a small userspace libc wrapper:

```c
write()
read()
exit()
sleep()
open()
close()

```

Explain how control moves:

```text
Userspace
    ↓
syscall
    ↓
Kernel
    ↓
validation
    ↓
kernel subsystem
    ↓
return to userspace

```

---

### PHASE 11 — ELF executable loader

Implement an ELF64 parser and loader.

AxiomOS should be able to load compiled userspace executables.

Explain:

- ELF headers
- program headers
- loadable segments
- entry point
- virtual mappings
- permissions

Implement:

```text
exec("/bin/program")

```

---

### PHASE 12 — Virtual filesystem

Create a VFS abstraction supporting concepts such as:

```c
open()
read()
write()
close()
seek()
stat()

```

Represent:

- files
- directories
- mount points
- file descriptors

First support an in-memory filesystem.

Then add an actual disk filesystem.

Possible filesystem target:

- FAT32

Eventually support paths such as:

```text
/
/bin
/etc
/dev
/home
/tmp

```

---

### PHASE 13 — Disk driver

Add virtual disk support in QEMU.

Implement either:

- ATA
- AHCI
- VirtIO block

Explain the engineering tradeoffs before choosing one.

Read and write actual disk sectors.

Connect the disk subsystem to the filesystem.

---

### PHASE 14 — Shell

Create a real userspace shell.

Prompt:

```text
ayham@axiom:/$

```

Support commands:

```text
help
clear
echo
ls
cd
pwd
cat
mkdir
touch
rm
cp
mv
ps
kill
meminfo
uptime
uname
reboot
shutdown

```

Eventually support:

```text
program arguments
environment variables
PATH
pipes
input redirection
output redirection
background processes

```

Example:

```bash
cat log.txt | grep error > errors.txt

```

---

### PHASE 15 — Process management

Expand process support.

Implement:

- parent/child relationships
- process IDs
- fork
- exec
- wait
- exit status
- signals or a simplified signal mechanism
- sleeping
- blocking
- scheduler queues

Create:

```bash
ps

```

that displays something like:

```text
PID   STATE      NAME
1     RUNNING    init
2     SLEEPING   shell
3     READY      demo

```

---

### PHASE 16 — Synchronization

Implement kernel synchronization primitives:

- spinlocks
- mutexes
- semaphores
- wait queues

Explain:

- race conditions
- deadlocks
- starvation
- atomic operations

Create tests deliberately demonstrating race conditions before fixing them.

---

### PHASE 17 — SMP / Multicore

When the single-core kernel is stable, implement basic multicore support.

Detect multiple CPUs.

Initialize application processors.

Eventually schedule tasks across multiple CPU cores.

Discuss:

- per-CPU data
- synchronization
- scheduler locking
- TLB shootdowns

Do not implement this prematurely.

---

### PHASE 18 — Networking

Implement a networking subsystem.

Where feasible, build meaningful protocol components ourselves rather than depending on an external networking stack.

Roadmap:

```text
Ethernet
   ↓
ARP
   ↓
IPv4
   ↓
ICMP
   ↓
UDP
   ↓
TCP
   ↓
DNS
   ↓
HTTP

```

Use QEMU networking.

Major milestone:

```bash
ping <address>

```

from AxiomOS.

Later:

```bash
httpget example.com

```

---

### PHASE 19 — Userspace standard library

Build a small custom libc for AxiomOS.

Implement useful portions of:

```text
stdio
stdlib
string
ctype
unistd-like interfaces

```

Examples:

```c
printf()
malloc()
free()
memcpy()
memset()
strlen()
strcmp()
strcpy()
atoi()

```

Userspace programs should link against our own library.

---

### PHASE 20 — Security

Add defensive operating-system mechanisms:

- user/kernel isolation
- NX pages
- stack protection where possible
- syscall input validation
- invalid pointer checking
- permissions
- user/group concepts
- executable permissions
- ASLR if realistic
- guard pages
- kernel stack protections

Explain every mechanism rather than merely enabling it.

---

### PHASE 21 — Advanced scheduler

Replace or extend round-robin.

Consider:

- priority scheduling
- multilevel feedback queues
- scheduler fairness
- CPU affinity

Create benchmarking programs to compare algorithms.

---

### PHASE 22 — IPC

Implement inter-process communication.

Potential mechanisms:

- pipes
- shared memory
- message queues

Demonstrate two userspace programs communicating.

---

### PHASE 23 — Graphical framebuffer

Add graphics support.

Implement primitives:

```c
draw_pixel()
draw_line()
draw_rectangle()
draw_bitmap()
draw_text()

```

Create a framebuffer graphics library.

Eventually implement:

- mouse support
- windows
- basic compositor
- graphical terminal

GUI functionality is lower priority than kernel correctness.

---

### PHASE 24 — Networking applications

Once TCP/IP works, implement userspace programs such as:

```text
ping
ifconfig
dnslookup
httpget
simple HTTP server

```

---

### PHASE 25 — Developer tooling

**Implemented in the Phase-25 source; local QEMU acceptance remains the final gate.**

Build debugging and observability features.

Kernel commands or tools should expose:

```text
memory usage
processes
page mappings
interrupt counts
open files
network statistics
scheduler statistics
CPU information

```

Create a `/proc`-like pseudo-filesystem if appropriate.

---

## Testing requirements

Testing is extremely important.

Where possible, create automated tests for:

- physical memory allocation
- virtual memory
- heap allocation
- filesystem operations
- ELF parsing
- syscall validation
- scheduling
- networking
- data structures

Use QEMU's deterministic/testing capabilities when appropriate.

Do not simply assume something works because QEMU does not crash.

Add assertions.

Use GDB heavily.

Teach me useful GDB commands throughout development.

---

## Documentation requirements

Maintain:

```text
README.md
docs/architecture.md
docs/memory.md
docs/processes.md
docs/syscalls.md
docs/filesystem.md
docs/networking.md

```

Keep the documentation synchronized with the implementation.

Create architecture diagrams using Mermaid where useful.

For example:

```text
Applications
     │
     ▼
Userspace libc
     │
     ▼
System Call Interface
     │
     ▼
+----------------------------+
|          Kernel            |
| Scheduler | VFS | Network  |
| Memory    | IPC | Drivers  |
+----------------------------+
     │
     ▼
Hardware

```

---

## Git requirements

Treat this like a professional repository.

At appropriate milestones, recommend commits such as:

```text
feat: boot x86-64 kernel
feat: implement physical page allocator
feat: add virtual memory manager
feat: introduce preemptive scheduler
feat: enter ring 3 userspace
feat: add syscall interface
feat: implement ELF loader

```

Do not suggest committing broken intermediate states unless useful for development.

---

## Resume-quality requirements

Throughout development, record measurable achievements.

Examples:

- kernel size
- boot time
- supported RAM
- scheduler latency
- number of simultaneous processes
- filesystem throughput
- network throughput
- context-switch time
- syscall overhead

Create benchmarks when possible.

At the end, help me produce several strong resume bullet points based only on functionality that actually works.

Never exaggerate functionality.

---

## Extremely important rules

Do NOT:

- claim something works without giving me a way to test it
- invent APIs and then forget about them later
- produce pseudocode where real implementation is required
- hide complex OS functionality behind host Linux calls
- use Linux kernel code as the implementation
- copy a complete existing hobby operating system
- add random features before the underlying architecture is stable
- redesign previous systems without explaining why
- dump enormous amounts of unrelated code in one response

If we discover an architectural mistake, explain it clearly and refactor it properly.

Keep a mental model of everything we have already implemented so future code remains compatible.

When modifying an existing file, show the COMPLETE updated file unless I specifically ask for a patch/diff.

If context becomes too large, produce a concise project-state document containing:

- architecture
- directory tree
- implemented features
- APIs
- important structures
- known bugs
- next milestone

so that another AI session could continue development without losing context.

## Educational requirement

I am a Computer Engineering student.

Do not simply do the project for me.

Teach me enough that I could defend every major design decision in a technical interview.

When we implement an important mechanism, ask questions like:

- Why did we choose this data structure?
- Why does this code have to run in kernel mode?
- What happens during this context switch?
- Why does this page fault occur?
- What CPU state needs to be preserved?
- What security vulnerability would exist without this validation?

After major milestones, give me a short technical interview quiz about what we just built.

## First task

Start ONLY with:

### Phase 0 — Development Environment and Architecture Plan

Give me:

1. The proposed high-level architecture of AxiomOS.
2. A diagram of the system.
3. The initial directory layout.
4. The complete toolchain requirements.
5. Exact installation commands for a typical Ubuntu Linux machine.
6. The initial Makefile/build approach.
7. The boot strategy.
8. The memory-layout strategy.
9. Important engineering decisions we need to lock down before implementation.
10. A roadmap showing dependencies between subsystems.

Do NOT implement the scheduler, filesystem, networking, GUI, or other later features yet.

After Phase 0, begin Phase 1 only when I tell you:

`Continue to Phase 1.`

---

## Post-roadmap extension — Phase 26 Desktop GUI

The original Phase 0–25 roadmap ends at Developer Tooling. After that milestone,
AxiomOS continues with optional product/usability extensions. Phase 26 adds a
Ring-3 graphical desktop, taskbar/launchers, nested terminal workflow, and
best-effort PS/2 mouse input while preserving the original text shell as the
boot/recovery interface. See `docs/phase26.md`.
