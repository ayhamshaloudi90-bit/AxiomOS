# AxiomOS Phase 10 — complete new/modified source

This handoff contains every file added or modified for Phase 10 relative to the accepted Phase-9 regression-fixed tree.

## `Makefile`

```text
SHELL := /bin/bash


PROJECT := AxiomOS

MODE ?= normal
BUILD_DIR := build$(if $(filter-out normal,$(MODE)),-$(MODE))
VALID_MODES := normal divide invalid gp double_fault page_fault heap_double_free heap_guard
ifeq ($(filter $(MODE),$(VALID_MODES)),)
$(error Invalid MODE: $(MODE))
endif
OBJ_DIR := $(BUILD_DIR)/obj
ISO_ROOT := $(BUILD_DIR)/iso_root

KERNEL_ELF := $(BUILD_DIR)/$(PROJECT).elf
ISO_IMAGE := $(BUILD_DIR)/$(PROJECT).iso


CC := clang
LD := ld.lld
ASM := nasm

QEMU := qemu-system-x86_64
GDB := gdb

READELF := llvm-readelf
OBJDUMP := llvm-objdump


LIMINE_VERSION := v12.9.0
LIMINE_DIR := third_party/limine
LIMINE_ARCHIVE := $(BUILD_DIR)/limine-binary.tar.gz

LIMINE_URL := \
	https://github.com/Limine-Bootloader/Limine/releases/download/$(LIMINE_VERSION)/limine-binary.tar.gz


CFLAGS := \
	--target=x86_64-unknown-none-elf \
	-std=gnu11 \
	-Wall \
	-Wextra \
	-Werror \
	-O2 \
	-g \
	-ffreestanding \
	-fno-stack-protector \
	-fno-stack-check \
	-fno-pic \
	-fno-pie \
	-fno-lto \
	-ffunction-sections \
	-fdata-sections \
	-m64 \
	-march=x86-64 \
	-mabi=sysv \
	-mno-red-zone \
	-mno-80387 \
	-mno-mmx \
	-mno-sse \
	-mno-sse2 \
	-mcmodel=kernel


CPPFLAGS := \
	-Iinclude

ifneq ($(MODE),normal)
CPPFLAGS += -DAXIOM_TEST_$(shell echo $(MODE) | tr a-z A-Z)
endif


ASMFLAGS := \
	-f elf64 \
	-g \
	-F dwarf \
	-Wall

GASFLAGS := \
	--target=x86_64-unknown-none-elf \
	-m64 \
	-Iinclude


LDFLAGS := \
	-m elf_x86_64 \
	-nostdlib \
	-static \
	-z max-page-size=0x1000 \
	-z noexecstack \
	--gc-sections \
	-T linker/x86_64.ld


C_SOURCES := \
	arch/x86_64/boot/limine_requests.c \
	drivers/serial/serial.c \
	drivers/timer/apic_timer.c \
	drivers/keyboard/ps2_keyboard.c \
	process/task.c \
	process/scheduler.c \
	kernel/syscall/syscall.c \
	kernel/terminal/terminal.c \
	kernel/terminal/kprintf.c \
	kernel/core/kernel.c \
	kernel/core/panic.c \
	memory/pmm.c \
	memory/pmm_selftest.c \
	memory/vmm.c \
	memory/vmm_selftest.c \
	memory/heap.c \
	memory/heap_selftest.c \
	arch/x86_64/cpu/gdt.c \
	arch/x86_64/cpu/selftest.c \
	arch/x86_64/interrupts/idt.c \
	arch/x86_64/interrupts/pic.c \
	arch/x86_64/interrupts/apic.c


ASM_SOURCES := \
	arch/x86_64/boot/entry.asm \
	arch/x86_64/cpu/gdt_load.asm \
	arch/x86_64/cpu/register_probe.asm \
	arch/x86_64/interrupts/isr_stubs.asm

GAS_SOURCES := \
	arch/x86_64/syscall/syscall_entry.S \
	userspace/phase9_program.S \
	userspace/phase10_program.S

C_OBJECTS := \
	$(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SOURCES))


ASM_OBJECTS := \
	$(patsubst %.asm,$(OBJ_DIR)/%.o,$(ASM_SOURCES))

GAS_OBJECTS := \
	$(patsubst %.S,$(OBJ_DIR)/%.o,$(GAS_SOURCES))

OBJECTS := \
	$(ASM_OBJECTS) \
	$(GAS_OBJECTS) \
	$(C_OBJECTS)


.DEFAULT_GOAL := all


.PHONY: \
	all \
	deps \
	run \
	debug \
	test \
	inspect \
	clean \
	distclean \
	help


all: $(ISO_IMAGE)


deps: $(LIMINE_DIR)/limine


$(LIMINE_DIR)/limine:
	@echo "Fetching Limine $(LIMINE_VERSION)..."

	mkdir -p $(BUILD_DIR)

	curl \
		-fL \
		$(LIMINE_URL) \
		-o $(LIMINE_ARCHIVE)

	rm -rf $(LIMINE_DIR)

	mkdir -p $(LIMINE_DIR)

	tar \
		-xzf $(LIMINE_ARCHIVE) \
		-C $(LIMINE_DIR) \
		--strip-components=1

	$(MAKE) -C $(LIMINE_DIR)


$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)

	$(CC) \
		$(CFLAGS) \
		$(CPPFLAGS) \
		-MMD -MP \
		-c $< \
		-o $@


$(OBJ_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)

	$(ASM) \
		$(ASMFLAGS) \
		$< \
		-o $@

$(OBJ_DIR)/%.o: %.S
	@mkdir -p $(dir $@)

	$(CC) \
		$(GASFLAGS) \
		-c $< \
		-o $@


$(KERNEL_ELF): $(OBJECTS) linker/x86_64.ld
	@mkdir -p $(BUILD_DIR)

	$(LD) \
		$(LDFLAGS) \
		$(OBJECTS) \
		-o $(KERNEL_ELF)

	@echo
	@echo "Built kernel:"
	@echo "  $(KERNEL_ELF)"


$(ISO_IMAGE): \
	$(KERNEL_ELF) \
	boot/limine/limine.conf \
	$(LIMINE_DIR)/limine

	rm -rf $(ISO_ROOT)

	mkdir -p $(ISO_ROOT)/boot/limine
	mkdir -p $(ISO_ROOT)/EFI/BOOT


	cp \
		$(KERNEL_ELF) \
		$(ISO_ROOT)/boot/$(PROJECT).elf


	cp \
		boot/limine/limine.conf \
		$(ISO_ROOT)/boot/limine/


	cp \
		$(LIMINE_DIR)/limine-bios.sys \
		$(LIMINE_DIR)/limine-bios-cd.bin \
		$(LIMINE_DIR)/limine-uefi-cd.bin \
		$(ISO_ROOT)/boot/limine/


	cp \
		$(LIMINE_DIR)/BOOTX64.EFI \
		$(ISO_ROOT)/EFI/BOOT/


	xorriso \
		-as mkisofs \
		-R \
		-r \
		-J \
		-b boot/limine/limine-bios-cd.bin \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		-hfsplus \
		-apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part \
		--efi-boot-image \
		--protective-msdos-label \
		$(ISO_ROOT) \
		-o $(ISO_IMAGE)


	$(LIMINE_DIR)/limine \
		bios-install \
		$(ISO_IMAGE)


	rm -rf $(ISO_ROOT)


	@echo
	@echo "Built bootable image:"
	@echo "  $(ISO_IMAGE)"


run: $(ISO_IMAGE)
	$(QEMU) \
		-machine q35 \
		-m 256M \
		-cdrom $(ISO_IMAGE) \
		-boot d \
		-serial stdio \
		-monitor none \
		-no-reboot \
		-no-shutdown


debug: $(ISO_IMAGE)
	$(QEMU) \
		-machine q35 \
		-m 256M \
		-cdrom $(ISO_IMAGE) \
		-boot d \
		-serial stdio \
		-monitor none \
		-no-reboot \
		-no-shutdown \
		-S \
		-gdb tcp::1234


test: $(ISO_IMAGE)
	bash tests/phase1_boot.sh $(ISO_IMAGE)
	bash tests/phase2_terminal.sh $(ISO_IMAGE)
	$(MAKE) test-phase3
	$(MAKE) test-phase4
	$(MAKE) test-phase5
	$(MAKE) test-phase6
	$(MAKE) test-phase7
	$(MAKE) test-phase8
	$(MAKE) test-phase9
	$(MAKE) test-phase10


inspect: $(KERNEL_ELF)
	@echo "========== ELF HEADER =========="
	$(READELF) -h $(KERNEL_ELF)

	@echo
	@echo "========== PROGRAM HEADERS =========="
	$(READELF) -l $(KERNEL_ELF)

	@echo
	@echo "========== DISASSEMBLY =========="
	$(OBJDUMP) -d $(KERNEL_ELF)


clean:
	rm -rf build build-*


distclean: clean
	rm -rf $(LIMINE_DIR)


help:
	@echo "AxiomOS Phase 10 targets:"
	@echo
	@echo "  make          Build kernel and bootable ISO"
	@echo "  make run      Boot AxiomOS using QEMU"
	@echo "  make debug    Boot QEMU paused for GDB"
	@echo "  make test     Run Phase 1 through Phase 10 tests"
	@echo "  make test-phase6 Run heap + corruption tests"
	@echo "  make test-phase7 Run timer + keyboard IRQ/input tests"
	@echo "  make test-phase8 Run preemptive scheduler/context-switch tests"
	@echo "  make test-phase9 Run Ring 3 userspace/isolation tests"
	@echo "  make test-phase10 Run SYSCALL/SYSRET ABI and user-copy tests"
	@echo "  make MODE=heap_double_free run  Trigger double-free panic"
	@echo "  make MODE=heap_guard run        Trigger tail-guard panic"
	@echo "  make inspect  Inspect generated ELF64 kernel"
	@echo "  make clean    Remove generated build files"
	@echo "  make distclean Remove build files and Limine"

.PHONY: test-phase3 test-phase4 test-phase5 test-phase6 test-phase7 test-phase8 test-phase9 test-phase10

test-phase3:
	python3 tests/phase3_cpu.py

test-phase4:
	python3 tests/phase4_pmm.py

test-phase5:
	python3 tests/phase5_vmm.py

test-phase6:
	python3 tests/phase6_heap.py

test-phase7:
	python3 tests/phase7_devices.py

test-phase8:
	python3 tests/phase8_scheduler.py

test-phase9:
	python3 tests/phase9_userspace.py

test-phase10:
	python3 tests/phase10_syscalls.py

# Track C header changes too; each fault mode has its own object directory.
-include $(C_OBJECTS:.o=.d)
```

## `README.md`

```markdown
# AxiomOS

AxiomOS is a freestanding x86-64 hobby operating-system kernel written in C and
assembly, built with Clang/LLD/NASM and booted by Limine v12.9.0 under QEMU. It
is a higher-half ELF64 kernel and does not use the host libc.

## Current milestone

Phases 0–9 provide boot, terminal output, exceptions, PMM/VMM, a kernel heap,
APIC timer/keyboard IRQs, preemptive multitasking, Ring-3 execution, separate
user address spaces, and hardware-enforced kernel isolation.

Phase 10 adds the first real userspace-to-kernel ABI:

- x86-64 `SYSCALL` / `SYSRETQ`;
- per-task trusted kernel stacks for syscall entry;
- `write`, `read`, `_exit`, `sleep`, `getpid`, and `yield`;
- errno-style failures and reserved future syscall numbers;
- validated user-pointer copying through the VMM;
- scheduler-backed sleeping and explicit yielding;
- minimal userspace syscall wrappers;
- a Ring-3 demo that prints through the kernel, reads a keyboard byte, and exits.

Implemented foundation now includes:

- bootable higher-half x86-64 kernel;
- framebuffer terminal, serial mirror, custom `kprintf()`;
- GDT/TSS, IDT, exceptions and panic diagnostics;
- Local APIC timer and I/O APIC keyboard routing;
- physical/virtual memory managers and dynamic kernel heap;
- preemptive round-robin scheduler and private task stacks;
- Ring-3 tasks with separate CR3 roots;
- user/kernel page protection;
- validated x86-64 syscall boundary;
- automated regression tests through Phase 10.

## Common commands

```bash
make
make run
make test
make test-phase8
make test-phase9
make test-phase10
make MODE=page_fault run
make MODE=heap_double_free run
make MODE=heap_guard run
make debug
```

`make test` runs the complete Phase 1 through Phase 10 regression suite.

The next milestone is Phase 11: an ELF64 executable loader so userspace programs
stop being embedded raw one-page images and can be loaded as real executables.

[Architecture](docs/architecture.md) | [Memory](docs/memory.md) |
[Tasks](docs/processes.md) | [Syscalls](docs/syscalls.md) |
[Phase 10](docs/phase10.md) | [Project state](docs/project-state.md) |
[Original roadmap](docs/roadmap.md)
```

## `arch/x86_64/syscall/syscall_entry.S`

```asm
.section .text.syscall,"ax",@progbits
.code64

.extern syscall_dispatch
.extern syscall_kernel_stack_top

.section .bss.syscall,"aw",@nobits
.p2align 3
syscall_scratch_user_rsp:
    .quad 0

.section .text.syscall,"ax",@progbits
.global syscall_entry
.type syscall_entry, @function
syscall_entry:
    /*
     * SYSCALL enters CPL0 without changing RSP. IA32_FMASK clears IF before
     * this instruction stream runs, so the single-CPU scratch slot is safe
     * until the user RSP has been moved onto the task's private kernel stack.
     */
    movq %rsp, syscall_scratch_user_rsp(%rip)
    movq syscall_kernel_stack_top(%rip), %rsp

    /* Build struct syscall_frame from high addresses downward. */
    pushq syscall_scratch_user_rsp(%rip) /* user_rsp */
    pushq %r11                          /* user_rflags */
    pushq %rcx                          /* user_rip */
    pushq %rax
    pushq %rbx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    cld
    movq %rsp, %rdi
    call syscall_dispatch

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rbx
    popq %rax
    popq %rcx                          /* SYSRET RIP */
    popq %r11                          /* SYSRET RFLAGS */
    movq (%rsp), %rsp                  /* restore user RSP */
    sysretq
.size syscall_entry, .-syscall_entry

.section .note.GNU-stack,"",@progbits
```

## `docs/architecture.md`

```markdown
# Architecture at Phase 9

- Target: x86-64, one bootstrap CPU, freestanding C plus x86-64 assembly.
- Limine v12.9.0 boots a higher-half ELF64 kernel.
- Serial output mirrors the framebuffer terminal/custom `kprintf()`.
- GDT now contains Ring-0 code/data, Ring-3 data/code, and a 64-bit TSS.
- The TSS `RSP0` is changed on task switches so Ring-3 interrupts land on that
  task's trusted kernel stack.
- A 256-entry IDT provides exception/interrupt entry; user page faults can kill
  only the offending Ring-3 task.
- The legacy 8259 PIC remains masked on q35.
- Local APIC supplies the 100 Hz periodic timer; I/O APIC routes PS/2 keyboard
  IRQ1.
- PMM manages usable RAM as 4 KiB physical frames with a bitmap.
- VMM owns the kernel PML4 and can create separate user PML4 roots.
- User CR3 roots have private lower-half mappings and shared supervisor-only
  kernel upper-half mappings.
- Kernel heap provides `kmalloc`/`kcalloc`/`krealloc`/`kfree`.
- Scheduler policy remains fixed five-tick preemptive round robin.
- The ISR dispatcher returns the exact saved frame/stack that assembly should
  restore, enabling true kernel-stack switching and Ring-0/Ring-3 transitions.
- Kernel threads use the kernel CR3; Ring-3 tasks each use their own CR3.
- Phase-9 userspace currently uses a tiny raw code image at `0x400000`, a data
  page at `0x401000`, and a four-page user stack near the top of the lower
  canonical half.
- Syscalls, ELF user executables and user libc remain future phases.

## Phase 10 syscall boundary

Ring-3 tasks now enter the kernel through architectural x86-64
`SYSCALL/SYSRETQ`. The entry code switches immediately from the untrusted user
stack to the current task's kernel stack. Pointer arguments are validated
against that task's user page tables before any copy occurs. `sleep()` and
`yield()` integrate with the Phase-8 scheduler, while user tasks retain their
Phase-9 isolated CR3 roots.
```

## `docs/phase10.md`

```markdown
# Phase 10 — x86-64 system calls

## Goal

Give Ring-3 programs a controlled way to request kernel services without being
allowed to call kernel functions or touch kernel memory directly.

AxiomOS Phase 10 uses the architectural 64-bit `SYSCALL` / `SYSRETQ` mechanism.
It does not use a DPL-3 `int 0x80` compatibility shortcut.

## Entry path

At boot, `syscall_init()` programs:

- `IA32_EFER.SCE` to enable SYSCALL/SYSRET;
- `IA32_STAR` with AxiomOS kernel/user GDT selector relationships;
- `IA32_LSTAR` with `syscall_entry`;
- `IA32_FMASK` so TF, IF, and DF are cleared on entry.

Unlike an interrupt gate, `SYSCALL` does not switch stacks. AxiomOS therefore
maintains the currently scheduled task's trusted kernel-stack top in
`syscall_kernel_stack_top`. The assembly entry stub moves off the user stack
before calling C.

```text
Ring 3
  |
  | syscall
  v
CPU -> CPL0, RCX=user RIP, R11=user RFLAGS
  |
  v
switch RSP to current task's kernel stack
  |
  v
save syscall frame
  |
  v
syscall_dispatch()
  |
  v
restore registers and user RSP
  |
  | sysretq
  v
Ring 3
```

The project is still single-CPU, so the tiny pre-stack-switch scratch slot is a
single-CPU implementation detail. SMP will require per-CPU syscall entry state.

## ABI

Phase 10 follows a Linux-like x86-64 register convention:

```text
RAX  syscall number
RDI  arg0
RSI  arg1
RDX  arg2
R10  arg3
R8   arg4
R9   arg5
RAX  return value
```

`RCX` and `R11` are clobbered by the CPU's SYSCALL/SYSRET mechanism.

Implemented calls:

| Number | Call | Phase-10 behavior |
|---:|---|---|
| 1 | `write(fd, buf, len)` | fd 1/2 -> kernel terminal |
| 2 | `read(fd, buf, len)` | fd 0 -> non-blocking keyboard buffer |
| 3 | `_exit(status)` | terminate current task |
| 4 | `sleep(ms)` | scheduler-backed sleeping state |
| 5 | `getpid()` | current task ID |
| 6 | `yield()` | force one round-robin reschedule |
| 7 | `open(path, flags)` | reserved; `-ENOSYS` until VFS |
| 8 | `close(fd)` | reserved; `-ENOSYS` until VFS |
| 9 | `fork()` | reserved; `-ENOSYS` |
| 10 | `exec()` | reserved; `-ENOSYS` |
| 11 | `mmap()` | reserved; `-ENOSYS` |

Errors are returned as negative integers (`-EBADF`, `-EFAULT`, `-EINVAL`,
`-ENOSYS`).

## User-pointer safety

The syscall layer never trusts a Ring-3 pointer. Phase 10 adds VMM helpers that
walk the task's page tables and require every page in a requested range to be:

- lower-half/canonical;
- present;
- user accessible;
- writable when the kernel intends to copy data into userspace.

After validation, bytes are copied through the HHDM using the translated
physical pages. The kernel does not simply dereference an arbitrary user
virtual address while running in Ring 0.

The demo intentionally calls `write()` with `0xFFFFFFFF80000000`. The syscall
must return `-EFAULT` instead of generating a kernel-mode page fault.

## Scheduler integration

Phase 10 turns `TASK_SLEEPING` into an active scheduler state. `sleep(ms)` marks
the current task sleeping until a future scheduling tick, then the timer wakes
it back to READY. `yield()` expires the task's quantum and waits until the
round-robin scheduler has switched away and eventually back.

A task inside a syscall can therefore be preempted safely on its own kernel
stack and later resume the syscall before `SYSRETQ` returns to Ring 3.

## Userspace wrapper shim

`userspace/phase10_program.S` contains the first libc-style wrappers:

```text
write()
read()
_exit()
sleep()
getpid()
yield()
open()
close()
```

`libc/include/axiom/unistd.h` records their C-facing signatures. This is not yet
the full userspace libc planned for Phase 19; it establishes the ABI that that
library will later wrap normally.

## Phase-10 demo

The Ring-3 demo:

1. calls `getpid()`;
2. prints `Hello via SYS_write from Ring 3!`;
3. passes a kernel pointer to `write()` and expects `-EFAULT`;
4. calls `open()` and expects `-ENOSYS` because no VFS exists yet;
5. calls `yield()`;
6. sleeps for 30 ms;
7. polls `read()` for a bounded period;
8. if a key arrives, prints `Phase 10 userspace read: <key>`;
9. exits with status 37.

The read wait is deliberately short and bounded so older regression tests can still boot the newest
kernel without requiring manual input. `make test-phase10` injects a real QEMU
keyboard event and requires the user task to read it through `SYS_read`.

## Acceptance

Run:

```bash
make clean
make
make test-phase10
make test
```

Phase 10 is accepted only after the dedicated syscall test and the full Phase
1-10 regression suite pass under QEMU.
```

## `docs/processes.md`

```markdown
# Tasks, scheduling, and userspace — Phase 9

AxiomOS uses a preemptive round-robin scheduler driven by the 100 Hz Local APIC
timer. The default quantum remains five ticks (about 50 ms).

Each `struct task` now records:

- task ID and name;
- RUNNING/READY/BLOCKED/SLEEPING/TERMINATED state;
- Ring-0 or Ring-3 privilege;
- pointer to the saved hardware/ISR CPU frame;
- private kernel stack and kernel-stack top;
- CR3/PML4 address-space root;
- kernel-thread entry/argument where applicable;
- user code/data/stack physical pages where applicable;
- user entry point and user stack top;
- fault vector/error/address diagnostics;
- quantum/runtime/context-switch counters.

Kernel threads share the kernel CR3. Ring-3 tasks each receive a separate CR3
whose lower half is private and whose higher half shares supervisor-only kernel
mappings.

The common ISR epilogue restores the frame pointer returned by the scheduler.
This gives AxiomOS real stack switching for kernel threads and makes Ring-0 to
Ring-3 `IRETQ` transitions possible.

When a Ring-3 page fault attempts to cross the kernel boundary, AxiomOS
terminates only the faulting task and schedules another runnable task.

## Phase 10 scheduler interaction

`TASK_SLEEPING` is now functional. A sleeping task records a wake scheduling
tick; the APIC timer promotes it back to READY after the deadline. The syscall
layer uses this for `sleep(ms)`. `yield()` expires the current quantum and waits
for a real round-robin switch before returning to userspace. User `_exit()`
records an exit status and leaves the task TERMINATED.
```

## `docs/project-state.md`

```markdown
# AxiomOS handoff — Phase 10 implementation

Development environment: Linux/Distrobox on Steam Deck. Run commands from the
directory containing `Makefile`. Roadmap: Phase 0 through Phase 25.

Accepted foundation before Phase 10:

- Phase 0: development environment.
- Phase 1: bootable x86-64 kernel and serial output.
- Phase 2: framebuffer terminal and custom formatter.
- Phase 3: GDT/TSS, IDT, exceptions/interrupts and panic diagnostics.
- Phase 4: 4 KiB bitmap physical page allocator.
- Phase 5: AxiomOS-owned page tables, mappings and page faults.
- Phase 6: PMM/VMM-backed first-fit/coalescing kernel heap.
- Phase 7: q35 Local APIC 100 Hz timer + I/O-APIC PS/2 keyboard.
- Phase 8: preemptive round-robin scheduler with true task-stack switching.
- Phase 9: Ring-3 tasks, separate CR3 roots and hardware kernel isolation.

Phase 10 adds:

- architectural x86-64 `SYSCALL` / `SYSRETQ` entry and return;
- MSR setup for EFER/STAR/LSTAR/FMASK;
- per-task trusted syscall kernel-stack selection;
- Linux-like register syscall ABI;
- `SYS_write`, `SYS_read`, `SYS_exit`, `SYS_sleep`, `SYS_getpid`, `SYS_yield`;
- reserved `open/close/fork/exec/mmap` numbers returning `-ENOSYS`;
- negative errno-style return values;
- VMM user-range permission validation and safe user-copy helpers;
- kernel-pointer rejection with `-EFAULT` rather than a kernel #PF;
- working scheduler `TASK_SLEEPING` wakeups and explicit yield;
- minimal userspace libc-style assembly wrappers;
- Ring-3 syscall demo that prints, sleeps, reads keyboard input and exits;
- `tests/phase10_syscalls.py`, including QEMU keyboard injection.

Important limitations:

- single CPU only; syscall entry scratch state is not SMP-ready yet;
- raw one-page user images, no ELF loader until Phase 11;
- `read()` is non-blocking at the ABI layer; the demo polls with `sleep()`;
- no VFS yet, so open/close are intentionally `-ENOSYS`;
- no fork/exec/mmap implementation yet;
- no descriptor table or per-process file table yet;
- no normal terminated-task resource reaper yet;
- no full userspace libc yet (Phase 19).

Acceptance commands:

```bash
make clean
make
make test-phase9
make test-phase10
make test
```

Next milestone: Phase 11 — ELF64 executable loader.
```

## `docs/syscalls.md`

```markdown
# AxiomOS syscall ABI

Phase 10 introduces the first kernel/userspace service boundary using x86-64
`SYSCALL` / `SYSRETQ`.

The stable call numbers are declared in `include/axiom/abi/syscall.h`. Arguments
use `RDI, RSI, RDX, R10, R8, R9`; `RAX` contains the syscall number on entry and
the signed result on return.

Implemented now: `write`, `read`, `_exit`, `sleep`, `getpid`, and `yield`.
`open`, `close`, `fork`, `exec`, and `mmap` already have reserved ABI numbers
but intentionally return `-ENOSYS` until the owning filesystem/process/memory
phases exist.

All pointer-bearing calls validate Ring-3 ranges through the VMM before copying
bytes. A bad user pointer must become `-EFAULT`, not a Ring-0 page fault.

See [Phase 10](phase10.md) for the entry path, register ABI, validation model,
and acceptance tests.
```

## `docs/validation.md`

```markdown
# Validation status — Phase 10

Static validation performed before packaging:

- every C translation unit compiles with the project's freestanding Clang flags
  and `-Wall -Wextra -Werror`;
- all existing deliberate build modes compile: normal, divide, invalid, gp,
  double_fault, page_fault, heap_double_free, heap_guard;
- `syscall_entry.S`, the Phase-9 image and the Phase-10 image assemble with the
  freestanding target;
- the new C/GAS objects complete a relocatable LLD link;
- the Phase-10 user image is below the one-page (4 KiB) raw-image limit;
- all Python tests byte-compile and shell tests pass `bash -n`.

Runtime acceptance must be performed in the user's development container:

```bash
make clean
make
make test-phase10
make test
```

`make test-phase10` must prove real `SYSCALL/SYSRETQ` transitions, `SYS_write`,
keyboard-backed `SYS_read`, sleeping/yielding, `getpid`, exit status, `-EFAULT`
for a kernel pointer, `-ENOSYS` for not-yet-owned subsystems, and keyboard
coexistence after the syscall demo.
```

## `include/axiom/abi/syscall.h`

```c
#ifndef AXIOM_ABI_SYSCALL_H
#define AXIOM_ABI_SYSCALL_H

/* Stable Phase-10 syscall numbers shared by kernel and userspace. */
#define AXIOM_SYS_WRITE   1
#define AXIOM_SYS_READ    2
#define AXIOM_SYS_EXIT    3
#define AXIOM_SYS_SLEEP   4
#define AXIOM_SYS_GETPID  5
#define AXIOM_SYS_YIELD   6
#define AXIOM_SYS_OPEN    7
#define AXIOM_SYS_CLOSE   8
#define AXIOM_SYS_FORK    9
#define AXIOM_SYS_EXEC    10
#define AXIOM_SYS_MMAP    11

#define AXIOM_SYSCALL_MAX_NUMBER AXIOM_SYS_MMAP

/* Small errno subset; syscalls return the negative value on failure. */
#define AXIOM_EBADF   9
#define AXIOM_EFAULT  14
#define AXIOM_EINVAL  22
#define AXIOM_ENOSYS  38

#endif
```

## `include/axiom/abi/user_layout.h`

```c
#ifndef AXIOM_ABI_USER_LAYOUT_H
#define AXIOM_ABI_USER_LAYOUT_H

#define AXIOM_USER_CODE_BASE  0x0000000000400000
#define AXIOM_USER_DATA_BASE  0x0000000000401000
#define AXIOM_USER_STACK_TOP  0x00007FFFFFF00000

#define AXIOM_PHASE10_PID_OFFSET           0
#define AXIOM_PHASE10_BAD_WRITE_OFFSET     8
#define AXIOM_PHASE10_OPEN_RESULT_OFFSET   16
#define AXIOM_PHASE10_READ_CHAR_OFFSET     24
#define AXIOM_PHASE10_MAGIC_OFFSET         128
#define AXIOM_PHASE10_MAGIC                0x4158494F4D533130
#define AXIOM_PHASE10_EXIT_STATUS           37

#endif
```

## `include/axiom/kernel/syscall.h`

```c
#ifndef AXIOM_KERNEL_SYSCALL_H
#define AXIOM_KERNEL_SYSCALL_H

#include <stddef.h>
#include <stdint.h>

struct syscall_frame {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t user_rip;
    uint64_t user_rflags;
    uint64_t user_rsp;
};

_Static_assert(sizeof(struct syscall_frame) == 128, "syscall frame size");
_Static_assert(offsetof(struct syscall_frame, rax) == 96, "syscall RAX offset");
_Static_assert(offsetof(struct syscall_frame, user_rip) == 104, "syscall RIP offset");
_Static_assert(offsetof(struct syscall_frame, user_rsp) == 120, "syscall RSP offset");

struct syscall_stats {
    uint64_t total_calls;
    uint64_t write_calls;
    uint64_t read_calls;
    uint64_t exit_calls;
    uint64_t sleep_calls;
    uint64_t getpid_calls;
    uint64_t yield_calls;
    uint64_t rejected_pointers;
    uint64_t unimplemented_calls;
    uint64_t bytes_written;
    uint64_t bytes_read;
};

int syscall_init(void);
int syscall_initialized(void);
void syscall_set_kernel_stack(uintptr_t stack_top);
void syscall_dispatch(struct syscall_frame *frame);
struct syscall_stats syscall_get_stats(void);

#endif
```

## `include/axiom/memory/vmm.h`

```c
#ifndef AXIOM_MEMORY_VMM_H
#define AXIOM_MEMORY_VMM_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/memory/address.h>

#define VMM_PAGE_SIZE 4096ULL

/* Hardware-compatible leaf mapping flags exposed by the paging API. */
#define VMM_FLAG_WRITABLE      (1ULL << 1)
#define VMM_FLAG_USER          (1ULL << 2)
#define VMM_FLAG_WRITE_THROUGH (1ULL << 3)
#define VMM_FLAG_CACHE_DISABLE (1ULL << 4)
#define VMM_FLAG_GLOBAL        (1ULL << 8)
#define VMM_FLAG_NO_EXECUTE    (1ULL << 63)

/* Reserved lower-half virtual addresses used only by Phase-5 validation. */
#define VMM_SELFTEST_ADDRESS   0x0000600000000000ULL
#define VMM_FAULT_TEST_ADDRESS 0x0000612345600000ULL

struct vmm_stats {
    paddr_t root_table;
    uint64_t page_table_pages;
    uint64_t hhdm_offset;
};

int vmm_init(void);

/* Active-kernel-address-space compatibility API from Phase 5. */
int map_page(vaddr_t virtual_address, paddr_t physical_address, uint64_t flags);
int unmap_page(vaddr_t virtual_address);
paddr_t virt_to_phys(vaddr_t virtual_address);

/* Phase 9: isolated lower-half address spaces sharing supervisor kernel maps. */
paddr_t vmm_create_user_address_space(void);
int vmm_destroy_user_address_space(paddr_t root_table);
int vmm_map_page_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address,
    paddr_t physical_address,
    uint64_t flags
);
paddr_t vmm_virt_to_phys_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address
);
int vmm_activate_address_space(paddr_t root_table);
paddr_t vmm_current_address_space(void);
paddr_t vmm_kernel_address_space(void);

/* Phase 10: validate and copy user buffers without dereferencing them in Ring 0. */
int vmm_user_range_accessible(
    paddr_t root_table,
    vaddr_t user_address,
    size_t length,
    int write_access
);
int vmm_copy_from_user(
    paddr_t root_table,
    void *destination,
    vaddr_t user_source,
    size_t length
);
int vmm_copy_to_user(
    paddr_t root_table,
    vaddr_t user_destination,
    const void *source,
    size_t length
);

struct vmm_stats vmm_get_stats(void);
int phase5_vmm_selftest(void);

#endif
```

## `include/axiom/process/scheduler.h`

```c
#ifndef AXIOM_PROCESS_SCHEDULER_H
#define AXIOM_PROCESS_SCHEDULER_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/interrupts.h>
#include <axiom/process/task.h>

#define SCHEDULER_MAX_TASKS 8u
#define SCHEDULER_TASK_STACK_SIZE (16u * 1024u)
#define SCHEDULER_DEFAULT_QUANTUM_TICKS 5u

struct scheduler_stats {
    uint64_t task_count;
    uint64_t runnable_tasks;
    uint64_t context_switches;
    uint64_t preemptions;
    uint64_t scheduling_ticks;
    uint64_t user_fault_terminations;
    uint64_t current_task_id;
};

int scheduler_init(void);

int task_create(
    const char *name,
    task_entry_t entry,
    void *argument,
    uint64_t *task_id_out
);

/* Create a Ring-3 task in its own address space from a <=4 KiB code image. */
int user_task_create(
    const char *name,
    const void *image,
    size_t image_size,
    uint64_t *task_id_out
);

int scheduler_start(void);
int scheduler_running(void);

/* Return the interrupt frame that the assembly epilogue should restore. */
struct interrupt_frame *scheduler_on_timer_interrupt(
    struct interrupt_frame *frame
);

/* Kill a faulting Ring-3 task and return the next runnable task's frame. */
struct interrupt_frame *scheduler_handle_user_fault(
    struct interrupt_frame *frame,
    uint64_t vector,
    uint64_t error_code,
    vaddr_t fault_address
);

int scheduler_sleep_current(uint64_t ticks);
int scheduler_yield_current(void);
_Noreturn void task_exit_current(int64_t status);

const struct task *scheduler_current_task(void);

struct scheduler_stats scheduler_get_stats(void);
size_t scheduler_task_count(void);
const struct task *scheduler_task_at(size_t index);
const struct task *scheduler_task_by_id(uint64_t id);

#endif
```

## `include/axiom/process/task.h`

```c
#ifndef AXIOM_PROCESS_TASK_H
#define AXIOM_PROCESS_TASK_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/user_layout.h>
#include <axiom/arch/interrupts.h>
#include <axiom/memory/address.h>

#define TASK_USER_CODE_BASE   ((vaddr_t)AXIOM_USER_CODE_BASE)
#define TASK_USER_DATA_BASE   ((vaddr_t)AXIOM_USER_DATA_BASE)
#define TASK_USER_STACK_TOP   ((vaddr_t)AXIOM_USER_STACK_TOP)
#define TASK_USER_STACK_PAGES 4u

#define TASK_USER_MESSAGE_MAGIC_OFFSET 64u
#define TASK_USER_MESSAGE_MAGIC 0x4158494F4D555345ULL

typedef void (*task_entry_t)(void *argument);

enum task_state {
    TASK_RUNNING = 0,
    TASK_READY,
    TASK_BLOCKED,
    TASK_SLEEPING,
    TASK_TERMINATED,
};

enum task_privilege {
    TASK_PRIVILEGE_KERNEL = 0,
    TASK_PRIVILEGE_USER = 3,
};

struct task {
    uint64_t id;
    const char *name;
    enum task_state state;
    enum task_privilege privilege;

    /* Actual frame restored by the common ISR epilogue. */
    struct interrupt_frame *saved_frame;

    void *kernel_stack_base;
    size_t kernel_stack_size;
    uintptr_t kernel_stack_top;
    paddr_t address_space;

    task_entry_t entry;
    void *argument;

    paddr_t user_code_page;
    paddr_t user_data_page;
    paddr_t user_stack_pages[TASK_USER_STACK_PAGES];
    vaddr_t user_entry;
    vaddr_t user_stack_top;

    uint64_t fault_vector;
    uint64_t fault_error_code;
    vaddr_t fault_address;
    int64_t exit_code;
    uint64_t wake_tick;

    uint64_t quantum_ticks;
    uint64_t ticks_in_slice;
    uint64_t runtime_ticks;
    uint64_t context_switches;
};

const char *task_state_name(enum task_state state);
const char *task_privilege_name(enum task_privilege privilege);

#endif
```

## `kernel/core/kernel.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/syscall.h>
#include <axiom/abi/user_layout.h>
#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>

#include <axiom/boot/limine.h>
#include <axiom/drivers/keyboard.h>
#include <axiom/drivers/serial.h>
#include <axiom/drivers/timer.h>
#include <axiom/kernel/syscall.h>
#include <axiom/memory/heap.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>
#include <axiom/process/scheduler.h>

#include <axiom/terminal/kprintf.h>
#include <axiom/terminal/terminal.h>



#define PHASE7_LINE_CAPACITY 128u
#define PHASE8_WORKER_TARGET 1000ULL
#define PHASE8_TEST_TIMEOUT_TICKS 300ULL
#define PHASE9_TEST_TIMEOUT_TICKS 500ULL
#define PHASE10_TEST_TIMEOUT_TICKS 1500ULL
#define PHASE9_KERNEL_PROBE_ADDRESS 0xFFFFFFFF80000000ULL

static volatile uint64_t phase8_worker_a_count;
static volatile uint64_t phase8_worker_b_count;


extern const uint8_t phase9_user_hello_start[];
extern const uint8_t phase9_user_hello_end[];
extern const uint8_t phase9_user_fault_start[];
extern const uint8_t phase9_user_fault_end[];
extern const uint8_t phase10_user_start[];
extern const uint8_t phase10_user_end[];

static int phase9_message_ready(const struct task *task, char *message, size_t capacity)
{
    const uint8_t *data;
    const uint64_t *magic;
    size_t index;

    if (task == 0 || task->user_data_page == PADDR_INVALID ||
        message == 0 || capacity == 0u) {
        return 0;
    }

    data = (const uint8_t *)pmm_phys_to_hhdm(task->user_data_page);
    if (data == 0) {
        return 0;
    }

    magic = (const uint64_t *)(const void *)(
        data + TASK_USER_MESSAGE_MAGIC_OFFSET
    );

    if (*magic != TASK_USER_MESSAGE_MAGIC) {
        return 0;
    }

    for (index = 0u; index + 1u < capacity; ++index) {
        message[index] = (char)data[index];

        if (message[index] == '\0') {
            return 1;
        }
    }

    message[capacity - 1u] = '\0';
    return 1;
}

static int phase10_results_ready(
    const struct task *task,
    uint64_t *pid_out,
    int64_t *bad_write_out,
    int64_t *open_result_out,
    char *read_character_out
)
{
    const uint8_t *data;
    uint64_t magic;

    if (task == 0 || task->user_data_page == PADDR_INVALID ||
        pid_out == 0 || bad_write_out == 0 || open_result_out == 0 ||
        read_character_out == 0) {
        return 0;
    }

    data = (const uint8_t *)pmm_phys_to_hhdm(task->user_data_page);
    if (data == 0) {
        return 0;
    }

    magic = *(const uint64_t *)(const void *)(
        data + AXIOM_PHASE10_MAGIC_OFFSET
    );

    if (magic != AXIOM_PHASE10_MAGIC) {
        return 0;
    }

    *pid_out = *(const uint64_t *)(const void *)(
        data + AXIOM_PHASE10_PID_OFFSET
    );
    *bad_write_out = *(const int64_t *)(const void *)(
        data + AXIOM_PHASE10_BAD_WRITE_OFFSET
    );
    *open_result_out = *(const int64_t *)(const void *)(
        data + AXIOM_PHASE10_OPEN_RESULT_OFFSET
    );
    *read_character_out = (char)data[AXIOM_PHASE10_READ_CHAR_OFFSET];
    return 1;
}


static void phase8_counter_worker(void *argument)
{
    volatile uint64_t *counter = (volatile uint64_t *)argument;

    for (;;) {
        ++(*counter);
        __asm__ volatile ("pause" ::: "memory");
    }
}


static _Noreturn void phase7_input_loop(void)
{
    char line[PHASE7_LINE_CAPACITY];
    size_t length = 0u;

    line[0] = '\0';

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );

    kprintf("Phase 7 input ready. Type into AxiomOS.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    kprintf("axiom> ");

    for (;;) {
        char character;
        int consumed = 0;

        while (keyboard_read_char(&character)) {
            consumed = 1;

            if (character == '\n') {
                const struct keyboard_stats stats = keyboard_get_stats();

                terminal_putchar('\n');
                line[length] = '\0';

                kprintf("Keyboard line: %s\n", line);
                kprintf(
                    "Keyboard IRQs/scancodes/chars/dropped: "
                    "%llu/%llu/%llu/%llu\n",
                    (unsigned long long)stats.irq_count,
                    (unsigned long long)stats.scancode_count,
                    (unsigned long long)stats.character_count,
                    (unsigned long long)stats.dropped_characters
                );

                length = 0u;
                line[0] = '\0';
                kprintf("axiom> ");
                continue;
            }

            if (character == '\b') {
                if (length != 0u) {
                    --length;
                    line[length] = '\0';
                    terminal_putchar('\b');
                }

                continue;
            }

            if (length + 1u < PHASE7_LINE_CAPACITY) {
                line[length++] = character;
                line[length] = '\0';
                terminal_putchar(character);
            }
        }

        if (!consumed) {
            /*
             * Sleep until a hardware interrupt arrives. The periodic APIC timer IRQ
             * also closes the tiny empty-buffer/HALT race.
             */
            __asm__ volatile ("hlt" ::: "memory");
        }
    }
}


void kernel_main(void)
{
    uint64_t line;
    uint64_t scroll_lines;
    struct pmm_stats memory_stats;


    /*
     * Phase-1 early debugging output remains available.
     */
    serial_init();


    if (!limine_base_revision_supported()) {
        serial_write_string(
            "AxiomOS boot error: "
            "unsupported Limine protocol revision.\n"
        );

        return;
    }


    /*
     * Keep this exact line so the Phase-1 regression test continues to work.
     */
    serial_write_string(
        "AxiomOS kernel booted successfully.\n"
    );


    /*
     * Initialise the Phase-2 framebuffer terminal.
     */
    if (!terminal_init()) {
        serial_write_string(
            "AxiomOS terminal error: "
            "framebuffer terminal unavailable.\n"
        );

        return;
    }


    /*
     * Exercise scrolling by printing slightly more than one screen of text.
     */
    scroll_lines =
        (uint64_t)terminal_rows()
        +
        3ULL;


    for (line = 1; line <= scroll_lines; ++line) {
        kprintf(
            "Scroll exercise line %llu\n",
            (unsigned long long)line
        );
    }


    terminal_clear();


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );


    kprintf(
        "AxiomOS Phase 2 terminal online.\n"
    );


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


    kprintf(
        "Terminal geometry: %llux%llu cells\n",
        (unsigned long long)terminal_columns(),
        (unsigned long long)terminal_rows()
    );


    kprintf(
        "Decimal test: %u\n",
        123456789u
    );


    kprintf(
        "Hex test: 0x%X\n",
        0xDEADBEEFu
    );


    kprintf(
        "64-bit test: %llu\n",
        18446744073709551615ULL
    );


    kprintf(
        "Kernel entry: %p\n",
        (void *)(uintptr_t)&kernel_main
    );


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );


    kprintf(
        "Color output: OK\n"
    );


    terminal_set_color(
        TERMINAL_COLOR_YELLOW,
        TERMINAL_COLOR_BLACK
    );


    kprintf(
        "Phase 2 terminal test complete.\n"
    );


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


    /*
     * Phase 3 CPU architecture initialization and regression tests.
     */
    gdt_init();
    interrupts_init();

    kprintf("GDT/TSS loaded. IDT: 256 gates installed.\n");
    kprintf("PIC remapped: IRQs masked; IF=0.\n");

    phase3_selftest();


    /*
     * Phase 4: discover usable physical RAM and initialise the 4 KiB page
     * allocator. Only LIMINE_MEMMAP_USABLE pages are managed at this stage.
     */
    if (!pmm_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        kprintf("Phase 4 PMM initialization: FAILED\n");
        return;
    }


    memory_stats = pmm_get_stats();


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );

    kprintf("AxiomOS Phase 4 physical memory manager online.\n");


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    kprintf(
        "PMM memory map entries: %llu\n",
        (unsigned long long)memory_stats.memory_map_entries
    );

    kprintf(
        "PMM total RAM-like memory: %llu MiB\n",
        (unsigned long long)(
            memory_stats.total_memory_bytes /
            (1024ULL * 1024ULL)
        )
    );

    kprintf(
        "PMM usable memory: %llu MiB (%llu pages)\n",
        (unsigned long long)(
            memory_stats.usable_memory_bytes /
            (1024ULL * 1024ULL)
        ),
        (unsigned long long)memory_stats.usable_pages
    );

    kprintf(
        "PMM metadata: %llu pages at physical 0x%llX\n",
        (unsigned long long)memory_stats.metadata_pages,
        (unsigned long long)memory_stats.metadata_base
    );

    kprintf(
        "PMM allocated pages: %llu\n",
        (unsigned long long)memory_stats.allocated_pages
    );

    kprintf(
        "PMM free pages: %llu\n",
        (unsigned long long)memory_stats.free_pages
    );


    if (!phase4_pmm_selftest()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        return;
    }


    memory_stats = pmm_get_stats();

    kprintf(
        "PMM post-test allocated pages: %llu\n",
        (unsigned long long)memory_stats.allocated_pages
    );

    kprintf(
        "PMM post-test free pages: %llu\n",
        (unsigned long long)memory_stats.free_pages
    );


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );

    kprintf("Phase 4 physical memory manager complete.\n");


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


    /*
     * Phase 5: clone Limine's active 4-level paging hierarchy into physical
     * pages owned by our PMM, switch CR3, and expose 4 KiB map/unmap/translate
     * primitives. Existing kernel/HHDM/framebuffer mappings are preserved by
     * the clone.
     */
    if (!vmm_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        kprintf("Phase 5 VMM initialization: FAILED\n");
        return;
    }


    {
        const struct vmm_stats virtual_memory = vmm_get_stats();
        const paddr_t kernel_main_physical =
            virt_to_phys((vaddr_t)(uintptr_t)&kernel_main);


        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );

        kprintf("AxiomOS Phase 5 virtual memory manager online.\n");


        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );

        kprintf(
            "VMM root PML4: physical 0x%llX\n",
            (unsigned long long)virtual_memory.root_table
        );

        kprintf(
            "VMM owned page-table pages: %llu\n",
            (unsigned long long)virtual_memory.page_table_pages
        );

        kprintf(
            "VMM HHDM offset: 0x%llX\n",
            (unsigned long long)virtual_memory.hhdm_offset
        );

        kprintf(
            "kernel_main physical address: 0x%llX\n",
            (unsigned long long)kernel_main_physical
        );


        if (kernel_main_physical == PADDR_INVALID) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );

            kprintf("Phase 5 kernel translation check: FAILED\n");
            return;
        }
    }


    if (!phase5_vmm_selftest()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        return;
    }


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );

    kprintf("Phase 5 virtual memory manager complete.\n");


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


#ifdef AXIOM_TEST_PAGE_FAULT
    kprintf("Test: triggering unmapped page read.\n");

    {
        volatile const uint64_t *fault_address =
            (volatile const uint64_t *)(uintptr_t)VMM_FAULT_TEST_ADDRESS;

        volatile uint64_t ignored = *fault_address;
        (void)ignored;
    }

    kprintf("Phase 5 page-fault test: FAILED to fault\n");
#endif

    /*
     * Phase 6: higher-half kernel heap backed by Phase-4 physical pages and
     * Phase-5 virtual mappings. The normal self-test leaves no live
     * allocations but may leave additional heap pages mapped for reuse.
     */
    if (!heap_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        kprintf("Phase 6 heap initialization: FAILED\n");
        return;
    }


    {
        const struct heap_stats initial_heap = heap_get_stats();


        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );

        kprintf("AxiomOS Phase 6 kernel heap online.\n");


        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );

        kprintf(
            "Heap base: 0x%llX\n",
            (unsigned long long)HEAP_BASE_ADDRESS
        );

        kprintf(
            "Heap mapped pages: %llu\n",
            (unsigned long long)initial_heap.mapped_pages
        );

        kprintf(
            "Heap initial free bytes: %llu\n",
            (unsigned long long)initial_heap.free_bytes
        );
    }


    if (!phase6_heap_selftest()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        return;
    }


    {
        const struct heap_stats final_heap = heap_get_stats();

        kprintf(
            "Heap mapped pages after test: %llu\n",
            (unsigned long long)final_heap.mapped_pages
        );

        kprintf(
            "Heap active allocations: %llu\n",
            (unsigned long long)final_heap.active_allocations
        );

        kprintf(
            "Heap bytes in use: %llu\n",
            (unsigned long long)final_heap.bytes_in_use
        );

        kprintf(
            "Heap free bytes: %llu\n",
            (unsigned long long)final_heap.free_bytes
        );

        kprintf(
            "Heap largest free block: %llu\n",
            (unsigned long long)final_heap.largest_free_block
        );

        kprintf(
            "Heap total allocations: %llu\n",
            (unsigned long long)final_heap.total_allocations
        );

        kprintf(
            "Heap total frees: %llu\n",
            (unsigned long long)final_heap.total_frees
        );

        kprintf(
            "Heap reallocations: %llu\n",
            (unsigned long long)final_heap.total_reallocations
        );

        kprintf(
            "Heap failed allocations: %llu\n",
            (unsigned long long)final_heap.failed_allocations
        );
    }


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );

    kprintf("Phase 6 kernel heap complete.\n");


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


#ifdef AXIOM_TEST_HEAP_DOUBLE_FREE
    {
        void *first = kmalloc(64);
        void *second = kmalloc(64);

        if (first == 0 || second == 0) {
            kprintf("Phase 6 double-free setup: FAILED\n");
            return;
        }

        kfree(first);
        kprintf("Test: triggering heap double free.\n");
        kfree(first);

        kprintf("Phase 6 double-free test: FAILED to panic\n");
        kfree(second);
    }
#endif


#ifdef AXIOM_TEST_HEAP_GUARD
    {
        uint8_t *buffer = (uint8_t *)kmalloc(32);

        if (buffer == 0) {
            kprintf("Phase 6 guard-corruption setup: FAILED\n");
            return;
        }

        buffer[32] = 0xA5u;

        kprintf("Test: triggering heap tail-guard corruption.\n");
        kfree(buffer);

        kprintf("Phase 6 guard-corruption test: FAILED to panic\n");
    }
#endif


    /*
     * Phase 7: IRQ-driven time and keyboard input. q35 is an APIC-era
     * platform, so the local APIC provides the periodic timer and the I/O
     * APIC routes the PS/2 keyboard. The legacy PIC remains fully masked.
     */
    if (!timer_init(100u)) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 7 timer initialization: FAILED\n");
        return;
    }

    if (!keyboard_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 7 keyboard initialization: FAILED\n");
        return;
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("AxiomOS Phase 7 timer + keyboard online.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );
    kprintf("APIC timer frequency: %u Hz\n", timer_frequency());
    kprintf("Interrupt controller: Local APIC + I/O APIC\n");

    interrupts_enable();

    if (!interrupts_enabled()) {
        kprintf("Phase 7 interrupt enable: FAILED\n");
        return;
    }

    {
        const uint64_t before = timer_ticks();
        uint64_t elapsed;

        timer_wait_ticks(10u);
        elapsed = timer_ticks() - before;

        if (elapsed < 10u) {
            kprintf("Phase 7 timer test: FAILED\n");
            return;
        }

        kprintf(
            "Timer self-test ticks: %llu\n",
            (unsigned long long)elapsed
        );
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 7 timer test: OK\n");
    kprintf("Phase 7 timer + keyboard drivers complete.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    /*
     * Phase 8: preemptive kernel-thread multitasking. Build every task while
     * IF=0 because task creation allocates private stacks from the Phase-6
     * heap, which is intentionally not interrupt-safe yet.
     */
    interrupts_disable();

    if (!scheduler_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 8 scheduler initialization: FAILED\n");
        return;
    }

    phase8_worker_a_count = 0u;
    phase8_worker_b_count = 0u;

    {
        uint64_t worker_a_id;
        uint64_t worker_b_id;

        if (!task_create(
                "phase8-worker-a",
                phase8_counter_worker,
                (void *)&phase8_worker_a_count,
                &worker_a_id
            ) ||
            !task_create(
                "phase8-worker-b",
                phase8_counter_worker,
                (void *)&phase8_worker_b_count,
                &worker_b_id
            )) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 8 task creation: FAILED\n");
            return;
        }

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 8 preemptive scheduler online.\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Scheduler policy: preemptive round-robin\n");
        kprintf(
            "Scheduler quantum: %u timer ticks\n",
            SCHEDULER_DEFAULT_QUANTUM_TICKS
        );
        kprintf(
            "Phase 8 worker task IDs: %llu, %llu\n",
            (unsigned long long)worker_a_id,
            (unsigned long long)worker_b_id
        );
    }

    if (!scheduler_start()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 8 scheduler start: FAILED\n");
        return;
    }

    interrupts_enable();

    {
        const uint64_t start = timer_ticks();

        while ((phase8_worker_a_count < PHASE8_WORKER_TARGET ||
                phase8_worker_b_count < PHASE8_WORKER_TARGET) &&
               (timer_ticks() - start) < PHASE8_TEST_TIMEOUT_TICKS) {
            /*
             * The bootstrap thread sleeps. Worker A and worker B never yield;
             * only timer preemption can move execution between all three.
             */
            __asm__ volatile ("hlt" ::: "memory");
        }
    }

    {
        const struct scheduler_stats scheduler = scheduler_get_stats();

        if (phase8_worker_a_count < PHASE8_WORKER_TARGET ||
            phase8_worker_b_count < PHASE8_WORKER_TARGET ||
            scheduler.context_switches < 3u ||
            scheduler.preemptions < 3u ||
            scheduler.task_count != 3u) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 8 preemptive multitasking test: FAILED\n");
            return;
        }

        kprintf(
            "Phase 8 task count: %llu\n",
            (unsigned long long)scheduler.task_count
        );
        kprintf(
            "Phase 8 worker A count: %llu\n",
            (unsigned long long)phase8_worker_a_count
        );
        kprintf(
            "Phase 8 worker B count: %llu\n",
            (unsigned long long)phase8_worker_b_count
        );
        kprintf(
            "Phase 8 context switches: %llu\n",
            (unsigned long long)scheduler.context_switches
        );
        kprintf(
            "Phase 8 timer preemptions: %llu\n",
            (unsigned long long)scheduler.preemptions
        );
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 8 preemptive multitasking test: OK\n");
    kprintf("Phase 8 multitasking complete.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    /*
     * Phase 9: create two Ring-3 tasks while interrupts are disabled. The first
     * writes a message into its own user data page. The second deliberately
     * reads a supervisor-only kernel address and must be killed by #PF.
     */
    interrupts_disable();

    {
        uint64_t hello_id;
        uint64_t fault_id;
        const size_t hello_size =
            (size_t)(phase9_user_hello_end - phase9_user_hello_start);
        const size_t fault_size =
            (size_t)(phase9_user_fault_end - phase9_user_fault_start);

        if (!user_task_create(
                "phase9-user-hello",
                phase9_user_hello_start,
                hello_size,
                &hello_id
            ) ||
            !user_task_create(
                "phase9-user-protection",
                phase9_user_fault_start,
                fault_size,
                &fault_id
            )) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 9 user task creation: FAILED\n");
            return;
        }

        {
            const struct task *hello = scheduler_task_by_id(hello_id);
            const struct task *fault = scheduler_task_by_id(fault_id);
            const paddr_t kernel_space = vmm_kernel_address_space();

            if (hello == 0 || fault == 0 ||
                hello->address_space == PADDR_INVALID ||
                fault->address_space == PADDR_INVALID ||
                hello->address_space == fault->address_space ||
                hello->address_space == kernel_space ||
                fault->address_space == kernel_space) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 9 address-space creation: FAILED\n");
                return;
            }

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_CYAN,
                TERMINAL_COLOR_BLACK
            );
            kprintf("AxiomOS Phase 9 Ring 3 userspace online.\n");

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_GREY,
                TERMINAL_COLOR_BLACK
            );
            kprintf(
                "Phase 9 user hello task ID: %llu\n",
                (unsigned long long)hello_id
            );
            kprintf(
                "Phase 9 protection task ID: %llu\n",
                (unsigned long long)fault_id
            );
            kprintf("Phase 9 user privilege: Ring 3\n");
            kprintf(
                "Phase 9 hello address space: 0x%llX\n",
                (unsigned long long)hello->address_space
            );
            kprintf(
                "Phase 9 protection address space: 0x%llX\n",
                (unsigned long long)fault->address_space
            );
            kprintf("Phase 9 separate address spaces: OK\n");
        }

        interrupts_enable();

        {
            const uint64_t start = timer_ticks();
            char user_message[64];
            int hello_ready = 0;
            int protection_ready = 0;

            user_message[0] = '\0';

            while ((timer_ticks() - start) < PHASE9_TEST_TIMEOUT_TICKS) {
                const struct task *hello = scheduler_task_by_id(hello_id);
                const struct task *fault = scheduler_task_by_id(fault_id);

                hello_ready = phase9_message_ready(
                    hello,
                    user_message,
                    sizeof(user_message)
                );

                protection_ready =
                    fault != 0 &&
                    fault->state == TASK_TERMINATED &&
                    fault->fault_vector == 14u &&
                    fault->fault_address == PHASE9_KERNEL_PROBE_ADDRESS &&
                    (fault->fault_error_code & 0x5ULL) == 0x5ULL;

                if (hello_ready && protection_ready) {
                    break;
                }

                __asm__ volatile ("hlt" ::: "memory");
            }

            if (!hello_ready || !protection_ready) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 9 userspace isolation test: FAILED\n");
                return;
            }

            {
                const struct task *fault = scheduler_task_by_id(fault_id);
                const struct scheduler_stats stats = scheduler_get_stats();

                kprintf("%s\n", user_message);
                kprintf("Phase 9 kernel protection fault: OK\n");
                kprintf(
                    "Phase 9 protection fault vector: %llu\n",
                    (unsigned long long)fault->fault_vector
                );
                kprintf(
                    "Phase 9 protection fault address: 0x%llX\n",
                    (unsigned long long)fault->fault_address
                );
                kprintf(
                    "Phase 9 protection fault error: 0x%llX\n",
                    (unsigned long long)fault->fault_error_code
                );
                kprintf(
                    "Phase 9 user fault terminations: %llu\n",
                    (unsigned long long)stats.user_fault_terminations
                );
            }
        }
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 9 userspace isolation test: OK\n");
    kprintf("Phase 9 userspace complete.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    /*
     * Phase 10: install the x86-64 SYSCALL/SYSRET ABI and launch a Ring-3
     * program that exercises write/read/getpid/sleep/yield/exit. open() is
     * intentionally reserved but returns -ENOSYS until the VFS exists.
     */
    interrupts_disable();

    if (!syscall_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 10 syscall initialization: FAILED\n");
        return;
    }

    {
        uint64_t syscall_task_id;
        const size_t image_size =
            (size_t)(phase10_user_end - phase10_user_start);

        if (!user_task_create(
                "phase10-syscall-demo",
                phase10_user_start,
                image_size,
                &syscall_task_id
            )) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 10 user task creation: FAILED\n");
            return;
        }

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 10 syscall interface online.\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Syscall mechanism: x86-64 SYSCALL/SYSRET\n");
        kprintf(
            "Phase 10 user task ID: %llu\n",
            (unsigned long long)syscall_task_id
        );
        kprintf("Phase 10 demo: type one key when userspace asks for input.\n");

        interrupts_enable();

        {
            const uint64_t start = timer_ticks();
            int results_ready = 0;
            uint64_t pid_result = 0u;
            int64_t bad_write_result = 0;
            int64_t open_result = 0;
            char read_character = '\0';

            while ((timer_ticks() - start) < PHASE10_TEST_TIMEOUT_TICKS) {
                const struct task *task = scheduler_task_by_id(syscall_task_id);

                results_ready = phase10_results_ready(
                    task,
                    &pid_result,
                    &bad_write_result,
                    &open_result,
                    &read_character
                );

                if (results_ready && task != 0 &&
                    task->state == TASK_TERMINATED) {
                    break;
                }

                __asm__ volatile ("hlt" ::: "memory");
            }

            {
                const struct task *task = scheduler_task_by_id(syscall_task_id);
                const struct syscall_stats syscall_stats = syscall_get_stats();

                if (!results_ready || task == 0 ||
                    task->state != TASK_TERMINATED ||
                    task->exit_code != AXIOM_PHASE10_EXIT_STATUS ||
                    pid_result != syscall_task_id ||
                    bad_write_result != -(int64_t)AXIOM_EFAULT ||
                    open_result != -(int64_t)AXIOM_ENOSYS ||
                    syscall_stats.write_calls < 2u ||
                    syscall_stats.read_calls < 1u ||
                    syscall_stats.sleep_calls < 1u ||
                    syscall_stats.getpid_calls < 1u ||
                    syscall_stats.yield_calls < 1u ||
                    syscall_stats.exit_calls < 1u ||
                    syscall_stats.rejected_pointers < 1u ||
                    syscall_stats.unimplemented_calls < 1u) {
                    terminal_set_color(
                        TERMINAL_COLOR_LIGHT_RED,
                        TERMINAL_COLOR_BLACK
                    );
                    kprintf("Phase 10 syscall self-test: FAILED\n");
                    return;
                }

                kprintf(
                    "Phase 10 getpid result: %llu\n",
                    (unsigned long long)pid_result
                );
                kprintf(
                    "Phase 10 invalid pointer result: %lld\n",
                    (long long)bad_write_result
                );
                kprintf(
                    "Phase 10 open result: %lld\n",
                    (long long)open_result
                );
                if (read_character != '\0') {
                    kprintf(
                        "Phase 10 read character: %c\n",
                        read_character
                    );
                } else {
                    kprintf("Phase 10 read character: <none>\n");
                }
                kprintf(
                    "Phase 10 exit status: %lld\n",
                    (long long)task->exit_code
                );
                kprintf(
                    "Phase 10 syscall calls: %llu\n",
                    (unsigned long long)syscall_stats.total_calls
                );
                kprintf(
                    "Phase 10 bytes written/read: %llu/%llu\n",
                    (unsigned long long)syscall_stats.bytes_written,
                    (unsigned long long)syscall_stats.bytes_read
                );
                kprintf(
                    "Phase 10 rejected user pointers: %llu\n",
                    (unsigned long long)syscall_stats.rejected_pointers
                );
                kprintf("Phase 10 user-copy validation: OK\n");
            }
        }
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 10 syscall self-test: OK\n");
    kprintf("Phase 10 system calls complete.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    /* Keep the interactive kernel input demonstration alive. */
    phase7_input_loop();
}
```

## `kernel/syscall/syscall.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/syscall.h>
#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/drivers/keyboard.h>
#include <axiom/drivers/timer.h>
#include <axiom/kernel/panic.h>
#include <axiom/kernel/syscall.h>
#include <axiom/memory/vmm.h>
#include <axiom/process/scheduler.h>
#include <axiom/terminal/terminal.h>

#define IA32_EFER  0xC0000080u
#define IA32_STAR  0xC0000081u
#define IA32_LSTAR 0xC0000082u
#define IA32_FMASK 0xC0000084u

#define EFER_SCE (1ULL << 0)
#define RFLAGS_TF (1ULL << 8)
#define RFLAGS_IF (1ULL << 9)
#define RFLAGS_DF (1ULL << 10)

#define SYSCALL_WRITE_MAX 4096u
#define SYSCALL_READ_MAX  4096u
#define SYSCALL_SLEEP_MAX_MS 60000ULL

/* Assembly reads this immediately after SYSCALL while IF is masked. */
uintptr_t syscall_kernel_stack_top;

static struct syscall_stats stats;
static int initialized;

extern void syscall_entry(void);

static uint64_t rdmsr(uint32_t msr)
{
    uint32_t low;
    uint32_t high;

    __asm__ volatile (
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(msr)
    );

    return ((uint64_t)high << 32) | low;
}

static void wrmsr(uint32_t msr, uint64_t value)
{
    __asm__ volatile (
        "wrmsr"
        :
        : "c"(msr),
          "a"((uint32_t)value),
          "d"((uint32_t)(value >> 32))
        : "memory"
    );
}

static int cpu_supports_syscall(void)
{
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;

    eax = 0x80000000u;
    __asm__ volatile (
        "cpuid"
        : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
    );

    if (eax < 0x80000001u) {
        return 0;
    }

    eax = 0x80000001u;
    __asm__ volatile (
        "cpuid"
        : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
    );

    return (edx & (1u << 11)) != 0u;
}

static int64_t syscall_error(int error)
{
    return -(int64_t)error;
}

static const struct task *current_user_task(void)
{
    const struct task *task = scheduler_current_task();

    if (task == 0 || task->privilege != TASK_PRIVILEGE_USER ||
        task->address_space == PADDR_INVALID) {
        return 0;
    }

    return task;
}

static int64_t sys_write(uint64_t fd, vaddr_t buffer, uint64_t count)
{
    const struct task *task = current_user_task();
    uint64_t offset = 0u;
    uint8_t local[128];

    ++stats.write_calls;

    if (fd != 1u && fd != 2u) {
        return syscall_error(AXIOM_EBADF);
    }

    if (count > SYSCALL_WRITE_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }

    if (count == 0u) {
        return 0;
    }

    if (task == 0 ||
        !vmm_user_range_accessible(task->address_space, buffer, (size_t)count, 0)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    while (offset < count) {
        const size_t chunk =
            (count - offset < sizeof(local)) ?
            (size_t)(count - offset) : sizeof(local);
        size_t index;

        if (!vmm_copy_from_user(
                task->address_space,
                local,
                buffer + offset,
                chunk
            )) {
            ++stats.rejected_pointers;
            return syscall_error(AXIOM_EFAULT);
        }

        for (index = 0u; index < chunk; ++index) {
            terminal_putchar((char)local[index]);
        }

        offset += chunk;
    }

    stats.bytes_written += count;
    return (int64_t)count;
}

static int64_t sys_read(uint64_t fd, vaddr_t buffer, uint64_t count)
{
    const struct task *task = current_user_task();
    uint64_t copied = 0u;

    ++stats.read_calls;

    if (fd != 0u) {
        return syscall_error(AXIOM_EBADF);
    }

    if (count > SYSCALL_READ_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }

    if (count == 0u) {
        return 0;
    }

    if (task == 0 ||
        !vmm_user_range_accessible(task->address_space, buffer, (size_t)count, 1)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    while (copied < count) {
        char character;

        if (!keyboard_read_char(&character)) {
            break;
        }

        if (!vmm_copy_to_user(
                task->address_space,
                buffer + copied,
                &character,
                1u
            )) {
            ++stats.rejected_pointers;
            return syscall_error(AXIOM_EFAULT);
        }

        ++copied;
    }

    stats.bytes_read += copied;
    return (int64_t)copied;
}

static int64_t sys_sleep(uint64_t milliseconds)
{
    uint64_t ticks;
    const uint64_t frequency = timer_frequency();

    ++stats.sleep_calls;

    if (milliseconds > SYSCALL_SLEEP_MAX_MS || frequency == 0u) {
        return syscall_error(AXIOM_EINVAL);
    }

    if (milliseconds == 0u) {
        return 0;
    }

    if (milliseconds > (UINT64_MAX - 999ULL) / frequency) {
        return syscall_error(AXIOM_EINVAL);
    }

    ticks = (milliseconds * frequency + 999ULL) / 1000ULL;
    if (ticks == 0u) {
        ticks = 1u;
    }

    return scheduler_sleep_current(ticks) ? 0 : syscall_error(AXIOM_EINVAL);
}

static int64_t sys_getpid(void)
{
    const struct task *task = current_user_task();

    ++stats.getpid_calls;
    return task != 0 ? (int64_t)task->id : syscall_error(AXIOM_EINVAL);
}

static int64_t sys_yield(void)
{
    ++stats.yield_calls;
    return scheduler_yield_current() ? 0 : syscall_error(AXIOM_EINVAL);
}

int syscall_init(void)
{
    uint64_t efer;
    uint64_t star;

    if (initialized || interrupts_enabled() || !cpu_supports_syscall()) {
        return 0;
    }

    /*
     * SYSCALL: CS=0x08, SS=0x10.
     * SYSRET:  STAR.user+16 => 0x23 user code, STAR.user+8 => 0x1B user data.
     */
    star = ((uint64_t)0x10u << 48) | ((uint64_t)GDT_KERNEL_CODE << 32);

    efer = rdmsr(IA32_EFER);
    wrmsr(IA32_EFER, efer | EFER_SCE);
    wrmsr(IA32_STAR, star);
    wrmsr(IA32_LSTAR, (uint64_t)(uintptr_t)&syscall_entry);
    wrmsr(IA32_FMASK, RFLAGS_TF | RFLAGS_IF | RFLAGS_DF);

    syscall_kernel_stack_top = gdt_kernel_stack();

    stats.total_calls = 0u;
    stats.write_calls = 0u;
    stats.read_calls = 0u;
    stats.exit_calls = 0u;
    stats.sleep_calls = 0u;
    stats.getpid_calls = 0u;
    stats.yield_calls = 0u;
    stats.rejected_pointers = 0u;
    stats.unimplemented_calls = 0u;
    stats.bytes_written = 0u;
    stats.bytes_read = 0u;
    initialized = 1;
    return 1;
}

int syscall_initialized(void)
{
    return initialized;
}

void syscall_set_kernel_stack(uintptr_t stack_top)
{
    syscall_kernel_stack_top = stack_top;
}

void syscall_dispatch(struct syscall_frame *frame)
{
    int64_t result;

    if (!initialized || frame == 0) {
        return;
    }

    ++stats.total_calls;

    if (current_user_task() == 0) {
        kernel_panic("SYSCALL entered without a Ring-3 current task");
    }

    /* Never let SYSRETQ carry a malicious non-canonical user stack to CPL3. */
    if (frame->user_rip >= 0x0000800000000000ULL ||
        frame->user_rsp >= 0x0000800000000000ULL) {
        ++stats.rejected_pointers;
        task_exit_current(-(int64_t)AXIOM_EFAULT);
    }

    switch (frame->rax) {
        case AXIOM_SYS_WRITE:
            result = sys_write(frame->rdi, (vaddr_t)frame->rsi, frame->rdx);
            break;

        case AXIOM_SYS_READ:
            result = sys_read(frame->rdi, (vaddr_t)frame->rsi, frame->rdx);
            break;

        case AXIOM_SYS_EXIT:
            ++stats.exit_calls;
            task_exit_current((int64_t)frame->rdi);

        case AXIOM_SYS_SLEEP:
            result = sys_sleep(frame->rdi);
            break;

        case AXIOM_SYS_GETPID:
            result = sys_getpid();
            break;

        case AXIOM_SYS_YIELD:
            result = sys_yield();
            break;

        case AXIOM_SYS_OPEN:
        case AXIOM_SYS_CLOSE:
        case AXIOM_SYS_FORK:
        case AXIOM_SYS_EXEC:
        case AXIOM_SYS_MMAP:
        default:
            ++stats.unimplemented_calls;
            result = syscall_error(AXIOM_ENOSYS);
            break;
    }

    frame->rax = (uint64_t)result;
}

struct syscall_stats syscall_get_stats(void)
{
    return stats;
}
```

## `libc/include/axiom/unistd.h`

```c
#ifndef AXIOM_LIBC_UNISTD_H
#define AXIOM_LIBC_UNISTD_H

#include <stddef.h>
#include <stdint.h>

/*
 * Phase-10 userspace ABI. The initial demo supplies these wrappers in its
 * position-independent user image. A later userspace libc phase will turn
 * these declarations into a normal linkable library.
 */
long write(int fd, const void *buffer, size_t count);
long read(int fd, void *buffer, size_t count);
_Noreturn void _exit(int status);
long sleep(uint64_t milliseconds);
long getpid(void);
long yield(void);
long open(const char *path, int flags);
long close(int fd);

#endif
```

## `memory/vmm.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/boot/limine.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>

#define PAGE_ENTRY_PRESENT       (1ULL << 0)
#define PAGE_ENTRY_WRITABLE      (1ULL << 1)
#define PAGE_ENTRY_USER          (1ULL << 2)
#define PAGE_ENTRY_LARGE         (1ULL << 7)

#define PAGE_ADDRESS_MASK_4K     0x000FFFFFFFFFF000ULL
#define PAGE_ADDRESS_MASK_2M     0x000FFFFFFFE00000ULL
#define PAGE_ADDRESS_MASK_1G     0x000FFFFFC0000000ULL

#define PAGE_OFFSET_MASK_4K      (VMM_PAGE_SIZE - 1ULL)
#define PAGE_OFFSET_MASK_2M      ((1ULL << 21) - 1ULL)
#define PAGE_OFFSET_MASK_1G      ((1ULL << 30) - 1ULL)

#define PAGE_TABLE_ENTRIES       512ULL


static struct {
    paddr_t root_table;
    uint64_t hhdm_offset;
    uint64_t page_table_pages;
    int initialized;
} vmm;


static uint64_t pml4_index(vaddr_t address)
{
    return (address >> 39) & 0x1FFULL;
}


static uint64_t pdpt_index(vaddr_t address)
{
    return (address >> 30) & 0x1FFULL;
}


static uint64_t pd_index(vaddr_t address)
{
    return (address >> 21) & 0x1FFULL;
}


static uint64_t pt_index(vaddr_t address)
{
    return (address >> 12) & 0x1FFULL;
}


static int is_page_aligned(uint64_t address)
{
    return (address & PAGE_OFFSET_MASK_4K) == 0ULL;
}


static int is_canonical(vaddr_t address)
{
    const uint64_t upper = address >> 48;
    const uint64_t sign = (address >> 47) & 1ULL;

    return sign != 0ULL ? upper == 0xFFFFULL : upper == 0ULL;
}


static uint64_t read_cr3(void)
{
    uint64_t value;

    __asm__ volatile ("mov %%cr3, %0" : "=r"(value));
    return value;
}


static void write_cr3(paddr_t root)
{
    __asm__ volatile ("mov %0, %%cr3" : : "r"(root) : "memory");
}


static void invalidate_page(vaddr_t address)
{
    __asm__ volatile ("invlpg (%0)" : : "r"((uintptr_t)address) : "memory");
}


static uint64_t *raw_hhdm_table(paddr_t physical_address)
{
    if (physical_address == PADDR_INVALID ||
        UINT64_MAX - vmm.hhdm_offset < physical_address) {

        return 0;
    }

    return (uint64_t *)(uintptr_t)(vmm.hhdm_offset + physical_address);
}


static uint64_t *owned_table(paddr_t physical_address)
{
    return (uint64_t *)pmm_phys_to_hhdm(physical_address);
}


static void clear_table(uint64_t *table)
{
    uint64_t index;

    for (index = 0; index < PAGE_TABLE_ENTRIES; ++index) {
        table[index] = 0ULL;
    }
}


static int table_is_empty(const uint64_t *table)
{
    uint64_t index;

    for (index = 0; index < PAGE_TABLE_ENTRIES; ++index) {
        if ((table[index] & PAGE_ENTRY_PRESENT) != 0ULL) {
            return 0;
        }
    }

    return 1;
}


static int entry_is_large(uint64_t entry, unsigned level)
{
    return (level == 3U || level == 2U) &&
           (entry & PAGE_ENTRY_LARGE) != 0ULL;
}


static void free_owned_tree(paddr_t table_physical, unsigned level);
static paddr_t virt_to_phys_in_root(paddr_t root_table, vaddr_t virtual_address);


static paddr_t clone_table_recursive(paddr_t source_physical, unsigned level)
{
    uint64_t *source;
    uint64_t *destination;
    paddr_t destination_physical;
    uint64_t index;


    source = raw_hhdm_table(source_physical);

    if (source == 0) {
        return PADDR_INVALID;
    }


    destination_physical = pmm_alloc_page();

    if (destination_physical == PADDR_INVALID) {
        return PADDR_INVALID;
    }


    destination = owned_table(destination_physical);

    if (destination == 0) {
        (void)pmm_free_page(destination_physical);
        return PADDR_INVALID;
    }


    clear_table(destination);
    ++vmm.page_table_pages;


    for (index = 0; index < PAGE_TABLE_ENTRIES; ++index) {
        const uint64_t entry = source[index];


        if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
            destination[index] = entry;
            continue;
        }


        if (level == 1U || entry_is_large(entry, level)) {
            destination[index] = entry;
            continue;
        }


        {
            const paddr_t child_source = entry & PAGE_ADDRESS_MASK_4K;
            const paddr_t child_destination =
                clone_table_recursive(child_source, level - 1U);


            if (child_destination == PADDR_INVALID) {
                free_owned_tree(destination_physical, level);
                return PADDR_INVALID;
            }


            destination[index] =
                child_destination |
                (entry & ~PAGE_ADDRESS_MASK_4K);
        }
    }


    return destination_physical;
}


static void free_owned_tree(paddr_t table_physical, unsigned level)
{
    uint64_t *table;
    uint64_t index;


    table = owned_table(table_physical);

    if (table == 0) {
        return;
    }


    if (level > 1U) {
        for (index = 0; index < PAGE_TABLE_ENTRIES; ++index) {
            const uint64_t entry = table[index];


            if ((entry & PAGE_ENTRY_PRESENT) == 0ULL ||
                entry_is_large(entry, level)) {

                continue;
            }


            free_owned_tree(
                entry & PAGE_ADDRESS_MASK_4K,
                level - 1U
            );
        }
    }


    if (pmm_free_page(table_physical)) {
        if (vmm.page_table_pages > 0ULL) {
            --vmm.page_table_pages;
        }
    }
}


static int allocate_child_table(
    uint64_t *parent_entry,
    uint64_t leaf_flags,
    paddr_t *new_physical
)
{
    paddr_t physical;
    uint64_t *table;
    uint64_t flags;


    physical = pmm_alloc_page();

    if (physical == PADDR_INVALID) {
        return 0;
    }


    table = owned_table(physical);

    if (table == 0) {
        (void)pmm_free_page(physical);
        return 0;
    }


    clear_table(table);


    flags = PAGE_ENTRY_PRESENT | PAGE_ENTRY_WRITABLE;

    if ((leaf_flags & VMM_FLAG_USER) != 0ULL) {
        flags |= PAGE_ENTRY_USER;
    }


    *parent_entry = physical | flags;
    *new_physical = physical;
    ++vmm.page_table_pages;

    return 1;
}


static void rollback_new_tables(
    uint64_t **parent_entries,
    paddr_t *physical_pages,
    uint64_t count
)
{
    while (count > 0ULL) {
        --count;

        *parent_entries[count] = 0ULL;

        if (pmm_free_page(physical_pages[count]) &&
            vmm.page_table_pages > 0ULL) {

            --vmm.page_table_pages;
        }
    }
}


int vmm_init(void)
{
    struct limine_memmap_response *memory_map;
    uint64_t hhdm_offset;
    paddr_t source_root;
    paddr_t cloned_root;


    if (vmm.initialized) {
        return 1;
    }


    if (!limine_get_memory_boot_info(&memory_map, &hhdm_offset)) {
        return 0;
    }


    if (memory_map == 0) {
        return 0;
    }


    vmm.hhdm_offset = hhdm_offset;
    vmm.page_table_pages = 0ULL;


    source_root = read_cr3() & PAGE_ADDRESS_MASK_4K;

    if (source_root == 0ULL) {
        return 0;
    }


    cloned_root = clone_table_recursive(source_root, 4U);

    if (cloned_root == PADDR_INVALID) {
        return 0;
    }


    vmm.root_table = cloned_root;

    /*
     * The cloned hierarchy contains all current kernel, HHDM and framebuffer
     * mappings. Switching CR3 therefore preserves the currently executing
     * instruction stream and stack while making AxiomOS the page-table owner.
     */
    write_cr3(vmm.root_table);


    if ((read_cr3() & PAGE_ADDRESS_MASK_4K) != vmm.root_table) {
        write_cr3(source_root);
        free_owned_tree(vmm.root_table, 4U);
        vmm.root_table = PADDR_INVALID;
        return 0;
    }


    vmm.initialized = 1;
    return 1;
}


static int map_page_in_root(
    paddr_t root_table,
    vaddr_t virtual_address,
    paddr_t physical_address,
    uint64_t flags
)
{
    uint64_t *pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;

    uint64_t *parent_entries[3];
    paddr_t new_tables[3];
    uint64_t *user_entries[3];
    uint64_t created;
    uint64_t user_entry_count;

    uint64_t index4;
    uint64_t index3;
    uint64_t index2;
    uint64_t index1;


    if (!vmm.initialized ||
        !is_page_aligned(virtual_address) ||
        !is_page_aligned(physical_address) ||
        physical_address == PADDR_INVALID ||
        (physical_address & ~PAGE_ADDRESS_MASK_4K) != 0ULL ||
        !is_canonical(virtual_address)) {

        return 0;
    }


    index4 = pml4_index(virtual_address);
    index3 = pdpt_index(virtual_address);
    index2 = pd_index(virtual_address);
    index1 = pt_index(virtual_address);

    created = 0ULL;
    user_entry_count = 0ULL;


    pml4 = owned_table(root_table);

    if (pml4 == 0) {
        return 0;
    }


    if ((pml4[index4] & PAGE_ENTRY_PRESENT) == 0ULL) {
        parent_entries[created] = &pml4[index4];

        if (!allocate_child_table(
                &pml4[index4],
                flags,
                &new_tables[created])) {

            return 0;
        }

        ++created;
    } else if ((flags & VMM_FLAG_USER) != 0ULL) {
        user_entries[user_entry_count++] = &pml4[index4];
    }


    pdpt = owned_table(pml4[index4] & PAGE_ADDRESS_MASK_4K);

    if (pdpt == 0) {
        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    if ((pdpt[index3] & PAGE_ENTRY_PRESENT) != 0ULL &&
        (pdpt[index3] & PAGE_ENTRY_LARGE) != 0ULL) {

        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    if ((pdpt[index3] & PAGE_ENTRY_PRESENT) == 0ULL) {
        parent_entries[created] = &pdpt[index3];

        if (!allocate_child_table(
                &pdpt[index3],
                flags,
                &new_tables[created])) {

            rollback_new_tables(parent_entries, new_tables, created);
            return 0;
        }

        ++created;
    } else if ((flags & VMM_FLAG_USER) != 0ULL) {
        user_entries[user_entry_count++] = &pdpt[index3];
    }


    pd = owned_table(pdpt[index3] & PAGE_ADDRESS_MASK_4K);

    if (pd == 0) {
        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    if ((pd[index2] & PAGE_ENTRY_PRESENT) != 0ULL &&
        (pd[index2] & PAGE_ENTRY_LARGE) != 0ULL) {

        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    if ((pd[index2] & PAGE_ENTRY_PRESENT) == 0ULL) {
        parent_entries[created] = &pd[index2];

        if (!allocate_child_table(
                &pd[index2],
                flags,
                &new_tables[created])) {

            rollback_new_tables(parent_entries, new_tables, created);
            return 0;
        }

        ++created;
    } else if ((flags & VMM_FLAG_USER) != 0ULL) {
        user_entries[user_entry_count++] = &pd[index2];
    }


    pt = owned_table(pd[index2] & PAGE_ADDRESS_MASK_4K);

    if (pt == 0) {
        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    if ((pt[index1] & PAGE_ENTRY_PRESENT) != 0ULL) {
        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    pt[index1] =
        physical_address |
        PAGE_ENTRY_PRESENT |
        (
            flags & (
                VMM_FLAG_WRITABLE |
                VMM_FLAG_USER |
                VMM_FLAG_WRITE_THROUGH |
                VMM_FLAG_CACHE_DISABLE |
                VMM_FLAG_GLOBAL |
                VMM_FLAG_NO_EXECUTE
            )
        );


    while (user_entry_count > 0ULL) {
        --user_entry_count;
        *user_entries[user_entry_count] |= PAGE_ENTRY_USER;
    }


    if ((read_cr3() & PAGE_ADDRESS_MASK_4K) == root_table) {
        invalidate_page(virtual_address);
    }

    return 1;
}




int map_page(
    vaddr_t virtual_address,
    paddr_t physical_address,
    uint64_t flags
)
{
    return map_page_in_root(
        vmm.root_table,
        virtual_address,
        physical_address,
        flags
    );
}

int vmm_map_page_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address,
    paddr_t physical_address,
    uint64_t flags
)
{
    if (!vmm.initialized || root_table == PADDR_INVALID) {
        return 0;
    }

    /* Non-kernel roots may only receive user-accessible lower-half mappings. */
    if (root_table != vmm.root_table) {
        if (virtual_address >= 0x0000800000000000ULL ||
            (flags & VMM_FLAG_USER) == 0ULL) {
            return 0;
        }
    }

    return map_page_in_root(
        root_table,
        virtual_address,
        physical_address,
        flags
    );
}

paddr_t vmm_create_user_address_space(void)
{
    uint64_t *kernel_root;
    uint64_t *user_root;
    paddr_t root_physical;
    uint64_t index;

    if (!vmm.initialized) {
        return PADDR_INVALID;
    }

    root_physical = pmm_alloc_page();
    if (root_physical == PADDR_INVALID) {
        return PADDR_INVALID;
    }

    user_root = owned_table(root_physical);
    kernel_root = owned_table(vmm.root_table);

    if (user_root == 0 || kernel_root == 0) {
        (void)pmm_free_page(root_physical);
        return PADDR_INVALID;
    }

    clear_table(user_root);

    /*
     * Keep the lower canonical half empty for the process. The upper half is
     * shared with the kernel, but PML4 USER is forcibly clear so Ring 3 cannot
     * traverse into any kernel/HHDM/heap/MMIO mapping.
     */
    for (index = 256ULL; index < PAGE_TABLE_ENTRIES; ++index) {
        user_root[index] = kernel_root[index] & ~PAGE_ENTRY_USER;
    }

    ++vmm.page_table_pages;
    return root_physical;
}


static void free_user_table_tree(paddr_t table_physical, unsigned level)
{
    uint64_t *table = owned_table(table_physical);
    uint64_t index;

    if (table == 0) {
        return;
    }

    if (level > 1U) {
        for (index = 0ULL; index < PAGE_TABLE_ENTRIES; ++index) {
            const uint64_t entry = table[index];

            if ((entry & PAGE_ENTRY_PRESENT) == 0ULL ||
                entry_is_large(entry, level)) {
                continue;
            }

            free_user_table_tree(entry & PAGE_ADDRESS_MASK_4K, level - 1U);
        }
    }

    if (pmm_free_page(table_physical) && vmm.page_table_pages > 0ULL) {
        --vmm.page_table_pages;
    }
}

int vmm_destroy_user_address_space(paddr_t root_table)
{
    uint64_t *root;
    uint64_t index;

    if (!vmm.initialized || root_table == PADDR_INVALID ||
        root_table == vmm.root_table ||
        vmm_current_address_space() == root_table) {
        return 0;
    }

    root = owned_table(root_table);
    if (root == 0) {
        return 0;
    }

    /* Only lower-half structures belong exclusively to this user space. */
    for (index = 0ULL; index < 256ULL; ++index) {
        const uint64_t entry = root[index];

        if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
            continue;
        }

        if ((entry & PAGE_ENTRY_LARGE) != 0ULL) {
            continue;
        }

        free_user_table_tree(entry & PAGE_ADDRESS_MASK_4K, 3U);
        root[index] = 0ULL;
    }

    if (!pmm_free_page(root_table)) {
        return 0;
    }

    if (vmm.page_table_pages > 0ULL) {
        --vmm.page_table_pages;
    }

    return 1;
}

int vmm_activate_address_space(paddr_t root_table)
{
    if (!vmm.initialized || root_table == PADDR_INVALID ||
        !is_page_aligned(root_table) || owned_table(root_table) == 0) {
        return 0;
    }

    write_cr3(root_table);
    return (read_cr3() & PAGE_ADDRESS_MASK_4K) == root_table;
}

paddr_t vmm_current_address_space(void)
{
    if (!vmm.initialized) {
        return PADDR_INVALID;
    }

    return read_cr3() & PAGE_ADDRESS_MASK_4K;
}

paddr_t vmm_kernel_address_space(void)
{
    return vmm.initialized ? vmm.root_table : PADDR_INVALID;
}

paddr_t virt_to_phys(vaddr_t virtual_address)
{
    return virt_to_phys_in_root(vmm.root_table, virtual_address);
}

paddr_t vmm_virt_to_phys_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address
)
{
    if (!vmm.initialized || root_table == PADDR_INVALID) {
        return PADDR_INVALID;
    }

    return virt_to_phys_in_root(root_table, virtual_address);
}

int unmap_page(vaddr_t virtual_address)
{
    uint64_t *pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;

    paddr_t pdpt_physical;
    paddr_t pd_physical;
    paddr_t pt_physical;

    const uint64_t index4 = pml4_index(virtual_address);
    const uint64_t index3 = pdpt_index(virtual_address);
    const uint64_t index2 = pd_index(virtual_address);
    const uint64_t index1 = pt_index(virtual_address);


    if (!vmm.initialized ||
        !is_page_aligned(virtual_address) ||
        !is_canonical(virtual_address)) {

        return 0;
    }


    pml4 = owned_table(vmm.root_table);

    if (pml4 == 0 || (pml4[index4] & PAGE_ENTRY_PRESENT) == 0ULL) {
        return 0;
    }


    pdpt_physical = pml4[index4] & PAGE_ADDRESS_MASK_4K;
    pdpt = owned_table(pdpt_physical);

    if (pdpt == 0 ||
        (pdpt[index3] & PAGE_ENTRY_PRESENT) == 0ULL ||
        (pdpt[index3] & PAGE_ENTRY_LARGE) != 0ULL) {

        return 0;
    }


    pd_physical = pdpt[index3] & PAGE_ADDRESS_MASK_4K;
    pd = owned_table(pd_physical);

    if (pd == 0 ||
        (pd[index2] & PAGE_ENTRY_PRESENT) == 0ULL ||
        (pd[index2] & PAGE_ENTRY_LARGE) != 0ULL) {

        return 0;
    }


    pt_physical = pd[index2] & PAGE_ADDRESS_MASK_4K;
    pt = owned_table(pt_physical);

    if (pt == 0 || (pt[index1] & PAGE_ENTRY_PRESENT) == 0ULL) {
        return 0;
    }


    pt[index1] = 0ULL;
    invalidate_page(virtual_address);


    if (table_is_empty(pt)) {
        pd[index2] = 0ULL;

        if (pmm_free_page(pt_physical) && vmm.page_table_pages > 0ULL) {
            --vmm.page_table_pages;
        }


        if (table_is_empty(pd)) {
            pdpt[index3] = 0ULL;

            if (pmm_free_page(pd_physical) && vmm.page_table_pages > 0ULL) {
                --vmm.page_table_pages;
            }


            if (table_is_empty(pdpt)) {
                pml4[index4] = 0ULL;

                if (pmm_free_page(pdpt_physical) &&
                    vmm.page_table_pages > 0ULL) {

                    --vmm.page_table_pages;
                }
            }
        }
    }


    return 1;
}


static paddr_t virt_to_phys_in_root(paddr_t root_table, vaddr_t virtual_address)
{
    uint64_t *pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;

    uint64_t entry;


    if (!vmm.initialized || !is_canonical(virtual_address)) {
        return PADDR_INVALID;
    }


    pml4 = owned_table(root_table);

    if (pml4 == 0) {
        return PADDR_INVALID;
    }


    entry = pml4[pml4_index(virtual_address)];

    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return PADDR_INVALID;
    }


    pdpt = owned_table(entry & PAGE_ADDRESS_MASK_4K);

    if (pdpt == 0) {
        return PADDR_INVALID;
    }


    entry = pdpt[pdpt_index(virtual_address)];

    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return PADDR_INVALID;
    }


    if ((entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return
            (entry & PAGE_ADDRESS_MASK_1G) |
            (virtual_address & PAGE_OFFSET_MASK_1G);
    }


    pd = owned_table(entry & PAGE_ADDRESS_MASK_4K);

    if (pd == 0) {
        return PADDR_INVALID;
    }


    entry = pd[pd_index(virtual_address)];

    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return PADDR_INVALID;
    }


    if ((entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return
            (entry & PAGE_ADDRESS_MASK_2M) |
            (virtual_address & PAGE_OFFSET_MASK_2M);
    }


    pt = owned_table(entry & PAGE_ADDRESS_MASK_4K);

    if (pt == 0) {
        return PADDR_INVALID;
    }


    entry = pt[pt_index(virtual_address)];

    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return PADDR_INVALID;
    }


    return
        (entry & PAGE_ADDRESS_MASK_4K) |
        (virtual_address & PAGE_OFFSET_MASK_4K);
}


static int user_mapping_info(
    paddr_t root_table,
    vaddr_t virtual_address,
    int write_access,
    paddr_t *physical_out
)
{
    uint64_t *pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;
    uint64_t entry;

    if (!vmm.initialized || root_table == PADDR_INVALID ||
        virtual_address >= 0x0000800000000000ULL ||
        !is_canonical(virtual_address) || physical_out == 0) {
        return 0;
    }

    pml4 = owned_table(root_table);
    if (pml4 == 0) {
        return 0;
    }

    entry = pml4[pml4_index(virtual_address)];
    if ((entry & (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER)) !=
        (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER) ||
        (write_access && (entry & PAGE_ENTRY_WRITABLE) == 0ULL)) {
        return 0;
    }

    pdpt = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pdpt == 0) {
        return 0;
    }

    entry = pdpt[pdpt_index(virtual_address)];
    if ((entry & (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER)) !=
        (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER) ||
        (write_access && (entry & PAGE_ENTRY_WRITABLE) == 0ULL) ||
        (entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return 0;
    }

    pd = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pd == 0) {
        return 0;
    }

    entry = pd[pd_index(virtual_address)];
    if ((entry & (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER)) !=
        (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER) ||
        (write_access && (entry & PAGE_ENTRY_WRITABLE) == 0ULL) ||
        (entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return 0;
    }

    pt = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pt == 0) {
        return 0;
    }

    entry = pt[pt_index(virtual_address)];
    if ((entry & (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER)) !=
        (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER) ||
        (write_access && (entry & PAGE_ENTRY_WRITABLE) == 0ULL)) {
        return 0;
    }

    *physical_out =
        (entry & PAGE_ADDRESS_MASK_4K) |
        (virtual_address & PAGE_OFFSET_MASK_4K);
    return 1;
}

int vmm_user_range_accessible(
    paddr_t root_table,
    vaddr_t user_address,
    size_t length,
    int write_access
)
{
    size_t checked = 0u;

    if (length == 0u) {
        return 1;
    }

    if (user_address >= 0x0000800000000000ULL ||
        UINT64_MAX - user_address < (uint64_t)(length - 1u)) {
        return 0;
    }

    while (checked < length) {
        const vaddr_t address = user_address + (vaddr_t)checked;
        const size_t page_remaining =
            (size_t)(VMM_PAGE_SIZE - (address & PAGE_OFFSET_MASK_4K));
        const size_t chunk =
            (length - checked < page_remaining) ?
            (length - checked) : page_remaining;
        paddr_t physical;

        if (!user_mapping_info(root_table, address, write_access, &physical)) {
            return 0;
        }

        checked += chunk;
    }

    return 1;
}

int vmm_copy_from_user(
    paddr_t root_table,
    void *destination,
    vaddr_t user_source,
    size_t length
)
{
    uint8_t *out = (uint8_t *)destination;
    size_t copied = 0u;

    if ((length != 0u && destination == 0) ||
        !vmm_user_range_accessible(root_table, user_source, length, 0)) {
        return 0;
    }

    while (copied < length) {
        const vaddr_t address = user_source + (vaddr_t)copied;
        const size_t page_remaining =
            (size_t)(VMM_PAGE_SIZE - (address & PAGE_OFFSET_MASK_4K));
        const size_t chunk =
            (length - copied < page_remaining) ?
            (length - copied) : page_remaining;
        paddr_t physical;
        const uint8_t *source;
        size_t index;

        if (!user_mapping_info(root_table, address, 0, &physical)) {
            return 0;
        }

        source = (const uint8_t *)pmm_phys_to_hhdm(physical);
        if (source == 0) {
            return 0;
        }

        for (index = 0u; index < chunk; ++index) {
            out[copied + index] = source[index];
        }

        copied += chunk;
    }

    return 1;
}

int vmm_copy_to_user(
    paddr_t root_table,
    vaddr_t user_destination,
    const void *source,
    size_t length
)
{
    const uint8_t *in = (const uint8_t *)source;
    size_t copied = 0u;

    if ((length != 0u && source == 0) ||
        !vmm_user_range_accessible(root_table, user_destination, length, 1)) {
        return 0;
    }

    while (copied < length) {
        const vaddr_t address = user_destination + (vaddr_t)copied;
        const size_t page_remaining =
            (size_t)(VMM_PAGE_SIZE - (address & PAGE_OFFSET_MASK_4K));
        const size_t chunk =
            (length - copied < page_remaining) ?
            (length - copied) : page_remaining;
        paddr_t physical;
        uint8_t *destination;
        size_t index;

        if (!user_mapping_info(root_table, address, 1, &physical)) {
            return 0;
        }

        destination = (uint8_t *)pmm_phys_to_hhdm(physical);
        if (destination == 0) {
            return 0;
        }

        for (index = 0u; index < chunk; ++index) {
            destination[index] = in[copied + index];
        }

        copied += chunk;
    }

    return 1;
}


struct vmm_stats vmm_get_stats(void)
{
    struct vmm_stats stats;

    stats.root_table = vmm.root_table;
    stats.page_table_pages = vmm.page_table_pages;
    stats.hhdm_offset = vmm.hhdm_offset;

    return stats;
}
```

## `process/scheduler.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/kernel/panic.h>
#include <axiom/kernel/syscall.h>
#include <axiom/memory/heap.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>
#include <axiom/process/scheduler.h>

static struct task tasks[SCHEDULER_MAX_TASKS];
static size_t task_count_value;
static size_t current_index;
static uint64_t next_task_id;
static uint64_t total_context_switches;
static uint64_t total_preemptions;
static uint64_t total_scheduling_ticks;
static uint64_t total_user_fault_terminations;
static int initialized;
static int running;

extern uint8_t kernel_stack_bottom[];
extern uint8_t kernel_stack_top[];

static void bytes_clear(void *memory, size_t count)
{
    uint8_t *bytes = (uint8_t *)memory;
    size_t index;

    for (index = 0u; index < count; ++index) {
        bytes[index] = 0u;
    }
}

static void bytes_copy(void *destination, const void *source, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    size_t index;

    for (index = 0u; index < count; ++index) {
        out[index] = in[index];
    }
}

static void frame_clear(struct interrupt_frame *frame)
{
    bytes_clear(frame, sizeof(*frame));
}

static void task_reset(struct task *task)
{
    size_t index;

    task->id = 0u;
    task->name = 0;
    task->state = TASK_TERMINATED;
    task->privilege = TASK_PRIVILEGE_KERNEL;
    task->saved_frame = 0;
    task->kernel_stack_base = 0;
    task->kernel_stack_size = 0u;
    task->kernel_stack_top = 0u;
    task->address_space = PADDR_INVALID;
    task->entry = 0;
    task->argument = 0;
    task->user_code_page = PADDR_INVALID;
    task->user_data_page = PADDR_INVALID;

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        task->user_stack_pages[index] = PADDR_INVALID;
    }

    task->user_entry = 0u;
    task->user_stack_top = 0u;
    task->fault_vector = 0u;
    task->fault_error_code = 0u;
    task->fault_address = 0u;
    task->exit_code = 0;
    task->wake_tick = 0u;
    task->quantum_ticks = SCHEDULER_DEFAULT_QUANTUM_TICKS;
    task->ticks_in_slice = 0u;
    task->runtime_ticks = 0u;
    task->context_switches = 0u;
}

static int allocate_kernel_stack(struct task *task)
{
    uintptr_t top;
    void *stack = kmalloc(SCHEDULER_TASK_STACK_SIZE);

    if (stack == 0) {
        return 0;
    }

    top = ((uintptr_t)stack + SCHEDULER_TASK_STACK_SIZE) & ~(uintptr_t)0xFu;
    task->kernel_stack_base = stack;
    task->kernel_stack_size = SCHEDULER_TASK_STACK_SIZE;
    task->kernel_stack_top = top;
    return 1;
}

static _Noreturn void task_bootstrap(struct task *task)
{
    if (task == 0 || task->entry == 0) {
        kernel_panic("scheduler entered an invalid task bootstrap");
    }

    task->entry(task->argument);
    task_exit_current(0);
}

static struct interrupt_frame *build_kernel_initial_frame(struct task *task)
{
    uintptr_t address;
    uintptr_t initial_rsp;
    struct interrupt_frame *frame;

    /*
     * In 64-bit mode the CPU saves SS:RSP for every interrupt frame, even
     * without a privilege change, and IRETQ restores them. Build the synthetic
     * kernel-thread frame with a real private-stack RSP and kernel-data SS.
     *
     * A normal SysV C function sees RSP == 8 (mod 16) at entry because CALL
     * pushed a return address. IRETQ does not do that, so reserve one dummy
     * quadword at the top of the task stack and restore RSP to that slot.
     */
    initial_rsp = task->kernel_stack_top - sizeof(uint64_t);
    *(uint64_t *)initial_rsp = 0u;

    address = initial_rsp - sizeof(struct interrupt_frame);
    frame = (struct interrupt_frame *)address;
    frame_clear(frame);

    frame->rip = (uint64_t)(uintptr_t)&task_bootstrap;
    frame->cs = GDT_KERNEL_CODE;
    frame->rflags = 0x202ULL;
    frame->rsp = (uint64_t)initial_rsp;
    frame->ss = GDT_KERNEL_DATA;
    frame->rdi = (uint64_t)(uintptr_t)task;
    return frame;
}

static struct interrupt_frame *build_user_initial_frame(struct task *task)
{
    uintptr_t address;
    struct interrupt_frame *frame;

    address = task->kernel_stack_top - sizeof(struct interrupt_frame);
    frame = (struct interrupt_frame *)address;
    frame_clear(frame);

    frame->rip = task->user_entry;
    frame->cs = GDT_USER_CODE;
    frame->rflags = 0x202ULL;
    frame->rsp = task->user_stack_top - 8ULL;
    frame->ss = GDT_USER_DATA;
    return frame;
}

static size_t find_next_ready(size_t after)
{
    size_t step;

    for (step = 1u; step <= task_count_value; ++step) {
        const size_t index = (after + step) % task_count_value;

        if (tasks[index].state == TASK_READY) {
            return index;
        }
    }

    return SCHEDULER_MAX_TASKS;
}

static struct interrupt_frame *switch_to(size_t next_index)
{
    struct task *next;

    if (next_index >= task_count_value) {
        kernel_panic("scheduler selected invalid task index");
    }

    next = &tasks[next_index];

    if (next->saved_frame == 0 ||
        next->address_space == PADDR_INVALID ||
        next->kernel_stack_top == 0u) {
        kernel_panic("scheduler selected incomplete task context");
    }

    if (vmm_current_address_space() != next->address_space &&
        !vmm_activate_address_space(next->address_space)) {
        kernel_panic("scheduler failed to switch address spaces");
    }

    gdt_set_kernel_stack(next->kernel_stack_top);
    syscall_set_kernel_stack(next->kernel_stack_top);
    next->state = TASK_RUNNING;
    next->ticks_in_slice = 0u;
    ++next->context_switches;

    current_index = next_index;
    ++total_context_switches;
    return next->saved_frame;
}

int scheduler_init(void)
{
    size_t index;
    struct task *bootstrap;
    const paddr_t kernel_root = vmm_kernel_address_space();

    if (initialized || interrupts_enabled() || kernel_root == PADDR_INVALID) {
        return 0;
    }

    for (index = 0u; index < SCHEDULER_MAX_TASKS; ++index) {
        task_reset(&tasks[index]);
    }

    bootstrap = &tasks[0];
    bootstrap->id = 0u;
    bootstrap->name = "bootstrap";
    bootstrap->state = TASK_RUNNING;
    bootstrap->privilege = TASK_PRIVILEGE_KERNEL;
    bootstrap->kernel_stack_base = kernel_stack_bottom;
    bootstrap->kernel_stack_size = (size_t)(kernel_stack_top - kernel_stack_bottom);
    bootstrap->kernel_stack_top = (uintptr_t)kernel_stack_top;
    bootstrap->address_space = kernel_root;

    gdt_set_kernel_stack(bootstrap->kernel_stack_top);
    syscall_set_kernel_stack(bootstrap->kernel_stack_top);

    task_count_value = 1u;
    current_index = 0u;
    next_task_id = 1u;
    total_context_switches = 0u;
    total_preemptions = 0u;
    total_scheduling_ticks = 0u;
    total_user_fault_terminations = 0u;
    running = 0;
    initialized = 1;
    return 1;
}

int task_create(
    const char *name,
    task_entry_t entry,
    void *argument,
    uint64_t *task_id_out
)
{
    struct task *task;

    if (!initialized || interrupts_enabled() || entry == 0 ||
        task_count_value >= SCHEDULER_MAX_TASKS) {
        return 0;
    }

    task = &tasks[task_count_value];
    task_reset(task);

    if (!allocate_kernel_stack(task)) {
        return 0;
    }

    task->id = next_task_id++;
    task->name = name != 0 ? name : "kernel-thread";
    task->state = TASK_READY;
    task->privilege = TASK_PRIVILEGE_KERNEL;
    task->address_space = vmm_kernel_address_space();
    task->entry = entry;
    task->argument = argument;
    task->saved_frame = build_kernel_initial_frame(task);

    if (task_id_out != 0) {
        *task_id_out = task->id;
    }

    ++task_count_value;
    return 1;
}

static void free_user_task_resources(struct task *task)
{
    size_t index;

    if (task->address_space != PADDR_INVALID &&
        task->address_space != vmm_kernel_address_space()) {
        (void)vmm_destroy_user_address_space(task->address_space);
    }

    if (task->user_code_page != PADDR_INVALID) {
        (void)pmm_free_page(task->user_code_page);
    }

    if (task->user_data_page != PADDR_INVALID) {
        (void)pmm_free_page(task->user_data_page);
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        if (task->user_stack_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(task->user_stack_pages[index]);
        }
    }

    if (task->kernel_stack_base != 0) {
        kfree(task->kernel_stack_base);
    }

    task_reset(task);
}

int user_task_create(
    const char *name,
    const void *image,
    size_t image_size,
    uint64_t *task_id_out
)
{
    struct task *task;
    uint8_t *code_bytes;
    uint8_t *data_bytes;
    size_t index;

    if (!initialized || interrupts_enabled() || image == 0 || image_size == 0u ||
        image_size > VMM_PAGE_SIZE || task_count_value >= SCHEDULER_MAX_TASKS) {
        return 0;
    }

    task = &tasks[task_count_value];
    task_reset(task);

    if (!allocate_kernel_stack(task)) {
        return 0;
    }

    task->address_space = vmm_create_user_address_space();
    if (task->address_space == PADDR_INVALID) {
        free_user_task_resources(task);
        return 0;
    }

    task->user_code_page = pmm_alloc_page();
    task->user_data_page = pmm_alloc_page();

    if (task->user_code_page == PADDR_INVALID ||
        task->user_data_page == PADDR_INVALID) {
        free_user_task_resources(task);
        return 0;
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        task->user_stack_pages[index] = pmm_alloc_page();

        if (task->user_stack_pages[index] == PADDR_INVALID) {
            free_user_task_resources(task);
            return 0;
        }
    }

    code_bytes = (uint8_t *)pmm_phys_to_hhdm(task->user_code_page);
    data_bytes = (uint8_t *)pmm_phys_to_hhdm(task->user_data_page);

    if (code_bytes == 0 || data_bytes == 0) {
        free_user_task_resources(task);
        return 0;
    }

    bytes_clear(code_bytes, VMM_PAGE_SIZE);
    bytes_clear(data_bytes, VMM_PAGE_SIZE);
    bytes_copy(code_bytes, image, image_size);

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        void *stack_page = pmm_phys_to_hhdm(task->user_stack_pages[index]);

        if (stack_page == 0) {
            free_user_task_resources(task);
            return 0;
        }

        bytes_clear(stack_page, VMM_PAGE_SIZE);
    }

    if (!vmm_map_page_in_address_space(
            task->address_space,
            TASK_USER_CODE_BASE,
            task->user_code_page,
            VMM_FLAG_USER
        ) ||
        !vmm_map_page_in_address_space(
            task->address_space,
            TASK_USER_DATA_BASE,
            task->user_data_page,
            VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE
        )) {
        free_user_task_resources(task);
        return 0;
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        const vaddr_t address =
            TASK_USER_STACK_TOP -
            (vaddr_t)((TASK_USER_STACK_PAGES - index) * VMM_PAGE_SIZE);

        if (!vmm_map_page_in_address_space(
                task->address_space,
                address,
                task->user_stack_pages[index],
                VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE
            )) {
            free_user_task_resources(task);
            return 0;
        }
    }

    task->id = next_task_id++;
    task->name = name != 0 ? name : "user-task";
    task->state = TASK_READY;
    task->privilege = TASK_PRIVILEGE_USER;
    task->user_entry = TASK_USER_CODE_BASE;
    task->user_stack_top = TASK_USER_STACK_TOP;
    task->saved_frame = build_user_initial_frame(task);

    if (task_id_out != 0) {
        *task_id_out = task->id;
    }

    ++task_count_value;
    return 1;
}

int scheduler_start(void)
{
    if (!initialized || running || interrupts_enabled() || task_count_value < 2u) {
        return 0;
    }

    running = 1;
    return 1;
}

int scheduler_running(void)
{
    return running;
}

static void wake_sleeping_tasks(void)
{
    size_t index;

    for (index = 0u; index < task_count_value; ++index) {
        struct task *task = &tasks[index];

        if (task->state == TASK_SLEEPING &&
            task->wake_tick <= total_scheduling_ticks) {
            task->state = TASK_READY;
            task->wake_tick = 0u;
        }
    }
}


struct interrupt_frame *scheduler_on_timer_interrupt(struct interrupt_frame *frame)
{
    struct task *current;
    size_t next_index;

    if (!running || frame == 0 || task_count_value == 0u) {
        return frame;
    }

    current = &tasks[current_index];
    current->saved_frame = frame;

    ++total_scheduling_ticks;
    wake_sleeping_tasks();
    ++current->runtime_ticks;
    ++current->ticks_in_slice;

    if (current->state == TASK_RUNNING &&
        current->ticks_in_slice < current->quantum_ticks) {
        return frame;
    }

    next_index = find_next_ready(current_index);

    if (next_index == SCHEDULER_MAX_TASKS) {
        if (current->state == TASK_RUNNING) {
            current->ticks_in_slice = 0u;
            return frame;
        }

        kernel_panic("scheduler has no runnable task");
    }

    if (current->state == TASK_RUNNING) {
        current->state = TASK_READY;
    }
    current->ticks_in_slice = 0u;
    ++total_preemptions;
    return switch_to(next_index);
}

struct interrupt_frame *scheduler_handle_user_fault(
    struct interrupt_frame *frame,
    uint64_t vector,
    uint64_t error_code,
    vaddr_t fault_address
)
{
    struct task *current;
    size_t next_index;

    if (!running || frame == 0 || current_index >= task_count_value) {
        return 0;
    }

    current = &tasks[current_index];

    if (current->privilege != TASK_PRIVILEGE_USER ||
        (frame->cs & 3ULL) != 3ULL) {
        return 0;
    }

    current->saved_frame = frame;
    current->state = TASK_TERMINATED;
    current->fault_vector = vector;
    current->fault_error_code = error_code;
    current->fault_address = fault_address;
    ++total_user_fault_terminations;

    next_index = find_next_ready(current_index);
    if (next_index == SCHEDULER_MAX_TASKS) {
        return 0;
    }

    return switch_to(next_index);
}

int scheduler_sleep_current(uint64_t ticks)
{
    struct task *current;
    const uint64_t task_id =
        (current_index < task_count_value) ? tasks[current_index].id : UINT64_MAX;

    if (!running || !initialized || current_index >= task_count_value ||
        interrupts_enabled()) {
        return 0;
    }

    if (ticks == 0u) {
        return scheduler_yield_current();
    }

    current = &tasks[current_index];
    current->state = TASK_SLEEPING;
    current->wake_tick = total_scheduling_ticks + ticks;
    current->ticks_in_slice = current->quantum_ticks;

    interrupts_enable();

    for (;;) {
        __asm__ volatile ("hlt" ::: "memory");

        if (current_index < task_count_value &&
            tasks[current_index].id == task_id &&
            tasks[current_index].state == TASK_RUNNING &&
            total_scheduling_ticks >= current->wake_tick) {
            break;
        }
    }

    interrupts_disable();
    current->wake_tick = 0u;
    return 1;
}

int scheduler_yield_current(void)
{
    struct task *current;
    const uint64_t before_switches = total_context_switches;
    const uint64_t task_id =
        (current_index < task_count_value) ? tasks[current_index].id : UINT64_MAX;

    if (!running || !initialized || current_index >= task_count_value ||
        interrupts_enabled()) {
        return 0;
    }

    current = &tasks[current_index];
    current->ticks_in_slice = current->quantum_ticks;

    interrupts_enable();

    while (total_context_switches == before_switches ||
           current_index >= task_count_value ||
           tasks[current_index].id != task_id) {
        __asm__ volatile ("hlt" ::: "memory");
    }

    interrupts_disable();
    return 1;
}

_Noreturn void task_exit_current(int64_t status)
{
    if (!running || !initialized || current_index >= task_count_value) {
        kernel_panic("task_exit_current called without a running scheduler");
    }

    interrupts_disable();
    tasks[current_index].exit_code = status;
    tasks[current_index].state = TASK_TERMINATED;
    tasks[current_index].ticks_in_slice = tasks[current_index].quantum_ticks;

    for (;;) {
        __asm__ volatile ("sti; hlt" ::: "memory");
    }
}

const struct task *scheduler_current_task(void)
{
    if (!initialized || current_index >= task_count_value) {
        return 0;
    }

    return &tasks[current_index];
}

struct scheduler_stats scheduler_get_stats(void)
{
    struct scheduler_stats stats;
    uint64_t runnable = 0u;
    size_t index;

    for (index = 0u; index < task_count_value; ++index) {
        if (tasks[index].state == TASK_RUNNING ||
            tasks[index].state == TASK_READY) {
            ++runnable;
        }
    }

    stats.task_count = task_count_value;
    stats.runnable_tasks = runnable;
    stats.context_switches = total_context_switches;
    stats.preemptions = total_preemptions;
    stats.scheduling_ticks = total_scheduling_ticks;
    stats.user_fault_terminations = total_user_fault_terminations;
    stats.current_task_id =
        task_count_value != 0u ? tasks[current_index].id : UINT64_MAX;
    return stats;
}

size_t scheduler_task_count(void)
{
    return task_count_value;
}

const struct task *scheduler_task_at(size_t index)
{
    if (index >= task_count_value) {
        return 0;
    }

    return &tasks[index];
}

const struct task *scheduler_task_by_id(uint64_t id)
{
    size_t index;

    for (index = 0u; index < task_count_value; ++index) {
        if (tasks[index].id == id) {
            return &tasks[index];
        }
    }

    return 0;
}
```

## `tests/phase10_syscalls.py`

```python
#!/usr/bin/env python3
"""Boot AxiomOS and validate the Phase-10 x86-64 syscall ABI."""

from pathlib import Path
import re
import socket
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def wait_for_text(process, serial, marker, deadline):
    while True:
        text = serial.read_text(errors="replace")

        if marker in text:
            return

        require(process.poll() is None, f"QEMU exited before {marker!r}")

        if time.monotonic() >= deadline:
            tail = "\n".join(text.replace("\r", "").splitlines()[-60:])
            raise AssertionError(
                f"phase10 timed out waiting for {marker!r}; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )

        time.sleep(0.05)


def send_keys(monitor_path, keys):
    deadline = time.monotonic() + 5

    while not monitor_path.exists():
        require(time.monotonic() < deadline,
                "QEMU monitor socket was not created")
        time.sleep(0.05)

    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
        connection.settimeout(2)
        connection.connect(str(monitor_path))

        try:
            connection.recv(4096)
        except socket.timeout:
            pass

        for key in keys:
            connection.sendall(f"sendkey {key} 30\n".encode("ascii"))
            time.sleep(0.08)


def parse_decimal(text, label, signed=False):
    pattern = r"(-?\d+)" if signed else r"(\d+)"
    match = re.search(rf"^{re.escape(label)}: {pattern}$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


def test_phase10():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase10-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = directory / "phase10-serial.log"
    qemu_log = directory / "phase10-qemu.log"
    monitor = directory / "phase10-monitor.sock"

    serial.write_text("")
    monitor.unlink(missing_ok=True)

    command = [
        "qemu-system-x86_64",
        "-machine", "q35",
        "-accel", "tcg",
        "-m", "256M",
        "-cdrom", str(directory / "AxiomOS.iso"),
        "-boot", "d",
        "-display", "none",
        "-serial", f"file:{serial}",
        "-monitor", f"unix:{monitor},server=on,wait=off",
        "-no-reboot",
        "-no-shutdown",
    ]

    with qemu_log.open("w") as output:
        process = subprocess.Popen(command, stdout=output, stderr=subprocess.STDOUT)

        try:
            wait_for_text(
                process,
                serial,
                "Phase 10 userspace waiting for keyboard input...",
                time.monotonic() + 60,
            )

            # Exercise SYS_read through the actual IRQ-driven keyboard buffer.
            send_keys(monitor, ["x"])

            wait_for_text(
                process,
                serial,
                "Phase 10 system calls complete.",
                time.monotonic() + 20,
            )

            wait_for_text(
                process,
                serial,
                "Phase 7 input ready. Type into AxiomOS.",
                time.monotonic() + 5,
            )

            text = serial.read_text(errors="replace").replace("\r", "")

            for expected in [
                "Phase 9 userspace complete.",
                "AxiomOS Phase 10 syscall interface online.",
                "Syscall mechanism: x86-64 SYSCALL/SYSRET",
                "Hello via SYS_write from Ring 3!",
                "Phase 10 userspace read: x",
                "Phase 10 user-copy validation: OK",
                "Phase 10 syscall self-test: OK",
                "Phase 10 system calls complete.",
            ]:
                require(expected in text, f"phase10: missing output: {expected}")

            require("KERNEL PANIC" not in text, "phase10: kernel panicked")
            require("Phase 10 syscall self-test: FAILED" not in text,
                    "phase10: syscall self-test failed")

            task_id = parse_decimal(text, "Phase 10 user task ID")
            pid = parse_decimal(text, "Phase 10 getpid result")
            bad_pointer = parse_decimal(
                text, "Phase 10 invalid pointer result", signed=True
            )
            open_result = parse_decimal(text, "Phase 10 open result", signed=True)
            exit_status = parse_decimal(text, "Phase 10 exit status", signed=True)
            calls = parse_decimal(text, "Phase 10 syscall calls")
            rejected = parse_decimal(text, "Phase 10 rejected user pointers")

            bytes_match = re.search(
                r"^Phase 10 bytes written/read: (\d+)/(\d+)$",
                text,
                re.MULTILINE,
            )
            require(bytes_match is not None,
                    "phase10: missing written/read byte statistics")
            bytes_written, bytes_read = (int(value) for value in bytes_match.groups())

            require(pid == task_id,
                    f"phase10: getpid returned {pid}, task id is {task_id}")
            require(bad_pointer == -14,
                    f"phase10: invalid pointer returned {bad_pointer}, expected -14")
            require(open_result == -38,
                    f"phase10: open returned {open_result}, expected -38/ENOSYS")
            require(exit_status == 37,
                    f"phase10: exit status {exit_status}, expected 37")
            require(calls >= 8, "phase10: too few syscall transitions observed")
            require(rejected >= 1, "phase10: bad user pointer was not rejected")
            require(bytes_written > 0, "phase10: SYS_write transferred no bytes")
            require(bytes_read >= 1, "phase10: SYS_read transferred no bytes")

            # The older kernel input loop must still work after syscall activity.
            send_keys(monitor, ["s", "y", "s", "ret"])
            wait_for_text(
                process,
                serial,
                "Keyboard line: sys",
                time.monotonic() + 10,
            )

            final_text = serial.read_text(errors="replace").replace("\r", "")
            require("Keyboard line: sys" in final_text,
                    "phase10: keyboard input broke after syscall execution")
            require("KERNEL PANIC" not in final_text,
                    "phase10: kernel panicked after syscall execution")

            print("PASS: Phase 10 x86-64 SYSCALL/SYSRET ABI")
            print(f"  user task id/getpid: {task_id}/{pid}")
            print(f"  syscall calls:       {calls}")
            print("PASS: Phase 10 validated user copies")
            print(f"  bad pointer result:  {bad_pointer}")
            print(f"  bytes written/read:  {bytes_written}/{bytes_read}")
            print("PASS: Phase 10 read/sleep/yield/exit + keyboard coexistence")
        finally:
            if process.poll() is None:
                process.terminate()

            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()

            monitor.unlink(missing_ok=True)


if __name__ == "__main__":
    test_phase10()
    print("PASS: all Phase 10 syscall tests.")
```

## `userspace/phase10_program.S`

```asm
#include <axiom/abi/syscall.h>
#include <axiom/abi/user_layout.h>

.section .rodata.phase10_user,"a",@progbits
.code64

.equ USER_DATA, AXIOM_USER_DATA_BASE
.equ KERNEL_PROBE_ADDRESS, 0xFFFFFFFF80000000

.global phase10_user_start
.global phase10_user_end
phase10_user_start:
    movw $0x1B, %ax
    movw %ax, %ds
    movw %ax, %es

    movabs $USER_DATA, %r12

    call getpid
    movq %rax, AXIOM_PHASE10_PID_OFFSET(%r12)

    movl $1, %edi
    leaq .Lhello(%rip), %rsi
    movl $(.Lhello_end - .Lhello), %edx
    call write

    /* A kernel pointer must be rejected rather than faulting Ring 0. */
    movl $1, %edi
    movabs $KERNEL_PROBE_ADDRESS, %rsi
    movl $4, %edx
    call write
    movq %rax, AXIOM_PHASE10_BAD_WRITE_OFFSET(%r12)

    /* open() has a stable ABI number but the VFS does not exist until Phase 12. */
    leaq .Lfake_path(%rip), %rdi
    xorl %esi, %esi
    call open
    movq %rax, AXIOM_PHASE10_OPEN_RESULT_OFFSET(%r12)

    call yield

    movl $30, %edi
    call sleep

    movl $1, %edi
    leaq .Lwaiting(%rip), %rsi
    movl $(.Lwaiting_end - .Lwaiting), %edx
    call write

    movl $3, %r13d
.Lread_loop:
    movl $0, %edi
    leaq AXIOM_PHASE10_READ_CHAR_OFFSET(%r12), %rsi
    movl $1, %edx
    call read
    cmpq $1, %rax
    je .Lgot_input

    movl $10, %edi
    call sleep
    decl %r13d
    jnz .Lread_loop
    jmp .Lfinish

.Lgot_input:
    movl $1, %edi
    leaq .Lread_prefix(%rip), %rsi
    movl $(.Lread_prefix_end - .Lread_prefix), %edx
    call write

    movl $1, %edi
    leaq AXIOM_PHASE10_READ_CHAR_OFFSET(%r12), %rsi
    movl $1, %edx
    call write

    movl $1, %edi
    leaq .Lnewline(%rip), %rsi
    movl $1, %edx
    call write

.Lfinish:
    movabs $AXIOM_PHASE10_MAGIC, %rax
    movq %rax, AXIOM_PHASE10_MAGIC_OFFSET(%r12)

    movl $AXIOM_PHASE10_EXIT_STATUS, %edi
    call _exit
    ud2

/* Minimal Phase-10 libc-style syscall wrappers. */
.global write
write:
    movl $AXIOM_SYS_WRITE, %eax
    syscall
    ret

.global read
read:
    movl $AXIOM_SYS_READ, %eax
    syscall
    ret

.global _exit
_exit:
    movl $AXIOM_SYS_EXIT, %eax
    syscall
    ud2

.global sleep
sleep:
    movl $AXIOM_SYS_SLEEP, %eax
    syscall
    ret

.global getpid
getpid:
    movl $AXIOM_SYS_GETPID, %eax
    syscall
    ret

.global yield
yield:
    movl $AXIOM_SYS_YIELD, %eax
    syscall
    ret

.global open
open:
    movl $AXIOM_SYS_OPEN, %eax
    syscall
    ret

.global close
close:
    movl $AXIOM_SYS_CLOSE, %eax
    syscall
    ret

.Lhello:
    .ascii "Hello via SYS_write from Ring 3!\n"
.Lhello_end:

.Lwaiting:
    .ascii "Phase 10 userspace waiting for keyboard input...\n"
.Lwaiting_end:

.Lread_prefix:
    .ascii "Phase 10 userspace read: "
.Lread_prefix_end:

.Lnewline:
    .ascii "\n"

.Lfake_path:
    .asciz "/not-yet"

phase10_user_end:

.section .note.GNU-stack,"",@progbits
```

