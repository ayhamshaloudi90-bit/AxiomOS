# AxiomOS Phase 16 FIXED — complete new/modified source

This document records the Phase-16-related files from the corrected build tree. The ZIP remains authoritative.

## `Makefile`

```makefile
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
	kernel/sync/spinlock.c \
	kernel/sync/wait_queue.c \
	kernel/sync/mutex.c \
	kernel/sync/semaphore.c \
	kernel/sync/selftest.c \
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
	$(MAKE) test-phase16


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
	@echo "AxiomOS Phase 16 targets:"
	@echo
	@echo "  make          Build kernel and bootable ISO"
	@echo "  make run      Boot AxiomOS using QEMU"
	@echo "  make debug    Boot QEMU paused for GDB"
	@echo "  make test     Run Phase 1 through Phase 16 tests"
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
	@echo "  make test-phase16 Run synchronization/race/wait-queue tests"
	@echo "  make disk-image Create the 16 MiB persistent QEMU disk image"
	@echo "  make MODE=heap_double_free run  Trigger double-free panic"
	@echo "  make MODE=heap_guard run        Trigger tail-guard panic"
	@echo "  make inspect  Inspect generated ELF64 kernel"
	@echo "  make clean    Remove generated build files"
	@echo "  make distclean Remove build files and Limine"

.PHONY: test-phase3 test-phase4 test-phase5 test-phase6 test-phase7 test-phase8 test-phase9 test-phase10 test-phase11 test-phase12 test-phase13 test-phase14 test-phase15 test-phase16 disk-image

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

test-phase16:
	python3 tests/phase16_sync.py

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

Phases 0–15 provide boot, memory management, interrupts, preemptive scheduling,
Ring-3 isolation, syscalls, ELF64 loading, VFS + persistent AHCI storage, a real
interactive shell, and fork/exec/wait process management.

Phase 16 adds kernel synchronization:

- a deterministic lost-update race demonstration;
- IRQ-save x86 spinlocks for short critical sections;
- FIFO scheduler wait queues;
- sleeping mutexes with direct waiter handoff;
- counting semaphores;
- reusable BLOCKED/READY scheduler park/wakeup hooks;
- tests proving protected counters and a semaphore concurrency limit.

The synchronization layer is currently designed for AxiomOS's single CPU.
Phase 17 will extend the machine to SMP, where the same lock APIs must also
protect against another CPU executing truly in parallel.

## Common commands

```bash
make
make run
make test
make test-phase13
make test-phase14
make test-phase15
make test-phase16
make MODE=page_fault run
make MODE=heap_double_free run
make MODE=heap_guard run
make debug
```

`make test` runs the complete Phase 1 through Phase 16 regression suite.

When `make run` reaches the shell:

```text
AxiomOS shell ready. Type 'help' for commands.
axiom> help
```

The next milestone is Phase 17: SMP / multicore support.

[Architecture](docs/architecture.md) | [Memory](docs/memory.md) |
[Tasks](docs/processes.md) | [Syscalls](docs/syscalls.md) |
[Filesystem](docs/filesystem.md) | [Phase 13](docs/phase13.md) |
[Phase 14](docs/phase14.md) | [Phase 15](docs/phase15.md) | [Phase 16](docs/phase16.md) | [Project state](docs/project-state.md) |
[Original roadmap](docs/roadmap.md)
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

/* Phase 16: generic blocking/wakeup hooks used by kernel wait queues. IF=0. */
int scheduler_prepare_block_current(uint64_t expected_task_id);
int scheduler_park_current(uint64_t expected_task_id);
int scheduler_wake_task(uint64_t task_id);

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

int scheduler_prepare_block_current(uint64_t expected_task_id)
{
    struct task *current;

    if (!initialized || !running || interrupts_enabled() ||
        current_index >= task_count_value) {
        return 0;
    }

    current = &tasks[current_index];
    if (current->id != expected_task_id || current->state != TASK_RUNNING) {
        return 0;
    }

    current->state = TASK_BLOCKED;
    current->wait_target_id = UINT64_MAX;
    current->ticks_in_slice = current->quantum_ticks;
    return 1;
}

int scheduler_park_current(uint64_t expected_task_id)
{
    if (!initialized || !running || interrupts_enabled()) {
        return 0;
    }

    /*
     * The caller has already made the current task non-runnable while IF=0.
     * Enabling interrupts lets the next APIC timer interrupt switch away. A
     * wakeup changes this task to READY; once scheduled again it is RUNNING
     * and execution resumes here.
     */
    interrupts_enable();

    for (;;) {
        if (current_index < task_count_value &&
            tasks[current_index].id == expected_task_id &&
            tasks[current_index].state == TASK_RUNNING) {
            break;
        }

        __asm__ volatile ("hlt" ::: "memory");
    }

    interrupts_disable();
    return 1;
}

int scheduler_wake_task(uint64_t task_id)
{
    struct task *task;

    if (!initialized || !running || interrupts_enabled()) {
        return 0;
    }

    task = scheduler_task_by_id_mutable(task_id);
    if (task == 0 || task->state != TASK_BLOCKED) {
        return 0;
    }

    task->state = TASK_READY;
    task->wait_target_id = UINT64_MAX;
    return 1;
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
#include <axiom/sync/selftest.h>

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
static volatile int phase8_workers_stop;

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

    /*
     * These workers exist only to prove timer-driven preemption in Phase 8.
     * Leaving them alive forever would burn two full scheduler quanta in every
     * later round-robin cycle and distort blocking/synchronization tests.
     */
    while (!phase8_workers_stop) {
        ++(*counter);
        __asm__ volatile ("pause" ::: "memory");
    }

    task_exit_current(0);
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
    phase8_workers_stop = 0;

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

    /*
     * The two Phase-8 workers are test fixtures, not kernel services. Stop
     * them after their acceptance criteria have been measured so later phases
     * are not forced to share the CPU with permanent synthetic busy loops.
     */
    phase8_workers_stop = 1;

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
     * Phase 16: synchronization primitives. First demonstrate a lost-update
     * race on purpose, then prove spinlocks, sleeping mutexes, semaphores, and
     * FIFO wait queues repair/control the same kind of concurrency.
     */
    {
        struct sync_selftest_result sync_result;

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 16 synchronization online.\n");
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Synchronization CPU model: single CPU, preemptive tasks\n");
        kprintf("Spinlock policy: IRQ-save busy lock for short critical sections\n");
        kprintf("Mutex policy: blocking FIFO handoff\n");
        kprintf("Semaphore policy: counting tokens + blocking FIFO waiters\n");

        if (!sync_run_selftest(&sync_result)) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf(
                "Phase 16 synchronization failure stage: %llu\n",
                (unsigned long long)sync_result.failure_stage
            );
            kprintf("Phase 16 synchronization self-test: FAILED\n");
            return;
        }

        kprintf(
            "Phase 16 race expected/observed: %llu/%llu\n",
            (unsigned long long)sync_result.race_expected,
            (unsigned long long)sync_result.race_actual
        );
        kprintf("Phase 16 race condition demonstrated: OK\n");
        kprintf(
            "Phase 16 spinlock counter expected/actual: %llu/%llu\n",
            (unsigned long long)sync_result.spin_expected,
            (unsigned long long)sync_result.spin_actual
        );
        kprintf(
            "Phase 16 spinlock acquisitions: %llu\n",
            (unsigned long long)sync_result.spin_acquisitions
        );
        kprintf("Phase 16 spinlock protected counter: OK\n");
        kprintf(
            "Phase 16 mutex counter expected/actual: %llu/%llu\n",
            (unsigned long long)sync_result.mutex_expected,
            (unsigned long long)sync_result.mutex_actual
        );
        kprintf(
            "Phase 16 mutex blocks/wakeups: %llu/%llu\n",
            (unsigned long long)sync_result.mutex_blocks,
            (unsigned long long)sync_result.mutex_wakeups
        );
        kprintf("Phase 16 mutex protected counter: OK\n");
        kprintf(
            "Phase 16 semaphore limit/peak: %llu/%llu\n",
            (unsigned long long)sync_result.semaphore_limit,
            (unsigned long long)sync_result.semaphore_peak
        );
        kprintf(
            "Phase 16 semaphore blocks/wakeups: %llu/%llu\n",
            (unsigned long long)sync_result.semaphore_blocks,
            (unsigned long long)sync_result.semaphore_wakeups
        );
        kprintf("Phase 16 semaphore limit: OK\n");
        kprintf("Phase 16 wait queue wakeups: OK\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREEN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 16 synchronization self-test: OK\n");
        kprintf("Phase 16 synchronization complete.\n");
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
    }

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

## `include/axiom/sync/spinlock.h`

```c
#ifndef AXIOM_SYNC_SPINLOCK_H
#define AXIOM_SYNC_SPINLOCK_H

#include <stdint.h>

/*
 * Phase-16 uniprocessor spinlock.
 *
 * Holding the lock with interrupts disabled prevents the current CPU from
 * being preempted while it owns the lock. This makes spinning safe on the
 * current single-CPU kernel: another task cannot be descheduled while holding
 * the same lock and then leave us spinning forever waiting for itself.
 */
struct spinlock {
    volatile uint32_t locked;
    uint64_t acquisitions;
    uint64_t contentions;
};

void spinlock_init(struct spinlock *lock);

/* Acquire while saving whether interrupts were enabled on entry. */
uint64_t spin_lock_irqsave(struct spinlock *lock);

/* Release but deliberately leave interrupts disabled. */
void spin_unlock(struct spinlock *lock);

/* Release and restore the IF state returned by spin_lock_irqsave(). */
void spin_unlock_irqrestore(struct spinlock *lock, uint64_t irq_was_enabled);

#endif
```

## `include/axiom/sync/wait_queue.h`

```c
#ifndef AXIOM_SYNC_WAIT_QUEUE_H
#define AXIOM_SYNC_WAIT_QUEUE_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/process/scheduler.h>

#define WAIT_QUEUE_MAX_WAITERS SCHEDULER_MAX_TASKS

/*
 * FIFO of scheduler task IDs. The queue itself is intentionally lock-free:
 * callers protect it with the same spinlock that protects their condition.
 * That lets "condition false -> enqueue -> mark BLOCKED" happen atomically.
 */
struct wait_queue {
    uint64_t task_ids[WAIT_QUEUE_MAX_WAITERS];
    size_t head;
    size_t count;
    uint64_t sleeps;
    uint64_t wakeups;
};

void wait_queue_init(struct wait_queue *queue);
int wait_queue_enqueue_locked(struct wait_queue *queue, uint64_t task_id);
uint64_t wait_queue_dequeue_locked(struct wait_queue *queue);
int wait_queue_remove_locked(struct wait_queue *queue, uint64_t task_id);
size_t wait_queue_count(const struct wait_queue *queue);

#endif
```

## `include/axiom/sync/mutex.h`

```c
#ifndef AXIOM_SYNC_MUTEX_H
#define AXIOM_SYNC_MUTEX_H

#include <stdint.h>

#include <axiom/sync/spinlock.h>
#include <axiom/sync/wait_queue.h>

#define MUTEX_NO_OWNER UINT64_MAX

/* Sleeping, non-recursive kernel mutex with FIFO ownership handoff. */
struct mutex {
    struct spinlock guard;
    struct wait_queue waiters;
    uint64_t owner_id;
    uint64_t acquisitions;
    uint64_t blocks;
    uint64_t wakeups;
};

void mutex_init(struct mutex *mutex);
int mutex_lock(struct mutex *mutex);
int mutex_try_lock(struct mutex *mutex);
int mutex_unlock(struct mutex *mutex);
int mutex_is_locked(const struct mutex *mutex);

#endif
```

## `include/axiom/sync/semaphore.h`

```c
#ifndef AXIOM_SYNC_SEMAPHORE_H
#define AXIOM_SYNC_SEMAPHORE_H

#include <stdint.h>

#include <axiom/sync/spinlock.h>
#include <axiom/sync/wait_queue.h>

/* Counting semaphore. Waiters sleep and tokens are handed off FIFO. */
struct semaphore {
    struct spinlock guard;
    struct wait_queue waiters;
    uint64_t count;
    uint64_t waits;
    uint64_t posts;
    uint64_t blocks;
    uint64_t wakeups;
};

void semaphore_init(struct semaphore *semaphore, uint64_t initial_count);
int semaphore_wait(struct semaphore *semaphore);
int semaphore_try_wait(struct semaphore *semaphore);
void semaphore_post(struct semaphore *semaphore);
uint64_t semaphore_value(struct semaphore *semaphore);

#endif
```

## `include/axiom/sync/selftest.h`

```c
#ifndef AXIOM_SYNC_SELFTEST_H
#define AXIOM_SYNC_SELFTEST_H

#include <stdint.h>

enum sync_selftest_stage {
    SYNC_SELFTEST_STAGE_NONE = 0,
    SYNC_SELFTEST_STAGE_RACE = 1,
    SYNC_SELFTEST_STAGE_SPINLOCK = 2,
    SYNC_SELFTEST_STAGE_MUTEX = 3,
    SYNC_SELFTEST_STAGE_SEMAPHORE = 4
};

struct sync_selftest_result {
    uint64_t failure_stage;
    uint64_t race_expected;
    uint64_t race_actual;
    uint64_t spin_expected;
    uint64_t spin_actual;
    uint64_t spin_acquisitions;
    uint64_t mutex_expected;
    uint64_t mutex_actual;
    uint64_t mutex_blocks;
    uint64_t mutex_wakeups;
    uint64_t semaphore_limit;
    uint64_t semaphore_peak;
    uint64_t semaphore_blocks;
    uint64_t semaphore_wakeups;
};

int sync_run_selftest(struct sync_selftest_result *result);

#endif
```

## `kernel/sync/spinlock.c`

```c
#include <stdint.h>

#include <axiom/arch/interrupts.h>
#include <axiom/sync/spinlock.h>

static uint32_t exchange_locked(volatile uint32_t *memory, uint32_t value)
{
    __asm__ volatile (
        "xchgl %0, %1"
        : "+r" (value), "+m" (*memory)
        :
        : "memory"
    );
    return value;
}

void spinlock_init(struct spinlock *lock)
{
    if (lock == 0) {
        return;
    }

    lock->locked = 0u;
    lock->acquisitions = 0u;
    lock->contentions = 0u;
}

uint64_t spin_lock_irqsave(struct spinlock *lock)
{
    const uint64_t irq_was_enabled = interrupts_enabled() ? 1u : 0u;

    if (lock == 0) {
        return irq_was_enabled;
    }

    interrupts_disable();

    for (;;) {
        if (exchange_locked(&lock->locked, 1u) == 0u) {
            ++lock->acquisitions;
            return irq_was_enabled;
        }

        ++lock->contentions;
        while (lock->locked != 0u) {
            __asm__ volatile ("pause" ::: "memory");
        }
    }
}

void spin_unlock(struct spinlock *lock)
{
    if (lock == 0) {
        return;
    }

    __asm__ volatile ("" ::: "memory");
    lock->locked = 0u;
    __asm__ volatile ("" ::: "memory");
}

void spin_unlock_irqrestore(struct spinlock *lock, uint64_t irq_was_enabled)
{
    spin_unlock(lock);

    if (irq_was_enabled != 0u) {
        interrupts_enable();
    }
}
```

## `kernel/sync/wait_queue.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/sync/wait_queue.h>

void wait_queue_init(struct wait_queue *queue)
{
    size_t index;

    if (queue == 0) {
        return;
    }

    for (index = 0u; index < WAIT_QUEUE_MAX_WAITERS; ++index) {
        queue->task_ids[index] = UINT64_MAX;
    }

    queue->head = 0u;
    queue->count = 0u;
    queue->sleeps = 0u;
    queue->wakeups = 0u;
}

int wait_queue_enqueue_locked(struct wait_queue *queue, uint64_t task_id)
{
    size_t index;
    size_t tail;

    if (queue == 0 || task_id == UINT64_MAX ||
        queue->count >= WAIT_QUEUE_MAX_WAITERS) {
        return 0;
    }

    for (index = 0u; index < queue->count; ++index) {
        const size_t slot = (queue->head + index) % WAIT_QUEUE_MAX_WAITERS;
        if (queue->task_ids[slot] == task_id) {
            return 0;
        }
    }

    tail = (queue->head + queue->count) % WAIT_QUEUE_MAX_WAITERS;
    queue->task_ids[tail] = task_id;
    ++queue->count;
    ++queue->sleeps;
    return 1;
}

uint64_t wait_queue_dequeue_locked(struct wait_queue *queue)
{
    uint64_t task_id;

    if (queue == 0 || queue->count == 0u) {
        return UINT64_MAX;
    }

    task_id = queue->task_ids[queue->head];
    queue->task_ids[queue->head] = UINT64_MAX;
    queue->head = (queue->head + 1u) % WAIT_QUEUE_MAX_WAITERS;
    --queue->count;
    ++queue->wakeups;
    return task_id;
}

int wait_queue_remove_locked(struct wait_queue *queue, uint64_t task_id)
{
    size_t index;

    if (queue == 0 || queue->count == 0u) {
        return 0;
    }

    for (index = 0u; index < queue->count; ++index) {
        const size_t slot = (queue->head + index) % WAIT_QUEUE_MAX_WAITERS;

        if (queue->task_ids[slot] == task_id) {
            size_t shift;

            for (shift = index; shift + 1u < queue->count; ++shift) {
                const size_t from =
                    (queue->head + shift + 1u) % WAIT_QUEUE_MAX_WAITERS;
                const size_t to =
                    (queue->head + shift) % WAIT_QUEUE_MAX_WAITERS;
                queue->task_ids[to] = queue->task_ids[from];
            }

            queue->task_ids[
                (queue->head + queue->count - 1u) % WAIT_QUEUE_MAX_WAITERS
            ] = UINT64_MAX;
            --queue->count;
            return 1;
        }
    }

    return 0;
}

size_t wait_queue_count(const struct wait_queue *queue)
{
    return queue != 0 ? queue->count : 0u;
}
```

## `kernel/sync/mutex.c`

```c
#include <stdint.h>

#include <axiom/arch/interrupts.h>
#include <axiom/process/scheduler.h>
#include <axiom/sync/mutex.h>

void mutex_init(struct mutex *mutex)
{
    if (mutex == 0) {
        return;
    }

    spinlock_init(&mutex->guard);
    wait_queue_init(&mutex->waiters);
    mutex->owner_id = MUTEX_NO_OWNER;
    mutex->acquisitions = 0u;
    mutex->blocks = 0u;
    mutex->wakeups = 0u;
}

int mutex_lock(struct mutex *mutex)
{
    uint64_t irq_was_enabled;
    uint64_t task_id;
    const struct task *current;

    if (mutex == 0) {
        return 0;
    }

    current = scheduler_current_task();
    if (current == 0) {
        return 0;
    }
    task_id = current->id;

    irq_was_enabled = spin_lock_irqsave(&mutex->guard);

    if (mutex->owner_id == MUTEX_NO_OWNER) {
        mutex->owner_id = task_id;
        ++mutex->acquisitions;
        spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
        return 1;
    }

    /* Non-recursive by design. */
    if (mutex->owner_id == task_id) {
        spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
        return 0;
    }

    if (!wait_queue_enqueue_locked(&mutex->waiters, task_id) ||
        !scheduler_prepare_block_current(task_id)) {
        (void)wait_queue_remove_locked(&mutex->waiters, task_id);
        spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
        return 0;
    }

    ++mutex->blocks;

    /* Leave IF=0 until the scheduler has parked this blocked task. */
    spin_unlock(&mutex->guard);

    if (!scheduler_park_current(task_id)) {
        if (irq_was_enabled != 0u) {
            interrupts_enable();
        }
        return 0;
    }

    if (irq_was_enabled != 0u) {
        interrupts_enable();
    }

    /* unlock() directly hands ownership to the FIFO waiter it wakes. */
    irq_was_enabled = spin_lock_irqsave(&mutex->guard);
    if (mutex->owner_id != task_id) {
        spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
        return 0;
    }
    ++mutex->acquisitions;
    spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
    return 1;
}

int mutex_try_lock(struct mutex *mutex)
{
    uint64_t irq_was_enabled;
    const struct task *current;
    int acquired = 0;

    if (mutex == 0) {
        return 0;
    }

    current = scheduler_current_task();
    if (current == 0) {
        return 0;
    }

    irq_was_enabled = spin_lock_irqsave(&mutex->guard);
    if (mutex->owner_id == MUTEX_NO_OWNER) {
        mutex->owner_id = current->id;
        ++mutex->acquisitions;
        acquired = 1;
    }
    spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
    return acquired;
}

int mutex_unlock(struct mutex *mutex)
{
    uint64_t irq_was_enabled;
    uint64_t next_id;
    const struct task *current;

    if (mutex == 0) {
        return 0;
    }

    current = scheduler_current_task();
    if (current == 0) {
        return 0;
    }

    irq_was_enabled = spin_lock_irqsave(&mutex->guard);
    if (mutex->owner_id != current->id) {
        spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
        return 0;
    }

    next_id = wait_queue_dequeue_locked(&mutex->waiters);
    if (next_id == UINT64_MAX) {
        mutex->owner_id = MUTEX_NO_OWNER;
    } else {
        mutex->owner_id = next_id;
        if (!scheduler_wake_task(next_id)) {
            mutex->owner_id = MUTEX_NO_OWNER;
            spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
            return 0;
        }
        ++mutex->wakeups;
    }

    spin_unlock_irqrestore(&mutex->guard, irq_was_enabled);
    return 1;
}

int mutex_is_locked(const struct mutex *mutex)
{
    return mutex != 0 && mutex->owner_id != MUTEX_NO_OWNER;
}
```

## `kernel/sync/semaphore.c`

```c
#include <stdint.h>

#include <axiom/arch/interrupts.h>
#include <axiom/process/scheduler.h>
#include <axiom/sync/semaphore.h>

void semaphore_init(struct semaphore *semaphore, uint64_t initial_count)
{
    if (semaphore == 0) {
        return;
    }

    spinlock_init(&semaphore->guard);
    wait_queue_init(&semaphore->waiters);
    semaphore->count = initial_count;
    semaphore->waits = 0u;
    semaphore->posts = 0u;
    semaphore->blocks = 0u;
    semaphore->wakeups = 0u;
}

int semaphore_wait(struct semaphore *semaphore)
{
    uint64_t irq_was_enabled;
    uint64_t task_id;
    const struct task *current;

    if (semaphore == 0) {
        return 0;
    }

    current = scheduler_current_task();
    if (current == 0) {
        return 0;
    }
    task_id = current->id;

    irq_was_enabled = spin_lock_irqsave(&semaphore->guard);
    ++semaphore->waits;

    if (semaphore->count != 0u) {
        --semaphore->count;
        spin_unlock_irqrestore(&semaphore->guard, irq_was_enabled);
        return 1;
    }

    if (!wait_queue_enqueue_locked(&semaphore->waiters, task_id) ||
        !scheduler_prepare_block_current(task_id)) {
        (void)wait_queue_remove_locked(&semaphore->waiters, task_id);
        spin_unlock_irqrestore(&semaphore->guard, irq_was_enabled);
        return 0;
    }

    ++semaphore->blocks;
    spin_unlock(&semaphore->guard);

    if (!scheduler_park_current(task_id)) {
        if (irq_was_enabled != 0u) {
            interrupts_enable();
        }
        return 0;
    }

    if (irq_was_enabled != 0u) {
        interrupts_enable();
    }

    /* post() directly handed this waiter one token. */
    return 1;
}

int semaphore_try_wait(struct semaphore *semaphore)
{
    uint64_t irq_was_enabled;
    int acquired = 0;

    if (semaphore == 0) {
        return 0;
    }

    irq_was_enabled = spin_lock_irqsave(&semaphore->guard);
    ++semaphore->waits;
    if (semaphore->count != 0u) {
        --semaphore->count;
        acquired = 1;
    }
    spin_unlock_irqrestore(&semaphore->guard, irq_was_enabled);
    return acquired;
}

void semaphore_post(struct semaphore *semaphore)
{
    uint64_t irq_was_enabled;
    uint64_t task_id;

    if (semaphore == 0) {
        return;
    }

    irq_was_enabled = spin_lock_irqsave(&semaphore->guard);
    ++semaphore->posts;

    task_id = wait_queue_dequeue_locked(&semaphore->waiters);
    if (task_id != UINT64_MAX) {
        if (scheduler_wake_task(task_id)) {
            ++semaphore->wakeups;
        } else {
            ++semaphore->count;
        }
    } else {
        ++semaphore->count;
    }

    spin_unlock_irqrestore(&semaphore->guard, irq_was_enabled);
}

uint64_t semaphore_value(struct semaphore *semaphore)
{
    uint64_t irq_was_enabled;
    uint64_t value;

    if (semaphore == 0) {
        return 0u;
    }

    irq_was_enabled = spin_lock_irqsave(&semaphore->guard);
    value = semaphore->count;
    spin_unlock_irqrestore(&semaphore->guard, irq_was_enabled);
    return value;
}
```

## `kernel/sync/selftest.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/interrupts.h>
#include <axiom/drivers/timer.h>
#include <axiom/process/scheduler.h>
#include <axiom/sync/mutex.h>
#include <axiom/sync/selftest.h>
#include <axiom/sync/semaphore.h>
#include <axiom/sync/spinlock.h>

#define SYNC_TEST_TIMEOUT_TICKS 1600ULL
#define RACE_ITERATIONS 64ULL
#define SPIN_ITERATIONS 1000ULL
#define MUTEX_ITERATIONS 64ULL
#define SEMAPHORE_ITERATIONS 8ULL
#define SEMAPHORE_WORKERS 3u
#define SEMAPHORE_LIMIT 2ULL

static volatile uint64_t race_counter;
static volatile uint64_t race_loaded[2];
static volatile uint64_t race_stored[2];

static volatile uint64_t spin_counter;
static struct spinlock counter_spinlock;

static volatile uint64_t mutex_counter;
static struct mutex counter_mutex;

static volatile uint64_t semaphore_inside;
static volatile uint64_t semaphore_peak;
static struct semaphore test_semaphore;
static struct spinlock semaphore_monitor_lock;

struct worker_argument {
    uint64_t index;
};

static struct worker_argument two_worker_arguments[2];
static struct worker_argument semaphore_worker_arguments[SEMAPHORE_WORKERS];

static int cooperative_yield(void)
{
    const int restore_interrupts = interrupts_enabled();
    int result;

    interrupts_disable();
    result = scheduler_yield_current();
    if (restore_interrupts) {
        interrupts_enable();
    }
    return result;
}

static void race_worker(void *argument)
{
    const struct worker_argument *worker =
        (const struct worker_argument *)argument;
    const uint64_t index = worker != 0 ? worker->index : 0u;
    const uint64_t other = index == 0u ? 1u : 0u;
    uint64_t iteration;

    for (iteration = 0u; iteration < RACE_ITERATIONS; ++iteration) {
        const uint64_t snapshot = race_counter;

        race_loaded[index] = iteration + 1u;
        while (race_loaded[other] < iteration + 1u) {
            if (!cooperative_yield()) {
                task_exit_current(1);
            }
        }

        /* Both workers intentionally store from the same stale snapshot. */
        race_counter = snapshot + 1u;
        race_stored[index] = iteration + 1u;

        while (race_stored[other] < iteration + 1u) {
            if (!cooperative_yield()) {
                task_exit_current(1);
            }
        }
    }

    task_exit_current(0);
}

static void spin_worker(void *argument)
{
    uint64_t iteration;
    (void)argument;

    for (iteration = 0u; iteration < SPIN_ITERATIONS; ++iteration) {
        const uint64_t flags = spin_lock_irqsave(&counter_spinlock);
        ++spin_counter;
        spin_unlock_irqrestore(&counter_spinlock, flags);

        if ((iteration & 31u) == 0u && !cooperative_yield()) {
            task_exit_current(1);
        }
    }

    task_exit_current(0);
}

static void mutex_worker(void *argument)
{
    uint64_t iteration;
    (void)argument;

    for (iteration = 0u; iteration < MUTEX_ITERATIONS; ++iteration) {
        uint64_t snapshot;

        if (!mutex_lock(&counter_mutex)) {
            task_exit_current(1);
        }

        snapshot = mutex_counter;

        /*
         * Yield while holding the mutex so the other worker must actually
         * enter TASK_BLOCKED and sleep on the mutex wait queue.
         */
        if (!cooperative_yield()) {
            task_exit_current(1);
        }

        mutex_counter = snapshot + 1u;
        if (!mutex_unlock(&counter_mutex)) {
            task_exit_current(1);
        }

        if (!cooperative_yield()) {
            task_exit_current(1);
        }
    }

    task_exit_current(0);
}

static void semaphore_worker(void *argument)
{
    uint64_t iteration;
    (void)argument;

    for (iteration = 0u; iteration < SEMAPHORE_ITERATIONS; ++iteration) {
        uint64_t flags;

        if (!semaphore_wait(&test_semaphore)) {
            task_exit_current(1);
        }

        flags = spin_lock_irqsave(&semaphore_monitor_lock);
        ++semaphore_inside;
        if (semaphore_inside > semaphore_peak) {
            semaphore_peak = semaphore_inside;
        }
        spin_unlock_irqrestore(&semaphore_monitor_lock, flags);

        /* Keep a token while another task runs, forcing the third to block. */
        if (!cooperative_yield()) {
            task_exit_current(1);
        }

        flags = spin_lock_irqsave(&semaphore_monitor_lock);
        if (semaphore_inside == 0u) {
            spin_unlock_irqrestore(&semaphore_monitor_lock, flags);
            task_exit_current(1);
        }
        --semaphore_inside;
        spin_unlock_irqrestore(&semaphore_monitor_lock, flags);

        semaphore_post(&test_semaphore);

        if (!cooperative_yield()) {
            task_exit_current(1);
        }
    }

    task_exit_current(0);
}

static int create_workers(
    const char *name_prefix,
    task_entry_t entry,
    struct worker_argument *arguments,
    size_t count,
    uint64_t *ids
)
{
    size_t index;
    int created = 1;

    interrupts_disable();

    for (index = 0u; index < count; ++index) {
        arguments[index].index = index;
        if (!task_create(name_prefix, entry, &arguments[index], &ids[index])) {
            created = 0;
            break;
        }
    }

    interrupts_enable();
    return created;
}

static int wait_for_workers(const uint64_t *ids, size_t count)
{
    const uint64_t start = timer_ticks();

    for (;;) {
        size_t index;
        int complete = 1;

        for (index = 0u; index < count; ++index) {
            const struct task *task = scheduler_task_by_id(ids[index]);

            if (task == 0 || task->state != TASK_TERMINATED) {
                complete = 0;
                break;
            }
            if (task->exit_code != 0) {
                return 0;
            }
        }

        if (complete) {
            return 1;
        }

        if ((timer_ticks() - start) >= SYNC_TEST_TIMEOUT_TICKS) {
            return 0;
        }

        __asm__ volatile ("hlt" ::: "memory");
    }
}

static int run_two_workers(task_entry_t entry)
{
    uint64_t ids[2];

    if (!create_workers(
            "phase16-sync-worker",
            entry,
            two_worker_arguments,
            2u,
            ids
        )) {
        return 0;
    }

    return wait_for_workers(ids, 2u);
}

int sync_run_selftest(struct sync_selftest_result *result)
{
    uint64_t semaphore_ids[SEMAPHORE_WORKERS];

    if (result == 0) {
        return 0;
    }

    result->failure_stage = SYNC_SELFTEST_STAGE_NONE;
    if (!scheduler_running() || !interrupts_enabled()) {
        return 0;
    }

    result->race_expected = RACE_ITERATIONS * 2u;
    result->race_actual = 0u;
    result->spin_expected = SPIN_ITERATIONS * 2u;
    result->spin_actual = 0u;
    result->spin_acquisitions = 0u;
    result->mutex_expected = MUTEX_ITERATIONS * 2u;
    result->mutex_actual = 0u;
    result->mutex_blocks = 0u;
    result->mutex_wakeups = 0u;
    result->semaphore_limit = SEMAPHORE_LIMIT;
    result->semaphore_peak = 0u;
    result->semaphore_blocks = 0u;
    result->semaphore_wakeups = 0u;

    /* Stage 1: deliberately broken read-modify-write critical section. */
    result->failure_stage = SYNC_SELFTEST_STAGE_RACE;
    race_counter = 0u;
    race_loaded[0] = 0u;
    race_loaded[1] = 0u;
    race_stored[0] = 0u;
    race_stored[1] = 0u;

    if (!run_two_workers(race_worker)) {
        return 0;
    }

    result->race_actual = race_counter;
    if (race_counter >= result->race_expected) {
        return 0;
    }

    /* Stage 2: same shared-counter idea protected by a spinlock. */
    result->failure_stage = SYNC_SELFTEST_STAGE_SPINLOCK;
    spin_counter = 0u;
    spinlock_init(&counter_spinlock);
    if (!run_two_workers(spin_worker)) {
        return 0;
    }

    result->spin_actual = spin_counter;
    result->spin_acquisitions = counter_spinlock.acquisitions;
    if (spin_counter != result->spin_expected) {
        return 0;
    }

    /* Stage 3: sleeping mutex. Yielding while locked forces contention. */
    result->failure_stage = SYNC_SELFTEST_STAGE_MUTEX;
    mutex_counter = 0u;
    mutex_init(&counter_mutex);
    if (!run_two_workers(mutex_worker)) {
        return 0;
    }

    result->mutex_actual = mutex_counter;
    result->mutex_blocks = counter_mutex.blocks;
    result->mutex_wakeups = counter_mutex.wakeups;
    if (mutex_counter != result->mutex_expected ||
        counter_mutex.blocks == 0u || counter_mutex.wakeups == 0u ||
        mutex_is_locked(&counter_mutex)) {
        return 0;
    }

    /* Stage 4: counting semaphore allows exactly two concurrent entrants. */
    result->failure_stage = SYNC_SELFTEST_STAGE_SEMAPHORE;
    semaphore_inside = 0u;
    semaphore_peak = 0u;
    semaphore_init(&test_semaphore, SEMAPHORE_LIMIT);
    spinlock_init(&semaphore_monitor_lock);

    if (!create_workers(
            "phase16-semaphore-worker",
            semaphore_worker,
            semaphore_worker_arguments,
            SEMAPHORE_WORKERS,
            semaphore_ids
        ) ||
        !wait_for_workers(semaphore_ids, SEMAPHORE_WORKERS)) {
        return 0;
    }

    result->semaphore_peak = semaphore_peak;
    result->semaphore_blocks = test_semaphore.blocks;
    result->semaphore_wakeups = test_semaphore.wakeups;

    if (semaphore_inside != 0u ||
        semaphore_peak != SEMAPHORE_LIMIT ||
        test_semaphore.blocks == 0u || test_semaphore.wakeups == 0u ||
        wait_queue_count(&test_semaphore.waiters) != 0u ||
        semaphore_value(&test_semaphore) != SEMAPHORE_LIMIT) {
        return 0;
    }

    result->failure_stage = SYNC_SELFTEST_STAGE_NONE;
    return 1;
}
```

## `tests/phase16_sync.py`

```python
#!/usr/bin/env python3
"""Validate Phase-16 synchronization primitives and deliberate race demo."""

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
        if "Phase 16 synchronization self-test: FAILED" in text:
            match = re.search(
                r"Phase 16 synchronization failure stage: (\d+)", text
            )
            stage = match.group(1) if match is not None else "unknown"
            tail = "\n".join(text.splitlines()[-160:])
            raise AssertionError(
                f"phase16 kernel self-test failed at stage {stage}; "
                f"see {serial}\n--- serial tail ---\n{tail}"
            )
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-160:])
            raise AssertionError(
                f"phase16 timed out waiting for {marker!r}; see {serial}\n"
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
                f"phase16 timed out waiting for prompt #{count}\n"
                f"--- serial tail ---\n{tail}"
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
        "-": "minus",
    }
    require(character in mapping, f"phase16: no HMP key mapping for {character!r}")
    return mapping[character]


def send_command(monitor_path, command):
    send_hmp_keys(
        monitor_path,
        [key_for_character(character) for character in command] + ["ret"],
    )


def test_phase16():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase16-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    disk = directory / "phase16-test-disk.img"
    with disk.open("wb") as handle:
        handle.truncate(16 * 1024 * 1024)

    serial = directory / "phase16-serial.log"
    qemu_log = directory / "phase16-qemu.log"
    monitor = directory / "phase16-monitor.sock"
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
        process = subprocess.Popen(
            command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT
        )
        try:
            text = wait_for_text(
                process,
                serial,
                "AxiomOS shell ready. Type 'help' for commands.",
                time.monotonic() + 170,
            )

            require("AxiomOS Phase 16 synchronization online." in text,
                    "phase16: synchronization banner missing")
            require("Phase 16 race condition demonstrated: OK" in text,
                    "phase16: deliberate race was not demonstrated")
            require("Phase 16 spinlock protected counter: OK" in text,
                    "phase16: spinlock counter test missing")
            require("Phase 16 mutex protected counter: OK" in text,
                    "phase16: mutex counter test missing")
            require("Phase 16 semaphore limit: OK" in text,
                    "phase16: semaphore limit test missing")
            require("Phase 16 wait queue wakeups: OK" in text,
                    "phase16: wait-queue test missing")
            require("Phase 16 synchronization self-test: OK" in text,
                    "phase16: self-test did not complete")
            require("KERNEL PANIC" not in text, "phase16: kernel panicked")

            race = re.search(
                r"Phase 16 race expected/observed: (\d+)/(\d+)", text
            )
            require(race is not None, "phase16: race counts missing")
            require(int(race.group(1)) == 128,
                    "phase16: unexpected race-test expected count")
            require(int(race.group(2)) < int(race.group(1)),
                    "phase16: deliberate lost update did not occur")

            spin = re.search(
                r"Phase 16 spinlock counter expected/actual: (\d+)/(\d+)", text
            )
            require(spin is not None and spin.group(1) == spin.group(2),
                    "phase16: spinlock did not preserve the counter")

            mutex = re.search(
                r"Phase 16 mutex blocks/wakeups: (\d+)/(\d+)", text
            )
            require(mutex is not None, "phase16: mutex contention stats missing")
            require(int(mutex.group(1)) > 0 and int(mutex.group(2)) > 0,
                    "phase16: mutex never blocked/woke a task")

            semaphore = re.search(
                r"Phase 16 semaphore limit/peak: (\d+)/(\d+)", text
            )
            require(semaphore is not None,
                    "phase16: semaphore limit/peak missing")
            require(semaphore.group(1) == "2" and semaphore.group(2) == "2",
                    "phase16: semaphore did not enforce a limit of two")

            before_prompts = text.count("axiom> ")
            send_command(monitor, "echo phase16-ok")
            text = wait_for_text(
                process, serial, "phase16-ok", time.monotonic() + 20
            )
            text = wait_for_prompt_count(
                process, serial, before_prompts + 1, time.monotonic() + 20
            )
            require("KERNEL PANIC" not in text,
                    "phase16: shell integration panicked after synchronization tests")

            print("PASS: Phase 16 deliberate lost-update race demonstrated")
            print("PASS: Phase 16 IRQ-save spinlock protected critical section")
            print("PASS: Phase 16 blocking mutex + FIFO wait queue")
            print("PASS: Phase 16 counting semaphore concurrency limit")
            print("PASS: Phase 16 scheduler BLOCKED/READY wakeup integration")
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
    test_phase16()
    print("PASS: all Phase 16 synchronization tests.")
```

## `docs/phase16.md`

```markdown
# Phase 16 — Synchronization

Phase 16 adds kernel synchronization primitives and connects them to the
preemptive scheduler. The purpose is not merely to define lock types: AxiomOS
first demonstrates a real lost-update race and then proves that the new
primitives repair or control the same concurrency.

## Why synchronization is needed even on one CPU

AxiomOS still runs on one CPU, but the APIC timer can preempt one task between a
load and a later store. Two tasks that both execute:

```text
value = counter
counter = value + 1
```

can therefore both read the same old value and lose one increment. Phase 16's
race test makes that interleaving deterministic with a barrier: two workers
read the same counter value before either stores it. 128 intended increments
become 64, proving the bug instead of relying on luck.

## IRQ-save spinlocks

`struct spinlock` uses x86 `xchg` as the atomic acquire operation. Acquisition
also saves and disables interrupts. On the current single-CPU kernel this is an
important rule: a task holding a spinlock cannot be preempted, so another task
cannot run on the same CPU and spin forever waiting for a descheduled owner.

Spinlocks are therefore for very short kernel critical sections. They must not
be held across sleeping/blocking operations. Phase 16 uses one to protect a
shared counter; two workers produce exactly the expected 2000 increments.

## Wait queues and scheduler blocking

Phase 16 adds generic scheduler hooks:

```text
RUNNING
   ↓ prepare block while IF=0
BLOCKED
   ↓ park / context switch
another task runs
   ↓ wake task
READY
   ↓ scheduler chooses it
RUNNING
```

A `wait_queue` is a bounded FIFO of task IDs. The queue is protected by the
same spinlock as the condition being waited on, which prevents the classic
lost-wakeup race between checking a condition and going to sleep.

These hooks are intentionally generic because later pipes, sockets, device I/O
and IPC can use the same BLOCKED/READY mechanism.

## Sleeping mutex

`struct mutex` is non-recursive and uses:

- a short internal spinlock for metadata;
- an owner PID/task ID;
- a FIFO wait queue.

If free, ownership is immediate. If owned, the caller is enqueued, marked
BLOCKED, and parked. `mutex_unlock()` hands ownership directly to the oldest
waiter before waking it. Direct handoff prevents a newly arriving task from
stealing the mutex ahead of a task that was already waiting.

The self-test deliberately yields while holding the mutex so the second worker
must actually block. The final shared counter must still be exact.

## Counting semaphore

`struct semaphore` maintains a token count plus a FIFO wait queue. `wait()`
consumes a token or blocks. `post()` either increments the available count or
directly hands the token to the oldest blocked waiter.

The Phase-16 test starts three workers with a semaphore count of two. Workers
yield while holding a token, so two tasks are observed inside the protected
region while the third is forced to block. The measured concurrency peak must
be exactly two.

## Deadlock and starvation

Phase 16 does not attempt a general deadlock detector. A deadlock can still
occur if, for example, task A holds mutex X and waits for Y while task B holds Y
and waits for X. The engineering rule is to use a consistent lock ordering and
never sleep while holding a spinlock.

FIFO mutex/semaphore wait queues reduce starvation by waking waiters in arrival
order. They do not provide formal real-time fairness, especially once priorities
or multiple CPUs are introduced later.


## Phase-8 worker retirement

Phase 8 used two intentionally CPU-bound kernel threads to prove timer-driven
preemption. They are acceptance-test fixtures rather than kernel services.
Phase 16 now asks them to exit immediately after the Phase-8 test succeeds.
This prevents two synthetic busy loops from consuming scheduler quanta forever
and makes later blocking/synchronization timing representative of real work.

The synchronization worker timeout is also a safety bound only; it is now 1600
timer ticks so a slow emulator does not turn correct scheduling into a false
negative. On failure, the kernel prints the failing synchronization stage
number before the generic failure line.

## Acceptance output

A normal boot should include lines similar to:

```text
AxiomOS Phase 16 synchronization online.
Phase 16 race expected/observed: 128/64
Phase 16 race condition demonstrated: OK
Phase 16 spinlock counter expected/actual: 2000/2000
Phase 16 spinlock protected counter: OK
Phase 16 mutex counter expected/actual: 128/128
Phase 16 mutex blocks/wakeups: .../...
Phase 16 mutex protected counter: OK
Phase 16 semaphore limit/peak: 2/2
Phase 16 semaphore blocks/wakeups: .../...
Phase 16 semaphore limit: OK
Phase 16 wait queue wakeups: OK
Phase 16 synchronization self-test: OK
Phase 16 synchronization complete.
```

The exact block/wakeup counts are scheduler-interleaving dependent; the test
requires them to be nonzero rather than hardcoding a particular value.

Run:

```bash
make clean
make
make test-phase16
make test
```

`test-phase16` boots QEMU, validates the deliberate race, exact protected
counters, real mutex/semaphore blocking, the semaphore concurrency limit, then
types `echo phase16-ok` into the Ring-3 shell to ensure normal userspace still
works after the kernel synchronization tests.

## Deliberate limits

- single CPU only; SMP locking comes in Phase 17;
- fixed-size wait queues bounded by the fixed process/task table;
- spinlocks are non-recursive;
- mutexes are non-recursive;
- no priority inheritance;
- no deadlock detector;
- no timed mutex/semaphore waits yet.
```

## `docs/project-state.md`

```markdown
# AxiomOS handoff — Phase 16 implementation

Development environment: Linux/Distrobox on Steam Deck. Run commands from the
directory containing `Makefile`. Roadmap: Phase 0 through Phase 25.

Accepted foundation before Phase 16:

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
- Phase 15: fork/exec/wait, parent-child lifecycle, ps and SIGTERM.

Phase 16 adds:

- deterministic lost-update race demonstration;
- x86 atomic IRQ-save spinlocks;
- reusable FIFO kernel wait queues;
- generic scheduler BLOCKED/READY park/wakeup hooks;
- sleeping non-recursive mutexes with direct FIFO ownership handoff;
- counting semaphores with FIFO token handoff;
- a mutex contention test that proves tasks really block and wake;
- a semaphore test enforcing a concurrency limit of two;
- dedicated QEMU Phase-16 acceptance testing plus full regression integration.

Important limits:

- single CPU only; Phase 17 is SMP/multicore;
- no priority inheritance or deadlock detector;
- no timed lock waits;
- wait queues remain bounded by the fixed task table;
- scheduler policy remains round-robin.

Acceptance commands:

```bash
make clean
make
make test-phase16
make test
```

Next milestone: Phase 17 — SMP / multicore startup, per-CPU state and the locking
changes needed when two CPUs can execute kernel code at the same time.
```

## `docs/validation.md`

```markdown
# Validation

AxiomOS uses phase-specific QEMU regression tests plus a full cumulative suite.

Current acceptance commands:

```bash
make clean
make
make test-phase16
make test
```

`make test` runs Phase 1 through Phase 16. Destructive fault/corruption cases
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

## Phase 16 generic wait queues

Synchronization now reuses the scheduler's BLOCKED state outside `waitpid()`.
A kernel primitive can mark its current task BLOCKED while interrupts are off,
park it, then later wake its task ID back to READY. FIFO wait queues use these
hooks for sleeping mutexes and semaphores.

This matters beyond locks: future pipes, networking sockets, device completion,
and IPC can block callers on the same scheduler mechanism instead of polling.
```

## `docs/architecture.md`

```markdown
# Architecture at Phase 16

- Target: x86-64, one bootstrap CPU, freestanding C plus x86-64 assembly.
- Limine v12.9.0 boots a higher-half ELF64 kernel.
- Serial output mirrors the framebuffer terminal/custom `kprintf()`.
- GDT contains Ring-0 code/data, Ring-3 data/code, and a 64-bit TSS.
- Local APIC supplies the 100 Hz timer; I/O APIC routes PS/2 keyboard IRQ1.
- PMM manages 4 KiB frames; VMM owns the kernel PML4 and isolated user roots.
- Kernel heap provides dynamic allocation.
- Scheduler is five-tick preemptive round robin with reusable BLOCKED/READY wait-queue hooks.
- Ring-3 tasks use private user memory/stacks and trusted kernel stacks.
- Phase 15 provides fork/exec/wait parent-child process management.
- Phase 16 provides IRQ-save spinlocks, FIFO wait queues, sleeping mutexes, and counting semaphores.
- `SYSCALL/SYSRETQ` validates user pointers at the kernel boundary.
- ELF64 loader maps ET_EXEC PT_LOAD segments with R/W/X permissions.
- VFS provides paths, mounts, vnodes and per-task file descriptors.
- RAMFS backs `/` and `/tmp`.
- Phase 13 discovers q35 AHCI through PCI, exposes a SATA block device, and
  mounts persistent `diskfs` at `/disk`.

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
```
