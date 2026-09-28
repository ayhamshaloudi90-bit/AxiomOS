# AxiomOS Phase 15 — complete new/modified source

This handoff contains the complete contents of every file added or modified for Phase 15.

## `Makefile`

```make
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
USER_DIR := $(BUILD_DIR)/userspace
USER_PHASE11_OBJ := $(USER_DIR)/phase11_program.o
USER_PHASE11_ELF := $(USER_DIR)/phase11_demo.elf
USER_PHASE11_LAUNCHER_OBJ := $(USER_DIR)/phase11_launcher.o
USER_PHASE11_LAUNCHER_ELF := $(USER_DIR)/phase11_launcher.elf
USER_PHASE12_OBJ := $(USER_DIR)/phase12_program.o
USER_PHASE12_ELF := $(USER_DIR)/phase12_demo.elf
USER_PHASE13_OBJ := $(USER_DIR)/phase13_program.o
USER_PHASE13_ELF := $(USER_DIR)/phase13_demo.elf
USER_PHASE14_START_OBJ := $(USER_DIR)/phase14_shell_start.o
USER_PHASE14_C_OBJ := $(USER_DIR)/phase14_shell.o
USER_PHASE14_ELF := $(USER_DIR)/axiomsh.elf
USER_PHASE15_DEMO_START_OBJ := $(USER_DIR)/phase15_demo_start.o
USER_PHASE15_DEMO_C_OBJ := $(USER_DIR)/phase15_demo.o
USER_PHASE15_DEMO_ELF := $(USER_DIR)/phase15_demo.elf
USER_PHASE15_SLEEPER_START_OBJ := $(USER_DIR)/phase15_sleeper_start.o
USER_PHASE15_SLEEPER_C_OBJ := $(USER_DIR)/phase15_sleeper.o
USER_PHASE15_SLEEPER_ELF := $(USER_DIR)/phase15_sleeper.elf
DISK_IMAGE := $(BUILD_DIR)/axiom-disk.img


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

USER_CFLAGS := \
	--target=x86_64-unknown-none-elf \
	-std=gnu11 \
	-Wall -Wextra -Werror \
	-O2 -g \
	-ffreestanding -fno-builtin \
	-fno-stack-protector -fno-stack-check \
	-fno-pic -fno-pie -fno-lto \
	-fno-unwind-tables -fno-asynchronous-unwind-tables \
	-ffunction-sections -fdata-sections \
	-m64 -march=x86-64 -mabi=sysv -mno-red-zone \
	-mno-80387 -mno-mmx -mno-sse -mno-sse2 \
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
	arch/x86_64/pci/pci.c \
	drivers/storage/block.c \
	drivers/storage/ahci.c \
	process/task.c \
	process/scheduler.c \
	kernel/syscall/syscall.c \
	kernel/elf/elf64.c \
	kernel/lib/memory.c \
	filesystem/vfs.c \
	filesystem/ramfs.c \
	filesystem/bootstrap.c \
	filesystem/diskfs.c \
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


$(USER_PHASE11_OBJ): userspace/phase11_program.S include/axiom/abi/syscall.h include/axiom/abi/user_layout.h
	@mkdir -p $(USER_DIR)

	$(CC) \
		$(GASFLAGS) \
		-c $< \
		-o $@


$(USER_PHASE11_LAUNCHER_OBJ): userspace/phase11_launcher.S include/axiom/abi/syscall.h
	@mkdir -p $(USER_DIR)

	$(CC) \
		$(GASFLAGS) \
		-c $< \
		-o $@


$(USER_PHASE12_OBJ): userspace/phase12_program.S include/axiom/abi/syscall.h include/axiom/abi/fs.h
	@mkdir -p $(USER_DIR)

	$(CC) \
		$(GASFLAGS) \
		-c $< \
		-o $@


$(USER_PHASE11_LAUNCHER_ELF): $(USER_PHASE11_LAUNCHER_OBJ) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)

	$(LD) \
		-m elf_x86_64 \
		-nostdlib \
		-static \
		-z max-page-size=0x1000 \
		-z noexecstack \
		-T userspace/x86_64_user.ld \
		$(USER_PHASE11_LAUNCHER_OBJ) \
		-o $@

	@echo
	@echo "Built Phase 11 exec launcher ELF:"
	@echo "  $@"


$(USER_PHASE11_ELF): $(USER_PHASE11_OBJ) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)

	$(LD) \
		-m elf_x86_64 \
		-nostdlib \
		-static \
		-z max-page-size=0x1000 \
		-z noexecstack \
		-T userspace/x86_64_user.ld \
		$(USER_PHASE11_OBJ) \
		-o $@

	@echo
	@echo "Built Phase 11 userspace ELF:"
	@echo "  $@"


$(USER_PHASE12_ELF): $(USER_PHASE12_OBJ) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)

	$(LD) \
		-m elf_x86_64 \
		-nostdlib \
		-static \
		-z max-page-size=0x1000 \
		-z noexecstack \
		-T userspace/x86_64_user.ld \
		$(USER_PHASE12_OBJ) \
		-o $@

	@echo
	@echo "Built Phase 12 VFS demo ELF:"
	@echo "  $@"



$(USER_PHASE13_OBJ): userspace/phase13_program.S include/axiom/abi/syscall.h include/axiom/abi/fs.h
	@mkdir -p $(USER_DIR)

	$(CC) \
		$(GASFLAGS) \
		-c $< \
		-o $@


$(USER_PHASE13_ELF): $(USER_PHASE13_OBJ) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)

	$(LD) \
		-m elf_x86_64 \
		-nostdlib \
		-static \
		-z max-page-size=0x1000 \
		-z noexecstack \
		-T userspace/x86_64_user.ld \
		$(USER_PHASE13_OBJ) \
		-o $@

	@echo
	@echo "Built Phase 13 disk demo ELF:"
	@echo "  $@"

$(USER_PHASE14_START_OBJ): userspace/phase14_shell_start.S include/axiom/abi/syscall.h
	@mkdir -p $(USER_DIR)
	$(CC) $(GASFLAGS) -c $< -o $@

$(USER_PHASE14_C_OBJ): userspace/phase14_shell.c include/axiom/abi/syscall.h include/axiom/abi/fs.h include/axiom/abi/input.h
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE14_ELF): $(USER_PHASE14_START_OBJ) $(USER_PHASE14_C_OBJ) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)
	$(LD) \
		-m elf_x86_64 \
		-nostdlib \
		-static \
		-z max-page-size=0x1000 \
		-z noexecstack \
		--gc-sections \
		-T userspace/x86_64_user.ld \
		$(USER_PHASE14_START_OBJ) $(USER_PHASE14_C_OBJ) \
		-o $@

	@echo
	@echo "Built Phase 14 AxiomOS shell ELF:"
	@echo "  $@"

$(USER_PHASE15_DEMO_START_OBJ): userspace/phase15_demo_start.S include/axiom/abi/syscall.h
	@mkdir -p $(USER_DIR)
	$(CC) $(GASFLAGS) -c $< -o $@

$(USER_PHASE15_DEMO_C_OBJ): userspace/phase15_demo.c include/axiom/abi/syscall.h include/axiom/abi/process.h
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE15_DEMO_ELF): $(USER_PHASE15_DEMO_START_OBJ) $(USER_PHASE15_DEMO_C_OBJ) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections \
		-T userspace/x86_64_user.ld $(USER_PHASE15_DEMO_START_OBJ) $(USER_PHASE15_DEMO_C_OBJ) -o $@
	@echo
	@echo "Built Phase 15 process demo ELF:"
	@echo "  $@"

$(USER_PHASE15_SLEEPER_START_OBJ): userspace/phase15_sleeper_start.S include/axiom/abi/syscall.h
	@mkdir -p $(USER_DIR)
	$(CC) $(GASFLAGS) -c $< -o $@

$(USER_PHASE15_SLEEPER_C_OBJ): userspace/phase15_sleeper.c include/axiom/abi/syscall.h include/axiom/abi/process.h
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE15_SLEEPER_ELF): $(USER_PHASE15_SLEEPER_START_OBJ) $(USER_PHASE15_SLEEPER_C_OBJ) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections \
		-T userspace/x86_64_user.ld $(USER_PHASE15_SLEEPER_START_OBJ) $(USER_PHASE15_SLEEPER_C_OBJ) -o $@
	@echo
	@echo "Built Phase 15 sleeper ELF:"
	@echo "  $@"


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
	$(USER_PHASE11_ELF) \
	$(USER_PHASE11_LAUNCHER_ELF) \
	$(USER_PHASE12_ELF) \
	$(USER_PHASE13_ELF) \
	$(USER_PHASE14_ELF) \
	$(USER_PHASE15_DEMO_ELF) \
	$(USER_PHASE15_SLEEPER_ELF) \
	boot/limine/limine.conf \
	$(LIMINE_DIR)/limine

	rm -rf $(ISO_ROOT)

	mkdir -p $(ISO_ROOT)/boot/limine
	mkdir -p $(ISO_ROOT)/EFI/BOOT


	cp \
		$(KERNEL_ELF) \
		$(ISO_ROOT)/boot/$(PROJECT).elf


	cp \
		$(USER_PHASE11_ELF) \
		$(ISO_ROOT)/boot/phase11_demo.elf


	cp \
		$(USER_PHASE11_LAUNCHER_ELF) \
		$(ISO_ROOT)/boot/phase11_launcher.elf


	cp \
		$(USER_PHASE12_ELF) \
		$(ISO_ROOT)/boot/phase12_demo.elf


	cp \
		$(USER_PHASE13_ELF) \
		$(ISO_ROOT)/boot/phase13_demo.elf

	cp \
		$(USER_PHASE14_ELF) \
		$(ISO_ROOT)/boot/axiomsh.elf

	cp \
		$(USER_PHASE15_DEMO_ELF) \
		$(ISO_ROOT)/boot/phase15_demo.elf

	cp \
		$(USER_PHASE15_SLEEPER_ELF) \
		$(ISO_ROOT)/boot/phase15_sleeper.elf


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


$(DISK_IMAGE):
	@mkdir -p $(BUILD_DIR)
	truncate -s 16M $(DISK_IMAGE)


disk-image: $(DISK_IMAGE)


run: $(ISO_IMAGE) $(DISK_IMAGE)
	$(QEMU) \
		-machine q35 \
		-m 256M \
		-cdrom $(ISO_IMAGE) \
		-boot d \
		-drive file=$(DISK_IMAGE),format=raw,if=none,id=axiomdisk \
		-device ide-hd,drive=axiomdisk,bus=ide.0 \
		-serial stdio \
		-monitor none \
		-no-reboot \
		-no-shutdown


debug: $(ISO_IMAGE) $(DISK_IMAGE)
	$(QEMU) \
		-machine q35 \
		-m 256M \
		-cdrom $(ISO_IMAGE) \
		-boot d \
		-drive file=$(DISK_IMAGE),format=raw,if=none,id=axiomdisk \
		-device ide-hd,drive=axiomdisk,bus=ide.0 \
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
	$(MAKE) test-phase11
	$(MAKE) test-phase12
	$(MAKE) test-phase13
	$(MAKE) test-phase14
	$(MAKE) test-phase15


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
	@echo "AxiomOS Phase 15 targets:"
	@echo
	@echo "  make          Build kernel and bootable ISO"
	@echo "  make run      Boot AxiomOS using QEMU"
	@echo "  make debug    Boot QEMU paused for GDB"
	@echo "  make test     Run Phase 1 through Phase 15 tests"
	@echo "  make test-phase6 Run heap + corruption tests"
	@echo "  make test-phase7 Run timer + keyboard IRQ/input tests"
	@echo "  make test-phase8 Run preemptive scheduler/context-switch tests"
	@echo "  make test-phase9 Run Ring 3 userspace/isolation tests"
	@echo "  make test-phase10 Run SYSCALL/SYSRET ABI and user-copy tests"
	@echo "  make test-phase11 Run ELF64 loader/executable tests"
	@echo "  make test-phase12 Run VFS/RAM filesystem/file-descriptor tests"
	@echo "  make test-phase13 Run AHCI/block-device/persistent disk tests"
	@echo "  make test-phase14 Run interactive Ring-3 shell tests"
	@echo "  make test-phase15 Run fork/wait/ps/kill process-management tests"
	@echo "  make disk-image Create the 16 MiB persistent QEMU disk image"
	@echo "  make MODE=heap_double_free run  Trigger double-free panic"
	@echo "  make MODE=heap_guard run        Trigger tail-guard panic"
	@echo "  make inspect  Inspect generated ELF64 kernel"
	@echo "  make clean    Remove generated build files"
	@echo "  make distclean Remove build files and Limine"

.PHONY: test-phase3 test-phase4 test-phase5 test-phase6 test-phase7 test-phase8 test-phase9 test-phase10 test-phase11 test-phase12 test-phase13 test-phase14 test-phase15 disk-image

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

test-phase11:
	python3 tests/phase11_elf.py

test-phase12:
	python3 tests/phase12_vfs.py

test-phase13:
	python3 tests/phase13_disk.py

test-phase14:
	python3 tests/phase14_shell.py

test-phase15:
	python3 tests/phase15_processes.py

# Track C header changes too; each fault mode has its own object directory.
-include $(C_OBJECTS:.o=.d)
```

## `README.md`

```markdown
# AxiomOS

AxiomOS is a freestanding x86-64 hobby operating system written in C and
assembly, built with Clang/LLD/NASM and booted by Limine v12.9.0 under QEMU. It
is a higher-half ELF64 kernel and does not use the host libc.

## Current milestone

Phases 0–14 provide boot, memory management, interrupts, preemptive scheduling,
Ring-3 isolation, syscalls, ELF64 loading, VFS + persistent AHCI storage, and a
real interactive Ring-3 shell.

Phase 15 expands that into process management:

- eager-copy `fork()` with private child memory;
- parent/child PID tracking and `getppid()`;
- `exec()` image replacement with PID/PPID preservation;
- blocking `waitpid()` and zombie/reap semantics;
- shared, reference-counted open file descriptions across fork;
- process-table inspection exposed to userspace;
- shell `ps`, background `spawn`, `wait`, and `kill`;
- simplified SIGTERM termination for Ring-3 processes;
- sleeping and blocked tasks excluded from the runnable set.

The process layer is intentionally still small: fork is eager rather than COW,
the process table has 16 slots, SIGTERM has no user handlers, and there are no
pipes, process groups, sessions, job control, `argv`, or environment variables
yet.

## Common commands

```bash
make
make run
make test
make test-phase13
make test-phase14
make test-phase15
make MODE=page_fault run
make MODE=heap_double_free run
make MODE=heap_guard run
make debug
```

`make test` runs the complete Phase 1 through Phase 15 regression suite.

When `make run` reaches the shell:

```text
AxiomOS shell ready. Type 'help' for commands.
axiom> help
```

The next milestone is Phase 16: kernel synchronization primitives.

[Architecture](docs/architecture.md) | [Memory](docs/memory.md) |
[Tasks](docs/processes.md) | [Syscalls](docs/syscalls.md) |
[Filesystem](docs/filesystem.md) | [Phase 13](docs/phase13.md) |
[Phase 14](docs/phase14.md) | [Phase 15](docs/phase15.md) | [Project state](docs/project-state.md) |
[Original roadmap](docs/roadmap.md)
```

## `boot/limine/limine.conf`

```text
timeout: 0
serial: yes
serial_baudrate: 115200
verbose: yes

/AxiomOS
    protocol: limine
    path: boot():/boot/AxiomOS.elf
    module_path: boot():/boot/phase11_launcher.elf
    module_string: phase11-launcher
    module_path: boot():/boot/phase11_demo.elf
    module_string: phase11-demo
    module_path: boot():/boot/phase12_demo.elf
    module_string: phase12-demo
    module_path: boot():/boot/phase13_demo.elf
    module_string: phase13-demo
    module_path: boot():/boot/axiomsh.elf
    module_string: phase14-shell
    module_path: boot():/boot/phase15_demo.elf
    module_string: phase15-demo
    module_path: boot():/boot/phase15_sleeper.elf
    module_string: phase15-sleeper
```

## `include/axiom/abi/errno.h`

```c
#ifndef AXIOM_ABI_ERRNO_H
#define AXIOM_ABI_ERRNO_H

/* Small errno set shared by kernel and userspace. Syscalls return -errno. */
#define AXIOM_EPERM        1
#define AXIOM_ENOENT       2
#define AXIOM_ESRCH        3
#define AXIOM_EIO          5
#define AXIOM_EBADF        9
#define AXIOM_ECHILD       10
#define AXIOM_ENOMEM      12
#define AXIOM_EACCES      13
#define AXIOM_EFAULT      14
#define AXIOM_EBUSY       16
#define AXIOM_EEXIST      17
#define AXIOM_ENOTDIR     20
#define AXIOM_EISDIR      21
#define AXIOM_EINVAL      22
#define AXIOM_EMFILE      24
#define AXIOM_ENOSPC      28
#define AXIOM_ESPIPE      29
#define AXIOM_EROFS       30
#define AXIOM_ENAMETOOLONG 36
#define AXIOM_ENOSYS      38

#endif
```

## `include/axiom/abi/process.h`

```c
#ifndef AXIOM_ABI_PROCESS_H
#define AXIOM_ABI_PROCESS_H

#define AXIOM_PROCESS_NAME_MAX 32

#define AXIOM_PROC_RUNNING    0
#define AXIOM_PROC_READY      1
#define AXIOM_PROC_BLOCKED    2
#define AXIOM_PROC_SLEEPING   3
#define AXIOM_PROC_TERMINATED 4

#define AXIOM_PRIV_KERNEL 0
#define AXIOM_PRIV_USER   3

/* Phase 15 intentionally starts with one non-catchable termination signal. */
#define AXIOM_SIGTERM 15

#ifndef __ASSEMBLER__
#include <stdint.h>

struct axiom_process_info {
    uint64_t pid;
    uint64_t ppid;
    uint64_t runtime_ticks;
    int64_t exit_status;
    uint32_t state;
    uint32_t privilege;
    uint32_t termination_signal;
    uint32_t reserved;
    char name[AXIOM_PROCESS_NAME_MAX];
};
#endif

#endif
```

## `include/axiom/abi/syscall.h`

```c
#ifndef AXIOM_ABI_SYSCALL_H
#define AXIOM_ABI_SYSCALL_H

#include <axiom/abi/errno.h>
#include <axiom/abi/fs.h>
#include <axiom/abi/input.h>
#include <axiom/abi/process.h>

/* Stable syscall numbers shared by kernel and userspace. */
#define AXIOM_SYS_WRITE      1
#define AXIOM_SYS_READ       2
#define AXIOM_SYS_EXIT       3
#define AXIOM_SYS_SLEEP      4
#define AXIOM_SYS_GETPID     5
#define AXIOM_SYS_YIELD      6
#define AXIOM_SYS_OPEN       7
#define AXIOM_SYS_CLOSE      8
#define AXIOM_SYS_FORK       9
#define AXIOM_SYS_EXEC      10
#define AXIOM_SYS_MMAP      11
#define AXIOM_SYS_LSEEK     12
#define AXIOM_SYS_STAT      13
#define AXIOM_SYS_READDIR   14
#define AXIOM_SYS_MKDIR     15
#define AXIOM_SYS_SPAWN     16
#define AXIOM_SYS_WAITPID   17
#define AXIOM_SYS_CLEAR     18
#define AXIOM_SYS_KBDSTATS  19
#define AXIOM_SYS_PROCINFO  20
#define AXIOM_SYS_KILL      21
#define AXIOM_SYS_GETPPID   22

#define AXIOM_SYSCALL_MAX_NUMBER AXIOM_SYS_GETPPID

#endif
```

## `include/axiom/filesystem/vfs.h`

```c
#ifndef AXIOM_FILESYSTEM_VFS_H
#define AXIOM_FILESYSTEM_VFS_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/fs.h>

#define VFS_NAME_MAX 63u
#define VFS_PATH_MAX 255u
#define VFS_MAX_MOUNTS 8u

enum vfs_node_type {
    VFS_NODE_FILE = AXIOM_DT_FILE,
    VFS_NODE_DIRECTORY = AXIOM_DT_DIR,
};

struct vfs_node;
struct vfs_filesystem;

struct vfs_dirent {
    char name[VFS_NAME_MAX + 1u];
    uint32_t type;
};

struct vfs_node_ops {
    int (*lookup)(
        struct vfs_node *directory,
        const char *name,
        struct vfs_node **node_out
    );
    int (*create)(
        struct vfs_node *directory,
        const char *name,
        enum vfs_node_type type,
        struct vfs_node **node_out
    );
    int64_t (*read)(
        struct vfs_node *node,
        uint64_t offset,
        void *buffer,
        size_t count
    );
    int64_t (*write)(
        struct vfs_node *node,
        uint64_t offset,
        const void *buffer,
        size_t count
    );
    int (*truncate)(struct vfs_node *node, uint64_t size);
    int (*readdir)(
        struct vfs_node *directory,
        size_t index,
        struct vfs_dirent *entry_out
    );
};

struct vfs_node {
    const char *name;
    enum vfs_node_type type;
    uint32_t mode;
    uint64_t size;
    const struct vfs_node_ops *ops;
    void *private_data;
};

struct vfs_filesystem {
    const char *name;
    struct vfs_node *root;
    void *private_data;
};

struct vfs_file {
    struct vfs_node *node;
    uint64_t offset;
    uint32_t flags;
    uint32_t reference_count;
};

struct vfs_stats {
    uint64_t mount_count;
    uint64_t path_lookups;
    uint64_t opens;
    uint64_t closes;
    uint64_t reads;
    uint64_t writes;
    uint64_t seeks;
    uint64_t stats;
    uint64_t bytes_read;
    uint64_t bytes_written;
};

int vfs_init(void);
int vfs_initialized(void);

int vfs_mount(const char *path, struct vfs_filesystem *filesystem);
int vfs_lookup(const char *path, struct vfs_node **node_out);
int vfs_mkdir(const char *path);
int vfs_readdir(const char *path, size_t index, struct vfs_dirent *entry_out);

int vfs_open(const char *path, uint32_t flags, struct vfs_file **file_out);
int vfs_close(struct vfs_file *file);
int vfs_retain(struct vfs_file *file);
int64_t vfs_read(struct vfs_file *file, void *buffer, size_t count);
int64_t vfs_write(struct vfs_file *file, const void *buffer, size_t count);
int64_t vfs_seek(struct vfs_file *file, int64_t offset, int whence);
int vfs_stat(const char *path, struct axiom_stat *stat_out);

/* Kernel convenience helpers used by the ELF loader/bootstrap tests. */
int vfs_write_file(const char *path, const void *data, size_t size);
int vfs_read_all(const char *path, void **data_out, size_t *size_out);

struct vfs_stats vfs_get_stats(void);

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
    uint64_t open_calls;
    uint64_t close_calls;
    uint64_t fork_calls;
    uint64_t exec_calls;
    uint64_t exec_successes;
    uint64_t lseek_calls;
    uint64_t stat_calls;
    uint64_t readdir_calls;
    uint64_t mkdir_calls;
    uint64_t spawn_calls;
    uint64_t waitpid_calls;
    uint64_t clear_calls;
    uint64_t kbdstats_calls;
    uint64_t procinfo_calls;
    uint64_t kill_calls;
    uint64_t getppid_calls;
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

## `include/axiom/process/scheduler.h`

```c
#ifndef AXIOM_PROCESS_SCHEDULER_H
#define AXIOM_PROCESS_SCHEDULER_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/interrupts.h>
#include <axiom/process/task.h>

#define SCHEDULER_MAX_TASKS 16u
#define SCHEDULER_TASK_STACK_SIZE (16u * 1024u)
#define SCHEDULER_DEFAULT_QUANTUM_TICKS 5u

struct syscall_frame;

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

/* Phase 11: create a Ring-3 task by parsing a standalone ELF64 ET_EXEC image. */
int user_task_create_elf(
    const char *name,
    const void *elf_image,
    size_t elf_size,
    uint64_t *task_id_out
);

/* Replace the currently running Ring-3 process image with a new ELF. IF=0. */
int scheduler_exec_current_elf(
    const void *elf_image,
    size_t elf_size,
    vaddr_t *entry_out,
    vaddr_t *stack_out
);


/* Phase 15 process-management primitives. */
int scheduler_fork_current(
    const struct syscall_frame *parent_frame,
    uint64_t *child_id_out
);
int scheduler_wait_current_child(uint64_t child_id, int64_t *status_out);
int scheduler_terminate_task(uint64_t task_id, uint32_t signal_number);

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

struct task *scheduler_current_task_mutable(void);
const struct task *scheduler_current_task(void);

struct scheduler_stats scheduler_get_stats(void);
size_t scheduler_task_count(void);
const struct task *scheduler_task_at(size_t index);
const struct task *scheduler_task_by_id(uint64_t id);
struct task *scheduler_task_by_id_mutable(uint64_t id);

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
#define TASK_USER_ELF_MAX_PAGES 32u
#define TASK_MAX_FILES 16u

#define TASK_USER_MESSAGE_MAGIC_OFFSET 64u
#define TASK_USER_MESSAGE_MAGIC 0x4158494F4D555345ULL

typedef void (*task_entry_t)(void *argument);

struct vfs_file;

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
    uint64_t parent_id;
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

    /* Phase 11 ELF-backed process image pages. */
    paddr_t user_elf_pages[TASK_USER_ELF_MAX_PAGES];
    vaddr_t user_elf_virtual_pages[TASK_USER_ELF_MAX_PAGES];
    uint64_t user_elf_page_flags[TASK_USER_ELF_MAX_PAGES];
    size_t user_elf_page_count;
    uint16_t elf_program_headers;
    uint16_t elf_load_segments;
    uint64_t elf_file_bytes;
    uint64_t elf_memory_bytes;
    size_t elf_image_size;
    int elf_backed;

    vaddr_t user_entry;
    vaddr_t user_stack_top;

    /* Phase 12 per-process descriptor table. 0/1/2 remain stdin/out/err. */
    struct vfs_file *file_descriptors[TASK_MAX_FILES];

    uint64_t fault_vector;
    uint64_t fault_error_code;
    vaddr_t fault_address;
    int64_t exit_code;
    uint64_t wake_tick;
    uint64_t wait_target_id;
    int wait_collected;
    uint32_t termination_signal;

    uint64_t quantum_ticks;
    uint64_t ticks_in_slice;
    uint64_t runtime_ticks;
    uint64_t context_switches;
};

const char *task_state_name(enum task_state state);
const char *task_privilege_name(enum task_privilege privilege);

int task_install_file(struct task *task, struct vfs_file *file);
struct vfs_file *task_file(struct task *task, int fd);
struct vfs_file *task_take_file(struct task *task, int fd);
void task_close_all_files(struct task *task);
int task_share_files(struct task *destination, const struct task *source);

#endif
```

## `filesystem/bootstrap.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/boot/limine.h>
#include <axiom/filesystem/bootstrap.h>
#include <axiom/filesystem/ramfs.h>
#include <axiom/filesystem/vfs.h>

static const char motd[] =
    "Welcome to the AxiomOS virtual filesystem!\n";

static const char readme[] =
    "AxiomOS Phase 12: files now live behind the VFS abstraction.\n";

static int seed_module_file(const char *module_string, const char *vfs_path)
{
    const void *address;
    uint64_t size;
    const char *boot_path;

    if (!limine_get_module(module_string, &address, &size, &boot_path) ||
        size > (uint64_t)SIZE_MAX) {
        return 0;
    }

    (void)boot_path;
    return vfs_write_file(vfs_path, address, (size_t)size) == 0;
}

int filesystem_phase12_bootstrap(void)
{
    struct vfs_filesystem *rootfs;
    struct vfs_filesystem *tmpfs;

    if (!vfs_init()) {
        return 0;
    }

    rootfs = ramfs_create("rootfs");
    tmpfs = ramfs_create("tmpfs");
    if (rootfs == 0 || tmpfs == 0) {
        return 0;
    }

    if (vfs_mount("/", rootfs) < 0 ||
        vfs_mkdir("/bin") < 0 ||
        vfs_mkdir("/etc") < 0 ||
        vfs_mkdir("/dev") < 0 ||
        vfs_mkdir("/home") < 0 ||
        vfs_mkdir("/tmp") < 0) {
        return 0;
    }

    if (vfs_write_file("/etc/motd", motd, sizeof(motd) - 1u) < 0 ||
        vfs_write_file("/home/readme.txt", readme, sizeof(readme) - 1u) < 0) {
        return 0;
    }

    /*
     * System executables are boot-seeded into the root RAMFS for now. The VFS
     * and ELF layers do not care whether a later backend supplies these bytes.
     */
    if (!seed_module_file("phase11-launcher", "/bin/phase11-launcher") ||
        !seed_module_file("phase11-demo", "/bin/phase11-demo") ||
        !seed_module_file("phase12-demo", "/bin/phase12-demo") ||
        !seed_module_file("phase13-demo", "/bin/phase13-demo") ||
        !seed_module_file("phase14-shell", "/bin/axiomsh") ||
        !seed_module_file("phase15-demo", "/bin/phase15-demo") ||
        !seed_module_file("phase15-sleeper", "/bin/phase15-sleeper")) {
        return 0;
    }

    /* A second RAM filesystem proves that path resolution honors mounts. */
    if (vfs_mount("/tmp", tmpfs) < 0) {
        return 0;
    }

    return 1;
}
```

## `filesystem/vfs.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>
#include <axiom/filesystem/vfs.h>
#include <axiom/memory/heap.h>

struct vfs_mount_entry {
    char path[VFS_PATH_MAX + 1u];
    size_t path_length;
    struct vfs_filesystem *filesystem;
};

static struct vfs_mount_entry mounts[VFS_MAX_MOUNTS];
static size_t mount_count_value;
static struct vfs_stats stats;
static int initialized;

static size_t string_length(const char *text)
{
    size_t length = 0u;

    if (text == 0) {
        return 0u;
    }

    while (text[length] != '\0') {
        ++length;
    }

    return length;
}

static int strings_equal(const char *left, const char *right)
{
    size_t index = 0u;

    if (left == 0 || right == 0) {
        return 0;
    }

    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] != right[index]) {
            return 0;
        }
        ++index;
    }

    return left[index] == right[index];
}

static void string_copy(char *destination, const char *source, size_t capacity)
{
    size_t index = 0u;

    if (destination == 0 || capacity == 0u) {
        return;
    }

    if (source != 0) {
        while (index + 1u < capacity && source[index] != '\0') {
            destination[index] = source[index];
            ++index;
        }
    }

    destination[index] = '\0';
}

static int normalize_path(const char *path, char *output)
{
    char components[32][VFS_NAME_MAX + 1u];
    size_t component_count = 0u;
    size_t input_index = 0u;
    size_t output_index = 0u;

    if (path == 0 || output == 0 || path[0] != '/') {
        return -AXIOM_EINVAL;
    }

    while (path[input_index] != '\0') {
        char component[VFS_NAME_MAX + 1u];
        size_t length = 0u;

        while (path[input_index] == '/') {
            ++input_index;
        }

        if (path[input_index] == '\0') {
            break;
        }

        while (path[input_index] != '\0' && path[input_index] != '/') {
            if (length >= VFS_NAME_MAX) {
                return -AXIOM_ENAMETOOLONG;
            }

            component[length++] = path[input_index++];
        }
        component[length] = '\0';

        if (length == 1u && component[0] == '.') {
            continue;
        }

        if (length == 2u && component[0] == '.' && component[1] == '.') {
            if (component_count != 0u) {
                --component_count;
            }
            continue;
        }

        if (component_count >= 32u) {
            return -AXIOM_ENAMETOOLONG;
        }

        string_copy(
            components[component_count],
            component,
            sizeof(components[component_count])
        );
        ++component_count;
    }

    output[output_index++] = '/';

    for (size_t index = 0u; index < component_count; ++index) {
        const size_t length = string_length(components[index]);

        if (output_index + length + (index + 1u < component_count ? 1u : 0u) >
            VFS_PATH_MAX) {
            return -AXIOM_ENAMETOOLONG;
        }

        for (size_t character = 0u; character < length; ++character) {
            output[output_index++] = components[index][character];
        }

        if (index + 1u < component_count) {
            output[output_index++] = '/';
        }
    }

    output[output_index] = '\0';
    return 0;
}

static int mount_matches(
    const struct vfs_mount_entry *mount,
    const char *path
)
{
    size_t index;

    if (mount == 0 || path == 0) {
        return 0;
    }

    if (mount->path_length == 1u && mount->path[0] == '/') {
        return 1;
    }

    for (index = 0u; index < mount->path_length; ++index) {
        if (path[index] != mount->path[index]) {
            return 0;
        }
    }

    return path[mount->path_length] == '\0' ||
        path[mount->path_length] == '/';
}

static const struct vfs_mount_entry *best_mount(const char *normalized_path)
{
    const struct vfs_mount_entry *best = 0;
    size_t index;

    for (index = 0u; index < mount_count_value; ++index) {
        if (mount_matches(&mounts[index], normalized_path) &&
            (best == 0 || mounts[index].path_length > best->path_length)) {
            best = &mounts[index];
        }
    }

    return best;
}

static int lookup_normalized(
    const char *normalized_path,
    struct vfs_node **node_out
)
{
    const struct vfs_mount_entry *mount;
    struct vfs_node *current;
    const char *cursor;

    if (normalized_path == 0 || node_out == 0) {
        return -AXIOM_EINVAL;
    }

    mount = best_mount(normalized_path);
    if (mount == 0 || mount->filesystem == 0 ||
        mount->filesystem->root == 0) {
        return -AXIOM_ENOENT;
    }

    current = mount->filesystem->root;

    if (mount->path_length == 1u) {
        cursor = normalized_path + 1u;
    } else {
        cursor = normalized_path + mount->path_length;
        if (*cursor == '/') {
            ++cursor;
        }
    }

    while (*cursor != '\0') {
        char component[VFS_NAME_MAX + 1u];
        size_t length = 0u;
        struct vfs_node *next = 0;
        int result;

        while (cursor[length] != '\0' && cursor[length] != '/') {
            if (length >= VFS_NAME_MAX) {
                return -AXIOM_ENAMETOOLONG;
            }
            component[length] = cursor[length];
            ++length;
        }
        component[length] = '\0';

        if (current->type != VFS_NODE_DIRECTORY) {
            return -AXIOM_ENOTDIR;
        }
        if (current->ops == 0 || current->ops->lookup == 0) {
            return -AXIOM_ENOSYS;
        }

        result = current->ops->lookup(current, component, &next);
        if (result < 0) {
            return result;
        }
        if (next == 0) {
            return -AXIOM_ENOENT;
        }

        current = next;
        cursor += length;
        if (*cursor == '/') {
            ++cursor;
        }
    }

    *node_out = current;
    return 0;
}

static int split_parent(
    const char *normalized_path,
    char *parent_out,
    char *name_out
)
{
    size_t length;
    size_t slash;

    if (normalized_path == 0 || parent_out == 0 || name_out == 0 ||
        strings_equal(normalized_path, "/")) {
        return -AXIOM_EINVAL;
    }

    length = string_length(normalized_path);
    slash = length;

    while (slash > 0u && normalized_path[slash - 1u] != '/') {
        --slash;
    }

    if (slash == 0u || length - slash > VFS_NAME_MAX) {
        return -AXIOM_EINVAL;
    }

    string_copy(name_out, normalized_path + slash, VFS_NAME_MAX + 1u);

    if (slash == 1u) {
        parent_out[0] = '/';
        parent_out[1] = '\0';
    } else {
        size_t index;

        for (index = 0u; index + 1u < slash; ++index) {
            parent_out[index] = normalized_path[index];
        }
        parent_out[slash - 1u] = '\0';
    }

    return 0;
}

static int create_node_at_path(
    const char *normalized_path,
    enum vfs_node_type type,
    struct vfs_node **node_out
)
{
    char parent[VFS_PATH_MAX + 1u];
    char name[VFS_NAME_MAX + 1u];
    struct vfs_node *parent_node;
    struct vfs_node *created = 0;
    int result;

    result = split_parent(normalized_path, parent, name);
    if (result < 0) {
        return result;
    }

    result = lookup_normalized(parent, &parent_node);
    if (result < 0) {
        return result;
    }

    if (parent_node->type != VFS_NODE_DIRECTORY) {
        return -AXIOM_ENOTDIR;
    }
    if (parent_node->ops == 0 || parent_node->ops->create == 0) {
        return -AXIOM_EROFS;
    }

    result = parent_node->ops->create(parent_node, name, type, &created);
    if (result < 0) {
        return result;
    }

    if (node_out != 0) {
        *node_out = created;
    }
    return 0;
}

int vfs_init(void)
{
    size_t index;

    if (initialized) {
        return 0;
    }

    for (index = 0u; index < VFS_MAX_MOUNTS; ++index) {
        mounts[index].path[0] = '\0';
        mounts[index].path_length = 0u;
        mounts[index].filesystem = 0;
    }

    mount_count_value = 0u;
    stats.mount_count = 0u;
    stats.path_lookups = 0u;
    stats.opens = 0u;
    stats.closes = 0u;
    stats.reads = 0u;
    stats.writes = 0u;
    stats.seeks = 0u;
    stats.stats = 0u;
    stats.bytes_read = 0u;
    stats.bytes_written = 0u;
    initialized = 1;
    return 1;
}

int vfs_initialized(void)
{
    return initialized;
}

int vfs_mount(const char *path, struct vfs_filesystem *filesystem)
{
    char normalized[VFS_PATH_MAX + 1u];
    size_t index;
    int result;

    if (!initialized || filesystem == 0 || filesystem->root == 0 ||
        filesystem->name == 0) {
        return -AXIOM_EINVAL;
    }

    if (mount_count_value >= VFS_MAX_MOUNTS) {
        return -AXIOM_ENOSPC;
    }

    result = normalize_path(path, normalized);
    if (result < 0) {
        return result;
    }

    for (index = 0u; index < mount_count_value; ++index) {
        if (strings_equal(mounts[index].path, normalized)) {
            return -AXIOM_EBUSY;
        }
    }

    if (!strings_equal(normalized, "/")) {
        struct vfs_node *mountpoint = 0;

        result = lookup_normalized(normalized, &mountpoint);
        if (result < 0) {
            return result;
        }
        if (mountpoint->type != VFS_NODE_DIRECTORY) {
            return -AXIOM_ENOTDIR;
        }
    }

    string_copy(
        mounts[mount_count_value].path,
        normalized,
        sizeof(mounts[mount_count_value].path)
    );
    mounts[mount_count_value].path_length = string_length(normalized);
    mounts[mount_count_value].filesystem = filesystem;
    ++mount_count_value;
    stats.mount_count = mount_count_value;
    return 0;
}

int vfs_lookup(const char *path, struct vfs_node **node_out)
{
    char normalized[VFS_PATH_MAX + 1u];
    int result;

    if (!initialized || node_out == 0) {
        return -AXIOM_EINVAL;
    }

    ++stats.path_lookups;

    result = normalize_path(path, normalized);
    if (result < 0) {
        return result;
    }

    return lookup_normalized(normalized, node_out);
}

int vfs_mkdir(const char *path)
{
    char normalized[VFS_PATH_MAX + 1u];
    struct vfs_node *existing = 0;
    int result;

    if (!initialized) {
        return -AXIOM_ENOSYS;
    }

    result = normalize_path(path, normalized);
    if (result < 0) {
        return result;
    }

    result = lookup_normalized(normalized, &existing);
    if (result == 0) {
        return -AXIOM_EEXIST;
    }
    if (result != -AXIOM_ENOENT) {
        return result;
    }

    return create_node_at_path(normalized, VFS_NODE_DIRECTORY, 0);
}

int vfs_readdir(const char *path, size_t index, struct vfs_dirent *entry_out)
{
    struct vfs_node *directory;
    int result;

    if (!initialized || entry_out == 0) {
        return -AXIOM_EINVAL;
    }

    result = vfs_lookup(path, &directory);
    if (result < 0) {
        return result;
    }
    if (directory->type != VFS_NODE_DIRECTORY) {
        return -AXIOM_ENOTDIR;
    }
    if (directory->ops == 0 || directory->ops->readdir == 0) {
        return -AXIOM_ENOSYS;
    }

    return directory->ops->readdir(directory, index, entry_out);
}

int vfs_open(const char *path, uint32_t flags, struct vfs_file **file_out)
{
    char normalized[VFS_PATH_MAX + 1u];
    struct vfs_node *node = 0;
    struct vfs_file *file;
    const uint32_t access = flags & AXIOM_O_ACCMODE;
    int result;

    if (!initialized) {
        return -AXIOM_ENOSYS;
    }
    if (file_out == 0 || access > AXIOM_O_RDWR ||
        ((flags & AXIOM_O_TRUNC) != 0u && access == AXIOM_O_RDONLY)) {
        return -AXIOM_EINVAL;
    }

    result = normalize_path(path, normalized);
    if (result < 0) {
        return result;
    }

    ++stats.path_lookups;
    result = lookup_normalized(normalized, &node);

    if (result == -AXIOM_ENOENT && (flags & AXIOM_O_CREAT) != 0u) {
        result = create_node_at_path(normalized, VFS_NODE_FILE, &node);
    }
    if (result < 0) {
        return result;
    }
    if (node == 0) {
        return -AXIOM_ENOENT;
    }
    if (node->type == VFS_NODE_DIRECTORY) {
        return -AXIOM_EISDIR;
    }

    if ((flags & AXIOM_O_TRUNC) != 0u) {
        if (node->ops == 0 || node->ops->truncate == 0) {
            return -AXIOM_EROFS;
        }

        result = node->ops->truncate(node, 0u);
        if (result < 0) {
            return result;
        }
    }

    file = (struct vfs_file *)kmalloc(sizeof(*file));
    if (file == 0) {
        return -AXIOM_ENOMEM;
    }

    file->node = node;
    file->flags = flags;
    file->offset = (flags & AXIOM_O_APPEND) != 0u ? node->size : 0u;
    file->reference_count = 1u;
    *file_out = file;
    ++stats.opens;
    return 0;
}

int vfs_retain(struct vfs_file *file)
{
    if (file == 0 || file->reference_count == 0u ||
        file->reference_count == UINT32_MAX) {
        return -AXIOM_EBADF;
    }

    ++file->reference_count;
    return 0;
}

int vfs_close(struct vfs_file *file)
{
    if (file == 0 || file->reference_count == 0u) {
        return -AXIOM_EBADF;
    }

    --file->reference_count;
    if (file->reference_count == 0u) {
        kfree(file);
    }
    ++stats.closes;
    return 0;
}

int64_t vfs_read(struct vfs_file *file, void *buffer, size_t count)
{
    int64_t result;
    const uint32_t access = file != 0 ? file->flags & AXIOM_O_ACCMODE : 0u;

    if (file == 0 || file->node == 0) {
        return -AXIOM_EBADF;
    }
    if (count != 0u && buffer == 0) {
        return -AXIOM_EFAULT;
    }
    if (access == AXIOM_O_WRONLY) {
        return -AXIOM_EACCES;
    }
    if (file->node->ops == 0 || file->node->ops->read == 0) {
        return -AXIOM_EISDIR;
    }

    result = file->node->ops->read(file->node, file->offset, buffer, count);
    if (result > 0) {
        file->offset += (uint64_t)result;
        stats.bytes_read += (uint64_t)result;
    }
    ++stats.reads;
    return result;
}

int64_t vfs_write(struct vfs_file *file, const void *buffer, size_t count)
{
    int64_t result;
    const uint32_t access = file != 0 ? file->flags & AXIOM_O_ACCMODE : 0u;

    if (file == 0 || file->node == 0) {
        return -AXIOM_EBADF;
    }
    if (count != 0u && buffer == 0) {
        return -AXIOM_EFAULT;
    }
    if (access == AXIOM_O_RDONLY) {
        return -AXIOM_EACCES;
    }
    if (file->node->ops == 0 || file->node->ops->write == 0) {
        return -AXIOM_EISDIR;
    }

    if ((file->flags & AXIOM_O_APPEND) != 0u) {
        file->offset = file->node->size;
    }

    result = file->node->ops->write(file->node, file->offset, buffer, count);
    if (result > 0) {
        file->offset += (uint64_t)result;
        stats.bytes_written += (uint64_t)result;
    }
    ++stats.writes;
    return result;
}

int64_t vfs_seek(struct vfs_file *file, int64_t offset, int whence)
{
    int64_t base;
    int64_t next;

    if (file == 0 || file->node == 0) {
        return -AXIOM_EBADF;
    }

    if (file->node->size > (uint64_t)INT64_MAX ||
        file->offset > (uint64_t)INT64_MAX) {
        return -AXIOM_EINVAL;
    }

    switch (whence) {
        case AXIOM_SEEK_SET:
            base = 0;
            break;
        case AXIOM_SEEK_CUR:
            base = (int64_t)file->offset;
            break;
        case AXIOM_SEEK_END:
            base = (int64_t)file->node->size;
            break;
        default:
            return -AXIOM_EINVAL;
    }

    if ((offset > 0 && base > INT64_MAX - offset) ||
        (offset < 0 && base < INT64_MIN - offset)) {
        return -AXIOM_EINVAL;
    }

    next = base + offset;
    if (next < 0) {
        return -AXIOM_EINVAL;
    }

    file->offset = (uint64_t)next;
    ++stats.seeks;
    return next;
}

int vfs_stat(const char *path, struct axiom_stat *stat_out)
{
    struct vfs_node *node;
    int result;

    if (!initialized || stat_out == 0) {
        return -AXIOM_EINVAL;
    }

    result = vfs_lookup(path, &node);
    if (result < 0) {
        return result;
    }

    stat_out->size = node->size;
    stat_out->type = (uint32_t)node->type;
    stat_out->mode = node->mode;
    ++stats.stats;
    return 0;
}

int vfs_write_file(const char *path, const void *data, size_t size)
{
    struct vfs_file *file = 0;
    int result;
    int64_t written;

    result = vfs_open(
        path,
        AXIOM_O_CREAT | AXIOM_O_RDWR | AXIOM_O_TRUNC,
        &file
    );
    if (result < 0) {
        return result;
    }

    written = vfs_write(file, data, size);
    (void)vfs_close(file);

    if (written < 0) {
        return (int)written;
    }
    return (size_t)written == size ? 0 : -AXIOM_EIO;
}

int vfs_read_all(const char *path, void **data_out, size_t *size_out)
{
    struct axiom_stat status;
    struct vfs_file *file = 0;
    uint8_t *buffer;
    size_t total = 0u;
    int result;

    if (data_out == 0 || size_out == 0) {
        return -AXIOM_EINVAL;
    }

    result = vfs_stat(path, &status);
    if (result < 0) {
        return result;
    }
    if (status.type != AXIOM_DT_FILE || status.size > (uint64_t)SIZE_MAX) {
        return -AXIOM_EINVAL;
    }

    buffer = (uint8_t *)kmalloc(status.size == 0u ? 1u : (size_t)status.size);
    if (buffer == 0) {
        return -AXIOM_ENOMEM;
    }

    result = vfs_open(path, AXIOM_O_RDONLY, &file);
    if (result < 0) {
        kfree(buffer);
        return result;
    }

    while (total < (size_t)status.size) {
        const int64_t got = vfs_read(
            file,
            buffer + total,
            (size_t)status.size - total
        );

        if (got < 0) {
            (void)vfs_close(file);
            kfree(buffer);
            return (int)got;
        }
        if (got == 0) {
            break;
        }
        total += (size_t)got;
    }

    (void)vfs_close(file);

    if (total != (size_t)status.size) {
        kfree(buffer);
        return -AXIOM_EIO;
    }

    *data_out = buffer;
    *size_out = total;
    return 0;
}

struct vfs_stats vfs_get_stats(void)
{
    return stats;
}
```

## `process/task.c`

```c
#include <stddef.h>

#include <axiom/filesystem/vfs.h>
#include <axiom/process/task.h>

const char *task_state_name(enum task_state state)
{
    switch (state) {
        case TASK_RUNNING:
            return "RUNNING";
        case TASK_READY:
            return "READY";
        case TASK_BLOCKED:
            return "BLOCKED";
        case TASK_SLEEPING:
            return "SLEEPING";
        case TASK_TERMINATED:
            return "TERMINATED";
        default:
            return "UNKNOWN";
    }
}

const char *task_privilege_name(enum task_privilege privilege)
{
    return privilege == TASK_PRIVILEGE_USER ? "Ring 3" : "Ring 0";
}

int task_install_file(struct task *task, struct vfs_file *file)
{
    size_t index;

    if (task == 0 || file == 0) {
        return -1;
    }

    for (index = 3u; index < TASK_MAX_FILES; ++index) {
        if (task->file_descriptors[index] == 0) {
            task->file_descriptors[index] = file;
            return (int)index;
        }
    }

    return -1;
}

struct vfs_file *task_file(struct task *task, int fd)
{
    if (task == 0 || fd < 3 || (size_t)fd >= TASK_MAX_FILES) {
        return 0;
    }

    return task->file_descriptors[fd];
}

struct vfs_file *task_take_file(struct task *task, int fd)
{
    struct vfs_file *file;

    if (task == 0 || fd < 3 || (size_t)fd >= TASK_MAX_FILES) {
        return 0;
    }

    file = task->file_descriptors[fd];
    task->file_descriptors[fd] = 0;
    return file;
}

void task_close_all_files(struct task *task)
{
    size_t index;

    if (task == 0) {
        return;
    }

    for (index = 3u; index < TASK_MAX_FILES; ++index) {
        if (task->file_descriptors[index] != 0) {
            (void)vfs_close(task->file_descriptors[index]);
            task->file_descriptors[index] = 0;
        }
    }
}

int task_share_files(struct task *destination, const struct task *source)
{
    size_t index;

    if (destination == 0 || source == 0) {
        return 0;
    }

    for (index = 3u; index < TASK_MAX_FILES; ++index) {
        struct vfs_file *file = source->file_descriptors[index];

        if (file == 0) {
            continue;
        }

        if (vfs_retain(file) < 0) {
            task_close_all_files(destination);
            return 0;
        }
        destination->file_descriptors[index] = file;
    }

    return 1;
}
```

## `process/scheduler.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/elf/elf64.h>
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

static void free_user_task_resources(struct task *task);

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
    task->parent_id = UINT64_MAX;
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

    for (index = 0u; index < TASK_USER_ELF_MAX_PAGES; ++index) {
        task->user_elf_pages[index] = PADDR_INVALID;
        task->user_elf_virtual_pages[index] = 0u;
        task->user_elf_page_flags[index] = 0u;
    }

    for (index = 0u; index < TASK_MAX_FILES; ++index) {
        task->file_descriptors[index] = 0;
    }

    task->user_elf_page_count = 0u;
    task->elf_program_headers = 0u;
    task->elf_load_segments = 0u;
    task->elf_file_bytes = 0u;
    task->elf_memory_bytes = 0u;
    task->elf_image_size = 0u;
    task->elf_backed = 0;

    task->user_entry = 0u;
    task->user_stack_top = 0u;
    task->fault_vector = 0u;
    task->fault_error_code = 0u;
    task->fault_address = 0u;
    task->exit_code = 0;
    task->wake_tick = 0u;
    task->wait_target_id = UINT64_MAX;
    task->wait_collected = 0;
    task->termination_signal = 0u;
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

static void release_terminated_task_slot(struct task *task)
{
    if (task == 0) {
        return;
    }

    if (task->privilege == TASK_PRIVILEGE_USER) {
        free_user_task_resources(task);
        return;
    }

    /* Slot zero is the permanent bootstrap task and is never reclaimed. */
    if (task->kernel_stack_base != 0 &&
        task->kernel_stack_base != (void *)kernel_stack_bottom) {
        kfree(task->kernel_stack_base);
    }

    task_reset(task);
}

static int task_parent_is_alive(const struct task *task)
{
    size_t index;

    if (task == 0 || task->parent_id == UINT64_MAX) {
        return 0;
    }

    for (index = 0u; index < task_count_value; ++index) {
        if (tasks[index].id == task->parent_id &&
            tasks[index].state != TASK_TERMINATED) {
            return 1;
        }
    }

    return 0;
}

static int task_slot_reclaimable(const struct task *task)
{
    if (task == 0 || task->state != TASK_TERMINATED) {
        return 0;
    }

    /*
     * Parentless historical phase tasks can be reused immediately. A real
     * child stays as a small zombie until waitpid() collects its status, unless
     * its parent has already died.
     */
    return task->parent_id == UINT64_MAX || task->wait_collected ||
        !task_parent_is_alive(task);
}

static struct task *prepare_creation_slot(size_t *slot_index_out)
{
    size_t index;

    if (slot_index_out == 0) {
        return 0;
    }

    for (index = 1u; index < task_count_value; ++index) {
        if (index != current_index && task_slot_reclaimable(&tasks[index])) {
            release_terminated_task_slot(&tasks[index]);
            *slot_index_out = index;
            return &tasks[index];
        }
    }

    if (task_count_value >= SCHEDULER_MAX_TASKS) {
        return 0;
    }

    index = task_count_value;
    task_reset(&tasks[index]);
    *slot_index_out = index;
    return &tasks[index];
}

static void commit_creation_slot(size_t slot_index)
{
    if (slot_index == task_count_value) {
        ++task_count_value;
    }
}

int task_create(
    const char *name,
    task_entry_t entry,
    void *argument,
    uint64_t *task_id_out
)
{
    struct task *task;
    size_t slot_index;

    if (!initialized || interrupts_enabled() || entry == 0) {
        return 0;
    }

    task = prepare_creation_slot(&slot_index);
    if (task == 0) {
        return 0;
    }

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

    commit_creation_slot(slot_index);
    return 1;
}

static void free_user_task_resources(struct task *task)
{
    size_t index;

    task_close_all_files(task);

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

    for (index = 0u; index < task->user_elf_page_count; ++index) {
        if (task->user_elf_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(task->user_elf_pages[index]);
        }
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
    size_t slot_index;

    if (!initialized || interrupts_enabled() || image == 0 || image_size == 0u ||
        image_size > VMM_PAGE_SIZE) {
        return 0;
    }

    task = prepare_creation_slot(&slot_index);
    if (task == 0) {
        return 0;
    }

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

    commit_creation_slot(slot_index);
    return 1;
}


int user_task_create_elf(
    const char *name,
    const void *elf_image,
    size_t elf_size,
    uint64_t *task_id_out
)
{
    struct task *task;
    struct elf64_load_result loaded;
    size_t index;
    size_t slot_index;

    if (!initialized || interrupts_enabled() || elf_image == 0 ||
        elf_size == 0u) {
        return 0;
    }

    task = prepare_creation_slot(&slot_index);
    if (task == 0) {
        return 0;
    }

    if (!allocate_kernel_stack(task)) {
        return 0;
    }

    task->address_space = vmm_create_user_address_space();
    if (task->address_space == PADDR_INVALID) {
        free_user_task_resources(task);
        return 0;
    }

    if (!elf64_load_executable(
            task->address_space,
            elf_image,
            elf_size,
            &loaded
        )) {
        free_user_task_resources(task);
        return 0;
    }

    if (loaded.page_count > TASK_USER_ELF_MAX_PAGES) {
        free_user_task_resources(task);
        return 0;
    }

    task->user_elf_page_count = loaded.page_count;
    for (index = 0u; index < loaded.page_count; ++index) {
        task->user_elf_pages[index] = loaded.pages[index].physical_address;
        task->user_elf_virtual_pages[index] = loaded.pages[index].virtual_address;
        task->user_elf_page_flags[index] = loaded.pages[index].vmm_flags;
    }

    task->elf_program_headers = loaded.program_headers;
    task->elf_load_segments = loaded.load_segments;
    task->elf_file_bytes = loaded.file_bytes;
    task->elf_memory_bytes = loaded.memory_bytes;
    task->elf_image_size = elf_size;
    task->elf_backed = 1;

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        void *stack_page;
        const vaddr_t address =
            TASK_USER_STACK_TOP -
            (vaddr_t)((TASK_USER_STACK_PAGES - index) * VMM_PAGE_SIZE);

        task->user_stack_pages[index] = pmm_alloc_page();
        if (task->user_stack_pages[index] == PADDR_INVALID) {
            free_user_task_resources(task);
            return 0;
        }

        stack_page = pmm_phys_to_hhdm(task->user_stack_pages[index]);
        if (stack_page == 0) {
            free_user_task_resources(task);
            return 0;
        }
        bytes_clear(stack_page, VMM_PAGE_SIZE);

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
    task->name = name != 0 ? name : "elf-user-task";
    task->state = TASK_READY;
    task->privilege = TASK_PRIVILEGE_USER;
    task->user_entry = loaded.entry;
    task->user_stack_top = TASK_USER_STACK_TOP;
    task->saved_frame = build_user_initial_frame(task);

    if (task_id_out != 0) {
        *task_id_out = task->id;
    }

    commit_creation_slot(slot_index);
    return 1;
}


int scheduler_exec_current_elf(
    const void *elf_image,
    size_t elf_size,
    vaddr_t *entry_out,
    vaddr_t *stack_out
)
{
    struct task *task;
    struct elf64_load_result loaded;
    paddr_t new_root = PADDR_INVALID;
    paddr_t new_stack_pages[TASK_USER_STACK_PAGES];
    paddr_t old_stack_pages[TASK_USER_STACK_PAGES];
    paddr_t old_elf_pages[TASK_USER_ELF_MAX_PAGES];
    paddr_t old_root;
    paddr_t old_code;
    paddr_t old_data;
    size_t old_elf_count;
    size_t index;

    if (!initialized || !running || interrupts_enabled() ||
        current_index >= task_count_value || elf_image == 0 || elf_size == 0u ||
        entry_out == 0 || stack_out == 0) {
        return 0;
    }

    task = &tasks[current_index];
    if (task->privilege != TASK_PRIVILEGE_USER ||
        task->address_space == PADDR_INVALID) {
        return 0;
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        new_stack_pages[index] = PADDR_INVALID;
    }

    new_root = vmm_create_user_address_space();
    if (new_root == PADDR_INVALID) {
        return 0;
    }

    if (!elf64_load_executable(new_root, elf_image, elf_size, &loaded)) {
        (void)vmm_destroy_user_address_space(new_root);
        return 0;
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        void *stack_page;
        const vaddr_t address =
            TASK_USER_STACK_TOP -
            (vaddr_t)((TASK_USER_STACK_PAGES - index) * VMM_PAGE_SIZE);

        new_stack_pages[index] = pmm_alloc_page();
        if (new_stack_pages[index] == PADDR_INVALID) {
            goto fail_new_image;
        }

        stack_page = pmm_phys_to_hhdm(new_stack_pages[index]);
        if (stack_page == 0) {
            goto fail_new_image;
        }
        bytes_clear(stack_page, VMM_PAGE_SIZE);

        if (!vmm_map_page_in_address_space(
                new_root,
                address,
                new_stack_pages[index],
                VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE
            )) {
            goto fail_new_image;
        }
    }

    old_root = task->address_space;
    old_code = task->user_code_page;
    old_data = task->user_data_page;
    old_elf_count = task->user_elf_page_count;

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        old_stack_pages[index] = task->user_stack_pages[index];
    }
    for (index = 0u; index < TASK_USER_ELF_MAX_PAGES; ++index) {
        old_elf_pages[index] = task->user_elf_pages[index];
    }

    if (!vmm_activate_address_space(new_root)) {
        goto fail_new_image;
    }

    task->address_space = new_root;
    task->user_code_page = PADDR_INVALID;
    task->user_data_page = PADDR_INVALID;

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        task->user_stack_pages[index] = new_stack_pages[index];
    }
    for (index = 0u; index < TASK_USER_ELF_MAX_PAGES; ++index) {
        task->user_elf_pages[index] = PADDR_INVALID;
        task->user_elf_virtual_pages[index] = 0u;
        task->user_elf_page_flags[index] = 0u;
    }
    task->user_elf_page_count = loaded.page_count;
    for (index = 0u; index < loaded.page_count; ++index) {
        task->user_elf_pages[index] = loaded.pages[index].physical_address;
        task->user_elf_virtual_pages[index] = loaded.pages[index].virtual_address;
        task->user_elf_page_flags[index] = loaded.pages[index].vmm_flags;
    }

    task->elf_program_headers = loaded.program_headers;
    task->elf_load_segments = loaded.load_segments;
    task->elf_file_bytes = loaded.file_bytes;
    task->elf_memory_bytes = loaded.memory_bytes;
    task->elf_image_size = elf_size;
    task->elf_backed = 1;
    task->user_entry = loaded.entry;
    task->user_stack_top = TASK_USER_STACK_TOP;
    task->fault_vector = 0u;
    task->fault_error_code = 0u;
    task->fault_address = 0u;
    task->exit_code = 0;
    task->wake_tick = 0u;

    /* The new root is active, so the old user page tables can now be released. */
    if (old_root != vmm_kernel_address_space()) {
        (void)vmm_destroy_user_address_space(old_root);
    }
    if (old_code != PADDR_INVALID) {
        (void)pmm_free_page(old_code);
    }
    if (old_data != PADDR_INVALID) {
        (void)pmm_free_page(old_data);
    }
    for (index = 0u; index < old_elf_count; ++index) {
        if (old_elf_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(old_elf_pages[index]);
        }
    }
    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        if (old_stack_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(old_stack_pages[index]);
        }
    }

    *entry_out = loaded.entry;
    *stack_out = TASK_USER_STACK_TOP - 8ULL;
    return 1;

fail_new_image:
    /* Tear down page tables first, then release the leaf physical frames. */
    if (vmm_current_address_space() == new_root) {
        (void)vmm_activate_address_space(task->address_space);
    }
    (void)vmm_destroy_user_address_space(new_root);

    for (index = 0u; index < loaded.page_count; ++index) {
        if (loaded.pages[index].physical_address != PADDR_INVALID) {
            (void)pmm_free_page(loaded.pages[index].physical_address);
        }
    }
    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        if (new_stack_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(new_stack_pages[index]);
        }
    }
    return 0;
}


static int clone_leaf_page(
    paddr_t destination_root,
    vaddr_t virtual_address,
    paddr_t source_page,
    uint64_t flags,
    paddr_t *destination_page_out
)
{
    paddr_t destination_page;
    void *source_bytes;
    void *destination_bytes;

    if (source_page == PADDR_INVALID || destination_page_out == 0) {
        return 0;
    }

    destination_page = pmm_alloc_page();
    if (destination_page == PADDR_INVALID) {
        return 0;
    }

    source_bytes = pmm_phys_to_hhdm(source_page);
    destination_bytes = pmm_phys_to_hhdm(destination_page);
    if (source_bytes == 0 || destination_bytes == 0) {
        (void)pmm_free_page(destination_page);
        return 0;
    }

    bytes_copy(destination_bytes, source_bytes, VMM_PAGE_SIZE);

    if (!vmm_map_page_in_address_space(
            destination_root,
            virtual_address,
            destination_page,
            flags
        )) {
        (void)pmm_free_page(destination_page);
        return 0;
    }

    *destination_page_out = destination_page;
    return 1;
}

static struct interrupt_frame *build_fork_child_frame(
    struct task *child,
    const struct syscall_frame *parent_frame
)
{
    uintptr_t address;
    struct interrupt_frame *frame;

    address = child->kernel_stack_top - sizeof(struct interrupt_frame);
    frame = (struct interrupt_frame *)address;
    frame_clear(frame);

    frame->r15 = parent_frame->r15;
    frame->r14 = parent_frame->r14;
    frame->r13 = parent_frame->r13;
    frame->r12 = parent_frame->r12;
    frame->r11 = parent_frame->user_rflags;
    frame->r10 = parent_frame->r10;
    frame->r9 = parent_frame->r9;
    frame->r8 = parent_frame->r8;
    frame->rbp = parent_frame->rbp;
    frame->rdi = parent_frame->rdi;
    frame->rsi = parent_frame->rsi;
    frame->rdx = parent_frame->rdx;
    frame->rcx = parent_frame->user_rip;
    frame->rbx = parent_frame->rbx;
    frame->rax = 0u; /* fork() returns zero in the child. */
    frame->rip = parent_frame->user_rip;
    frame->cs = GDT_USER_CODE;
    frame->rflags = parent_frame->user_rflags;
    frame->rsp = parent_frame->user_rsp;
    frame->ss = GDT_USER_DATA;
    return frame;
}

int scheduler_fork_current(
    const struct syscall_frame *parent_frame,
    uint64_t *child_id_out
)
{
    struct task *parent;
    struct task *child;
    size_t slot_index;
    size_t index;

    if (!initialized || !running || interrupts_enabled() ||
        parent_frame == 0 || current_index >= task_count_value) {
        return 0;
    }

    parent = &tasks[current_index];
    if (parent->privilege != TASK_PRIVILEGE_USER ||
        parent->address_space == PADDR_INVALID) {
        return 0;
    }

    child = prepare_creation_slot(&slot_index);
    if (child == 0 || !allocate_kernel_stack(child)) {
        return 0;
    }

    child->address_space = vmm_create_user_address_space();
    if (child->address_space == PADDR_INVALID) {
        free_user_task_resources(child);
        return 0;
    }

    if (parent->user_code_page != PADDR_INVALID &&
        !clone_leaf_page(
            child->address_space,
            TASK_USER_CODE_BASE,
            parent->user_code_page,
            VMM_FLAG_USER,
            &child->user_code_page
        )) {
        free_user_task_resources(child);
        return 0;
    }

    if (parent->user_data_page != PADDR_INVALID &&
        !clone_leaf_page(
            child->address_space,
            TASK_USER_DATA_BASE,
            parent->user_data_page,
            VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE,
            &child->user_data_page
        )) {
        free_user_task_resources(child);
        return 0;
    }

    child->user_elf_page_count = parent->user_elf_page_count;
    for (index = 0u; index < parent->user_elf_page_count; ++index) {
        if (!clone_leaf_page(
                child->address_space,
                parent->user_elf_virtual_pages[index],
                parent->user_elf_pages[index],
                parent->user_elf_page_flags[index],
                &child->user_elf_pages[index]
            )) {
            free_user_task_resources(child);
            return 0;
        }
        child->user_elf_virtual_pages[index] =
            parent->user_elf_virtual_pages[index];
        child->user_elf_page_flags[index] = parent->user_elf_page_flags[index];
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        const vaddr_t address =
            TASK_USER_STACK_TOP -
            (vaddr_t)((TASK_USER_STACK_PAGES - index) * VMM_PAGE_SIZE);

        if (!clone_leaf_page(
                child->address_space,
                address,
                parent->user_stack_pages[index],
                VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE,
                &child->user_stack_pages[index]
            )) {
            free_user_task_resources(child);
            return 0;
        }
    }

    if (!task_share_files(child, parent)) {
        free_user_task_resources(child);
        return 0;
    }

    child->id = next_task_id++;
    child->parent_id = parent->id;
    child->name = parent->name != 0 ? parent->name : "fork-child";
    child->state = TASK_READY;
    child->privilege = TASK_PRIVILEGE_USER;
    child->elf_program_headers = parent->elf_program_headers;
    child->elf_load_segments = parent->elf_load_segments;
    child->elf_file_bytes = parent->elf_file_bytes;
    child->elf_memory_bytes = parent->elf_memory_bytes;
    child->elf_image_size = parent->elf_image_size;
    child->elf_backed = parent->elf_backed;
    child->user_entry = parent->user_entry;
    child->user_stack_top = parent->user_stack_top;
    child->saved_frame = build_fork_child_frame(child, parent_frame);
    child->wait_collected = 0;

    if (child_id_out != 0) {
        *child_id_out = child->id;
    }

    commit_creation_slot(slot_index);
    return 1;
}

static void wake_waiters_for_child(uint64_t child_id)
{
    size_t index;

    for (index = 0u; index < task_count_value; ++index) {
        if (tasks[index].state == TASK_BLOCKED &&
            tasks[index].wait_target_id == child_id) {
            tasks[index].state = TASK_READY;
            tasks[index].wait_target_id = UINT64_MAX;
        }
    }
}

int scheduler_wait_current_child(uint64_t child_id, int64_t *status_out)
{
    struct task *parent;
    struct task *child;
    const uint64_t parent_id =
        current_index < task_count_value ? tasks[current_index].id : UINT64_MAX;

    if (!initialized || !running || interrupts_enabled() ||
        current_index >= task_count_value || status_out == 0) {
        return 0;
    }

    parent = &tasks[current_index];

    for (;;) {
        child = scheduler_task_by_id_mutable(child_id);
        if (child == 0 || child->parent_id != parent_id || child->wait_collected) {
            return 0;
        }

        if (child->state == TASK_TERMINATED) {
            *status_out = child->exit_code;
            child->wait_collected = 1;
            return 1;
        }

        parent->state = TASK_BLOCKED;
        parent->wait_target_id = child_id;
        parent->ticks_in_slice = parent->quantum_ticks;

        interrupts_enable();
        for (;;) {
            __asm__ volatile ("hlt" ::: "memory");

            if (current_index < task_count_value &&
                tasks[current_index].id == parent_id &&
                tasks[current_index].state == TASK_RUNNING) {
                break;
            }
        }
        interrupts_disable();
    }
}

int scheduler_terminate_task(uint64_t task_id, uint32_t signal_number)
{
    struct task *target;

    if (!initialized || !running || interrupts_enabled() || signal_number == 0u) {
        return 0;
    }

    target = scheduler_task_by_id_mutable(task_id);
    if (target == 0 || target->privilege != TASK_PRIVILEGE_USER ||
        target->state == TASK_TERMINATED) {
        return 0;
    }

    if (current_index < task_count_value && target == &tasks[current_index]) {
        task_exit_current((int64_t)(128u + signal_number));
    }

    task_close_all_files(target);
    target->termination_signal = signal_number;
    target->exit_code = (int64_t)(128u + signal_number);
    target->state = TASK_TERMINATED;
    target->wake_tick = 0u;
    target->wait_target_id = UINT64_MAX;
    wake_waiters_for_child(target->id);
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
    task_close_all_files(current);
    current->state = TASK_TERMINATED;
    current->fault_vector = vector;
    current->fault_error_code = error_code;
    current->fault_address = fault_address;
    current->exit_code = (int64_t)(128u + vector);
    current->termination_signal = 0u;
    wake_waiters_for_child(current->id);
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
    task_close_all_files(&tasks[current_index]);
    tasks[current_index].exit_code = status;
    tasks[current_index].termination_signal = 0u;
    tasks[current_index].state = TASK_TERMINATED;
    tasks[current_index].ticks_in_slice = tasks[current_index].quantum_ticks;
    wake_waiters_for_child(tasks[current_index].id);

    for (;;) {
        __asm__ volatile ("sti; hlt" ::: "memory");
    }
}

struct task *scheduler_current_task_mutable(void)
{
    if (!initialized || current_index >= task_count_value) {
        return 0;
    }

    return &tasks[current_index];
}

const struct task *scheduler_current_task(void)
{
    return scheduler_current_task_mutable();
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

struct task *scheduler_task_by_id_mutable(uint64_t id)
{
    size_t index;

    for (index = 0u; index < task_count_value; ++index) {
        if (tasks[index].id == id) {
            return &tasks[index];
        }
    }

    return 0;
}

const struct task *scheduler_task_by_id(uint64_t id)
{
    return scheduler_task_by_id_mutable(id);
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
#include <axiom/filesystem/vfs.h>
#include <axiom/kernel/panic.h>
#include <axiom/kernel/syscall.h>
#include <axiom/memory/heap.h>
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

static struct task *current_user_task(void)
{
    struct task *task = scheduler_current_task_mutable();

    if (task == 0 || task->privilege != TASK_PRIVILEGE_USER ||
        task->address_space == PADDR_INVALID) {
        return 0;
    }

    return task;
}

static int copy_user_path(
    const struct task *task,
    vaddr_t user_path,
    char *destination,
    size_t capacity
)
{
    size_t index;

    if (task == 0 || destination == 0 || capacity < 2u) {
        return -AXIOM_EINVAL;
    }

    for (index = 0u; index + 1u < capacity; ++index) {
        if (!vmm_copy_from_user(
                task->address_space,
                &destination[index],
                user_path + (vaddr_t)index,
                1u
            )) {
            return -AXIOM_EFAULT;
        }

        if (destination[index] == '\0') {
            return 0;
        }
    }

    destination[capacity - 1u] = '\0';
    return -AXIOM_ENAMETOOLONG;
}

static int64_t sys_write(uint64_t fd, vaddr_t buffer, uint64_t count)
{
    struct task *task = current_user_task();
    struct vfs_file *file = 0;
    uint64_t transferred = 0u;
    uint8_t local[128];

    ++stats.write_calls;

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

    if (fd != 1u && fd != 2u) {
        if (fd > (uint64_t)INT32_MAX) {
            return syscall_error(AXIOM_EBADF);
        }
        file = task_file(task, (int)fd);
        if (file == 0) {
            return syscall_error(AXIOM_EBADF);
        }
    }

    while (transferred < count) {
        const size_t chunk =
            (count - transferred < sizeof(local)) ?
            (size_t)(count - transferred) : sizeof(local);

        if (!vmm_copy_from_user(
                task->address_space,
                local,
                buffer + transferred,
                chunk
            )) {
            ++stats.rejected_pointers;
            return transferred != 0u ?
                (int64_t)transferred : syscall_error(AXIOM_EFAULT);
        }

        if (file == 0) {
            size_t index;

            for (index = 0u; index < chunk; ++index) {
                terminal_putchar((char)local[index]);
            }
            transferred += chunk;
        } else {
            const int64_t written = vfs_write(file, local, chunk);

            if (written < 0) {
                return transferred != 0u ? (int64_t)transferred : written;
            }
            if (written == 0) {
                break;
            }
            transferred += (uint64_t)written;
            if ((size_t)written < chunk) {
                break;
            }
        }
    }

    stats.bytes_written += transferred;
    return (int64_t)transferred;
}

static int64_t sys_read(uint64_t fd, vaddr_t buffer, uint64_t count)
{
    struct task *task = current_user_task();
    struct vfs_file *file = 0;
    uint64_t transferred = 0u;
    uint8_t local[128];

    ++stats.read_calls;

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

    if (fd != 0u) {
        if (fd > (uint64_t)INT32_MAX) {
            return syscall_error(AXIOM_EBADF);
        }
        file = task_file(task, (int)fd);
        if (file == 0) {
            return syscall_error(AXIOM_EBADF);
        }
    }

    if (file == 0) {
        while (transferred < count) {
            char character;

            if (!keyboard_read_char(&character)) {
                break;
            }

            if (!vmm_copy_to_user(
                    task->address_space,
                    buffer + transferred,
                    &character,
                    1u
                )) {
                ++stats.rejected_pointers;
                return transferred != 0u ?
                    (int64_t)transferred : syscall_error(AXIOM_EFAULT);
            }
            ++transferred;
        }
    } else {
        while (transferred < count) {
            const size_t chunk =
                (count - transferred < sizeof(local)) ?
                (size_t)(count - transferred) : sizeof(local);
            const int64_t got = vfs_read(file, local, chunk);

            if (got < 0) {
                return transferred != 0u ? (int64_t)transferred : got;
            }
            if (got == 0) {
                break;
            }

            if (!vmm_copy_to_user(
                    task->address_space,
                    buffer + transferred,
                    local,
                    (size_t)got
                )) {
                ++stats.rejected_pointers;
                return transferred != 0u ?
                    (int64_t)transferred : syscall_error(AXIOM_EFAULT);
            }

            transferred += (uint64_t)got;
            if ((size_t)got < chunk) {
                break;
            }
        }
    }

    stats.bytes_read += transferred;
    return (int64_t)transferred;
}

static int64_t sys_open(vaddr_t user_path, uint64_t flags)
{
    struct task *task = current_user_task();
    struct vfs_file *file = 0;
    char path[VFS_PATH_MAX + 1u];
    int copied;
    int result;
    int fd;

    ++stats.open_calls;

    if (!vfs_initialized()) {
        ++stats.unimplemented_calls;
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0 || flags > UINT32_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    result = vfs_open(path, (uint32_t)flags, &file);
    if (result < 0) {
        return result;
    }

    fd = task_install_file(task, file);
    if (fd < 0) {
        (void)vfs_close(file);
        return syscall_error(AXIOM_EMFILE);
    }

    return fd;
}

static int64_t sys_close(uint64_t fd)
{
    struct task *task = current_user_task();
    struct vfs_file *file;

    ++stats.close_calls;

    if (!vfs_initialized()) {
        ++stats.unimplemented_calls;
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0 || fd > (uint64_t)INT32_MAX) {
        return syscall_error(AXIOM_EBADF);
    }

    file = task_take_file(task, (int)fd);
    if (file == 0) {
        return syscall_error(AXIOM_EBADF);
    }

    return vfs_close(file);
}

static int64_t sys_fork(struct syscall_frame *frame)
{
    uint64_t child_id = 0u;

    ++stats.fork_calls;

    if (frame == 0 || current_user_task() == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    if (!scheduler_fork_current(frame, &child_id)) {
        return syscall_error(AXIOM_ENOMEM);
    }

    return (int64_t)child_id;
}

static int64_t sys_exec(vaddr_t user_path, struct syscall_frame *frame)
{
    struct task *task = current_user_task();
    char path[VFS_PATH_MAX + 1u];
    void *image = 0;
    size_t image_size = 0u;
    vaddr_t entry;
    vaddr_t stack;
    int copied;
    int result;

    ++stats.exec_calls;

    if (!vfs_initialized()) {
        ++stats.unimplemented_calls;
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0 || frame == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    result = vfs_read_all(path, &image, &image_size);
    if (result < 0) {
        return result;
    }

    if (!scheduler_exec_current_elf(
            image,
            image_size,
            &entry,
            &stack
        )) {
        kfree(image);
        return syscall_error(AXIOM_EINVAL);
    }
    kfree(image);

    /* exec() starts the new image with a clean user register set. */
    frame->r15 = 0u;
    frame->r14 = 0u;
    frame->r13 = 0u;
    frame->r12 = 0u;
    frame->r10 = 0u;
    frame->r9 = 0u;
    frame->r8 = 0u;
    frame->rbp = 0u;
    frame->rdi = 0u;
    frame->rsi = 0u;
    frame->rdx = 0u;
    frame->rbx = 0u;
    frame->user_rip = entry;
    frame->user_rflags = 0x202ULL;
    frame->user_rsp = stack;

    ++stats.exec_successes;
    return 0;
}

static int64_t sys_lseek(uint64_t fd, int64_t offset, uint64_t whence)
{
    struct task *task = current_user_task();
    struct vfs_file *file;

    ++stats.lseek_calls;

    if (!vfs_initialized()) {
        ++stats.unimplemented_calls;
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0 || fd > (uint64_t)INT32_MAX || whence > INT32_MAX) {
        return syscall_error(AXIOM_EBADF);
    }

    file = task_file(task, (int)fd);
    if (file == 0) {
        return syscall_error(AXIOM_EBADF);
    }

    return vfs_seek(file, offset, (int)whence);
}

static int64_t sys_stat(vaddr_t user_path, vaddr_t user_stat)
{
    struct task *task = current_user_task();
    char path[VFS_PATH_MAX + 1u];
    struct axiom_stat status;
    int copied;
    int result;

    ++stats.stat_calls;

    if (!vfs_initialized()) {
        ++stats.unimplemented_calls;
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    if (!vmm_user_range_accessible(
            task->address_space,
            user_stat,
            sizeof(status),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    result = vfs_stat(path, &status);
    if (result < 0) {
        return result;
    }

    if (!vmm_copy_to_user(
            task->address_space,
            user_stat,
            &status,
            sizeof(status)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    return 0;
}


static int64_t sys_readdir(vaddr_t user_path, uint64_t index, vaddr_t user_entry)
{
    struct task *task = current_user_task();
    char path[VFS_PATH_MAX + 1u];
    struct vfs_dirent entry;
    struct axiom_dirent user_value;
    int copied;
    int result;

    ++stats.readdir_calls;

    if (!vfs_initialized()) {
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0 || index > (uint64_t)SIZE_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    if (!vmm_user_range_accessible(
            task->address_space,
            user_entry,
            sizeof(user_value),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    result = vfs_readdir(path, (size_t)index, &entry);
    if (result <= 0) {
        return result;
    }

    for (size_t i = 0u; i < sizeof(user_value.name); ++i) {
        user_value.name[i] = entry.name[i];
        if (entry.name[i] == '\0') {
            for (++i; i < sizeof(user_value.name); ++i) {
                user_value.name[i] = '\0';
            }
            break;
        }
    }
    user_value.type = entry.type;

    if (!vmm_copy_to_user(
            task->address_space,
            user_entry,
            &user_value,
            sizeof(user_value)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    return 1;
}

static int64_t sys_mkdir(vaddr_t user_path)
{
    struct task *task = current_user_task();
    char path[VFS_PATH_MAX + 1u];
    int copied;

    ++stats.mkdir_calls;

    if (!vfs_initialized()) {
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    return vfs_mkdir(path);
}

static int64_t sys_spawn(vaddr_t user_path)
{
    struct task *parent = current_user_task();
    struct task *child;
    char path[VFS_PATH_MAX + 1u];
    void *image = 0;
    size_t image_size = 0u;
    uint64_t child_id;
    int copied;
    int result;

    ++stats.spawn_calls;

    if (!vfs_initialized()) {
        return syscall_error(AXIOM_ENOSYS);
    }
    if (parent == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(parent, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    result = vfs_read_all(path, &image, &image_size);
    if (result < 0) {
        return result;
    }

    if (!user_task_create_elf("spawned-elf", image, image_size, &child_id)) {
        kfree(image);
        return syscall_error(AXIOM_EINVAL);
    }
    kfree(image);

    child = scheduler_task_by_id_mutable(child_id);
    if (child == 0) {
        return syscall_error(AXIOM_EINVAL);
    }
    child->parent_id = parent->id;
    return (int64_t)child_id;
}

static int64_t sys_waitpid(uint64_t pid, vaddr_t user_status)
{
    struct task *parent = current_user_task();
    int64_t status;

    ++stats.waitpid_calls;

    if (parent == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    if (user_status != 0u &&
        !vmm_user_range_accessible(
            parent->address_space,
            user_status,
            sizeof(status),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    if (!scheduler_wait_current_child(pid, &status)) {
        return syscall_error(AXIOM_ECHILD);
    }

    if (user_status != 0u &&
        !vmm_copy_to_user(
            parent->address_space,
            user_status,
            &status,
            sizeof(status)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    return (int64_t)pid;
}

static int64_t sys_clear(void)
{
    ++stats.clear_calls;
    terminal_clear();
    return 0;
}

static int64_t sys_kbdstats(vaddr_t user_stats)
{
    struct task *task = current_user_task();
    const struct keyboard_stats source = keyboard_get_stats();
    struct axiom_keyboard_stats destination;

    ++stats.kbdstats_calls;

    if (task == 0 ||
        !vmm_user_range_accessible(
            task->address_space,
            user_stats,
            sizeof(destination),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    destination.irq_count = source.irq_count;
    destination.scancode_count = source.scancode_count;
    destination.character_count = source.character_count;
    destination.dropped_characters = source.dropped_characters;

    if (!vmm_copy_to_user(
            task->address_space,
            user_stats,
            &destination,
            sizeof(destination)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    return 0;
}

static int64_t sys_procinfo(uint64_t index, vaddr_t user_info)
{
    struct task *caller = current_user_task();
    const struct task *task;
    struct axiom_process_info info;
    size_t character;

    ++stats.procinfo_calls;

    if (caller == 0 || index > (uint64_t)SIZE_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }
    if (!vmm_user_range_accessible(
            caller->address_space,
            user_info,
            sizeof(info),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    task = scheduler_task_at((size_t)index);
    if (task == 0) {
        return 0;
    }

    info.pid = task->id;
    info.ppid = task->parent_id;
    info.runtime_ticks = task->runtime_ticks;
    info.exit_status = task->exit_code;
    info.state = (uint32_t)task->state;
    info.privilege = (uint32_t)task->privilege;
    info.termination_signal = task->termination_signal;
    info.reserved = 0u;

    for (character = 0u; character + 1u < sizeof(info.name); ++character) {
        if (task->name == 0 || task->name[character] == '\0') {
            break;
        }
        info.name[character] = task->name[character];
    }
    info.name[character++] = '\0';
    while (character < sizeof(info.name)) {
        info.name[character++] = '\0';
    }

    if (!vmm_copy_to_user(
            caller->address_space,
            user_info,
            &info,
            sizeof(info)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    return 1;
}

static int64_t sys_kill(uint64_t pid, uint64_t signal_number)
{
    struct task *caller = current_user_task();
    const struct task *target;

    ++stats.kill_calls;

    if (caller == 0 || signal_number != AXIOM_SIGTERM) {
        return syscall_error(AXIOM_EINVAL);
    }

    target = scheduler_task_by_id(pid);
    if (target == 0 || target->state == TASK_TERMINATED) {
        return syscall_error(AXIOM_ESRCH);
    }
    if (target->privilege != TASK_PRIVILEGE_USER) {
        return syscall_error(AXIOM_EPERM);
    }

    if (!scheduler_terminate_task(pid, (uint32_t)signal_number)) {
        return syscall_error(AXIOM_ESRCH);
    }
    return 0;
}

static int64_t sys_getppid(void)
{
    const struct task *task = current_user_task();

    ++stats.getppid_calls;
    if (task == 0) {
        return syscall_error(AXIOM_EINVAL);
    }
    return task->parent_id == UINT64_MAX ? 0 : (int64_t)task->parent_id;
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
    stats.open_calls = 0u;
    stats.close_calls = 0u;
    stats.fork_calls = 0u;
    stats.exec_calls = 0u;
    stats.exec_successes = 0u;
    stats.lseek_calls = 0u;
    stats.stat_calls = 0u;
    stats.readdir_calls = 0u;
    stats.mkdir_calls = 0u;
    stats.spawn_calls = 0u;
    stats.waitpid_calls = 0u;
    stats.clear_calls = 0u;
    stats.kbdstats_calls = 0u;
    stats.procinfo_calls = 0u;
    stats.kill_calls = 0u;
    stats.getppid_calls = 0u;
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
            result = sys_open((vaddr_t)frame->rdi, frame->rsi);
            break;

        case AXIOM_SYS_CLOSE:
            result = sys_close(frame->rdi);
            break;

        case AXIOM_SYS_FORK:
            result = sys_fork(frame);
            break;

        case AXIOM_SYS_EXEC:
            result = sys_exec((vaddr_t)frame->rdi, frame);
            break;

        case AXIOM_SYS_LSEEK:
            result = sys_lseek(
                frame->rdi,
                (int64_t)frame->rsi,
                frame->rdx
            );
            break;

        case AXIOM_SYS_STAT:
            result = sys_stat((vaddr_t)frame->rdi, (vaddr_t)frame->rsi);
            break;

        case AXIOM_SYS_READDIR:
            result = sys_readdir(
                (vaddr_t)frame->rdi,
                frame->rsi,
                (vaddr_t)frame->rdx
            );
            break;

        case AXIOM_SYS_MKDIR:
            result = sys_mkdir((vaddr_t)frame->rdi);
            break;

        case AXIOM_SYS_SPAWN:
            result = sys_spawn((vaddr_t)frame->rdi);
            break;

        case AXIOM_SYS_WAITPID:
            result = sys_waitpid(frame->rdi, (vaddr_t)frame->rsi);
            break;

        case AXIOM_SYS_CLEAR:
            result = sys_clear();
            break;

        case AXIOM_SYS_KBDSTATS:
            result = sys_kbdstats((vaddr_t)frame->rdi);
            break;

        case AXIOM_SYS_PROCINFO:
            result = sys_procinfo(frame->rdi, (vaddr_t)frame->rsi);
            break;

        case AXIOM_SYS_KILL:
            result = sys_kill(frame->rdi, frame->rsi);
            break;

        case AXIOM_SYS_GETPPID:
            result = sys_getppid();
            break;

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
#include <axiom/drivers/block.h>
#include <axiom/drivers/serial.h>
#include <axiom/drivers/timer.h>
#include <axiom/elf/elf64.h>
#include <axiom/filesystem/bootstrap.h>
#include <axiom/filesystem/diskfs.h>
#include <axiom/filesystem/vfs.h>
#include <axiom/kernel/syscall.h>
#include <axiom/memory/heap.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>
#include <axiom/process/scheduler.h>

#include <axiom/terminal/kprintf.h>
#include <axiom/terminal/terminal.h>



#define PHASE8_WORKER_TARGET 1000ULL
#define PHASE8_TEST_TIMEOUT_TICKS 300ULL
#define PHASE9_TEST_TIMEOUT_TICKS 500ULL
#define PHASE10_TEST_TIMEOUT_TICKS 1500ULL
#define PHASE11_TEST_TIMEOUT_TICKS 1000ULL
#define PHASE12_TEST_TIMEOUT_TICKS 1000ULL
#define PHASE13_TEST_TIMEOUT_TICKS 1500ULL
#define PHASE9_KERNEL_PROBE_ADDRESS 0xFFFFFFFF80000000ULL

static volatile uint64_t phase8_worker_a_count;
static volatile uint64_t phase8_worker_b_count;

static const char phase13_persistent_message[] =
    "Persistent storage works across AxiomOS reboots!\n";

static int phase13_bytes_equal(
    const void *left_memory,
    const void *right_memory,
    size_t count
)
{
    const uint8_t *left = (const uint8_t *)left_memory;
    const uint8_t *right = (const uint8_t *)right_memory;
    size_t index;

    if (left == 0 || right == 0) {
        return 0;
    }

    for (index = 0u; index < count; ++index) {
        if (left[index] != right[index]) {
            return 0;
        }
    }
    return 1;
}

static int phase13_persistent_file_matches(int *exists_out)
{
    void *data = 0;
    size_t size = 0u;
    int result;

    if (exists_out == 0) {
        return 0;
    }

    result = vfs_read_all("/disk/persist.txt", &data, &size);
    if (result == -AXIOM_ENOENT) {
        *exists_out = 0;
        return 1;
    }
    if (result < 0) {
        return 0;
    }

    *exists_out = 1;
    result = size == sizeof(phase13_persistent_message) - 1u &&
        phase13_bytes_equal(
            data,
            phase13_persistent_message,
            sizeof(phase13_persistent_message) - 1u
        );
    kfree(data);
    return result;
}


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


static int phase11_results_ready(
    const struct task *task,
    uint64_t *pid_out,
    uint64_t *bss_zero_out,
    uint64_t *bss_probe_out
)
{
    uint64_t values[4];

    if (task == 0 || pid_out == 0 || bss_zero_out == 0 || bss_probe_out == 0 ||
        !task->elf_backed || task->address_space == PADDR_INVALID) {
        return 0;
    }

    if (!vmm_copy_from_user(
            task->address_space,
            values,
            (vaddr_t)AXIOM_PHASE11_DATA_BASE,
            sizeof(values)
        )) {
        return 0;
    }

    if (values[1] != AXIOM_PHASE11_MAGIC) {
        return 0;
    }

    *pid_out = values[0];
    *bss_zero_out = values[2];
    *bss_probe_out = values[3];
    return 1;
}

static int phase11_permissions_ok(const struct task *task)
{
    uint64_t text_flags;
    uint64_t rodata_flags;
    uint64_t data_flags;

    if (task == 0 || task->address_space == PADDR_INVALID ||
        !vmm_mapping_flags_in_address_space(
            task->address_space,
            AXIOM_PHASE11_TEXT_BASE,
            &text_flags
        ) ||
        !vmm_mapping_flags_in_address_space(
            task->address_space,
            AXIOM_PHASE11_RODATA_BASE,
            &rodata_flags
        ) ||
        !vmm_mapping_flags_in_address_space(
            task->address_space,
            AXIOM_PHASE11_DATA_BASE,
            &data_flags
        )) {
        return 0;
    }

    return
        (text_flags & VMM_FLAG_USER) != 0ULL &&
        (text_flags & VMM_FLAG_WRITABLE) == 0ULL &&
        (text_flags & VMM_FLAG_NO_EXECUTE) == 0ULL &&
        (rodata_flags & VMM_FLAG_USER) != 0ULL &&
        (rodata_flags & VMM_FLAG_WRITABLE) == 0ULL &&
        (rodata_flags & VMM_FLAG_NO_EXECUTE) != 0ULL &&
        (data_flags & VMM_FLAG_USER) != 0ULL &&
        (data_flags & VMM_FLAG_WRITABLE) != 0ULL &&
        (data_flags & VMM_FLAG_NO_EXECUTE) != 0ULL;
}


static void phase8_counter_worker(void *argument)
{
    volatile uint64_t *counter = (volatile uint64_t *)argument;

    for (;;) {
        ++(*counter);
        __asm__ volatile ("pause" ::: "memory");
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


    /*
     * Phase 12's VFS is initialized here so Phase 11's accepted exec() test
     * now resolves /bin/phase11-demo through normal VFS lookup rather than a
     * syscall-local boot-module registry. The visible Phase-12 acceptance test
     * still runs after Phase 11, preserving the roadmap/output order.
     */
    interrupts_disable();
    if (!filesystem_phase12_bootstrap()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 12 VFS bootstrap: FAILED\n");
        return;
    }

    /*
     * Phase 11: start from a standalone ELF launcher, then prove SYS_exec by
     * replacing that process with /bin/phase11-demo, another standalone ELF.
     * Phase 12 is already bootstrapped, so exec now resolves the target through
     * the VFS while the launcher itself remains a Limine-delivered test input.
     */
    interrupts_disable();

    {
        const void *launcher_image;
        uint64_t launcher_size_u64;
        const char *launcher_path;
        const void *demo_image;
        uint64_t demo_size_u64;
        const char *demo_path;
        uint64_t elf_task_id;

        if (!limine_get_module(
                "phase11-launcher",
                &launcher_image,
                &launcher_size_u64,
                &launcher_path
            ) ||
            !limine_get_module(
                "phase11-demo",
                &demo_image,
                &demo_size_u64,
                &demo_path
            ) ||
            launcher_size_u64 > (uint64_t)SIZE_MAX ||
            demo_size_u64 > (uint64_t)SIZE_MAX ||
            !user_task_create_elf(
                "phase11-exec-launcher",
                launcher_image,
                (size_t)launcher_size_u64,
                &elf_task_id
            )) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 11 ELF task creation: FAILED (%s)\n", elf64_last_error());
            return;
        }

        (void)demo_image;
        (void)demo_size_u64;

        {
            const struct task *task = scheduler_task_by_id(elf_task_id);

            if (task == 0 || !task->elf_backed ||
                task->user_entry != AXIOM_PHASE11_TEXT_BASE ||
                !phase11_permissions_ok(task)) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 11 ELF mapping validation: FAILED\n");
                return;
            }

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_CYAN,
                TERMINAL_COLOR_BLACK
            );
            kprintf("AxiomOS Phase 11 ELF64 loader online.\n");

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_GREY,
                TERMINAL_COLOR_BLACK
            );
            kprintf("ELF launcher source: %s (Limine module)\n", launcher_path);
            kprintf("ELF exec target: /bin/phase11-demo -> %s\n", demo_path);
            kprintf("ELF type: ET_EXEC x86-64\n");
            kprintf(
                "Phase 11 ELF task ID: %llu\n",
                (unsigned long long)elf_task_id
            );
        }

        interrupts_enable();

        {
            const uint64_t start = timer_ticks();
            int results_ready = 0;
            uint64_t pid_result = 0u;
            uint64_t bss_was_zero = 0u;
            uint64_t bss_probe = UINT64_MAX;

            while ((timer_ticks() - start) < PHASE11_TEST_TIMEOUT_TICKS) {
                const struct task *task = scheduler_task_by_id(elf_task_id);

                results_ready = phase11_results_ready(
                    task,
                    &pid_result,
                    &bss_was_zero,
                    &bss_probe
                );

                if (results_ready && task != 0 &&
                    task->state == TASK_TERMINATED) {
                    break;
                }

                __asm__ volatile ("hlt" ::: "memory");
            }

            {
                const struct task *task = scheduler_task_by_id(elf_task_id);
                const struct syscall_stats syscall_stats = syscall_get_stats();

                if (!results_ready || task == 0 ||
                    task->state != TASK_TERMINATED ||
                    task->exit_code != AXIOM_PHASE11_EXIT_STATUS ||
                    pid_result != elf_task_id ||
                    bss_was_zero != 1u || bss_probe != 0u ||
                    !task->elf_backed ||
                    task->user_entry != AXIOM_PHASE11_TEXT_BASE ||
                    task->elf_program_headers != 3u ||
                    task->elf_load_segments != 3u ||
                    task->user_elf_page_count != 3u ||
                    !phase11_permissions_ok(task) ||
                    syscall_stats.exec_calls < 1u ||
                    syscall_stats.exec_successes < 1u) {
                    terminal_set_color(
                        TERMINAL_COLOR_LIGHT_RED,
                        TERMINAL_COLOR_BLACK
                    );
                    kprintf("Phase 11 ELF executable test: FAILED\n");
                    return;
                }

                kprintf(
                    "ELF entry point: 0x%llX\n",
                    (unsigned long long)task->user_entry
                );
                kprintf(
                    "ELF program headers: %u\n",
                    (unsigned)task->elf_program_headers
                );
                kprintf(
                    "ELF PT_LOAD segments: %u\n",
                    (unsigned)task->elf_load_segments
                );
                kprintf(
                    "ELF mapped pages: %llu\n",
                    (unsigned long long)task->user_elf_page_count
                );
                kprintf(
                    "ELF file/memory bytes: %llu/%llu\n",
                    (unsigned long long)task->elf_file_bytes,
                    (unsigned long long)task->elf_memory_bytes
                );
                kprintf("Phase 11 segment permissions: RX/R/RW+NX OK\n");
                kprintf(
                    "Phase 11 exec transitions: %llu/%llu\n",
                    (unsigned long long)syscall_stats.exec_successes,
                    (unsigned long long)syscall_stats.exec_calls
                );
                kprintf(
                    "Phase 11 getpid result: %llu\n",
                    (unsigned long long)pid_result
                );
                kprintf("Phase 11 exec preserved task ID: OK\n");
                kprintf("Phase 11 BSS zero-fill: OK\n");
                kprintf(
                    "Phase 11 exit status: %lld\n",
                    (long long)task->exit_code
                );
            }
        }
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 11 ELF executable test: OK\n");
    kprintf("Phase 11 ELF loader complete.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    /*
     * Phase 12: load a standalone ELF through the VFS itself. The program
     * exercises open/read/write/close/lseek/stat on rootfs and the separately
     * mounted /tmp RAM filesystem, then execs /bin/phase11-demo through VFS.
     */
    interrupts_disable();

    {
        struct axiom_stat bin_stat;
        struct axiom_stat tmp_stat;
        void *phase12_image = 0;
        size_t phase12_image_size = 0u;
        uint64_t phase12_task_id;
        const struct syscall_stats syscall_before = syscall_get_stats();
        const struct vfs_stats vfs_before = vfs_get_stats();

        if (vfs_stat("/bin", &bin_stat) < 0 ||
            vfs_stat("/tmp", &tmp_stat) < 0 ||
            bin_stat.type != AXIOM_DT_DIR ||
            tmp_stat.type != AXIOM_DT_DIR ||
            vfs_read_all(
                "/bin/phase12-demo",
                &phase12_image,
                &phase12_image_size
            ) < 0 ||
            !user_task_create_elf(
                "phase12-vfs-demo",
                phase12_image,
                phase12_image_size,
                &phase12_task_id
            )) {
            if (phase12_image != 0) {
                kfree(phase12_image);
            }
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 12 VFS task creation: FAILED\n");
            return;
        }

        kfree(phase12_image);

        {
            const struct vfs_stats mounted = vfs_get_stats();

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_CYAN,
                TERMINAL_COLOR_BLACK
            );
            kprintf("AxiomOS Phase 12 virtual filesystem online.\n");

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_GREY,
                TERMINAL_COLOR_BLACK
            );
            kprintf("VFS root filesystem: ramfs mounted at /\n");
            kprintf("VFS secondary mount: ramfs mounted at /tmp\n");
            kprintf(
                "VFS mount count: %llu\n",
                (unsigned long long)mounted.mount_count
            );
            kprintf("VFS /bin type: DIRECTORY\n");
            kprintf("VFS /tmp type: DIRECTORY (mount point)\n");
            kprintf(
                "Phase 12 ELF task ID: %llu\n",
                (unsigned long long)phase12_task_id
            );
            kprintf("Phase 12 ELF source: /bin/phase12-demo (VFS)\n");
        }

        interrupts_enable();

        {
            const uint64_t start = timer_ticks();
            int target_ready = 0;
            uint64_t pid_result = 0u;
            uint64_t bss_was_zero = 0u;
            uint64_t bss_probe = UINT64_MAX;

            while ((timer_ticks() - start) < PHASE12_TEST_TIMEOUT_TICKS) {
                const struct task *task = scheduler_task_by_id(phase12_task_id);

                target_ready = phase11_results_ready(
                    task,
                    &pid_result,
                    &bss_was_zero,
                    &bss_probe
                );

                if (target_ready && task != 0 &&
                    task->state == TASK_TERMINATED) {
                    break;
                }

                __asm__ volatile ("hlt" ::: "memory");
            }

            interrupts_disable();

            {
                const struct task *task = scheduler_task_by_id(phase12_task_id);
                const struct syscall_stats syscall_after = syscall_get_stats();
                const struct vfs_stats vfs_after = vfs_get_stats();
                void *roundtrip_data = 0;
                size_t roundtrip_size = 0u;
                static const char expected_roundtrip[] =
                    "RAM filesystem round-trip works!\n";
                int roundtrip_ok = 0;

                if (vfs_read_all(
                        "/tmp/phase12.txt",
                        &roundtrip_data,
                        &roundtrip_size
                    ) == 0 &&
                    roundtrip_size == sizeof(expected_roundtrip) - 1u) {
                    size_t index;

                    roundtrip_ok = 1;
                    for (index = 0u; index < roundtrip_size; ++index) {
                        if (((const char *)roundtrip_data)[index] !=
                            expected_roundtrip[index]) {
                            roundtrip_ok = 0;
                            break;
                        }
                    }
                }

                if (roundtrip_data != 0) {
                    kfree(roundtrip_data);
                }

                if (!target_ready || task == 0 ||
                    task->state != TASK_TERMINATED ||
                    task->exit_code != AXIOM_PHASE11_EXIT_STATUS ||
                    pid_result != phase12_task_id ||
                    bss_was_zero != 1u || bss_probe != 0u ||
                    !roundtrip_ok ||
                    vfs_after.mount_count != 2u ||
                    syscall_after.open_calls < syscall_before.open_calls + 2u ||
                    syscall_after.close_calls < syscall_before.close_calls + 2u ||
                    syscall_after.lseek_calls < syscall_before.lseek_calls + 1u ||
                    syscall_after.stat_calls < syscall_before.stat_calls + 2u ||
                    syscall_after.exec_successes < syscall_before.exec_successes + 1u ||
                    vfs_after.reads <= vfs_before.reads ||
                    vfs_after.writes <= vfs_before.writes) {
                    terminal_set_color(
                        TERMINAL_COLOR_LIGHT_RED,
                        TERMINAL_COLOR_BLACK
                    );
                    kprintf("Phase 12 VFS self-test: FAILED\n");
                    return;
                }

                kprintf("Phase 12 file descriptor base: 3\n");
                kprintf("Phase 12 /tmp file round-trip: OK\n");
                kprintf("Phase 12 seek/stat operations: OK\n");
                kprintf("Phase 12 exec through VFS: OK\n");
                kprintf(
                    "Phase 12 VFS opens/closes: %llu/%llu\n",
                    (unsigned long long)vfs_after.opens,
                    (unsigned long long)vfs_after.closes
                );
                kprintf(
                    "Phase 12 VFS bytes read/written: %llu/%llu\n",
                    (unsigned long long)vfs_after.bytes_read,
                    (unsigned long long)vfs_after.bytes_written
                );
            }
        }
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 12 VFS self-test: OK\n");
    kprintf("Phase 12 virtual filesystem complete.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    /*
     * Phase 13: discover the q35 AHCI controller through PCI, expose its SATA
     * disk as a 512-byte block device, mount a tiny persistent filesystem at
     * /disk, and run a Ring-3 program against that mount. Earlier regression
     * tests intentionally boot without a hard disk, so absence is non-fatal.
     */
    interrupts_disable();

    if (block_init()) {
        struct block_device *disk = block_primary();
        struct diskfs_info disk_info;
        struct vfs_filesystem *persistent_fs;
        int persistent_before = 0;
        int mkdir_result;
        void *phase13_image = 0;
        size_t phase13_image_size = 0u;
        uint64_t phase13_task_id = 0u;

        mkdir_result = vfs_mkdir("/disk");
        if (mkdir_result < 0 && mkdir_result != -AXIOM_EEXIST) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 13 diskfs mount: FAILED\n");
            return;
        }

        persistent_fs = diskfs_create(disk, &disk_info);
        if (persistent_fs == 0 || vfs_mount("/disk", persistent_fs) < 0) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 13 diskfs mount: FAILED\n");
            return;
        }

        if (!phase13_persistent_file_matches(&persistent_before)) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 13 persistent file validation: FAILED\n");
            return;
        }

        if (vfs_read_all(
                "/bin/phase13-demo",
                &phase13_image,
                &phase13_image_size
            ) < 0 ||
            !user_task_create_elf(
                "phase13-disk-demo",
                phase13_image,
                phase13_image_size,
                &phase13_task_id
            )) {
            if (phase13_image != 0) {
                kfree(phase13_image);
            }
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 13 disk task creation: FAILED\n");
            return;
        }
        kfree(phase13_image);

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 13 persistent storage online.\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Storage controller: PCI AHCI (q35 SATA)\n");
        kprintf("Block device: %s (512-byte sectors)\n", disk->name);
        kprintf(
            "Phase 13 disk sectors: %llu\n",
            (unsigned long long)disk->sector_count
        );
        kprintf("VFS disk mount: diskfs mounted at /disk\n");
        kprintf(
            "Phase 13 disk freshly formatted: %s\n",
            disk_info.freshly_formatted ? "yes" : "no"
        );
        kprintf(
            "Phase 13 persistent file before user: %s\n",
            persistent_before ? "present" : "absent"
        );
        kprintf(
            "Phase 13 disk task ID: %llu\n",
            (unsigned long long)phase13_task_id
        );

        interrupts_enable();

        {
            const uint64_t start = timer_ticks();

            while ((timer_ticks() - start) < PHASE13_TEST_TIMEOUT_TICKS) {
                const struct task *task = scheduler_task_by_id(phase13_task_id);
                if (task != 0 && task->state == TASK_TERMINATED) {
                    break;
                }
                __asm__ volatile ("hlt" ::: "memory");
            }
        }

        interrupts_disable();

        {
            const struct task *task = scheduler_task_by_id(phase13_task_id);
            struct block_stats block_stats;
            struct diskfs_info final_disk_info;
            int persistent_after = 0;

            if (task == 0 || task->state != TASK_TERMINATED ||
                task->exit_code != 13 ||
                !phase13_persistent_file_matches(&persistent_after) ||
                !persistent_after) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 13 disk round-trip: FAILED\n");
                return;
            }

            if (!block_flush(disk)) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 13 disk flush: FAILED\n");
                return;
            }

            block_stats = block_get_stats();
            final_disk_info = diskfs_get_info();

            kprintf("Phase 13 disk round-trip: OK\n");
            kprintf(
                "Phase 13 diskfs files: %u\n",
                (unsigned)final_disk_info.file_count
            );
            kprintf(
                "Phase 13 block sectors read: %llu\n",
                (unsigned long long)block_stats.sectors_read
            );
            kprintf(
                "Phase 13 block sectors written: %llu\n",
                (unsigned long long)block_stats.sectors_written
            );
            kprintf(
                "Phase 13 block flushes: %llu\n",
                (unsigned long long)block_stats.flush_commands
            );
        }

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREEN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 13 persistent storage complete.\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
    } else {
        kprintf("Phase 13 block device: not present; persistent demo skipped.\n");
    }

    interrupts_enable();

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("AxiomOS Phase 15 process management online.\n");
    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Process table capacity: %u tasks\n", (unsigned)SCHEDULER_MAX_TASKS);
    kprintf("fork policy: eager private-page copy\n");
    kprintf("wait policy: blocking parent + zombie reap\n");
    kprintf("signal policy: SIGTERM immediate termination\n");

    /*
     * Phase 14: launch the interactive command shell as a normal Ring-3 ELF.
     * The shell consumes keyboard input through SYS_read, uses the VFS through
     * file syscalls, and launches other ELF programs through spawn/waitpid.
     */
    {
        void *shell_image = 0;
        size_t shell_image_size = 0u;
        uint64_t shell_task_id = 0u;

        interrupts_disable();

        if (vfs_read_all(
                "/bin/axiomsh",
                &shell_image,
                &shell_image_size
            ) < 0 ||
            !user_task_create_elf(
                "axiomsh",
                shell_image,
                shell_image_size,
                &shell_task_id
            )) {
            if (shell_image != 0) {
                kfree(shell_image);
            }
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 14 shell task creation: FAILED\n");
            return;
        }

        kfree(shell_image);

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 14 interactive shell online.\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Shell executable: /bin/axiomsh\n");
        kprintf("Shell privilege: Ring 3\n");
        kprintf(
            "Phase 14 shell task ID: %llu\n",
            (unsigned long long)shell_task_id
        );
        kprintf("Phase 14 shell launch complete.\n");

        interrupts_enable();
    }

    /* Bootstrap task becomes an idle host while the Ring-3 shell runs. */
    for (;;) {
        __asm__ volatile ("hlt" ::: "memory");
    }
}
```

## `userspace/phase14_shell.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>
#include <axiom/abi/fs.h>
#include <axiom/abi/input.h>
#include <axiom/abi/process.h>
#include <axiom/abi/syscall.h>

#define SHELL_LINE_MAX 256u
#define SHELL_PATH_MAX 256u
#define SHELL_IO_CHUNK 256u

static char cwd[SHELL_PATH_MAX] = "/";

static long syscall0(long number)
{
    register long rax __asm__("rax") = number;
    __asm__ volatile("syscall" : "+a"(rax) : : "rcx", "r11", "memory");
    return rax;
}

static long syscall1(long number, long first)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = first;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi) : "rcx", "r11", "memory");
    return rax;
}

static long syscall2(long number, long first, long second)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = first;
    register long rsi __asm__("rsi") = second;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi) : "rcx", "r11", "memory");
    return rax;
}

static long syscall3(long number, long first, long second, long third)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = first;
    register long rsi __asm__("rsi") = second;
    register long rdx __asm__("rdx") = third;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi), "d"(rdx) : "rcx", "r11", "memory");
    return rax;
}

static size_t text_length(const char *text)
{
    size_t length = 0u;
    if (text == 0) return 0u;
    while (text[length] != '\0') ++length;
    return length;
}

static int text_equal(const char *left, const char *right)
{
    size_t index = 0u;
    if (left == 0 || right == 0) return 0;
    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] != right[index]) return 0;
        ++index;
    }
    return left[index] == right[index];
}

static void copy_text(char *destination, const char *source, size_t capacity)
{
    size_t index = 0u;
    if (destination == 0 || capacity == 0u) return;
    if (source != 0) {
        while (index + 1u < capacity && source[index] != '\0') {
            destination[index] = source[index];
            ++index;
        }
    }
    destination[index] = '\0';
}

static void print_n(const char *text, size_t count)
{
    if (count != 0u) {
        (void)syscall3(AXIOM_SYS_WRITE, 1, (long)(uintptr_t)text, (long)count);
    }
}

static void print(const char *text)
{
    print_n(text, text_length(text));
}

static void print_u64(uint64_t value)
{
    char buffer[32];
    size_t used = 0u;
    if (value == 0u) {
        print("0");
        return;
    }
    while (value != 0u && used < sizeof(buffer)) {
        buffer[used++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (used != 0u) {
        --used;
        print_n(&buffer[used], 1u);
    }
}

static void print_i64(int64_t value)
{
    if (value < 0) {
        print("-");
        print_u64((uint64_t)(-(value + 1)) + 1u);
    } else {
        print_u64((uint64_t)value);
    }
}

static char *skip_spaces(char *text)
{
    while (*text == ' ' || *text == '\t') ++text;
    return text;
}

static char *next_token(char **cursor)
{
    char *start;
    char *position;
    if (cursor == 0 || *cursor == 0) return 0;
    position = skip_spaces(*cursor);
    if (*position == '\0') {
        *cursor = position;
        return 0;
    }
    start = position;
    while (*position != '\0' && *position != ' ' && *position != '\t') ++position;
    if (*position != '\0') *position++ = '\0';
    *cursor = position;
    return start;
}

static int normalize_path(const char *input, char *output, size_t capacity)
{
    char combined[SHELL_PATH_MAX * 2u];
    char *parts[32];
    size_t count = 0u;
    size_t length = 0u;
    char *cursor;

    if (input == 0 || output == 0 || capacity < 2u) return 0;

    if (input[0] == '/') {
        copy_text(combined, input, sizeof(combined));
    } else {
        size_t cwd_len = text_length(cwd);
        size_t input_len = text_length(input);
        if (cwd_len + input_len + 2u > sizeof(combined)) return 0;
        copy_text(combined, cwd, sizeof(combined));
        length = text_length(combined);
        if (length > 1u && combined[length - 1u] != '/') combined[length++] = '/';
        if (length == 1u && combined[0] == '/') { }
        for (size_t i = 0u; i < input_len; ++i) combined[length++] = input[i];
        combined[length] = '\0';
    }

    cursor = combined;
    while (*cursor != '\0') {
        char *start;
        while (*cursor == '/') ++cursor;
        if (*cursor == '\0') break;
        start = cursor;
        while (*cursor != '\0' && *cursor != '/') ++cursor;
        if (*cursor != '\0') *cursor++ = '\0';
        if (text_equal(start, ".")) continue;
        if (text_equal(start, "..")) {
            if (count != 0u) --count;
            continue;
        }
        if (count >= 32u) return 0;
        parts[count++] = start;
    }

    length = 0u;
    output[length++] = '/';
    for (size_t i = 0u; i < count; ++i) {
        const size_t part_len = text_length(parts[i]);
        if (length + part_len + 1u >= capacity) return 0;
        for (size_t j = 0u; j < part_len; ++j) output[length++] = parts[i][j];
        if (i + 1u < count) output[length++] = '/';
    }
    output[length] = '\0';
    return 1;
}

static long open_file(const char *path, long flags)
{
    return syscall2(AXIOM_SYS_OPEN, (long)(uintptr_t)path, flags);
}

static long close_file(long fd) { return syscall1(AXIOM_SYS_CLOSE, fd); }
static long stat_path(const char *path, struct axiom_stat *status)
{
    return syscall2(AXIOM_SYS_STAT, (long)(uintptr_t)path, (long)(uintptr_t)status);
}

static void print_error(const char *operation, long error)
{
    print(operation);
    print(": error ");
    print_i64(error);
    print("\n");
}

static int parse_u64(const char *text, uint64_t *value_out)
{
    uint64_t value = 0u;
    size_t index = 0u;

    if (text == 0 || value_out == 0 || text[0] == '\0') return 0;
    while (text[index] != '\0') {
        const unsigned digit = (unsigned)(text[index] - '0');
        if (digit > 9u || value > (UINT64_MAX - digit) / 10u) return 0;
        value = value * 10u + digit;
        ++index;
    }
    *value_out = value;
    return 1;
}

static const char *process_state_name(uint32_t state)
{
    switch (state) {
        case AXIOM_PROC_RUNNING: return "RUNNING";
        case AXIOM_PROC_READY: return "READY";
        case AXIOM_PROC_BLOCKED: return "BLOCKED";
        case AXIOM_PROC_SLEEPING: return "SLEEPING";
        case AXIOM_PROC_TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

static void command_ps(void)
{
    uint64_t index = 0u;
    long result;
    struct axiom_process_info info;

    print("PID  PPID  STATE       PRIV  TICKS  NAME\n");
    for (;;) {
        result = syscall2(
            AXIOM_SYS_PROCINFO,
            (long)index,
            (long)(uintptr_t)&info
        );
        if (result == 0) break;
        if (result < 0) {
            print_error("ps", result);
            return;
        }

        print_u64(info.pid); print("  ");
        if (info.ppid == UINT64_MAX) print("-"); else print_u64(info.ppid);
        print("  "); print(process_state_name(info.state)); print("  ");
        print(info.privilege == AXIOM_PRIV_USER ? "USER" : "KERN");
        print("  "); print_u64(info.runtime_ticks); print("  ");
        print(info.name); print("\n");
        ++index;
    }
}

static int resolve_program_path(const char *command, char *path, size_t capacity)
{
    if (command == 0 || path == 0 || capacity == 0u) return 0;

    if (command[0] == '/' || command[0] == '.') {
        return normalize_path(command, path, capacity);
    }

    {
        static const char prefix[] = "/bin/";
        const size_t prefix_length = text_length(prefix);
        const size_t command_length = text_length(command);
        size_t index;

        if (prefix_length + command_length + 1u > capacity) return 0;
        copy_text(path, prefix, capacity);
        for (index = 0u; index < command_length; ++index) {
            path[prefix_length + index] = command[index];
        }
        path[prefix_length + command_length] = '\0';
    }
    return 1;
}

static void command_spawn_background(const char *program, const char *extra)
{
    char path[SHELL_PATH_MAX];
    long pid;

    if (program == 0) {
        print("spawn: missing program\n");
        return;
    }
    if (extra != 0 && *skip_spaces((char *)extra) != '\0') {
        print("spawn: program arguments are not supported yet\n");
        return;
    }
    if (!resolve_program_path(program, path, sizeof(path))) {
        print("spawn: program path too long\n");
        return;
    }

    pid = syscall1(AXIOM_SYS_SPAWN, (long)(uintptr_t)path);
    if (pid < 0) {
        print_error("spawn", pid);
        return;
    }
    print("[started pid "); print_u64((uint64_t)pid); print("]\n");
}

static void command_wait(const char *argument)
{
    uint64_t pid;
    int64_t status = 0;
    long result;

    if (!parse_u64(argument, &pid)) {
        print("wait: usage: wait <pid>\n");
        return;
    }
    result = syscall2(AXIOM_SYS_WAITPID, (long)pid, (long)(uintptr_t)&status);
    if (result < 0) {
        print_error("wait", result);
        return;
    }
    print("[process "); print_u64(pid); print(" exited "); print_i64(status); print("]\n");
}

static void command_kill(const char *argument)
{
    uint64_t pid;
    long result;

    if (!parse_u64(argument, &pid)) {
        print("kill: usage: kill <pid>\n");
        return;
    }
    result = syscall2(AXIOM_SYS_KILL, (long)pid, AXIOM_SIGTERM);
    if (result < 0) {
        print_error("kill", result);
        return;
    }
    print("sent SIGTERM to "); print_u64(pid); print("\n");
}

static void command_help(void)
{
    print("AxiomOS shell commands:\n");
    print("  help                 show this help\n");
    print("  clear                clear framebuffer terminal\n");
    print("  echo <text>          print text\n");
    print("  pwd                  print current directory\n");
    print("  cd <dir>             change shell directory\n");
    print("  ls [dir]             list a directory\n");
    print("  cat <file>           print a file\n");
    print("  stat <path>          show file type and size\n");
    print("  touch <file>         create a file\n");
    print("  write <file> <text>  replace file contents\n");
    print("  mkdir <dir>          create a directory\n");
    print("  kbdstats             show PS/2 keyboard counters\n");
    print("  ps                   show process table\n");
    print("  spawn <program>      start a program without waiting\n");
    print("  wait <pid>           wait for one of this shell's children\n");
    print("  kill <pid>           terminate a userspace process (SIGTERM)\n");
    print("  <program>            run /bin/<program> and wait\n");
    print("  /path/program        run an ELF by VFS path\n");
}

static void command_ls(const char *argument)
{
    char path[SHELL_PATH_MAX];
    struct axiom_dirent entry;
    uint64_t index = 0u;
    long result;

    if (!normalize_path(argument != 0 ? argument : ".", path, sizeof(path))) {
        print("ls: path too long\n");
        return;
    }

    for (;;) {
        result = syscall3(
            AXIOM_SYS_READDIR,
            (long)(uintptr_t)path,
            (long)index,
            (long)(uintptr_t)&entry
        );
        if (result == 0) break;
        if (result < 0) {
            print_error("ls", result);
            return;
        }
        print(entry.name);
        if (entry.type == AXIOM_DT_DIR) print("/");
        print("\n");
        ++index;
    }
}

static void command_cat(const char *argument)
{
    char path[SHELL_PATH_MAX];
    char buffer[SHELL_IO_CHUNK];
    long fd;
    long got;

    if (argument == 0) { print("cat: missing file\n"); return; }
    if (!normalize_path(argument, path, sizeof(path))) { print("cat: path too long\n"); return; }
    fd = open_file(path, AXIOM_O_RDONLY);
    if (fd < 0) { print_error("cat", fd); return; }
    for (;;) {
        got = syscall3(AXIOM_SYS_READ, fd, (long)(uintptr_t)buffer, sizeof(buffer));
        if (got < 0) { print_error("cat", got); break; }
        if (got == 0) break;
        print_n(buffer, (size_t)got);
    }
    (void)close_file(fd);
}

static void command_stat(const char *argument)
{
    char path[SHELL_PATH_MAX];
    struct axiom_stat status;
    long result;
    if (argument == 0) { print("stat: missing path\n"); return; }
    if (!normalize_path(argument, path, sizeof(path))) { print("stat: path too long\n"); return; }
    result = stat_path(path, &status);
    if (result < 0) { print_error("stat", result); return; }
    print("type: ");
    print(status.type == AXIOM_DT_DIR ? "directory" : "file");
    print("\nsize: ");
    print_u64(status.size);
    print(" bytes\n");
}

static void command_touch(const char *argument)
{
    char path[SHELL_PATH_MAX];
    long fd;
    if (argument == 0) { print("touch: missing file\n"); return; }
    if (!normalize_path(argument, path, sizeof(path))) { print("touch: path too long\n"); return; }
    fd = open_file(path, AXIOM_O_CREAT | AXIOM_O_RDWR);
    if (fd < 0) { print_error("touch", fd); return; }
    (void)close_file(fd);
}

static void command_write(const char *argument, const char *text)
{
    char path[SHELL_PATH_MAX];
    long fd;
    long result;
    if (argument == 0 || text == 0 || *text == '\0') {
        print("write: usage: write <file> <text>\n");
        return;
    }
    if (!normalize_path(argument, path, sizeof(path))) { print("write: path too long\n"); return; }
    fd = open_file(path, AXIOM_O_CREAT | AXIOM_O_WRONLY | AXIOM_O_TRUNC);
    if (fd < 0) { print_error("write", fd); return; }
    result = syscall3(AXIOM_SYS_WRITE, fd, (long)(uintptr_t)text, (long)text_length(text));
    if (result >= 0) {
        static const char newline = '\n';
        result = syscall3(AXIOM_SYS_WRITE, fd, (long)(uintptr_t)&newline, 1);
    }
    if (result < 0) print_error("write", result);
    (void)close_file(fd);
}

static void command_mkdir(const char *argument)
{
    char path[SHELL_PATH_MAX];
    long result;
    if (argument == 0) { print("mkdir: missing directory\n"); return; }
    if (!normalize_path(argument, path, sizeof(path))) { print("mkdir: path too long\n"); return; }
    result = syscall1(AXIOM_SYS_MKDIR, (long)(uintptr_t)path);
    if (result < 0) print_error("mkdir", result);
}

static void command_cd(const char *argument)
{
    char path[SHELL_PATH_MAX];
    struct axiom_stat status;
    long result;
    if (argument == 0) argument = "/";
    if (!normalize_path(argument, path, sizeof(path))) { print("cd: path too long\n"); return; }
    result = stat_path(path, &status);
    if (result < 0) { print_error("cd", result); return; }
    if (status.type != AXIOM_DT_DIR) { print("cd: not a directory\n"); return; }
    copy_text(cwd, path, sizeof(cwd));
}

static void command_kbdstats(void)
{
    struct axiom_keyboard_stats stats;
    long result = syscall1(AXIOM_SYS_KBDSTATS, (long)(uintptr_t)&stats);
    if (result < 0) { print_error("kbdstats", result); return; }
    print("Keyboard IRQs/scancodes/chars/dropped: ");
    print_u64(stats.irq_count); print("/");
    print_u64(stats.scancode_count); print("/");
    print_u64(stats.character_count); print("/");
    print_u64(stats.dropped_characters); print("\n");
}

static void run_program(const char *command, const char *extra)
{
    char path[SHELL_PATH_MAX];
    int64_t status = 0;
    long pid;
    long waited;

    if (extra != 0 && *skip_spaces((char *)extra) != '\0') {
        print("shell: external program arguments are not supported yet\n");
        return;
    }

    if (!resolve_program_path(command, path, sizeof(path))) {
        print("shell: program path too long\n");
        return;
    }

    pid = syscall1(AXIOM_SYS_SPAWN, (long)(uintptr_t)path);
    if (pid < 0) {
        if (pid == -AXIOM_ENOENT) {
            print("shell: command not found: "); print(command); print("\n");
        } else {
            print_error("spawn", pid);
        }
        return;
    }

    waited = syscall2(AXIOM_SYS_WAITPID, pid, (long)(uintptr_t)&status);
    if (waited < 0) {
        print_error("waitpid", waited);
        return;
    }
    print("[process "); print_u64((uint64_t)pid); print(" exited "); print_i64(status); print("]\n");
}

static void execute_line(char *line)
{
    char *cursor = line;
    char *command = next_token(&cursor);
    char *first;
    char *rest;

    if (command == 0) return;
    first = next_token(&cursor);
    rest = skip_spaces(cursor);

    if (text_equal(command, "help")) command_help();
    else if (text_equal(command, "clear")) (void)syscall0(AXIOM_SYS_CLEAR);
    else if (text_equal(command, "echo")) { if (first != 0) { print(first); if (*rest != '\0') { print(" "); print(rest); } } print("\n"); }
    else if (text_equal(command, "pwd")) { print(cwd); print("\n"); }
    else if (text_equal(command, "cd")) command_cd(first);
    else if (text_equal(command, "ls")) command_ls(first);
    else if (text_equal(command, "cat")) command_cat(first);
    else if (text_equal(command, "stat")) command_stat(first);
    else if (text_equal(command, "touch")) command_touch(first);
    else if (text_equal(command, "write")) command_write(first, rest);
    else if (text_equal(command, "mkdir")) command_mkdir(first);
    else if (text_equal(command, "kbdstats")) command_kbdstats();
    else if (text_equal(command, "ps")) command_ps();
    else if (text_equal(command, "spawn")) command_spawn_background(first, rest);
    else if (text_equal(command, "wait")) command_wait(first);
    else if (text_equal(command, "kill")) command_kill(first);
    else run_program(command, first != 0 ? first : 0);
}

static size_t read_line(char *line, size_t capacity)
{
    size_t length = 0u;
    for (;;) {
        char character = '\0';
        long got = syscall3(AXIOM_SYS_READ, 0, (long)(uintptr_t)&character, 1);
        if (got < 0) return 0u;
        if (got == 0) {
            (void)syscall1(AXIOM_SYS_SLEEP, 10);
            continue;
        }
        if (character == '\n') {
            print("\n");
            line[length] = '\0';
            return length;
        }
        if (character == '\b') {
            if (length != 0u) {
                --length;
                line[length] = '\0';
                print("\b");
            }
            continue;
        }
        if (character >= 32 && character < 127 && length + 1u < capacity) {
            line[length++] = character;
            line[length] = '\0';
            print_n(&character, 1u);
        }
    }
}

__attribute__((noreturn)) void shell_main(void)
{
    char line[SHELL_LINE_MAX];

    print("AxiomOS shell ready. Type 'help' for commands.\n");
    for (;;) {
        print("axiom> ");
        (void)read_line(line, sizeof(line));
        execute_line(line);
    }
}
```

## `userspace/phase15_demo.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/syscall.h>

static volatile uint64_t fork_marker = 0x1111111111111111ULL;

static long syscall0(long number)
{
    register long rax __asm__("rax") = number;
    __asm__ volatile("syscall" : "+a"(rax) : : "rcx", "r11", "memory");
    return rax;
}

static long syscall1(long number, long first)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = first;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi) : "rcx", "r11", "memory");
    return rax;
}

static long syscall2(long number, long first, long second)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = first;
    register long rsi __asm__("rsi") = second;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi) : "rcx", "r11", "memory");
    return rax;
}

static long syscall3(long number, long first, long second, long third)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = first;
    register long rsi __asm__("rsi") = second;
    register long rdx __asm__("rdx") = third;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi), "d"(rdx) : "rcx", "r11", "memory");
    return rax;
}

static size_t text_length(const char *text)
{
    size_t length = 0u;
    while (text[length] != '\0') ++length;
    return length;
}

static void print(const char *text)
{
    (void)syscall3(
        AXIOM_SYS_WRITE,
        1,
        (long)(uintptr_t)text,
        (long)text_length(text)
    );
}

static _Noreturn void exit_now(long status)
{
    (void)syscall1(AXIOM_SYS_EXIT, status);
    for (;;) { }
}

int phase15_main(void)
{
    const long parent_pid = syscall0(AXIOM_SYS_GETPID);
    long child;
    int64_t status = -1;

    print("Phase 15 fork demo starting.\n");

    child = syscall0(AXIOM_SYS_FORK);
    if (child < 0) {
        print("Phase 15 fork: FAILED\n");
        return 80;
    }

    if (child == 0) {
        static const char target[] = "/bin/phase11-demo";
        if (syscall0(AXIOM_SYS_GETPPID) != parent_pid) {
            exit_now(81);
        }
        print("Phase 15 child: fork returned 0; exec follows.\n");
        if (syscall1(AXIOM_SYS_EXEC, (long)(uintptr_t)target) < 0) {
            exit_now(82);
        }
        exit_now(83);
    }

    if (syscall2(AXIOM_SYS_WAITPID, child, (long)(uintptr_t)&status) != child ||
        status != 11) {
        print("Phase 15 fork/exec/wait: FAILED\n");
        return 84;
    }
    print("Phase 15 fork/exec/wait: OK\n");

    status = -1;
    child = syscall0(AXIOM_SYS_FORK);
    if (child < 0) {
        print("Phase 15 second fork: FAILED\n");
        return 85;
    }

    if (child == 0) {
        fork_marker = 0x2222222222222222ULL;
        (void)syscall1(AXIOM_SYS_SLEEP, 150);
        exit_now(42);
    }

    if (syscall2(AXIOM_SYS_WAITPID, child, (long)(uintptr_t)&status) != child ||
        status != 42) {
        print("Phase 15 blocked wait/sleep: FAILED\n");
        return 86;
    }
    print("Phase 15 blocked wait/sleep: OK\n");

    if (fork_marker != 0x1111111111111111ULL) {
        print("Phase 15 fork memory isolation: FAILED\n");
        return 87;
    }
    print("Phase 15 fork memory isolation: OK\n");
    print("Phase 15 process management demo complete.\n");
    return 15;
}
```

## `userspace/phase15_demo_start.S`

```asm
#include <axiom/abi/syscall.h>
.section .text.start,"ax",@progbits
.code64
.global _start
.type _start, @function
.extern phase15_main
_start:
    movw $0x1B, %ax
    movw %ax, %ds
    movw %ax, %es
    call phase15_main
    movq %rax, %rdi
    movl $AXIOM_SYS_EXIT, %eax
    syscall
    ud2
.size _start, .-_start
.section .note.GNU-stack,"",@progbits
```

## `userspace/phase15_sleeper.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/syscall.h>

static long syscall1(long number, long first)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = first;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi) : "rcx", "r11", "memory");
    return rax;
}

static long syscall3(long number, long first, long second, long third)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = first;
    register long rsi __asm__("rsi") = second;
    register long rdx __asm__("rdx") = third;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi), "d"(rdx) : "rcx", "r11", "memory");
    return rax;
}

static size_t text_length(const char *text)
{
    size_t length = 0u;
    while (text[length] != '\0') ++length;
    return length;
}

static void print(const char *text)
{
    (void)syscall3(
        AXIOM_SYS_WRITE,
        1,
        (long)(uintptr_t)text,
        (long)text_length(text)
    );
}

int phase15_sleeper_main(void)
{
    print("Phase 15 sleeper started; waiting for SIGTERM.\n");
    (void)syscall1(AXIOM_SYS_SLEEP, 60000);
    print("Phase 15 sleeper woke naturally.\n");
    return 0;
}
```

## `userspace/phase15_sleeper_start.S`

```asm
#include <axiom/abi/syscall.h>
.section .text.start,"ax",@progbits
.code64
.global _start
.type _start, @function
.extern phase15_sleeper_main
_start:
    movw $0x1B, %ax
    movw %ax, %ds
    movw %ax, %es
    call phase15_sleeper_main
    movq %rax, %rdi
    movl $AXIOM_SYS_EXIT, %eax
    syscall
    ud2
.size _start, .-_start
.section .note.GNU-stack,"",@progbits
```

## `tests/phase15_processes.py`

```python
#!/usr/bin/env python3
"""Validate Phase-15 fork/wait/process-table/kill lifecycle semantics."""

from pathlib import Path
import re
import socket
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def serial_text(path):
    return path.read_text(errors="replace").replace("\r", "")


def wait_for_text(process, serial, marker, deadline):
    while True:
        text = serial_text(serial)
        if marker in text:
            return text
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-140:])
            raise AssertionError(
                f"phase15 timed out waiting for {marker!r}; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )
        time.sleep(0.05)


def wait_for_prompt_count(process, serial, count, deadline):
    while True:
        text = serial_text(serial)
        if text.count("axiom> ") >= count:
            return text
        require(process.poll() is None, "QEMU exited while waiting for shell prompt")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-120:])
            raise AssertionError(
                f"phase15 timed out waiting for prompt #{count}\n--- serial tail ---\n{tail}"
            )
        time.sleep(0.05)


def send_hmp_keys(monitor_path, keys):
    deadline = time.monotonic() + 5
    while not monitor_path.exists():
        require(time.monotonic() < deadline, "QEMU monitor socket was not created")
        time.sleep(0.05)

    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
        connection.settimeout(2)
        connection.connect(str(monitor_path))
        try:
            connection.recv(4096)
        except socket.timeout:
            pass
        for key in keys:
            connection.sendall(f"sendkey {key} 20\n".encode("ascii"))
            time.sleep(0.035)


def key_for_character(character):
    if "a" <= character <= "z" or "0" <= character <= "9":
        return character
    mapping = {
        " ": "spc",
        "/": "slash",
        ".": "dot",
        "-": "minus",
        "_": "shift-minus",
    }
    require(character in mapping, f"phase15: no HMP key mapping for {character!r}")
    return mapping[character]


def send_command(monitor_path, command):
    send_hmp_keys(
        monitor_path,
        [key_for_character(character) for character in command] + ["ret"],
    )


def run_command(process, serial, monitor, command, marker=None, timeout=25):
    before = serial_text(serial)
    prompt_count = before.count("axiom> ")
    send_command(monitor, command)
    if marker is not None:
        wait_for_text(process, serial, marker, time.monotonic() + timeout)
    return wait_for_prompt_count(
        process, serial, prompt_count + 1, time.monotonic() + timeout
    )


def test_phase15():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase15-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    for filename in ("phase15_demo.elf", "phase15_sleeper.elf"):
        require((directory / "userspace" / filename).exists(),
                f"phase15: {filename} was not built")

    disk = directory / "phase15-test-disk.img"
    with disk.open("wb") as handle:
        handle.truncate(16 * 1024 * 1024)

    serial = directory / "phase15-serial.log"
    qemu_log = directory / "phase15-qemu.log"
    monitor = directory / "phase15-monitor.sock"
    serial.write_text("")
    monitor.unlink(missing_ok=True)

    command = [
        "qemu-system-x86_64",
        "-machine", "q35",
        "-accel", "tcg",
        "-m", "256M",
        "-cdrom", str(directory / "AxiomOS.iso"),
        "-boot", "d",
        "-drive", f"file={disk},format=raw,if=none,id=axiomdisk",
        "-device", "ide-hd,drive=axiomdisk,bus=ide.0",
        "-display", "none",
        "-serial", f"file:{serial}",
        "-monitor", f"unix:{monitor},server=on,wait=off",
        "-no-reboot",
        "-no-shutdown",
    ]

    with qemu_log.open("w") as output:
        process = subprocess.Popen(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)
        try:
            text = wait_for_text(
                process,
                serial,
                "AxiomOS shell ready. Type 'help' for commands.",
                time.monotonic() + 150,
            )
            require("AxiomOS Phase 15 process management online." in text,
                    "phase15: process-management banner missing")
            require("fork policy: eager private-page copy" in text,
                    "phase15: fork policy banner missing")
            require("KERNEL PANIC" not in text, "phase15: panic before shell")

            text = run_command(
                process, serial, monitor, "phase15-demo",
                "Phase 15 process management demo complete.", 35,
            )
            for marker in (
                "Phase 15 fork/exec/wait: OK",
                "Phase 15 blocked wait/sleep: OK",
                "Phase 15 fork memory isolation: OK",
            ):
                require(marker in text, f"phase15: missing {marker!r}")
            require(re.search(r"\[process \d+ exited 15\]", text) is not None,
                    "phase15: demo process exit status was not collected")

            text = run_command(process, serial, monitor, "ps", "PID  PPID  STATE")
            require("axiomsh" in text, "phase15: ps does not show the shell")
            require("RUNNING" in text or "READY" in text,
                    "phase15: ps does not expose scheduler state")

            text = run_command(
                process, serial, monitor, "spawn phase15-sleeper",
                "Phase 15 sleeper started; waiting for SIGTERM.", 25,
            )
            matches = re.findall(r"\[started pid (\d+)\]", text)
            require(matches, "phase15: background spawn did not report a pid")
            sleeper_pid = int(matches[-1])

            text = run_command(process, serial, monitor, "ps", "PID  PPID  STATE")
            require(re.search(rf"\b{sleeper_pid}\b.*SLEEPING.*spawned-elf", text) is not None,
                    "phase15: sleeping background child not visible in ps")

            run_command(
                process, serial, monitor, f"kill {sleeper_pid}",
                f"sent SIGTERM to {sleeper_pid}", 20,
            )
            text = run_command(
                process, serial, monitor, f"wait {sleeper_pid}",
                f"[process {sleeper_pid} exited 143]", 20,
            )
            require("error" not in text.split(f"wait {sleeper_pid}")[-1],
                    "phase15: wait after SIGTERM returned an error")

            # Reaping the killed child must make its slot reusable.
            text = run_command(
                process, serial, monitor, "phase11-demo",
                "Hello from an ELF64 executable in AxiomOS!", 25,
            )
            require(re.search(r"\[process \d+ exited 11\]", text) is not None,
                    "phase15: task slot was not reusable after wait/reap")

            final = serial_text(serial)
            require("KERNEL PANIC" not in final, "phase15: kernel panicked")
            require("Phase 15 fork: FAILED" not in final, "phase15: fork failed")
            require("Phase 15 fork/exec/wait: FAILED" not in final,
                    "phase15: fork/exec/wait failed")
            require("Phase 15 fork memory isolation: FAILED" not in final,
                    "phase15: fork memory was shared unexpectedly")

            print("PASS: Phase 15 eager-copy fork semantics")
            print("PASS: Phase 15 exec + blocking waitpid + zombie reap")
            print("PASS: Phase 15 process table / ps")
            print("PASS: Phase 15 simplified SIGTERM kill")
            print("PASS: Phase 15 scheduler blocking/sleep wakeups")
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
    test_phase15()
    print("PASS: all Phase 15 process-management tests.")
```

## `docs/phase15.md`

```markdown
# Phase 15 — Process management

Phase 15 turns the Phase-14 foreground `spawn()` mechanism into a fuller
process lifecycle. AxiomOS now supports parent/child relationships, eager-copy
`fork()`, `exec()`, blocking `waitpid()`, exit-status collection, process-table
inspection, sleeping/blocking scheduler states, and a deliberately small
SIGTERM-style termination mechanism.

## Process lifecycle

Every task already has a unique PID. User processes now additionally use:

- `parent_id` — PID of the creating process;
- `TASK_BLOCKED` — used while a parent waits for a child;
- `TASK_SLEEPING` — used by `sleep()` until an APIC-timer deadline;
- `TASK_TERMINATED` — also acts as the zombie state for an unreaped child;
- `wait_target_id` — child a blocked parent is waiting for;
- `wait_collected` — whether the parent collected the child's exit status;
- `termination_signal` — nonzero when the simplified signal path killed it.

A terminated child whose parent is still alive is not immediately reclaimed.
`waitpid()` reads its exit status and marks it reaped; only then may a future
process creation reuse the task slot and release the old address-space pages,
stack, and other resources. Parentless historical test tasks remain immediately
reclaimable so the incremental boot self-tests do not exhaust the table.

The Phase-15 process table has 16 slots. It remains intentionally fixed-size;
dynamic process-table allocation can be introduced later without changing the
userspace ABI.

## `fork()`

AxiomOS implements an eager-copy fork rather than copy-on-write.

When a Ring-3 process calls `fork()`:

1. the scheduler reserves a task slot and private kernel stack;
2. it creates a fresh user CR3/PML4;
3. every mapped ELF/code/data page is copied to a new physical page;
4. every user-stack page is copied to a new physical page;
5. the child's mappings preserve the parent's user/write/NX permissions;
6. open VFS file descriptions are retained and shared, so file offsets are
   shared across parent and child;
7. a synthetic interrupt frame is built from the parent's syscall return state;
8. the parent receives the child PID while the child resumes after the same
   `fork()` syscall with RAX=0.

This uses more RAM than copy-on-write, but makes ownership and teardown simple
and gives a clean baseline before introducing COW page-fault policy later.

## `exec()`

The existing ELF64 `exec()` path remains process-image replacement: it creates a
fresh address space, loads PT_LOAD segments through the Phase-11 ELF loader,
installs a new user stack, switches CR3, frees the previous image, and returns
to the new ELF entry point. PID and `parent_id` stay unchanged.

## Blocking `waitpid()`

Phase 14 implemented foreground waiting by repeatedly yielding. Phase 15 uses a
real blocked state:

```text
parent calls waitpid(child)
        ↓
parent -> TASK_BLOCKED
        ↓
scheduler runs other READY tasks
        ↓
child exits / faults / is killed
        ↓
waiting parent -> TASK_READY
        ↓
parent resumes inside waitpid()
        ↓
exit status copied to userspace
        ↓
child marked reaped
```

This is still a single-CPU scheduler, so state transitions are protected by the
same interrupt-disabled critical sections used by the existing scheduler.

## Simplified signals

Phase 15 adds only `SIGTERM` (`15`). There are no user signal handlers, masks,
pending-signal queues, or asynchronous delivery frames yet.

`kill(pid, SIGTERM)` immediately terminates a Ring-3 target, records exit status
`128 + 15 = 143`, closes its descriptors, and wakes a parent blocked in
`waitpid()`. Kernel tasks cannot be killed from userspace.

## Process-table ABI

`procinfo(index, &info)` exposes one scheduler slot at a time through a validated
Ring-3 buffer. `struct axiom_process_info` contains PID, PPID, state, privilege,
runtime ticks, exit status, terminating signal, and a short process name.

The Ring-3 shell uses it for:

```text
axiom> ps
PID  PPID  STATE       PRIV  TICKS  NAME
...
```

The shell also gains:

```text
spawn <program>   # background launch
wait <pid>        # collect one child
kill <pid>        # send SIGTERM
ps                # inspect process table
```

Normal external commands still run in the foreground through `spawn()+waitpid()`.

## Phase-15 demo

`/bin/phase15-demo` verifies:

- parent receives a positive PID from `fork()`;
- child receives zero;
- child's `getppid()` matches the parent;
- a forked child can `exec("/bin/phase11-demo")`;
- parent blocks and collects exit status 11;
- a second child sleeps and exits with status 42;
- the parent blocks until wakeup;
- a child write to a global variable does not modify the parent's copy.

`/bin/phase15-sleeper` exists for the interactive `ps`/`kill` test.

## Acceptance

```bash
make clean
make
make test-phase15
make test
```

The dedicated QEMU test drives the Ring-3 shell through PS/2 input, executes the
fork demo, checks `ps`, launches a sleeper in the background, confirms it is
SLEEPING, terminates it with SIGTERM, waits for status 143, and launches another
ELF afterward to prove the reaped slot is reusable.

## Deliberate limits

- eager copying instead of copy-on-write fork;
- fixed 16-slot process table;
- only SIGTERM, with immediate termination and no handlers;
- no `argv`/environment yet;
- no pipes, redirection, job-control process groups, sessions, or terminals;
- no SMP synchronization yet;
- scheduler policy remains round-robin.
```

## `docs/processes.md`

```markdown
# Tasks, scheduling, and process management — Phase 15

AxiomOS uses a single-CPU preemptive round-robin scheduler driven by the 100 Hz
Local APIC timer. The default quantum is five ticks (about 50 ms).

Each `struct task` records PID/PPID, RUNNING/READY/BLOCKED/SLEEPING/TERMINATED
state, Ring-0/Ring-3 privilege, saved CPU context, private kernel stack, CR3,
user image/stack physical pages, open files, fault/exit information, wait state,
and runtime/scheduling counters.

Kernel threads share the kernel CR3. Ring-3 tasks own separate lower-half
address spaces while sharing supervisor-only higher-half kernel mappings.

## Process creation

`spawn(path)` creates a fresh ELF-backed Ring-3 task from a VFS executable.
`fork()` instead clones the currently running userspace process. Phase 15 uses
an eager copy: user image pages and stack pages receive new physical frames and
identical mappings. The child's CPU return state is synthesized so the parent
sees the child PID and the child sees zero from the same syscall.

Open VFS file descriptions are reference-counted and shared across fork, so the
file offset is shared until descriptors are closed.

## Exec and exit

`exec(path)` replaces the current process image while preserving PID and PPID.
`exit(status)` closes descriptors, records status, changes the task to
TERMINATED, and wakes a parent waiting for that PID.

A terminated child with a live parent is retained as an unreaped zombie until
`waitpid()` collects its status. Reaped or orphaned terminated tasks can be
reclaimed by later task creation.

## Waiting, sleeping, and blocking

`sleep(ms)` places a task in SLEEPING until the APIC scheduling tick reaches its
wake deadline. `waitpid()` places a parent in BLOCKED with a target child PID.
The scheduler excludes BLOCKED and SLEEPING tasks from READY selection. Child
termination promotes the blocked parent back to READY.

This replaces Phase 14's yield-polling wait loop with a real scheduler state
transition.

## Simplified signal termination

`kill(pid, SIGTERM)` immediately terminates a Ring-3 target with status 143 and
wakes any waiting parent. Kernel tasks are protected. This is intentionally only
a first signal mechanism: no handler registration, masks, pending queues, or
signal-return frames exist yet.

## Process inspection

`procinfo(index, &info)` exposes scheduler process data to Ring 3. `/bin/axiomsh`
uses it for `ps` and also provides background `spawn`, `wait`, and `kill`
commands.
```

## `docs/syscalls.md`

```markdown
# AxiomOS syscall ABI

Phase 10 introduced the x86-64 `SYSCALL` / `SYSRETQ` boundary. Arguments use
`RDI, RSI, RDX, R10, R8, R9`; `RAX` contains the syscall number on entry and the
signed result on return.

Implemented calls now include:

```text
write
read
_exit
sleep
getpid
yield
open
close
exec
lseek
stat
```

``mmap` retains a stable ABI number but still returns `-ENOSYS` until its
owning virtual-memory/userspace-library work is implemented. `fork` is now
implemented by Phase 15.

All pointer-bearing calls validate Ring-3 ranges through the VMM before copying
bytes. Bad user pointers return `-EFAULT`; the kernel never trusts a raw user
pointer.

## File syscalls in Phase 12

`open()` resolves an absolute path through the VFS and installs a
`struct vfs_file` into the current task's descriptor table. Regular descriptors
start at 3 because 0/1/2 remain stdin/stdout/stderr.

`read()` and `write()` dispatch either to keyboard/terminal semantics for the
standard descriptors or to VFS file operations for regular descriptors.

`lseek()` changes a descriptor's independent file offset. `stat()` copies a
small `struct axiom_stat` containing size/type/mode back into validated user
memory. `close()` releases the open-file object and descriptor slot.

## `exec`

`SYS_exec` now copies the path from userspace, reads the target ELF bytes using
`vfs_read_all()`, and passes that kernel buffer into the existing Phase-11 ELF
image-replacement path. The temporary hardcoded boot-module executable registry
is gone.

## Phase 14 additions

The interactive Ring-3 shell extends the ABI without changing the x86-64
`SYSCALL/SYSRET` entry mechanism:

| Number | Call | Purpose |
|---:|---|---|
| 14 | `readdir` | Enumerate a VFS directory by index |
| 15 | `mkdir` | Create a directory |
| 16 | `spawn` | Load a VFS ELF into a new Ring-3 task |
| 17 | `waitpid` | Wait for a spawned child and collect exit status |
| 18 | `clear` | Clear the framebuffer terminal |
| 19 | `kbdstats` | Read PS/2 keyboard diagnostic counters |

`spawn()` is deliberately smaller than POSIX `fork()+exec()`: Phase 14 needs a
safe foreground command-launch mechanism, while full process semantics remain
Phase 15 work.

## Phase 15 process calls

| Number | Call | Purpose |
|---:|---|---|
| 9 | `fork` | Eager-copy the current Ring-3 process |
| 20 | `procinfo` | Copy one process-table entry to userspace |
| 21 | `kill` | Send the simplified SIGTERM to a Ring-3 process |
| 22 | `getppid` | Return the current process parent PID |

`waitpid` now blocks the parent in `TASK_BLOCKED` instead of polling with
`yield()`. A terminated child keeps its exit status until the parent reaps it.
Open file descriptions use reference counting so descriptors inherited across
`fork()` share the same VFS offset.
```

## `docs/project-state.md`

```markdown
# AxiomOS handoff — Phase 15 implementation

Development environment: Linux/Distrobox on Steam Deck. Run commands from the
directory containing `Makefile`. Roadmap: Phase 0 through Phase 25.

Accepted foundation before Phase 15:

- Phase 0: development environment.
- Phase 1: bootable x86-64 kernel and serial output.
- Phase 2: framebuffer terminal and custom formatter.
- Phase 3: GDT/TSS, IDT, exceptions/interrupts and panic diagnostics.
- Phase 4: bitmap physical page allocator.
- Phase 5: AxiomOS-owned virtual memory and page faults.
- Phase 6: PMM/VMM-backed kernel heap.
- Phase 7: q35 APIC timer + PS/2 keyboard.
- Phase 8: preemptive round-robin multitasking.
- Phase 9: isolated Ring-3 address spaces.
- Phase 10: x86-64 syscall boundary and user-copy validation.
- Phase 11: standalone ELF64 loading and exec image replacement.
- Phase 12: VFS, RAMFS, mounts, file descriptors and file syscalls.
- Phase 13: PCI/AHCI block storage and persistent `/disk` diskfs.
- Phase 14: `/bin/axiomsh`, a real interactive Ring-3 shell.

Phase 15 adds:

- eager-copy Ring-3 `fork()`;
- proper parent/child PID relationships and `getppid()`;
- preserved PID/PPID across `exec()`;
- true blocking `waitpid()` rather than yield polling;
- zombie/reap semantics for terminated children;
- shared reference-counted VFS open file descriptions across fork;
- scheduler-managed BLOCKED and SLEEPING states;
- process table exposure through `procinfo`;
- a simplified immediate SIGTERM path through `kill`;
- shell `ps`, background `spawn`, `wait`, and `kill`;
- `/bin/phase15-demo` for fork/exec/wait/memory-isolation validation;
- `/bin/phase15-sleeper` for interactive ps/kill validation;
- a dedicated QEMU Phase-15 acceptance test.

Important limits:

- fork eagerly copies pages; no copy-on-write yet;
- fixed 16-slot process table;
- only SIGTERM, with no user signal handlers/masks;
- no argv/environment passing;
- no pipes, redirection, process groups, sessions or job control;
- scheduler policy remains round-robin;
- single CPU only.

Acceptance commands:

```bash
make clean
make
make test-phase15
make test
```

Next milestone: Phase 16 — synchronization primitives and wait queues.
```

## `docs/validation.md`

```markdown
# Validation

AxiomOS uses phase-specific QEMU regression tests plus a full cumulative suite.

Current acceptance commands:

```bash
make clean
make
make test-phase12
make test
```

`make test` runs Phase 1 through Phase 12. Destructive fault/corruption cases
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
```

