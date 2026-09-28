# AxiomOS Phase 17 — complete new/modified source

This document contains the complete contents of every file added or modified for Phase 17.

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
	arch/x86_64/cpu/smp.c \
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
	arch/x86_64/cpu/smp_entry.S \
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
		-smp 4 \
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
		-smp 4 \
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
	$(MAKE) test-phase17


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
	@echo "AxiomOS Phase 17 targets:"
	@echo
	@echo "  make          Build kernel and bootable ISO"
	@echo "  make run      Boot AxiomOS using QEMU"
	@echo "  make debug    Boot QEMU paused for GDB"
	@echo "  make test     Run Phase 1 through Phase 17 tests"
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
	@echo "  make test-phase17 Run SMP/AP bring-up and parallel-core tests"
	@echo "  make disk-image Create the 16 MiB persistent QEMU disk image"
	@echo "  make MODE=heap_double_free run  Trigger double-free panic"
	@echo "  make MODE=heap_guard run        Trigger tail-guard panic"
	@echo "  make inspect  Inspect generated ELF64 kernel"
	@echo "  make clean    Remove generated build files"
	@echo "  make distclean Remove build files and Limine"

.PHONY: test-phase3 test-phase4 test-phase5 test-phase6 test-phase7 test-phase8 test-phase9 test-phase10 test-phase11 test-phase12 test-phase13 test-phase14 test-phase15 test-phase16 test-phase17 disk-image

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

test-phase17:
	python3 tests/phase17_smp.py

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

Phases 0–16 provide boot, memory management, interrupts, preemptive scheduling,
Ring-3 isolation, syscalls, ELF64 loading, VFS + persistent AHCI storage, a real
interactive shell, process management, and kernel synchronization.

Phase 17 adds foundational SMP / multicore support:

- Limine MP CPU discovery and AP release;
- four-CPU QEMU normal boots;
- AxiomOS kernel CR3 activation on each AP;
- private AP stacks and private GDT/TSS/IST state;
- per-CPU identity/online state;
- real BSP/AP parallel shared-memory work;
- an exact cross-CPU spinlock counter proof.

The general scheduler intentionally remains BSP-only. APs run the Phase-17 SMP
proof and then park; full multicore scheduling, IPIs and TLB shootdowns are not
claimed yet.

## Common commands

```bash
make
make run
make test
make test-phase13
make test-phase14
make test-phase15
make test-phase16
make test-phase17
make MODE=page_fault run
make MODE=heap_double_free run
make MODE=heap_guard run
make debug
```

`make test` runs the complete Phase 1 through Phase 17 regression suite.

When `make run` reaches the shell:

```text
AxiomOS shell ready. Type 'help' for commands.
axiom> help
```

The next milestone is Phase 18: networking.

[Architecture](docs/architecture.md) | [Memory](docs/memory.md) |
[Tasks](docs/processes.md) | [Syscalls](docs/syscalls.md) |
[Filesystem](docs/filesystem.md) | [Phase 13](docs/phase13.md) |
[Phase 14](docs/phase14.md) | [Phase 15](docs/phase15.md) | [Phase 16](docs/phase16.md) | [Phase 17](docs/phase17.md) | [Project state](docs/project-state.md) |
[Original roadmap](docs/roadmap.md)
```

## `include/axiom/boot/limine.h`

```c
#ifndef AXIOM_BOOT_LIMINE_H
#define AXIOM_BOOT_LIMINE_H

#include <stdint.h>


#define LIMINE_FRAMEBUFFER_RGB 1u

#define LIMINE_MEMMAP_USABLE                 0ULL
#define LIMINE_MEMMAP_RESERVED               1ULL
#define LIMINE_MEMMAP_ACPI_RECLAIMABLE       2ULL
#define LIMINE_MEMMAP_ACPI_NVS               3ULL
#define LIMINE_MEMMAP_BAD_MEMORY             4ULL
#define LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE 5ULL
#define LIMINE_MEMMAP_EXECUTABLE_AND_MODULES 6ULL
#define LIMINE_MEMMAP_FRAMEBUFFER            7ULL
#define LIMINE_MEMMAP_RESERVED_MAPPED        8ULL


struct limine_video_mode {
    uint64_t pitch;
    uint64_t width;
    uint64_t height;

    uint16_t bpp;

    uint8_t memory_model;

    uint8_t red_mask_size;
    uint8_t red_mask_shift;

    uint8_t green_mask_size;
    uint8_t green_mask_shift;

    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
};


struct limine_framebuffer {
    void *address;

    uint64_t width;
    uint64_t height;
    uint64_t pitch;

    uint16_t bpp;

    uint8_t memory_model;

    uint8_t red_mask_size;
    uint8_t red_mask_shift;

    uint8_t green_mask_size;
    uint8_t green_mask_shift;

    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;

    uint8_t unused[7];

    uint64_t edid_size;
    void *edid;

    uint64_t mode_count;
    struct limine_video_mode **modes;
};


struct limine_framebuffer_response {
    uint64_t revision;

    uint64_t framebuffer_count;

    struct limine_framebuffer **framebuffers;
};


struct limine_framebuffer_request {
    uint64_t id[4];

    uint64_t revision;

    struct limine_framebuffer_response *response;
};


struct limine_flanterm_fb_init_params {
    uint32_t *canvas;

    uint64_t canvas_size;

    uint32_t ansi_colours[8];
    uint32_t ansi_bright_colours[8];

    uint32_t default_bg;
    uint32_t default_fg;

    uint32_t default_bg_bright;
    uint32_t default_fg_bright;

    void *font;

    uint64_t font_width;
    uint64_t font_height;
    uint64_t font_spacing;

    uint64_t font_scale_x;
    uint64_t font_scale_y;

    uint64_t margin;
    uint64_t rotation;
};


struct limine_flanterm_fb_init_params_response {
    uint64_t revision;

    uint64_t entry_count;

    struct limine_flanterm_fb_init_params **entries;
};


struct limine_flanterm_fb_init_params_request {
    uint64_t id[4];

    uint64_t revision;

    struct limine_flanterm_fb_init_params_response *response;
};




struct limine_mp_info;
typedef void (*limine_goto_address)(struct limine_mp_info *);

struct limine_mp_info {
    uint32_t processor_id;
    uint32_t lapic_id;
    uint64_t reserved;
    limine_goto_address goto_address;
    uint64_t extra_argument;
};

struct limine_mp_response {
    uint64_t revision;
    uint32_t flags;
    uint32_t bsp_lapic_id;
    uint64_t cpu_count;
    struct limine_mp_info **cpus;
};

struct limine_mp_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_mp_response *response;
    uint64_t flags;
};

struct limine_memmap_entry {
    uint64_t base;
    uint64_t length;
    uint64_t type;
};


struct limine_memmap_response {
    uint64_t revision;
    uint64_t entry_count;
    struct limine_memmap_entry **entries;
};


struct limine_memmap_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_memmap_response *response;
};


struct limine_hhdm_response {
    uint64_t revision;
    uint64_t offset;
};




struct limine_uuid {
    uint32_t a;
    uint16_t b;
    uint16_t c;
    uint8_t d[8];
};


struct limine_file {
    uint64_t revision;
    void *address;
    uint64_t size;
    char *path;
    char *string;
    uint32_t media_type;
    uint32_t unused;
    uint8_t tftp_ipv4[4];
    uint32_t tftp_port;
    uint32_t partition_index;
    uint32_t mbr_disk_id;
    struct limine_uuid gpt_disk_uuid;
    struct limine_uuid gpt_part_uuid;
    struct limine_uuid part_uuid;
};


struct limine_module_response {
    uint64_t revision;
    uint64_t module_count;
    struct limine_file **modules;
};


struct limine_module_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_module_response *response;
    uint64_t internal_module_count;
    void *internal_modules;
};

struct limine_hhdm_request {
    uint64_t id[4];
    uint64_t revision;
    struct limine_hhdm_response *response;
};


int limine_base_revision_supported(void);


int limine_get_terminal_boot_info(
    struct limine_framebuffer **framebuffer,
    struct limine_flanterm_fb_init_params **font_params
);


int limine_get_memory_boot_info(
    struct limine_memmap_response **memory_map,
    uint64_t *hhdm_offset
);


struct limine_mp_response *limine_get_mp_response(void);


int limine_get_module(
    const char *module_string,
    const void **address,
    uint64_t *size,
    const char **path
);


#endif
```

## `arch/x86_64/boot/limine_requests.c`

```c
#include <stdint.h>

#include <axiom/boot/limine.h>


#define LIMINE_COMMON_MAGIC_0 0xc7b1dd30df4c8b88ULL
#define LIMINE_COMMON_MAGIC_1 0x0a82e883a194f07bULL


/*
 * Limine request-area start marker.
 */
__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[4] = {
    0xf6b8f4b39de7d1aeULL,
    0xfab91a6940fcb9cfULL,
    0x785c6ed015d3e316ULL,
    0x181e920a7852b9d9ULL
};


/*
 * AxiomOS currently targets Limine base revision 6.
 */
__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[3] = {
    0xf9562b2d5c95a6c8ULL,
    0x6a7b384944536bdcULL,
    6ULL
};


/*
 * Ask Limine for a graphical framebuffer.
 */
__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = {
        LIMINE_COMMON_MAGIC_0,
        LIMINE_COMMON_MAGIC_1,
        0x9d5827dcd881dd75ULL,
        0xa3148604f6fab11bULL
    },

    .revision = 0,

    .response = 0
};


/*
 * Ask Limine for the VGA-style bitmap font used by its framebuffer terminal.
 * AxiomOS only consumes the bitmap; rendering remains our responsibility.
 */
__attribute__((used, section(".limine_requests")))
static volatile struct limine_flanterm_fb_init_params_request
    flanterm_params_request = {
        .id = {
            LIMINE_COMMON_MAGIC_0,
            LIMINE_COMMON_MAGIC_1,
            0x3259399fe7c5f126ULL,
            0xe01c1c8c5db9d1a9ULL
        },

        .revision = 0,

        .response = 0
    };


/*
 * Phase 4: request the firmware/bootloader physical-memory map.
 */
__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = {
        LIMINE_COMMON_MAGIC_0,
        LIMINE_COMMON_MAGIC_1,
        0x67cf3d9d378a806fULL,
        0xe304acdfc50c3c62ULL
    },

    .revision = 0,

    .response = 0
};


/*
 * Phase 4: discover the Higher Half Direct Map offset. This lets the PMM
 * access RAM described by physical addresses while Limine's page tables are
 * still active.
 */
__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = {
        LIMINE_COMMON_MAGIC_0,
        LIMINE_COMMON_MAGIC_1,
        0x48dcf1cb8ad2b852ULL,
        0x63984e959a98244bULL
    },

    .revision = 0,

    .response = 0
};




/*
 * Phase 17: ask Limine to bootstrap and park every processor. Keep flags at
 * zero so x86-64 remains in xAPIC mode; the Phase-7 APIC driver deliberately
 * uses the MMIO xAPIC interface rather than x2APIC MSRs.
 */
__attribute__((used, section(".limine_requests")))
static volatile struct limine_mp_request mp_request = {
    .id = {
        LIMINE_COMMON_MAGIC_0,
        LIMINE_COMMON_MAGIC_1,
        0x95a67b819a1b857eULL,
        0xa0b61b723b6a73e0ULL
    },
    .revision = 0,
    .response = 0,
    .flags = 0
};


/*
 * Phase 11: receive standalone ELF executables as Limine modules.
 */
__attribute__((used, section(".limine_requests")))
static volatile struct limine_module_request module_request = {
    .id = {
        LIMINE_COMMON_MAGIC_0,
        LIMINE_COMMON_MAGIC_1,
        0x3e7e279702be32afULL,
        0xca1c4f3bd1280ceeULL
    },

    .revision = 0,
    .response = 0,
    .internal_module_count = 0,
    .internal_modules = 0
};


/*
 * Limine request-area end marker.
 */
__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[2] = {
    0xadc0e0531bb10d03ULL,
    0x9572709f31764c62ULL
};


int limine_base_revision_supported(void)
{
    return limine_base_revision[2] == 0ULL;
}


int limine_get_terminal_boot_info(
    struct limine_framebuffer **framebuffer,
    struct limine_flanterm_fb_init_params **font_params
)
{
    struct limine_framebuffer_response *framebuffer_response;
    struct limine_flanterm_fb_init_params_response *font_response;


    if (framebuffer == 0 || font_params == 0) {
        return 0;
    }


    framebuffer_response = framebuffer_request.response;
    font_response = flanterm_params_request.response;


    if (framebuffer_response == 0 || font_response == 0) {
        return 0;
    }


    if (framebuffer_response->framebuffer_count == 0 ||
        framebuffer_response->framebuffers == 0) {

        return 0;
    }


    if (font_response->entry_count == 0 ||
        font_response->entries == 0) {

        return 0;
    }


    if (framebuffer_response->framebuffers[0] == 0 ||
        font_response->entries[0] == 0) {

        return 0;
    }


    *framebuffer = framebuffer_response->framebuffers[0];
    *font_params = font_response->entries[0];


    return 1;
}


int limine_get_memory_boot_info(
    struct limine_memmap_response **memory_map,
    uint64_t *hhdm_offset
)
{
    struct limine_memmap_response *map_response;
    struct limine_hhdm_response *hhdm_response;


    if (memory_map == 0 || hhdm_offset == 0) {
        return 0;
    }


    map_response = memmap_request.response;
    hhdm_response = hhdm_request.response;


    if (map_response == 0 || hhdm_response == 0) {
        return 0;
    }


    if (map_response->entry_count == 0 || map_response->entries == 0) {
        return 0;
    }


    *memory_map = map_response;
    *hhdm_offset = hhdm_response->offset;


    return 1;
}


struct limine_mp_response *limine_get_mp_response(void)
{
    return mp_request.response;
}


static int strings_equal(const char *left, const char *right)
{
    if (left == 0 || right == 0) {
        return 0;
    }

    while (*left != '\0' && *right != '\0') {
        if (*left != *right) {
            return 0;
        }

        ++left;
        ++right;
    }

    return *left == *right;
}


int limine_get_module(
    const char *module_string,
    const void **address,
    uint64_t *size,
    const char **path
)
{
    struct limine_module_response *response;
    uint64_t index;

    if (module_string == 0 || address == 0 || size == 0 || path == 0) {
        return 0;
    }

    response = module_request.response;
    if (response == 0 || response->module_count == 0u ||
        response->modules == 0) {
        return 0;
    }

    for (index = 0u; index < response->module_count; ++index) {
        struct limine_file *file = response->modules[index];

        if (file != 0 && file->address != 0 && file->size != 0u &&
            strings_equal(file->string, module_string)) {
            *address = file->address;
            *size = file->size;
            *path = file->path != 0 ? file->path : "<unnamed-module>";
            return 1;
        }
    }

    return 0;
}
```

## `include/axiom/arch/smp.h`

```c
#ifndef AXIOM_ARCH_SMP_H
#define AXIOM_ARCH_SMP_H

#include <stdint.h>

#include <axiom/boot/limine.h>

#define SMP_MAX_CPUS 8u
#define SMP_AP_STACK_SIZE (16u * 1024u)
#define SMP_TEST_ITERATIONS 5000ULL

enum smp_cpu_role {
    SMP_CPU_BSP = 0,
    SMP_CPU_AP = 1,
};

struct smp_cpu {
    uint32_t processor_id;
    uint32_t lapic_id;
    uint64_t logical_index;
    uint64_t kernel_cr3;
    uintptr_t stack_top;
    volatile uint32_t online;
    volatile uint32_t work_complete;
    volatile uint64_t work_iterations;
    enum smp_cpu_role role;
};

struct smp_stats {
    uint64_t detected_cpus;
    uint64_t managed_cpus;
    uint64_t online_cpus;
    uint64_t aps_released;
    uint64_t aps_completed;
    uint64_t participant_mask;
    uint64_t expected_locked_count;
    uint64_t actual_locked_count;
    uint64_t spinlock_acquisitions;
    uint64_t spinlock_contentions;
    int multicore_test_ran;
    int multicore_test_passed;
};

/* Bring up secondary processors and run the Phase-17 parallel-work proof. */
int smp_init(void);

const struct smp_stats *smp_get_stats(void);
const struct smp_cpu *smp_cpu_at(uint64_t index);

/* Assembly entry published through Limine's MP goto_address field. */
void smp_ap_entry_asm(struct limine_mp_info *info);
_Noreturn void smp_ap_entry_c(struct smp_cpu *cpu);

#endif
```

## `arch/x86_64/cpu/smp.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/arch/smp.h>
#include <axiom/boot/limine.h>
#include <axiom/memory/vmm.h>
#include <axiom/sync/spinlock.h>

_Static_assert(offsetof(struct smp_cpu, kernel_cr3) == 16u, "smp_cpu CR3 offset");
_Static_assert(offsetof(struct smp_cpu, stack_top) == 24u, "smp_cpu stack offset");

#define SMP_READY_SPIN_LIMIT 200000000ULL
#define SMP_DONE_SPIN_LIMIT  400000000ULL

static struct smp_cpu cpus[SMP_MAX_CPUS];
static uint8_t ap_stacks[SMP_MAX_CPUS][SMP_AP_STACK_SIZE]
    __attribute__((aligned(16)));
static struct smp_stats stats;

static struct spinlock work_lock;
static volatile uint64_t locked_counter;
static volatile uint64_t aps_ready;
static volatile uint64_t aps_done;
static volatile uint64_t release_barrier;
static volatile uint64_t participant_mask;
static volatile int initialized;

static uint64_t atomic_load_u64(volatile uint64_t *value)
{
    return __atomic_load_n(value, __ATOMIC_ACQUIRE);
}

static void atomic_store_u64(volatile uint64_t *value, uint64_t new_value)
{
    __atomic_store_n(value, new_value, __ATOMIC_RELEASE);
}

static uint64_t atomic_add_u64(volatile uint64_t *value, uint64_t amount)
{
    return __atomic_add_fetch(value, amount, __ATOMIC_ACQ_REL);
}

static void atomic_or_u64(volatile uint64_t *value, uint64_t mask)
{
    (void)__atomic_fetch_or(value, mask, __ATOMIC_ACQ_REL);
}

static int wait_until_at_least(
    volatile uint64_t *value,
    uint64_t target,
    uint64_t spin_limit
)
{
    uint64_t spins = 0u;

    while (atomic_load_u64(value) < target) {
        if (spins++ >= spin_limit) {
            return 0;
        }
        __asm__ volatile ("pause" ::: "memory");
    }

    return 1;
}

static void perform_locked_work(struct smp_cpu *cpu)
{
    uint64_t iteration;

    if (cpu == 0) {
        return;
    }

    atomic_or_u64(&participant_mask, 1ULL << cpu->logical_index);

    for (iteration = 0u; iteration < SMP_TEST_ITERATIONS; ++iteration) {
        const uint64_t flags = spin_lock_irqsave(&work_lock);
        ++locked_counter;
        spin_unlock_irqrestore(&work_lock, flags);
    }

    cpu->work_iterations = SMP_TEST_ITERATIONS;
    cpu->work_complete = 1u;
}

_Noreturn void smp_ap_entry_c(struct smp_cpu *cpu)
{
    if (cpu == 0 || cpu->logical_index == 0u ||
        cpu->logical_index >= SMP_MAX_CPUS) {
        for (;;) {
            __asm__ volatile ("cli; hlt" ::: "memory");
        }
    }

    interrupts_disable();

    /*
     * Each AP receives its own GDT/TSS and loads the already-built shared IDT.
     * Interrupts remain disabled: the BSP alone owns the scheduler and timer
     * in this foundational SMP phase.
     */
    if (!gdt_init_secondary((uint32_t)cpu->logical_index, cpu->stack_top)) {
        for (;;) {
            __asm__ volatile ("cli; hlt" ::: "memory");
        }
    }
    interrupts_load_idt();

    cpu->online = 1u;
    atomic_add_u64(&aps_ready, 1u);

    while (atomic_load_u64(&release_barrier) == 0u) {
        __asm__ volatile ("pause" ::: "memory");
    }

    perform_locked_work(cpu);
    atomic_add_u64(&aps_done, 1u);

    /* AP scheduling/timers are intentionally deferred; park this CPU. */
    for (;;) {
        __asm__ volatile ("cli; hlt" ::: "memory");
    }
}

int smp_init(void)
{
    struct limine_mp_response *response;
    uint64_t managed = 0u;
    uint64_t ap_count = 0u;
    uint64_t index;
    uint64_t logical = 1u;
    struct smp_cpu *bsp = 0;

    if (initialized) {
        return stats.multicore_test_ran ? stats.multicore_test_passed : 1;
    }

    response = limine_get_mp_response();
    if (response == 0 || response->cpu_count == 0u || response->cpus == 0) {
        return 0;
    }

    stats.detected_cpus = response->cpu_count;
    stats.managed_cpus = response->cpu_count < SMP_MAX_CPUS
        ? response->cpu_count : SMP_MAX_CPUS;
    managed = stats.managed_cpus;

    for (index = 0u; index < response->cpu_count; ++index) {
        struct limine_mp_info *info = response->cpus[index];

        if (info != 0 && info->lapic_id == response->bsp_lapic_id) {
            bsp = &cpus[0];
            bsp->processor_id = info->processor_id;
            bsp->lapic_id = info->lapic_id;
            bsp->logical_index = 0u;
            bsp->kernel_cr3 = vmm_kernel_address_space();
            bsp->stack_top = gdt_kernel_stack();
            bsp->online = 1u;
            bsp->work_complete = 0u;
            bsp->work_iterations = 0u;
            bsp->role = SMP_CPU_BSP;
            break;
        }
    }

    if (bsp == 0) {
        return 0;
    }

    spinlock_init(&work_lock);
    locked_counter = 0u;
    aps_ready = 0u;
    aps_done = 0u;
    release_barrier = 0u;
    participant_mask = 0u;

    for (index = 0u; index < response->cpu_count && logical < managed; ++index) {
        struct limine_mp_info *info = response->cpus[index];
        struct smp_cpu *cpu;

        if (info == 0 || info->lapic_id == response->bsp_lapic_id) {
            continue;
        }

        cpu = &cpus[logical];
        cpu->processor_id = info->processor_id;
        cpu->lapic_id = info->lapic_id;
        cpu->logical_index = logical;
        cpu->kernel_cr3 = vmm_kernel_address_space();
        cpu->stack_top = (uintptr_t)&ap_stacks[logical][SMP_AP_STACK_SIZE];
        cpu->online = 0u;
        cpu->work_complete = 0u;
        cpu->work_iterations = 0u;
        cpu->role = SMP_CPU_AP;

        info->extra_argument = (uint64_t)(uintptr_t)cpu;
        __atomic_store_n(&info->goto_address, smp_ap_entry_asm, __ATOMIC_RELEASE);

        ++logical;
        ++ap_count;
    }

    stats.aps_released = ap_count;

    if (ap_count == 0u) {
        stats.online_cpus = 1u;
        stats.participant_mask = 1u;
        stats.multicore_test_ran = 0;
        stats.multicore_test_passed = 0;
        initialized = 1;
        return 1;
    }

    if (!wait_until_at_least(&aps_ready, ap_count, SMP_READY_SPIN_LIMIT)) {
        return 0;
    }

    stats.online_cpus = 1u + atomic_load_u64(&aps_ready);
    atomic_store_u64(&release_barrier, 1u);

    /* BSP participates at the same time as the released APs. */
    perform_locked_work(bsp);

    if (!wait_until_at_least(&aps_done, ap_count, SMP_DONE_SPIN_LIMIT)) {
        return 0;
    }

    stats.aps_completed = atomic_load_u64(&aps_done);
    stats.participant_mask = atomic_load_u64(&participant_mask);
    stats.expected_locked_count = stats.online_cpus * SMP_TEST_ITERATIONS;
    stats.actual_locked_count = locked_counter;
    stats.spinlock_acquisitions = work_lock.acquisitions;
    stats.spinlock_contentions = work_lock.contentions;
    stats.multicore_test_ran = 1;
    stats.multicore_test_passed =
        stats.online_cpus >= 2u &&
        stats.aps_completed == ap_count &&
        stats.actual_locked_count == stats.expected_locked_count &&
        stats.participant_mask == ((1ULL << stats.online_cpus) - 1ULL);

    initialized = 1;
    return stats.multicore_test_passed;
}

const struct smp_stats *smp_get_stats(void)
{
    return &stats;
}

const struct smp_cpu *smp_cpu_at(uint64_t index)
{
    if (index >= stats.managed_cpus) {
        return 0;
    }
    return &cpus[index];
}
```

## `arch/x86_64/cpu/smp_entry.S`

```asm
.text
.global smp_ap_entry_asm
.type smp_ap_entry_asm,@function
.extern smp_ap_entry_c

/*
 * Limine enters here with RDI = struct limine_mp_info *.
 * extra_argument (offset 24) points at AxiomOS struct smp_cpu.
 * struct smp_cpu offsets are asserted in smp.c:
 *   kernel_cr3 = 16
 *   stack_top  = 24
 */
smp_ap_entry_asm:
    cli
    movq 24(%rdi), %rax
    testq %rax, %rax
    jz .Lpark

    movq 16(%rax), %rcx
    movq %rcx, %cr3

    movq 24(%rax), %rsp
    andq $-16, %rsp
    xorq %rbp, %rbp

    movq %rax, %rdi
    call smp_ap_entry_c

.Lpark:
    cli
.Lhalt:
    hlt
    jmp .Lhalt
.size smp_ap_entry_asm, .-smp_ap_entry_asm

.section .note.GNU-stack,"",@progbits
```

## `include/axiom/arch/gdt.h`

```c
#ifndef AXIOM_ARCH_GDT_H
#define AXIOM_ARCH_GDT_H

#include <stdint.h>

#define GDT_KERNEL_CODE 0x08u
#define GDT_KERNEL_DATA 0x10u
#define GDT_USER_DATA   0x1Bu
#define GDT_USER_CODE   0x23u
#define GDT_TSS_SELECTOR 0x28u

void gdt_init(void);
int gdt_init_secondary(uint32_t cpu_index, uintptr_t stack_top);
void gdt_set_kernel_stack(uintptr_t stack_top);
uintptr_t gdt_kernel_stack(void);
int gdt_ist_contains(uint8_t index, uintptr_t address);

#endif
```

## `arch/x86_64/cpu/gdt.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/gdt.h>

/* Long-mode TSS: RSP0 is used when Ring 3 enters the kernel. */
struct task_state_segment {
    uint32_t reserved0;
    uint64_t rsp[3];
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

struct descriptor_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

_Static_assert(sizeof(struct task_state_segment) == 104, "TSS layout");
_Static_assert(offsetof(struct task_state_segment, ist) == 36, "TSS IST offset");
_Static_assert(sizeof(struct descriptor_pointer) == 10, "GDTR layout");

/* Null, ring-0 code/data, ring-3 data/code, and a two-slot TSS. */
static uint64_t gdt[7] __attribute__((aligned(16)));
static struct task_state_segment tss;
static uint8_t emergency_stacks[3][16384] __attribute__((aligned(16)));

#define GDT_SECONDARY_MAX_CPUS 8u
static uint64_t secondary_gdt[GDT_SECONDARY_MAX_CPUS][7]
    __attribute__((aligned(16)));
static struct task_state_segment secondary_tss[GDT_SECONDARY_MAX_CPUS];
static uint8_t secondary_emergency_stacks[GDT_SECONDARY_MAX_CPUS][3][16384]
    __attribute__((aligned(16)));

extern uint8_t kernel_stack_top[];
extern void gdt_load(const struct descriptor_pointer *pointer);

void gdt_init(void)
{
    const uint64_t base = (uint64_t)(uintptr_t)&tss;
    const uint64_t limit = sizeof(tss) - 1u;
    const struct descriptor_pointer pointer = {
        .limit = sizeof(gdt) - 1u,
        .base = (uint64_t)(uintptr_t)gdt
    };
    uint8_t i;

    gdt[0] = 0;
    gdt[1] = 0x00AF9A000000FFFFULL; /* Ring-0 code, present, L=1. */
    gdt[2] = 0x00CF92000000FFFFULL; /* Ring-0 writable data. */
    gdt[3] = 0x00CFF2000000FFFFULL; /* Ring-3 writable data. */
    gdt[4] = 0x00AFFA000000FFFFULL; /* Ring-3 code, present, L=1. */
    gdt[5] = (limit & 0xFFFFu)
           | ((base & 0xFFFFFFu) << 16)
           | (0x89ULL << 40) /* Present available 64-bit TSS. */
           | (((limit >> 16) & 0xFu) << 48)
           | (((base >> 24) & 0xFFu) << 56);
    gdt[6] = base >> 32;

    tss.rsp[0] = (uint64_t)(uintptr_t)kernel_stack_top;

    for (i = 0u; i < 3u; ++i) {
        tss.ist[i] = (uint64_t)(uintptr_t)&emergency_stacks[i][16384];
    }

    /* No I/O permission bitmap; its offset lies beyond the TSS limit. */
    tss.iomap_base = sizeof(tss);
    gdt_load(&pointer);
}

void gdt_set_kernel_stack(uintptr_t stack_top)
{
    tss.rsp[0] = (uint64_t)stack_top;
}

uintptr_t gdt_kernel_stack(void)
{
    return (uintptr_t)tss.rsp[0];
}

int gdt_ist_contains(uint8_t index, uintptr_t address)
{
    uintptr_t begin;

    if (index < 1u || index > 3u) {
        return 0;
    }

    begin = (uintptr_t)emergency_stacks[index - 1u];
    return address >= begin && address < begin + 16384u;
}


int gdt_init_secondary(uint32_t cpu_index, uintptr_t stack_top)
{
    uint64_t *cpu_gdt;
    struct task_state_segment *cpu_tss;
    uint64_t base;
    uint64_t limit;
    struct descriptor_pointer pointer;
    uint8_t i;

    if (cpu_index == 0u || cpu_index >= GDT_SECONDARY_MAX_CPUS ||
        stack_top == 0u) {
        return 0;
    }

    cpu_gdt = secondary_gdt[cpu_index];
    cpu_tss = &secondary_tss[cpu_index];
    base = (uint64_t)(uintptr_t)cpu_tss;
    limit = sizeof(*cpu_tss) - 1u;

    for (i = 0u; i < 7u; ++i) {
        cpu_gdt[i] = 0u;
    }
    for (i = 0u; i < sizeof(*cpu_tss); ++i) {
        ((uint8_t *)cpu_tss)[i] = 0u;
    }

    cpu_gdt[0] = 0;
    cpu_gdt[1] = 0x00AF9A000000FFFFULL;
    cpu_gdt[2] = 0x00CF92000000FFFFULL;
    cpu_gdt[3] = 0x00CFF2000000FFFFULL;
    cpu_gdt[4] = 0x00AFFA000000FFFFULL;
    cpu_gdt[5] = (limit & 0xFFFFu)
               | ((base & 0xFFFFFFu) << 16)
               | (0x89ULL << 40)
               | (((limit >> 16) & 0xFu) << 48)
               | (((base >> 24) & 0xFFu) << 56);
    cpu_gdt[6] = base >> 32;

    cpu_tss->rsp[0] = (uint64_t)stack_top;
    for (i = 0u; i < 3u; ++i) {
        cpu_tss->ist[i] = (uint64_t)(uintptr_t)
            &secondary_emergency_stacks[cpu_index][i][16384];
    }
    cpu_tss->iomap_base = sizeof(*cpu_tss);

    pointer.limit = sizeof(secondary_gdt[cpu_index]) - 1u;
    pointer.base = (uint64_t)(uintptr_t)cpu_gdt;
    gdt_load(&pointer);
    return 1;
}
```

## `include/axiom/arch/interrupts.h`

```c
#ifndef AXIOM_ARCH_INTERRUPTS_H
#define AXIOM_ARCH_INTERRUPTS_H

#include <stddef.h>
#include <stdint.h>

/* Exact layout made by isr_stubs.asm, followed by the long-mode CPU frame. */
struct interrupt_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
};

_Static_assert(sizeof(struct interrupt_frame) == 176, "ISR frame size");
_Static_assert(offsetof(struct interrupt_frame, vector) == 120, "ISR vector offset");
_Static_assert(offsetof(struct interrupt_frame, rip) == 136, "ISR RIP offset");
_Static_assert(offsetof(struct interrupt_frame, rsp) == 160, "ISR RSP offset");

typedef struct interrupt_frame *(*irq_handler_t)(struct interrupt_frame *frame);

void interrupts_init(void);
void interrupts_load_idt(void);
void interrupts_enable(void);
void interrupts_disable(void);
int interrupts_enabled(void);

int irq_register(uint8_t irq, irq_handler_t handler);

/* Returns the frame/stack context that the ISR epilogue must restore. */
struct interrupt_frame *interrupt_dispatch(struct interrupt_frame *frame);
void phase3_selftest(void);

_Noreturn void exception_panic(const struct interrupt_frame *frame);
_Noreturn void page_fault_panic(const struct interrupt_frame *frame);

#endif
```

## `arch/x86_64/interrupts/idt.c`

```c
#include <stdint.h>

#include <axiom/arch/apic.h>
#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/arch/pic.h>
#include <axiom/process/scheduler.h>
#include <axiom/terminal/kprintf.h>

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t attributes;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

_Static_assert(sizeof(struct idt_entry) == 16, "IDT gate size");
_Static_assert(sizeof(struct idt_pointer) == 10, "IDTR size");

static struct idt_entry idt[256] __attribute__((aligned(16)));
static irq_handler_t irq_handlers[16];

extern void (*const isr_stub_table[256])(void);

static uint64_t read_cr2(void)
{
    uint64_t value;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(value));
    return value;
}

void interrupts_load_idt(void)
{
    const struct idt_pointer pointer = {
        .limit = sizeof(idt) - 1u,
        .base = (uint64_t)(uintptr_t)idt
    };

    __asm__ volatile ("lidt %0" : : "m"(pointer) : "memory");
}


void interrupts_init(void)
{
    uint16_t vector;
    __asm__ volatile ("cli" ::: "memory");

    for (vector = 0; vector < 256; ++vector) {
        const uintptr_t address = (uintptr_t)isr_stub_table[vector];
        struct idt_entry *entry = &idt[vector];

        entry->offset_low = (uint16_t)address;
        entry->selector = GDT_KERNEL_CODE;
        entry->ist =
            vector == 8 ? 1 :
            vector == 2 ? 2 :
            vector == 18 ? 3 :
            0;
        entry->attributes = 0x8E;
        entry->offset_middle = (uint16_t)(address >> 16);
        entry->offset_high = (uint32_t)(address >> 32);
        entry->reserved = 0;
    }

    interrupts_load_idt();
    pic_init_masked();
}

void interrupts_enable(void)
{
    __asm__ volatile ("sti" ::: "memory");
}

void interrupts_disable(void)
{
    __asm__ volatile ("cli" ::: "memory");
}

int interrupts_enabled(void)
{
    uint64_t flags;
    __asm__ volatile ("pushfq; popq %0" : "=r"(flags));
    return (flags & (1ULL << 9)) != 0ULL;
}

int irq_register(uint8_t irq, irq_handler_t handler)
{
    uint64_t flags;

    __asm__ volatile ("pushfq; popq %0" : "=r"(flags));

    if (irq >= 16u || handler == 0 || (flags & (1ULL << 9)) != 0ULL) {
        return -1;
    }

    irq_handlers[irq] = handler;
    return 0;
}

struct interrupt_frame *interrupt_dispatch(struct interrupt_frame *frame)
{
    if (frame->vector == 3u) {
        kprintf("Breakpoint: resumed safely.\n");
        return frame;
    }

    if (frame->vector == 14u) {
        if ((frame->cs & 3ULL) == 3ULL && scheduler_running()) {
            struct interrupt_frame *next = scheduler_handle_user_fault(
                frame,
                14u,
                frame->error_code,
                read_cr2()
            );

            if (next != 0) {
                return next;
            }
        }

        page_fault_panic(frame);
    }

    if (frame->vector < 32u) {
        exception_panic(frame);
    }

    if (frame->vector < 48u) {
        const uint8_t irq = (uint8_t)(frame->vector - 32u);
        struct interrupt_frame *resume = frame;

        if (apic_active()) {
            if (irq_handlers[irq] != 0) {
                resume = irq_handlers[irq](frame);
                if (resume == 0) {
                    resume = frame;
                }
            }

            apic_eoi();
            return resume;
        }

        if (pic_is_spurious(irq)) {
            return frame;
        }

        if (irq_handlers[irq] != 0) {
            resume = irq_handlers[irq](frame);
            if (resume == 0) {
                resume = frame;
            }
        }

        pic_eoi(irq);
        return resume;
    }

    if (frame->vector == 0x80u) {
        kprintf("Software interrupt 0x80: returned safely.\n");
        return frame;
    }

    if (frame->vector == 0xFFu) {
        return frame;
    }

    exception_panic(frame);
}

#ifdef AXIOM_TEST_DOUBLE_FAULT
void phase3_disable_gp_gate(void)
{
    idt[13].attributes &= (uint8_t)~0x80u;
}
#endif
```

## `include/axiom/sync/spinlock.h`

```c
#ifndef AXIOM_SYNC_SPINLOCK_H
#define AXIOM_SYNC_SPINLOCK_H

#include <stdint.h>

/*
 * Phase-16/17 IRQ-save SMP spinlock.
 *
 * Holding the lock with interrupts disabled prevents the current CPU from
 * being preempted while it owns the lock. Disabling local interrupts prevents the owner CPU from being
 * preempted while the atomic x86 XCHG provides exclusion against other CPUs.
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
            (void)__atomic_add_fetch(&lock->acquisitions, 1u, __ATOMIC_RELAXED);
            return irq_was_enabled;
        }

        (void)__atomic_add_fetch(&lock->contentions, 1u, __ATOMIC_RELAXED);
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

    __atomic_store_n(&lock->locked, 0u, __ATOMIC_RELEASE);
}

void spin_unlock_irqrestore(struct spinlock *lock, uint64_t irq_was_enabled)
{
    spin_unlock(lock);

    if (irq_was_enabled != 0u) {
        interrupts_enable();
    }
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
#include <axiom/arch/smp.h>

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
        kprintf("Synchronization CPU model: BSP only; APs remain parked until Phase 17\n");
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
            if (sync_result.failure_stage == SYNC_SELFTEST_STAGE_MUTEX) {
                kprintf(
                    "Phase 16 failure mutex expected/actual: %llu/%llu\n",
                    (unsigned long long)sync_result.mutex_expected,
                    (unsigned long long)sync_result.mutex_actual
                );
                kprintf(
                    "Phase 16 failure mutex blocks/wakeups: %llu/%llu\n",
                    (unsigned long long)sync_result.mutex_blocks,
                    (unsigned long long)sync_result.mutex_wakeups
                );
            }
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
     * Phase 17: release Limine-parked application processors.  APs switch to
     * the AxiomOS kernel CR3, private kernel stacks, and private TSS instances.
     * The general scheduler deliberately remains BSP-only in this foundation
     * phase; APs run a parallel spinlock proof and then park with IF=0.
     */
    {
        const struct smp_stats *smp;
        uint64_t cpu_index;

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 17 SMP / multicore online.\n");
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );

        if (!smp_init()) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 17 SMP initialization/self-test: FAILED\n");
            return;
        }

        smp = smp_get_stats();
        kprintf(
            "Phase 17 CPUs detected/managed/online: %llu/%llu/%llu\n",
            (unsigned long long)smp->detected_cpus,
            (unsigned long long)smp->managed_cpus,
            (unsigned long long)smp->online_cpus
        );
        kprintf(
            "Phase 17 APs released/completed: %llu/%llu\n",
            (unsigned long long)smp->aps_released,
            (unsigned long long)smp->aps_completed
        );

        for (cpu_index = 0u; cpu_index < smp->managed_cpus; ++cpu_index) {
            const struct smp_cpu *cpu = smp_cpu_at(cpu_index);

            if (cpu == 0) {
                continue;
            }

            kprintf(
                "Phase 17 CPU %llu: processor=%u LAPIC=%u role=%s online=%u\n",
                (unsigned long long)cpu->logical_index,
                (unsigned)cpu->processor_id,
                (unsigned)cpu->lapic_id,
                cpu->role == SMP_CPU_BSP ? "BSP" : "AP",
                (unsigned)cpu->online
            );
        }

        if (smp->multicore_test_ran) {
            kprintf(
                "Phase 17 parallel participant mask: 0x%llX\n",
                (unsigned long long)smp->participant_mask
            );
            kprintf(
                "Phase 17 locked counter expected/actual: %llu/%llu\n",
                (unsigned long long)smp->expected_locked_count,
                (unsigned long long)smp->actual_locked_count
            );
            kprintf(
                "Phase 17 cross-CPU spinlock acquisitions/contentions: %llu/%llu\n",
                (unsigned long long)smp->spinlock_acquisitions,
                (unsigned long long)smp->spinlock_contentions
            );

            if (!smp->multicore_test_passed) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 17 multicore parallel-work test: FAILED\n");
                return;
            }

            kprintf("Phase 17 AP bring-up: OK\n");
            kprintf("Phase 17 cross-CPU spinlock: OK\n");
            kprintf("Phase 17 multicore parallel-work test: OK\n");
        } else {
            kprintf(
                "Phase 17 multicore self-test: SKIPPED (single CPU boot)\n"
            );
        }

        kprintf("Phase 17 scheduler policy: BSP-only; APs parked after self-test\n");
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREEN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 17 SMP initialization complete.\n");
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

## `tests/phase17_smp.py`

```python
#!/usr/bin/env python3
"""Validate Phase-17 x86-64 SMP discovery, AP bring-up, and cross-CPU locking."""

from pathlib import Path
import re
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
        if "Phase 17 SMP initialization/self-test: FAILED" in text or \
                "Phase 17 multicore parallel-work test: FAILED" in text:
            tail = "\n".join(text.splitlines()[-180:])
            raise AssertionError(
                f"phase17 kernel SMP self-test failed; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-180:])
            raise AssertionError(
                f"phase17 timed out waiting for {marker!r}; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )
        time.sleep(0.05)


def test_phase17():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase17-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = directory / "phase17-serial.log"
    qemu_log = directory / "phase17-qemu.log"
    serial.write_text("")

    command = [
        "qemu-system-x86_64",
        "-machine", "q35",
        "-accel", "tcg",
        "-smp", "4",
        "-m", "256M",
        "-cdrom", str(directory / "AxiomOS.iso"),
        "-boot", "d",
        "-display", "none",
        "-serial", f"file:{serial}",
        "-monitor", "none",
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
                time.monotonic() + 180,
            )

            require("AxiomOS Phase 17 SMP / multicore online." in text,
                    "phase17: SMP banner missing")
            require("Phase 17 AP bring-up: OK" in text,
                    "phase17: AP bring-up did not pass")
            require("Phase 17 cross-CPU spinlock: OK" in text,
                    "phase17: cross-CPU spinlock did not pass")
            require("Phase 17 multicore parallel-work test: OK" in text,
                    "phase17: multicore work test did not pass")
            require("Phase 17 SMP initialization complete." in text,
                    "phase17: completion marker missing")
            require("KERNEL PANIC" not in text, "phase17: kernel panicked")
            require("SKIPPED (single CPU boot)" not in text,
                    "phase17: test unexpectedly booted only one CPU")

            counts = re.search(
                r"Phase 17 CPUs detected/managed/online: (\d+)/(\d+)/(\d+)",
                text,
            )
            require(counts is not None, "phase17: CPU counts missing")
            detected, managed, online = map(int, counts.groups())
            require(detected >= 4, "phase17: QEMU did not expose four CPUs")
            require(managed >= 4 and online >= 4,
                    "phase17: not all requested CPUs became managed/online")

            aps = re.search(
                r"Phase 17 APs released/completed: (\d+)/(\d+)", text
            )
            require(aps is not None, "phase17: AP counts missing")
            require(int(aps.group(1)) >= 3,
                    "phase17: fewer than three APs were released")
            require(aps.group(1) == aps.group(2),
                    "phase17: not every released AP completed work")

            counter = re.search(
                r"Phase 17 locked counter expected/actual: (\d+)/(\d+)", text
            )
            require(counter is not None, "phase17: locked counter missing")
            require(counter.group(1) == counter.group(2),
                    "phase17: shared counter was corrupted across CPUs")
            require(int(counter.group(1)) == online * 5000,
                    "phase17: unexpected shared-work amount")

            mask = re.search(
                r"Phase 17 parallel participant mask: 0x([0-9A-Fa-f]+)", text
            )
            require(mask is not None, "phase17: participant mask missing")
            require(int(mask.group(1), 16).bit_count() == online,
                    "phase17: participant mask does not include every online CPU")

            print(f"PASS: Phase 17 detected {detected} CPUs; {online} online")
            print("PASS: Phase 17 application processors entered AxiomOS")
            print("PASS: Phase 17 per-CPU stacks/GDT/TSS initialized")
            print("PASS: Phase 17 BSP/AP parallel shared-memory work")
            print("PASS: Phase 17 cross-CPU spinlock preserved exact counter")
            print("PASS: Phase 17 shell still launches with APs parked")
        finally:
            if process.poll() is None:
                process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


if __name__ == "__main__":
    test_phase17()
    print("PASS: all Phase 17 SMP/multicore tests.")
```

## `docs/phase17.md`

```markdown
# Phase 17 — SMP / Multicore Bring-up

Phase 17 moves AxiomOS from a one-CPU execution model to a real multiprocessor
machine. The kernel now requests Limine's MP feature, discovers every x86-64
CPU exposed by QEMU, releases application processors (APs), switches them onto
AxiomOS-owned page tables, and gives each AP private kernel CPU state.

This phase deliberately does **not** schedule normal processes on APs yet. The
existing scheduler remains BSP-only while the APs prove cross-core execution
and then park. This separates CPU bring-up from the much larger problem of a
fully SMP scheduler.

## QEMU machine

Normal `make run` and `make debug` now use:

```text
-smp 4
```

so QEMU exposes four virtual CPUs. Older regression tests may still boot with
one CPU. On a one-CPU boot Phase 17 reports discovery and skips only the
multicore parallel-work proof; this preserves the historical phase tests.

`make test-phase17` always boots with four CPUs and requires the complete SMP
proof.

## Limine MP handoff

AxiomOS adds a Limine MP request with flags `0`. That intentionally keeps xAPIC
mode because the existing Phase-7 local APIC implementation uses the MMIO
xAPIC interface rather than x2APIC MSRs.

Limine provides:

```text
cpu_count
bsp_lapic_id
cpus[]
  └─ processor_id
  └─ lapic_id
  └─ goto_address
  └─ extra_argument
```

Secondary CPUs are already bootstrapped and parked by Limine. AxiomOS fills
`extra_argument` with a pointer to that CPU's `struct smp_cpu` and publishes
`smp_ap_entry_asm` through `goto_address` with release ordering.

## AP entry sequence

An AP cannot simply call normal kernel C code on Limine's temporary execution
state. Its entry sequence is:

```text
Limine parked AP
      ↓ publish goto_address
smp_ap_entry_asm
      ↓ CLI
read per-CPU pointer
      ↓
load AxiomOS kernel CR3
      ↓
switch to private 16 KiB AP stack
      ↓
smp_ap_entry_c
      ↓
load private GDT + private TSS
      ↓
load shared AxiomOS IDT
      ↓
mark CPU online
```

The CR3 change happens before using the permanent AxiomOS stack. The assembly
contains no stack memory accesses between the CR3 write and the RSP switch.

## Per-CPU state

AxiomOS currently supports up to eight managed CPUs in Phase 17. Each managed
CPU records:

```text
processor ID
LAPIC ID
logical AxiomOS CPU index
kernel CR3
private stack top
BSP/AP role
online state
work-completion state
```

Every AP also receives:

- a private 16 KiB kernel stack;
- a private GDT;
- a private 64-bit TSS;
- private emergency IST stacks.

The IDT itself is read-only after initialization and is shared between CPUs.

## Why the TSS must be per-CPU

`LTR` marks a TSS descriptor busy. Two processors must not share one mutable TSS
instance and one Ring-0 stack pointer. Even though Phase-17 APs do not enter
Ring 3, assigning private TSS state now establishes the correct architecture
for later multicore user scheduling.

## Cross-CPU spinlock proof

Phase 16's spinlock previously only had to prevent timer-preempted tasks on one
CPU from interleaving. Phase 17 makes it a true SMP lock.

The acquire path already uses x86 atomic `XCHG`. Phase 17 also makes the lock's
statistics atomic and uses a release store when unlocking.

All online CPUs synchronize at a start barrier. The BSP and every released AP
then perform:

```text
5,000 iterations each
      ↓
spin_lock_irqsave()
      ↓
shared_counter++
      ↓
spin_unlock_irqrestore()
```

For four CPUs the exact expected result is:

```text
4 × 5,000 = 20,000
```

A participant bitmask also proves that each online logical CPU entered the
parallel-work region.

## Current scheduler policy

The important deliberate limit is:

```text
BSP:
  timer
  interrupts
  scheduler
  kernel threads
  Ring-3 processes

APs:
  Phase-17 parallel proof
  then CLI + HLT park
```

This means AxiomOS is now an SMP-capable kernel at the CPU bring-up and shared
memory synchronization level, but **not yet a multicore process scheduler**.
That distinction is intentional and should not be exaggerated.

Moving the general scheduler onto all CPUs requires at least:

- per-CPU current-task state instead of one global `current_index`;
- a scheduler/run-queue lock or per-CPU run queues;
- per-CPU local APIC timers;
- safe task migration;
- cross-core reschedule IPIs;
- TLB shootdowns when mappings shared by another CPU change;
- auditing PMM/VMM/heap/VFS global state for SMP locking.

Those mechanisms are not faked in Phase 17.

## Acceptance output

With `-smp 4`, boot output should include roughly:

```text
AxiomOS Phase 17 SMP / multicore online.
Phase 17 CPUs detected/managed/online: 4/4/4
Phase 17 APs released/completed: 3/3
Phase 17 CPU 0: ... role=BSP online=1
Phase 17 CPU 1: ... role=AP online=1
Phase 17 CPU 2: ... role=AP online=1
Phase 17 CPU 3: ... role=AP online=1
Phase 17 parallel participant mask: 0xF
Phase 17 locked counter expected/actual: 20000/20000
Phase 17 AP bring-up: OK
Phase 17 cross-CPU spinlock: OK
Phase 17 multicore parallel-work test: OK
Phase 17 scheduler policy: BSP-only; APs parked after self-test
Phase 17 SMP initialization complete.
```

Then the normal Ring-3 shell must still launch.

Run:

```bash
make clean
make
make test-phase17
make test
```

## Deliberate limits

- maximum eight managed CPUs in the current static per-CPU table;
- scheduler remains BSP-only;
- AP local timers are not started;
- no inter-processor interrupts yet;
- no TLB shootdown protocol yet;
- no process/task migration between CPUs;
- APs park permanently after the Phase-17 proof;
- kernel subsystems other than the spinlock test are not yet advertised as
  safe for arbitrary AP execution.
```

## `docs/architecture.md`

```markdown
# Architecture at Phase 17

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
- Scheduler is five-tick preemptive round robin with reusable BLOCKED/READY
  wait-queue hooks, still deliberately BSP-only in Phase 17.
- Ring-3 tasks use private user memory/stacks and trusted kernel stacks.
- Phase 15 provides fork/exec/wait parent-child process management.
- Phase 16 provides spinlocks, FIFO wait queues, sleeping mutexes and counting
  semaphores.
- Phase 17 proves the spinlock across truly parallel CPUs using an exact shared
  counter test; APs park after that proof rather than running general tasks.
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

This is intentionally **not** a fully SMP scheduler yet. Moving arbitrary tasks
between CPUs requires per-CPU current-task state, run-queue locking or per-CPU
run queues, AP timers, reschedule IPIs, and TLB shootdowns.

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

`make test` runs Phase 1 through Phase 17. Destructive fault/corruption cases
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

`make test-phase17` boots QEMU with `-smp 4`, requires at least four detected/managed/online CPUs, verifies that every released AP enters AxiomOS code, checks the per-CPU participant mask, and requires the exact locked shared-counter total (`online CPUs × 20,000`). It also requires the normal Ring-3 shell to launch after APs park. Older phase tests may still boot one CPU; Phase 17 discovery remains active but its multicore proof reports SKIPPED on those boots rather than breaking historical regression tests.
```

## `docs/project-state.md`

```markdown
# AxiomOS handoff — Phase 17 implementation

Development environment: Linux/Distrobox on Steam Deck. Run commands from the
directory containing `Makefile`. Roadmap: Phase 0 through Phase 25.

Accepted foundation before Phase 17:

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
- Phase 16: spinlocks, wait queues, sleeping mutexes and semaphores.

Phase 17 adds:

- Limine MP feature request while deliberately retaining xAPIC mode;
- CPU discovery using processor IDs and LAPIC IDs;
- `make run` / `make debug` with four QEMU CPUs;
- release of Limine-parked application processors;
- AP switch to the AxiomOS kernel CR3;
- private 16 KiB AP kernel stacks;
- private GDT/TSS and IST stacks for every managed AP;
- a shared read-only IDT loaded on APs;
- per-CPU identity/online/work state for up to eight managed CPUs;
- a start barrier proving all released APs reached AxiomOS code;
- real BSP/AP parallel execution against shared memory;
- SMP-safe spinlock statistics and release semantics;
- an exact cross-core locked-counter acceptance test;
- dedicated `make test-phase17` using QEMU `-smp 4`.

Important limits:

- normal task/process scheduling remains BSP-only;
- APs perform the Phase-17 parallel test and then park with interrupts off;
- no AP local timers, scheduler run queues, IPIs, task migration, or TLB
  shootdowns yet;
- do not claim that arbitrary kernel subsystems are SMP-safe merely because AP
  bring-up works;
- the current static per-CPU table manages at most eight CPUs.

Acceptance commands:

```bash
make clean
make
make test-phase17
make test
```

Next milestone: Phase 18 — networking (Ethernet → ARP → IPv4 → ICMP → UDP →
TCP → DNS → HTTP), while preserving the Phase-17 BSP-only scheduler policy
until a later scheduler expansion deliberately distributes general tasks.
```
