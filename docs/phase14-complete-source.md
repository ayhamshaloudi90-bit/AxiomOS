# AxiomOS Phase 14 — complete new/modified source

This handoff contains every text file added or modified for Phase 14, relative to the accepted Phase-13 tree.

## Changed files

- `Makefile`
- `README.md`
- `boot/limine/limine.conf`
- `docs/filesystem.md`
- `docs/phase14.md`
- `docs/processes.md`
- `docs/project-state.md`
- `docs/syscalls.md`
- `docs/validation.md`
- `filesystem/bootstrap.c`
- `filesystem/diskfs.c`
- `include/axiom/abi/errno.h`
- `include/axiom/abi/fs.h`
- `include/axiom/abi/input.h`
- `include/axiom/abi/syscall.h`
- `include/axiom/kernel/syscall.h`
- `include/axiom/process/scheduler.h`
- `include/axiom/process/task.h`
- `kernel/core/kernel.c`
- `kernel/syscall/syscall.c`
- `libc/include/axiom/unistd.h`
- `process/scheduler.c`
- `tests/phase10_syscalls.py`
- `tests/phase11_elf.py`
- `tests/phase12_vfs.py`
- `tests/phase14_shell.py`
- `tests/phase7_devices.py`
- `tests/phase8_scheduler.py`
- `tests/phase9_userspace.py`
- `userspace/phase14_shell.c`
- `userspace/phase14_shell_start.S`

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
	@echo "AxiomOS Phase 14 targets:"
	@echo
	@echo "  make          Build kernel and bootable ISO"
	@echo "  make run      Boot AxiomOS using QEMU"
	@echo "  make debug    Boot QEMU paused for GDB"
	@echo "  make test     Run Phase 1 through Phase 14 tests"
	@echo "  make test-phase6 Run heap + corruption tests"
	@echo "  make test-phase7 Run timer + keyboard IRQ/input tests"
	@echo "  make test-phase8 Run preemptive scheduler/context-switch tests"
	@echo "  make test-phase9 Run Ring 3 userspace/isolation tests"
	@echo "  make test-phase10 Run SYSCALL/SYSRET ABI and user-copy tests"
	@echo "  make test-phase11 Run ELF64 loader/executable tests"
	@echo "  make test-phase12 Run VFS/RAM filesystem/file-descriptor tests"
	@echo "  make test-phase13 Run AHCI/block-device/persistent disk tests"
	@echo "  make test-phase14 Run interactive Ring-3 shell tests"
	@echo "  make disk-image Create the 16 MiB persistent QEMU disk image"
	@echo "  make MODE=heap_double_free run  Trigger double-free panic"
	@echo "  make MODE=heap_guard run        Trigger tail-guard panic"
	@echo "  make inspect  Inspect generated ELF64 kernel"
	@echo "  make clean    Remove generated build files"
	@echo "  make distclean Remove build files and Limine"

.PHONY: test-phase3 test-phase4 test-phase5 test-phase6 test-phase7 test-phase8 test-phase9 test-phase10 test-phase11 test-phase12 test-phase13 test-phase14 disk-image

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

Phases 0–13 provide boot, memory management, interrupts, preemptive scheduling,
Ring-3 isolation, a syscall ABI, ELF64 loading, a VFS, and persistent AHCI-backed
storage.

Phase 14 adds the first real interactive userspace shell:

- `/bin/axiomsh` runs as an ELF64 Ring-3 task;
- `help`, `clear`, `echo`, `pwd`, `cd`, `ls`, `cat`, `stat`, `touch`, `write`,
  `mkdir`, and `kbdstats`;
- external ELF launch by command name (`/bin/<name>`) or explicit VFS path;
- `spawn()` + `waitpid()` so the shell survives child execution;
- interactive access to RAMFS and the persistent `/disk` mount;
- automated QEMU keyboard testing of command parsing, file operations and ELF
  process execution.

The shell is intentionally small: there is no `argv` passing, piping,
redirection, environment, signals, or job control yet. Those build on the
process-management work in the next phases.

## Common commands

```bash
make
make run
make test
make test-phase13
make test-phase14
make MODE=page_fault run
make MODE=heap_double_free run
make MODE=heap_guard run
make debug
```

`make test` runs the complete Phase 1 through Phase 14 regression suite.

When `make run` reaches the shell:

```text
AxiomOS shell ready. Type 'help' for commands.
axiom> help
```

The next milestone is Phase 15: process management.

[Architecture](docs/architecture.md) | [Memory](docs/memory.md) |
[Tasks](docs/processes.md) | [Syscalls](docs/syscalls.md) |
[Filesystem](docs/filesystem.md) | [Phase 13](docs/phase13.md) |
[Phase 14](docs/phase14.md) | [Project state](docs/project-state.md) |
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
```

## `docs/filesystem.md`

```markdown
# AxiomOS filesystem architecture

Phase 12 introduced the virtual filesystem layer; Phase 13 adds a persistent
backend without changing the VFS or userspace file ABI.

## VFS

The VFS lives in `filesystem/vfs.c` and exposes generic filesystem/node/file
objects. Filesystem backends supply vnode operations for lookup, creation,
reading, writing, truncation, and directory enumeration.

Current generic operations include:

```text
vfs_mount
vfs_lookup
vfs_mkdir
vfs_readdir
vfs_open
vfs_read
vfs_write
vfs_close
vfs_seek
vfs_stat
vfs_read_all
vfs_write_file
```

Paths are normalized and mount selection uses the longest matching prefix.

## Current mounts

```text
/       -> root RAMFS
/tmp    -> secondary RAMFS
/disk   -> persistent diskfs when an AHCI disk is attached
```

`/bin`, `/etc`, `/dev`, `/home`, `/tmp`, and the `/disk` mountpoint live in the
RAM root filesystem. Boot ELF modules are still copied into `/bin`; persistent
user data can now live under `/disk`.

## RAM filesystem

`filesystem/ramfs.c` stores directory nodes, metadata, and file contents in the
kernel heap. RAMFS contents disappear on reboot.

## Persistent diskfs

`filesystem/diskfs.c` is a small Phase-13 backend built on the generic block
API. It uses a one-sector superblock, eight sectors of fixed directory entries,
and contiguous append-only data extents. The filesystem is deliberately simple
so the storage-driver path remains understandable.

The persistent path is:

```text
open/read/write/lseek/stat
        -> VFS
        -> diskfs
        -> block API
        -> AHCI DMA
        -> QEMU raw disk image
```

## File descriptors

Every task owns 16 descriptor slots. Slots 0, 1 and 2 retain stdin/stdout/
stderr semantics; VFS opens allocate from descriptor 3 upward. Descriptors hold
vnode, current offset, and open flags. Task exit/fault cleanup closes all
remaining regular file descriptors.

## Next step

Phase 14 adds the shell, allowing the human user to interact with these existing
file and executable APIs through commands rather than only automated Ring-3
demos.

## Phase 14 shell integration

The VFS is now directly usable from `/bin/axiomsh`. Directory enumeration is
exposed through `SYS_readdir`, and `mkdir`, `open`, `read`, `write`, `lseek`,
`stat`, and executable lookup all travel through the same VFS layer. The shell
can therefore use RAMFS (`/`, `/tmp`) and persistent diskfs (`/disk`) without
backend-specific code.
```

## `docs/phase14.md`

```markdown
# Phase 14 — Ring-3 interactive shell

Phase 14 replaces the old kernel input demonstration with a real userspace
shell, `/bin/axiomsh`.

## Architecture

The shell is a standalone ELF64 `ET_EXEC` program. It runs at CPL3 and uses the
existing Phase-10 syscall boundary rather than calling kernel/VFS functions
directly.

To make command execution possible without destroying the shell process,
Phase 14 extends the syscall ABI with:

- `readdir(path, index, entry)` — enumerate VFS directories;
- `mkdir(path)` — create a directory through the mounted filesystem;
- `spawn(path)` — load a VFS-backed ELF into a new Ring-3 task;
- `waitpid(pid, status)` — wait for a spawned child and retrieve its exit code;
- `clear()` — clear the framebuffer terminal;
- `kbdstats()` — expose the existing PS/2 diagnostic counters.

`spawn()` intentionally accepts only an executable path in this phase. `argv`,
environment variables, pipes, redirection, job control and signals are later
userspace/process-management work.

## Commands

```text
help
clear
echo <text>
pwd
cd <dir>
ls [dir]
cat <file>
stat <path>
touch <file>
write <file> <text>
mkdir <dir>
kbdstats
<program>
/path/program
```

An unqualified external command is resolved under `/bin`. For example:

```text
axiom> phase11-demo
```

spawns `/bin/phase11-demo`, waits for it, then returns to the shell prompt.

## Filesystem behavior

The shell can interact with every currently mounted VFS backend:

- `/`, `/tmp` — RAMFS;
- `/disk` — persistent Phase-13 diskfs when an AHCI disk is attached.

The shell keeps its current working directory in userspace and converts
relative names to absolute VFS paths before issuing syscalls.

## Acceptance

```bash
make clean
make
make test-phase14
make test
```

The dedicated test injects real QEMU PS/2 key events and verifies built-ins,
VFS reads/writes/directories, `/disk` access, keyboard diagnostics, and
spawn/wait of another Ring-3 ELF.
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

## Phase 11 ELF-backed tasks

`user_task_create_elf()` creates a normal isolated Ring-3 task from a validated
ELF64 `ET_EXEC` image. The ELF loader owns the executable image layout:
`PT_LOAD` headers choose virtual addresses and R/W/X permissions, while the
scheduler still supplies the task's trusted kernel stack and ordinary four-page
user stack. ELF-backed tasks record their entry point, program-header/load
segment counts, mapped image frames, and file-vs-memory byte totals for cleanup
and diagnostics.

## Phase 14 foreground child execution

Tasks now carry a `parent_id`. The shell's `spawn(path)` syscall creates a new
ELF-backed Ring-3 task and records the shell as its parent. `waitpid()` yields
the shell until that child reaches `TASK_TERMINATED`, then returns the child's
exit status. This is intentionally a minimal foreground process model; richer
lifecycle and process-management semantics are Phase 15.
```

## `docs/project-state.md`

```markdown
# AxiomOS handoff — Phase 14 implementation

Development environment: Linux/Distrobox on Steam Deck. Run commands from the
directory containing `Makefile`. Roadmap: Phase 0 through Phase 25.

Accepted foundation before Phase 14:

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

Phase 14 adds:

- `/bin/axiomsh`, a standalone Ring-3 ELF shell;
- line editing and an interactive `axiom> ` prompt;
- `help`, `clear`, `echo`, `pwd`, `cd`, `ls`, `cat`, `stat`, `touch`,
  `write`, `mkdir`, and `kbdstats` built-ins;
- VFS directory enumeration from userspace;
- userspace directory creation;
- `spawn(path)` and `waitpid(pid, status)` syscalls;
- parent tracking for spawned tasks;
- external command lookup under `/bin` or by explicit VFS path;
- foreground execution of ELF programs while preserving the shell process;
- interactive access to RAMFS and persistent `/disk` files;
- a QEMU keyboard-driven Phase-14 acceptance test;
- Phase-7/8/9/10 regression keyboard checks updated to use the real shell.

Important limits:

- external programs do not receive `argc/argv` yet;
- no pipes, redirection, environment variables, signals or job control;
- shell current working directory is userspace-local;
- no `fork()` yet; `spawn()` directly creates a new ELF task;
- process lifecycle features become richer in Phase 15;
- diskfs remains intentionally small/flat at its own root;
- single CPU only.

Acceptance commands:

```bash
make clean
make
make test-phase14
make test
```

Next milestone: Phase 15 — process management.
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

`fork` and `mmap` retain stable ABI numbers but return `-ENOSYS` until their
owning phases are implemented.

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
        !seed_module_file("phase14-shell", "/bin/axiomsh")) {
        return 0;
    }

    /* A second RAM filesystem proves that path resolution honors mounts. */
    if (vfs_mount("/tmp", tmpfs) < 0) {
        return 0;
    }

    return 1;
}
```

## `filesystem/diskfs.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>
#include <axiom/drivers/block.h>
#include <axiom/filesystem/diskfs.h>
#include <axiom/memory/heap.h>

#define DISKFS_MAGIC 0x3153464D4F495841ULL /* "AXIOMFS1" little-endian */
#define DISKFS_VERSION 1u
#define DISKFS_DIRECTORY_START 1u
#define DISKFS_DIRECTORY_SECTORS 8u
#define DISKFS_DATA_START 16u
#define DISKFS_MAX_FILES 64u
#define DISKFS_NAME_MAX 47u
#define DISKFS_FILE_MODE 0644u
#define DISKFS_ROOT_MODE 0755u

struct diskfs_superblock {
    uint64_t magic;
    uint32_t version;
    uint32_t sector_size;
    uint64_t total_sectors;
    uint64_t directory_start;
    uint32_t directory_sectors;
    uint32_t max_files;
    uint64_t data_start;
    uint64_t next_free_sector;
    uint8_t reserved[456];
};

struct diskfs_dir_entry {
    uint8_t used;
    uint8_t type;
    uint16_t reserved0;
    uint32_t start_sector;
    uint32_t sector_count;
    uint32_t size;
    char name[48];
};

_Static_assert(sizeof(struct diskfs_superblock) == BLOCK_SECTOR_SIZE,
               "diskfs superblock must be one sector");
_Static_assert(sizeof(struct diskfs_dir_entry) == 64u,
               "diskfs directory entry must be 64 bytes");
_Static_assert(DISKFS_MAX_FILES * sizeof(struct diskfs_dir_entry) ==
               DISKFS_DIRECTORY_SECTORS * BLOCK_SECTOR_SIZE,
               "diskfs directory table layout");

struct diskfs_instance;

struct diskfs_node {
    struct vfs_node vfs;
    struct diskfs_instance *filesystem;
    uint32_t entry_index;
    char name[48];
};

struct diskfs_instance {
    struct vfs_filesystem vfs;
    struct block_device *device;
    struct diskfs_superblock superblock;
    struct diskfs_dir_entry entries[DISKFS_MAX_FILES];
    struct diskfs_node nodes[DISKFS_MAX_FILES];
    struct diskfs_node root;
    struct diskfs_info info;
};

static struct diskfs_instance *active_instance;

static int diskfs_lookup(struct vfs_node *, const char *, struct vfs_node **);
static int diskfs_create_node(struct vfs_node *, const char *, enum vfs_node_type, struct vfs_node **);
static int64_t diskfs_read(struct vfs_node *, uint64_t, void *, size_t);
static int64_t diskfs_write(struct vfs_node *, uint64_t, const void *, size_t);
static int diskfs_truncate(struct vfs_node *, uint64_t);
static int diskfs_readdir(struct vfs_node *, size_t, struct vfs_dirent *);

static const struct vfs_node_ops diskfs_ops = {
    .lookup = diskfs_lookup,
    .create = diskfs_create_node,
    .read = diskfs_read,
    .write = diskfs_write,
    .truncate = diskfs_truncate,
    .readdir = diskfs_readdir,
};

static void bytes_clear(void *memory, size_t count)
{
    uint8_t *bytes = (uint8_t *)memory;
    size_t index;
    for (index = 0u; index < count; ++index) bytes[index] = 0u;
}

static void bytes_copy(void *destination, const void *source, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    size_t index;
    for (index = 0u; index < count; ++index) out[index] = in[index];
}

static size_t string_length(const char *text)
{
    size_t length = 0u;
    if (text == 0) return 0u;
    while (text[length] != '\0') ++length;
    return length;
}

static int strings_equal(const char *left, const char *right)
{
    size_t index = 0u;
    if (left == 0 || right == 0) return 0;
    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] != right[index]) return 0;
        ++index;
    }
    return left[index] == right[index];
}

static void string_copy(char *destination, const char *source, size_t capacity)
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

static struct diskfs_node *node_from_vfs(struct vfs_node *node)
{
    return node != 0 ? (struct diskfs_node *)node->private_data : 0;
}

static int persist_superblock(struct diskfs_instance *fs)
{
    return block_write(fs->device, 0u, 1u, &fs->superblock) &&
           block_flush(fs->device);
}

static int persist_directory_sector(struct diskfs_instance *fs, uint32_t entry_index)
{
    uint8_t sector[BLOCK_SECTOR_SIZE];
    const uint32_t entries_per_sector = BLOCK_SECTOR_SIZE / sizeof(struct diskfs_dir_entry);
    const uint32_t sector_index = entry_index / entries_per_sector;
    const uint32_t first_entry = sector_index * entries_per_sector;

    if (sector_index >= DISKFS_DIRECTORY_SECTORS) return 0;
    bytes_copy(sector, &fs->entries[first_entry], sizeof(sector));
    return block_write(
        fs->device,
        fs->superblock.directory_start + sector_index,
        1u,
        sector
    ) && block_flush(fs->device);
}

static void refresh_node(struct diskfs_instance *fs, uint32_t index)
{
    struct diskfs_node *node = &fs->nodes[index];
    const struct diskfs_dir_entry *entry = &fs->entries[index];

    node->filesystem = fs;
    node->entry_index = index;
    string_copy(node->name, entry->name, sizeof(node->name));
    node->vfs.name = node->name;
    node->vfs.type = VFS_NODE_FILE;
    node->vfs.mode = DISKFS_FILE_MODE;
    node->vfs.size = entry->size;
    node->vfs.ops = &diskfs_ops;
    node->vfs.private_data = node;
}

static int format_filesystem(struct diskfs_instance *fs)
{
    uint8_t zero[BLOCK_SECTOR_SIZE];
    uint32_t sector;

    if (fs->device->sector_count <= DISKFS_DATA_START + 1u ||
        fs->device->sector_count > UINT32_MAX) {
        return 0;
    }

    bytes_clear(&fs->superblock, sizeof(fs->superblock));
    bytes_clear(fs->entries, sizeof(fs->entries));
    bytes_clear(zero, sizeof(zero));

    fs->superblock.magic = DISKFS_MAGIC;
    fs->superblock.version = DISKFS_VERSION;
    fs->superblock.sector_size = BLOCK_SECTOR_SIZE;
    fs->superblock.total_sectors = fs->device->sector_count;
    fs->superblock.directory_start = DISKFS_DIRECTORY_START;
    fs->superblock.directory_sectors = DISKFS_DIRECTORY_SECTORS;
    fs->superblock.max_files = DISKFS_MAX_FILES;
    fs->superblock.data_start = DISKFS_DATA_START;
    fs->superblock.next_free_sector = DISKFS_DATA_START;

    if (!block_write(fs->device, 0u, 1u, &fs->superblock)) return 0;
    for (sector = 0u; sector < DISKFS_DIRECTORY_SECTORS; ++sector) {
        if (!block_write(
                fs->device,
                DISKFS_DIRECTORY_START + sector,
                1u,
                zero
            )) return 0;
    }
    return block_flush(fs->device);
}

static int load_directory(struct diskfs_instance *fs)
{
    uint32_t sector;
    uint8_t buffer[BLOCK_SECTOR_SIZE];

    for (sector = 0u; sector < DISKFS_DIRECTORY_SECTORS; ++sector) {
        if (!block_read(
                fs->device,
                fs->superblock.directory_start + sector,
                1u,
                buffer
            )) return 0;
        bytes_copy(
            (uint8_t *)fs->entries + (size_t)sector * BLOCK_SECTOR_SIZE,
            buffer,
            BLOCK_SECTOR_SIZE
        );
    }
    return 1;
}

static int superblock_valid(
    const struct diskfs_superblock *superblock,
    const struct block_device *device
)
{
    return superblock->magic == DISKFS_MAGIC &&
        superblock->version == DISKFS_VERSION &&
        superblock->sector_size == BLOCK_SECTOR_SIZE &&
        superblock->total_sectors == device->sector_count &&
        superblock->directory_start == DISKFS_DIRECTORY_START &&
        superblock->directory_sectors == DISKFS_DIRECTORY_SECTORS &&
        superblock->max_files == DISKFS_MAX_FILES &&
        superblock->data_start == DISKFS_DATA_START &&
        superblock->next_free_sector >= DISKFS_DATA_START &&
        superblock->next_free_sector <= device->sector_count;
}

static int ensure_extent(struct diskfs_node *node, uint64_t required_size)
{
    struct diskfs_instance *fs = node->filesystem;
    struct diskfs_dir_entry *entry = &fs->entries[node->entry_index];
    uint64_t required_sectors64 = (required_size + BLOCK_SECTOR_SIZE - 1u) /
        BLOCK_SECTOR_SIZE;
    uint32_t required_sectors;
    uint32_t new_start;
    uint8_t zero[BLOCK_SECTOR_SIZE];
    uint8_t copy[BLOCK_SECTOR_SIZE];
    uint32_t sector;

    if (required_sectors64 <= entry->sector_count) return 1;
    if (required_sectors64 > UINT32_MAX) return 0;
    required_sectors = (uint32_t)required_sectors64;

    if (fs->superblock.next_free_sector > fs->device->sector_count ||
        (uint64_t)required_sectors >
            fs->device->sector_count - fs->superblock.next_free_sector) {
        return 0;
    }

    new_start = (uint32_t)fs->superblock.next_free_sector;
    bytes_clear(zero, sizeof(zero));

    for (sector = 0u; sector < required_sectors; ++sector) {
        if (!block_write(fs->device, (uint64_t)new_start + sector, 1u, zero)) {
            return 0;
        }
    }

    for (sector = 0u; sector < entry->sector_count; ++sector) {
        if (!block_read(fs->device, (uint64_t)entry->start_sector + sector, 1u, copy) ||
            !block_write(fs->device, (uint64_t)new_start + sector, 1u, copy)) {
            return 0;
        }
    }

    entry->start_sector = new_start;
    entry->sector_count = required_sectors;
    fs->superblock.next_free_sector += required_sectors;
    fs->info.next_free_sector = fs->superblock.next_free_sector;

    return persist_superblock(fs) &&
        persist_directory_sector(fs, node->entry_index);
}

static int diskfs_lookup(
    struct vfs_node *directory,
    const char *name,
    struct vfs_node **node_out
)
{
    struct diskfs_node *root;
    struct diskfs_instance *fs;
    uint32_t index;

    if (directory == 0 || name == 0 || node_out == 0) return -AXIOM_EINVAL;
    if (directory->type != VFS_NODE_DIRECTORY) return -AXIOM_ENOTDIR;
    root = node_from_vfs(directory);
    if (root == 0 || root->filesystem == 0) return -AXIOM_EIO;
    fs = root->filesystem;

    for (index = 0u; index < DISKFS_MAX_FILES; ++index) {
        if (fs->entries[index].used && strings_equal(fs->entries[index].name, name)) {
            refresh_node(fs, index);
            *node_out = &fs->nodes[index].vfs;
            return 0;
        }
    }
    *node_out = 0;
    return -AXIOM_ENOENT;
}

static int diskfs_create_node(
    struct vfs_node *directory,
    const char *name,
    enum vfs_node_type type,
    struct vfs_node **node_out
)
{
    struct diskfs_node *root;
    struct diskfs_instance *fs;
    uint32_t index;
    struct vfs_node *existing = 0;

    if (directory == 0 || name == 0 || name[0] == '\0' ||
        string_length(name) > DISKFS_NAME_MAX) return -AXIOM_EINVAL;
    if (type != VFS_NODE_FILE) return -AXIOM_EROFS;
    if (diskfs_lookup(directory, name, &existing) == 0) return -AXIOM_EEXIST;

    root = node_from_vfs(directory);
    if (root == 0 || root->filesystem == 0) return -AXIOM_EIO;
    fs = root->filesystem;

    for (index = 0u; index < DISKFS_MAX_FILES; ++index) {
        if (!fs->entries[index].used) {
            struct diskfs_dir_entry *entry = &fs->entries[index];
            bytes_clear(entry, sizeof(*entry));
            entry->used = 1u;
            entry->type = AXIOM_DT_FILE;
            string_copy(entry->name, name, sizeof(entry->name));
            if (!persist_directory_sector(fs, index)) {
                bytes_clear(entry, sizeof(*entry));
                return -AXIOM_EIO;
            }
            ++fs->info.file_count;
            refresh_node(fs, index);
            if (node_out != 0) *node_out = &fs->nodes[index].vfs;
            return 0;
        }
    }
    return -AXIOM_ENOSPC;
}

static int64_t diskfs_read(
    struct vfs_node *vfs_node,
    uint64_t offset,
    void *buffer,
    size_t count
)
{
    struct diskfs_node *node = node_from_vfs(vfs_node);
    struct diskfs_dir_entry *entry;
    uint8_t sector_data[BLOCK_SECTOR_SIZE];
    uint8_t *out = (uint8_t *)buffer;
    uint64_t remaining;
    size_t copied = 0u;

    if (node == 0 || node->filesystem == 0 ||
        (count != 0u && buffer == 0)) return -AXIOM_EINVAL;
    entry = &node->filesystem->entries[node->entry_index];
    if (offset >= entry->size || count == 0u) return 0;

    remaining = entry->size - offset;
    if (remaining > (uint64_t)count) remaining = count;

    while ((uint64_t)copied < remaining) {
        const uint64_t position = offset + copied;
        const uint32_t sector_offset = (uint32_t)(position % BLOCK_SECTOR_SIZE);
        size_t chunk = BLOCK_SECTOR_SIZE - sector_offset;
        if ((uint64_t)chunk > remaining - copied) chunk = (size_t)(remaining - copied);

        if (!block_read(
                node->filesystem->device,
                (uint64_t)entry->start_sector + position / BLOCK_SECTOR_SIZE,
                1u,
                sector_data
            )) return -AXIOM_EIO;
        bytes_copy(out + copied, sector_data + sector_offset, chunk);
        copied += chunk;
    }
    return (int64_t)copied;
}

static int64_t diskfs_write(
    struct vfs_node *vfs_node,
    uint64_t offset,
    const void *buffer,
    size_t count
)
{
    struct diskfs_node *node = node_from_vfs(vfs_node);
    struct diskfs_instance *fs;
    struct diskfs_dir_entry *entry;
    const uint8_t *in = (const uint8_t *)buffer;
    uint8_t sector_data[BLOCK_SECTOR_SIZE];
    uint64_t end;
    size_t copied = 0u;

    if (node == 0 || node->filesystem == 0 ||
        (count != 0u && buffer == 0)) return -AXIOM_EINVAL;
    if (count == 0u) return 0;
    if (UINT64_MAX - offset < (uint64_t)count) return -AXIOM_ENOSPC;
    end = offset + count;
    if (end > UINT32_MAX) return -AXIOM_ENOSPC;

    fs = node->filesystem;
    entry = &fs->entries[node->entry_index];
    if (!ensure_extent(node, end)) return -AXIOM_ENOSPC;

    while (copied < count) {
        const uint64_t position = offset + copied;
        const uint32_t sector_offset = (uint32_t)(position % BLOCK_SECTOR_SIZE);
        size_t chunk = BLOCK_SECTOR_SIZE - sector_offset;
        const uint64_t lba = (uint64_t)entry->start_sector + position / BLOCK_SECTOR_SIZE;
        if (chunk > count - copied) chunk = count - copied;

        if (sector_offset != 0u || chunk != BLOCK_SECTOR_SIZE) {
            if (!block_read(fs->device, lba, 1u, sector_data)) return -AXIOM_EIO;
        } else {
            bytes_clear(sector_data, sizeof(sector_data));
        }
        bytes_copy(sector_data + sector_offset, in + copied, chunk);
        if (!block_write(fs->device, lba, 1u, sector_data)) return -AXIOM_EIO;
        copied += chunk;
    }

    if (end > entry->size) entry->size = (uint32_t)end;
    refresh_node(fs, node->entry_index);
    if (!persist_directory_sector(fs, node->entry_index) || !block_flush(fs->device)) {
        return -AXIOM_EIO;
    }
    return (int64_t)count;
}

static int diskfs_truncate(struct vfs_node *vfs_node, uint64_t size)
{
    struct diskfs_node *node = node_from_vfs(vfs_node);
    struct diskfs_instance *fs;
    struct diskfs_dir_entry *entry;

    if (node == 0 || node->filesystem == 0 || size > UINT32_MAX) return -AXIOM_EINVAL;
    fs = node->filesystem;
    entry = &fs->entries[node->entry_index];

    if (size > entry->size && !ensure_extent(node, size)) return -AXIOM_ENOSPC;
    entry->size = (uint32_t)size;
    refresh_node(fs, node->entry_index);
    return persist_directory_sector(fs, node->entry_index) ? 0 : -AXIOM_EIO;
}

static int diskfs_readdir(
    struct vfs_node *directory,
    size_t requested,
    struct vfs_dirent *entry_out
)
{
    struct diskfs_node *root = node_from_vfs(directory);
    struct diskfs_instance *fs;
    uint32_t index;
    size_t current = 0u;

    if (root == 0 || root->filesystem == 0 || entry_out == 0) return -AXIOM_EINVAL;
    fs = root->filesystem;

    for (index = 0u; index < DISKFS_MAX_FILES; ++index) {
        if (!fs->entries[index].used) continue;
        if (current++ == requested) {
            string_copy(entry_out->name, fs->entries[index].name, sizeof(entry_out->name));
            entry_out->type = AXIOM_DT_FILE;
            return 1;
        }
    }
    return 0;
}

struct vfs_filesystem *diskfs_create(
    struct block_device *device,
    struct diskfs_info *info_out
)
{
    struct diskfs_instance *fs;
    uint32_t index;
    uint32_t file_count = 0u;

    if (device == 0 || device->sector_size != BLOCK_SECTOR_SIZE) return 0;

    fs = (struct diskfs_instance *)kmalloc(sizeof(*fs));
    if (fs == 0) return 0;
    bytes_clear(fs, sizeof(*fs));
    fs->device = device;

    if (!block_read(device, 0u, 1u, &fs->superblock)) return 0;
    if (!superblock_valid(&fs->superblock, device)) {
        if (!format_filesystem(fs)) return 0;
        fs->info.freshly_formatted = 1;
    }
    if (!load_directory(fs)) return 0;

    for (index = 0u; index < DISKFS_MAX_FILES; ++index) {
        if (fs->entries[index].used) {
            ++file_count;
            refresh_node(fs, index);
        }
    }

    fs->root.filesystem = fs;
    fs->root.entry_index = UINT32_MAX;
    string_copy(fs->root.name, "/", sizeof(fs->root.name));
    fs->root.vfs.name = fs->root.name;
    fs->root.vfs.type = VFS_NODE_DIRECTORY;
    fs->root.vfs.mode = DISKFS_ROOT_MODE;
    fs->root.vfs.size = file_count;
    fs->root.vfs.ops = &diskfs_ops;
    fs->root.vfs.private_data = &fs->root;

    fs->vfs.name = "diskfs";
    fs->vfs.root = &fs->root.vfs;
    fs->vfs.private_data = fs;

    fs->info.total_sectors = device->sector_count;
    fs->info.data_start_sector = fs->superblock.data_start;
    fs->info.next_free_sector = fs->superblock.next_free_sector;
    fs->info.file_count = file_count;
    active_instance = fs;

    if (info_out != 0) *info_out = fs->info;
    return &fs->vfs;
}

struct diskfs_info diskfs_get_info(void)
{
    struct diskfs_info empty = {0};
    return active_instance != 0 ? active_instance->info : empty;
}
```

## `include/axiom/abi/errno.h`

```c
#ifndef AXIOM_ABI_ERRNO_H
#define AXIOM_ABI_ERRNO_H

/* Small errno set shared by kernel and userspace. Syscalls return -errno. */
#define AXIOM_ENOENT       2
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

## `include/axiom/abi/fs.h`

```c
#ifndef AXIOM_ABI_FS_H
#define AXIOM_ABI_FS_H

/* open(2)-style access and creation flags used by the AxiomOS ABI. */
#define AXIOM_O_RDONLY  0x0000
#define AXIOM_O_WRONLY  0x0001
#define AXIOM_O_RDWR    0x0002
#define AXIOM_O_ACCMODE 0x0003
#define AXIOM_O_CREAT   0x0040
#define AXIOM_O_TRUNC   0x0200
#define AXIOM_O_APPEND  0x0400

#define AXIOM_SEEK_SET 0
#define AXIOM_SEEK_CUR 1
#define AXIOM_SEEK_END 2

#define AXIOM_DT_FILE 1
#define AXIOM_DT_DIR  2

#define AXIOM_DIRENT_NAME_MAX 64

#ifndef __ASSEMBLER__
#include <stdint.h>

struct axiom_stat {
    uint64_t size;
    uint32_t type;
    uint32_t mode;
};

struct axiom_dirent {
    char name[AXIOM_DIRENT_NAME_MAX];
    uint32_t type;
};
#endif

#endif
```

## `include/axiom/abi/input.h`

```c
#ifndef AXIOM_ABI_INPUT_H
#define AXIOM_ABI_INPUT_H

#ifndef __ASSEMBLER__
#include <stdint.h>

struct axiom_keyboard_stats {
    uint64_t irq_count;
    uint64_t scancode_count;
    uint64_t character_count;
    uint64_t dropped_characters;
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

#define AXIOM_SYSCALL_MAX_NUMBER AXIOM_SYS_KBDSTATS

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
    const struct task *child;
    int64_t status;

    ++stats.waitpid_calls;

    if (parent == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    for (;;) {
        child = scheduler_task_by_id(pid);
        if (child == 0 || child->parent_id != parent->id) {
            return syscall_error(AXIOM_ECHILD);
        }
        if (child->state == TASK_TERMINATED) {
            status = child->exit_code;
            break;
        }
        if (!scheduler_yield_current()) {
            return syscall_error(AXIOM_EINVAL);
        }
    }

    if (user_status != 0u) {
        if (!vmm_user_range_accessible(
                parent->address_space,
                user_status,
                sizeof(status),
                1
            ) ||
            !vmm_copy_to_user(
                parent->address_space,
                user_status,
                &status,
                sizeof(status)
            )) {
            ++stats.rejected_pointers;
            return syscall_error(AXIOM_EFAULT);
        }
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

        case AXIOM_SYS_FORK:
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

#include <axiom/abi/fs.h>
#include <axiom/abi/input.h>

/*
 * Early userspace ABI declarations. The standalone assembly demos provide
 * wrappers today; Phase 19 will turn these into a normal userspace libc.
 */
long write(int fd, const void *buffer, size_t count);
long read(int fd, void *buffer, size_t count);
_Noreturn void _exit(int status);
long sleep(uint64_t milliseconds);
long getpid(void);
long yield(void);
long open(const char *path, int flags);
long close(int fd);
long exec(const char *path);
long lseek(int fd, long offset, int whence);
long stat(const char *path, struct axiom_stat *status);
long readdir(const char *path, unsigned long index, struct axiom_dirent *entry);
long mkdir(const char *path);
long spawn(const char *path);
long waitpid(long pid, long *status);
long clear(void);
long kbdstats(struct axiom_keyboard_stats *stats);

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

static struct task *prepare_creation_slot(size_t *slot_index_out)
{
    size_t index;

    if (slot_index_out == 0) {
        return 0;
    }

    /*
     * Reuse a terminated task before growing the high-water mark.  Earlier
     * phases intentionally keep terminated tasks around long enough for their
     * acceptance checks to inspect exit/fault state; the next task creation is
     * the natural point at which that stale slot can be reclaimed safely.
     */
    for (index = 1u; index < task_count_value; ++index) {
        if (index != current_index && tasks[index].state == TASK_TERMINATED) {
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
    }
    task->user_elf_page_count = loaded.page_count;
    for (index = 0u; index < loaded.page_count; ++index) {
        task->user_elf_pages[index] = loaded.pages[index].physical_address;
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
    tasks[current_index].state = TASK_TERMINATED;
    tasks[current_index].ticks_in_slice = tasks[current_index].quantum_ticks;

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
                "AxiomOS shell ready. Type 'help' for commands.",
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
            send_keys(monitor, ["k", "b", "d", "s", "t", "a", "t", "s", "ret"])
            wait_for_text(
                process,
                serial,
                "Keyboard IRQs/scancodes/chars/dropped:",
                time.monotonic() + 10,
            )

            final_text = serial.read_text(errors="replace").replace("\r", "")
            require("Keyboard IRQs/scancodes/chars/dropped:" in final_text,
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

## `tests/phase11_elf.py`

```python
#!/usr/bin/env python3
"""Boot AxiomOS and validate the Phase-11 ELF64 executable loader."""

from pathlib import Path
import re
import shutil
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
            tail = "\n".join(text.replace("\r", "").splitlines()[-70:])
            raise AssertionError(
                f"phase11 timed out waiting for {marker!r}; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )

        time.sleep(0.05)


def parse_decimal(text, label, signed=False):
    pattern = r"(-?\d+)" if signed else r"(\d+)"
    match = re.search(rf"^{re.escape(label)}: {pattern}$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


def parse_hex(text, label):
    match = re.search(rf"^{re.escape(label)}: 0x([0-9A-Fa-f]+)$", text, re.MULTILINE)
    require(match is not None, f"Missing hex field: {label}")
    return int(match.group(1), 16)


def inspect_userspace_elf(path):
    readelf = shutil.which("llvm-readelf") or shutil.which("readelf")
    require(readelf is not None, "phase11: llvm-readelf/readelf is unavailable")

    result = subprocess.run(
        [readelf, "-h", "-l", str(path)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=True,
    )
    text = result.stdout

    require("ELF64" in text, "phase11: user executable is not ELF64")
    require("EXEC (Executable file)" in text or "Type:                              EXEC" in text,
            "phase11: user executable is not ET_EXEC")
    require("Advanced Micro Devices X86-64" in text or "AMD x86-64" in text,
            "phase11: user executable is not x86-64")
    require("Entry point address:               0x400000" in text,
            "phase11: unexpected ELF entry point")

    load_lines = [line for line in text.splitlines() if line.strip().startswith("LOAD")]
    require(len(load_lines) == 3,
            f"phase11: expected 3 PT_LOAD entries, saw {len(load_lines)}")


def test_phase11():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase11-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    launcher_elf = directory / "userspace" / "phase11_launcher.elf"
    user_elf = directory / "userspace" / "phase11_demo.elf"
    require(launcher_elf.exists(), "phase11: exec launcher ELF was not built")
    require(user_elf.exists(), "phase11: standalone userspace ELF was not built")
    inspect_userspace_elf(launcher_elf)
    inspect_userspace_elf(user_elf)

    serial = directory / "phase11-serial.log"
    qemu_log = directory / "phase11-qemu.log"
    serial.write_text("")

    command = [
        "qemu-system-x86_64",
        "-machine", "q35",
        "-accel", "tcg",
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
        process = subprocess.Popen(command, stdout=output, stderr=subprocess.STDOUT)

        try:
            wait_for_text(
                process,
                serial,
                "Phase 11 ELF loader complete.",
                time.monotonic() + 80,
            )

            wait_for_text(
                process,
                serial,
                "AxiomOS shell ready. Type 'help' for commands.",
                time.monotonic() + 5,
            )

            text = serial.read_text(errors="replace").replace("\r", "")

            for expected in [
                "Phase 10 system calls complete.",
                "AxiomOS Phase 11 ELF64 loader online.",
                "ELF launcher source: /boot/phase11_launcher.elf (Limine module)",
                "ELF exec target: /bin/phase11-demo -> /boot/phase11_demo.elf",
                "ELF type: ET_EXEC x86-64",
                "Phase 11 segment permissions: RX/R/RW+NX OK",
                "Hello from an ELF64 executable in AxiomOS!",
                "Phase 11 exec preserved task ID: OK",
                "Phase 11 BSS zero-fill: OK",
                "Phase 11 ELF executable test: OK",
                "Phase 11 ELF loader complete.",
            ]:
                require(expected in text, f"phase11: missing output: {expected}")

            require("KERNEL PANIC" not in text, "phase11: kernel panicked")
            require("Phase 11 ELF task creation: FAILED" not in text,
                    "phase11: ELF task creation failed")
            require("Phase 11 ELF mapping validation: FAILED" not in text,
                    "phase11: ELF page-permission validation failed")
            require("Phase 11 ELF executable test: FAILED" not in text,
                    "phase11: ELF executable self-test failed")

            entry = parse_hex(text, "ELF entry point")
            phnum = parse_decimal(text, "ELF program headers")
            segments = parse_decimal(text, "ELF PT_LOAD segments")
            pages = parse_decimal(text, "ELF mapped pages")
            task_id = parse_decimal(text, "Phase 11 ELF task ID")
            pid = parse_decimal(text, "Phase 11 getpid result")
            exit_status = parse_decimal(text, "Phase 11 exit status", signed=True)
            exec_match = re.search(
                r"^Phase 11 exec transitions: (\d+)/(\d+)$",
                text,
                re.MULTILINE,
            )
            require(exec_match is not None,
                    "phase11: missing exec transition diagnostics")
            exec_successes, exec_calls = (int(value) for value in exec_match.groups())

            byte_match = re.search(
                r"^ELF file/memory bytes: (\d+)/(\d+)$",
                text,
                re.MULTILINE,
            )
            require(byte_match is not None,
                    "phase11: missing ELF file/memory byte diagnostics")
            file_bytes, memory_bytes = (int(value) for value in byte_match.groups())

            require(entry == 0x400000,
                    f"phase11: unexpected loaded entry 0x{entry:X}")
            require(phnum == 3, f"phase11: expected 3 program headers, got {phnum}")
            require(segments == 3,
                    f"phase11: expected 3 PT_LOAD segments, got {segments}")
            require(pages == 3,
                    f"phase11: expected 3 image pages, got {pages}")
            require(pid == task_id,
                    f"phase11: getpid returned {pid}, task id is {task_id}")
            require(exit_status == 11,
                    f"phase11: exit status {exit_status}, expected 11")
            require(memory_bytes > file_bytes,
                    "phase11: BSS did not make p_memsz larger than p_filesz")
            require(exec_calls >= 1 and exec_successes >= 1,
                    "phase11: SYS_exec did not replace the launcher image")

            print("PASS: Phase 11 standalone ELF64 executable")
            print(f"  entry:            0x{entry:X}")
            print(f"  program headers:  {phnum}")
            print(f"  PT_LOAD segments: {segments}")
            print(f"  mapped pages:     {pages}")
            print("PASS: Phase 11 ELF segment permissions + BSS zero-fill")
            print(f"  file/memory:      {file_bytes}/{memory_bytes} bytes")
            print("PASS: Phase 11 exec() image replacement (current kernel resolves target through VFS)")
            print(f"  exec success/calls: {exec_successes}/{exec_calls}")
            print("PASS: Phase 11 ELF process executed through existing syscall ABI")
            print(f"  task id/getpid:   {task_id}/{pid}")
            print(f"  exit status:      {exit_status}")
        finally:
            if process.poll() is None:
                process.terminate()

            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


if __name__ == "__main__":
    test_phase11()
    print("PASS: all Phase 11 ELF-loader tests.")
```

## `tests/phase12_vfs.py`

```python
#!/usr/bin/env python3
"""Boot AxiomOS and validate the Phase-12 VFS/RAM filesystem."""

from pathlib import Path
import re
import shutil
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
            tail = "\n".join(text.replace("\r", "").splitlines()[-90:])
            raise AssertionError(
                f"phase12 timed out waiting for {marker!r}; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )

        time.sleep(0.05)


def parse_decimal(text, label):
    match = re.search(rf"^{re.escape(label)}: (\d+)$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


def inspect_userspace_elf(path):
    readelf = shutil.which("llvm-readelf") or shutil.which("readelf")
    require(readelf is not None, "phase12: llvm-readelf/readelf is unavailable")

    result = subprocess.run(
        [readelf, "-h", "-l", str(path)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=True,
    )
    text = result.stdout

    require("ELF64" in text, "phase12: VFS demo is not ELF64")
    require("EXEC (Executable file)" in text or
            "Type:                              EXEC" in text,
            "phase12: VFS demo is not ET_EXEC")
    require("Entry point address:               0x400000" in text,
            "phase12: unexpected VFS demo entry point")

    load_lines = [line for line in text.splitlines() if line.strip().startswith("LOAD")]
    require(len(load_lines) == 3,
            f"phase12: expected 3 PT_LOAD entries, saw {len(load_lines)}")


def test_phase12():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase12-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    demo_elf = directory / "userspace" / "phase12_demo.elf"
    require(demo_elf.exists(), "phase12: VFS demo ELF was not built")
    inspect_userspace_elf(demo_elf)

    serial = directory / "phase12-serial.log"
    qemu_log = directory / "phase12-qemu.log"
    serial.write_text("")

    command = [
        "qemu-system-x86_64",
        "-machine", "q35",
        "-accel", "tcg",
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
        process = subprocess.Popen(command, stdout=output, stderr=subprocess.STDOUT)

        try:
            wait_for_text(
                process,
                serial,
                "Phase 12 virtual filesystem complete.",
                time.monotonic() + 90,
            )
            wait_for_text(
                process,
                serial,
                "AxiomOS shell ready. Type 'help' for commands.",
                time.monotonic() + 5,
            )

            text = serial.read_text(errors="replace").replace("\r", "")

            expected_lines = [
                "Phase 11 ELF loader complete.",
                "AxiomOS Phase 12 virtual filesystem online.",
                "VFS root filesystem: ramfs mounted at /",
                "VFS secondary mount: ramfs mounted at /tmp",
                "VFS /bin type: DIRECTORY",
                "VFS /tmp type: DIRECTORY (mount point)",
                "Phase 12 ELF source: /bin/phase12-demo (VFS)",
                "Phase 12 VFS motd: Welcome to the AxiomOS virtual filesystem!",
                "Phase 12 VFS readback: RAM filesystem round-trip works!",
                "Phase 12 /tmp file round-trip: OK",
                "Phase 12 seek/stat operations: OK",
                "Phase 12 exec through VFS: OK",
                "Phase 12 VFS self-test: OK",
                "Phase 12 virtual filesystem complete.",
            ]

            for expected in expected_lines:
                require(expected in text, f"phase12: missing output: {expected}")

            require("KERNEL PANIC" not in text, "phase12: kernel panicked")
            require("Phase 12 VFS bootstrap: FAILED" not in text,
                    "phase12: VFS bootstrap failed")
            require("Phase 12 VFS task creation: FAILED" not in text,
                    "phase12: VFS ELF task creation failed")
            require("Phase 12 userspace VFS demo: FAILED" not in text,
                    "phase12: userspace VFS demo failed")
            require("Phase 12 VFS self-test: FAILED" not in text,
                    "phase12: VFS self-test failed")

            mounts = parse_decimal(text, "VFS mount count")
            task_id = parse_decimal(text, "Phase 12 ELF task ID")
            fd_base = parse_decimal(text, "Phase 12 file descriptor base")

            open_close = re.search(
                r"^Phase 12 VFS opens/closes: (\d+)/(\d+)$",
                text,
                re.MULTILINE,
            )
            require(open_close is not None,
                    "phase12: missing VFS open/close diagnostics")
            opens, closes = (int(value) for value in open_close.groups())

            byte_counts = re.search(
                r"^Phase 12 VFS bytes read/written: (\d+)/(\d+)$",
                text,
                re.MULTILINE,
            )
            require(byte_counts is not None,
                    "phase12: missing VFS byte diagnostics")
            bytes_read, bytes_written = (int(value) for value in byte_counts.groups())

            require(mounts == 2, f"phase12: expected two mounts, got {mounts}")
            require(task_id > 0, "phase12: invalid userspace task id")
            require(fd_base == 3, f"phase12: first regular fd should be 3, got {fd_base}")
            require(opens >= 4 and closes >= 4,
                    "phase12: too little VFS open/close activity")
            require(bytes_read > 0 and bytes_written > 0,
                    "phase12: VFS moved no file data")
            require(text.count("Hello from an ELF64 executable in AxiomOS!") >= 2,
                    "phase12: VFS-backed exec target did not execute")

            print("PASS: Phase 12 VFS path and mount resolution")
            print(f"  mounts:            {mounts}")
            print("PASS: Phase 12 per-process file descriptors")
            print(f"  first regular fd:  {fd_base}")
            print("PASS: Phase 12 RAM filesystem read/write/seek/stat")
            print(f"  VFS opens/closes:  {opens}/{closes}")
            print(f"  bytes read/written:{bytes_read}/{bytes_written}")
            print("PASS: Phase 12 exec() resolves through the VFS")
        finally:
            if process.poll() is None:
                process.terminate()

            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


if __name__ == "__main__":
    test_phase12()
    print("PASS: all Phase 12 VFS tests.")
```

## `tests/phase14_shell.py`

```python
#!/usr/bin/env python3
"""Validate the Phase-14 interactive Ring-3 shell and command execution."""

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
            tail = "\n".join(text.splitlines()[-120:])
            raise AssertionError(
                f"phase14 timed out waiting for {marker!r}; see {serial}\n"
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
            tail = "\n".join(text.splitlines()[-100:])
            raise AssertionError(
                f"phase14 timed out waiting for prompt #{count}\n--- serial tail ---\n{tail}"
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
    require(character in mapping, f"phase14: no HMP key mapping for {character!r}")
    return mapping[character]


def send_command(monitor_path, command):
    keys = [key_for_character(character) for character in command]
    keys.append("ret")
    send_hmp_keys(monitor_path, keys)


def test_phase14():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase14-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    shell_elf = directory / "userspace" / "axiomsh.elf"
    require(shell_elf.exists(), "phase14: shell ELF was not built")

    disk = directory / "phase14-test-disk.img"
    with disk.open("wb") as handle:
        handle.truncate(16 * 1024 * 1024)

    serial = directory / "phase14-serial.log"
    qemu_log = directory / "phase14-qemu.log"
    monitor = directory / "phase14-monitor.sock"
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
                time.monotonic() + 140,
            )
            require("AxiomOS Phase 14 interactive shell online." in text,
                    "phase14: kernel did not launch shell")
            require("Shell executable: /bin/axiomsh" in text,
                    "phase14: wrong shell executable")
            require("Shell privilege: Ring 3" in text,
                    "phase14: shell is not reported as Ring 3")
            require("KERNEL PANIC" not in text, "phase14: kernel panicked before prompt")

            prompt = text.count("axiom> ")
            require(prompt >= 1, "phase14: initial prompt missing")

            cases = [
                ("help", "AxiomOS shell commands:"),
                ("pwd", "\n/\n"),
                ("ls /", "bin/"),
                ("cat /etc/motd", "Welcome to the AxiomOS virtual filesystem!"),
                ("write /tmp/shell.txt shell-data", None),
                ("cat /tmp/shell.txt", "shell-data"),
                ("mkdir /home/demo", None),
                ("ls /home", "demo/"),
                ("stat /etc/motd", "type: file"),
                ("cat /disk/persist.txt", "Persistent storage works across AxiomOS reboots!"),
                ("phase11-demo", "Hello from an ELF64 executable in AxiomOS!"),
                ("kbdstats", "Keyboard IRQs/scancodes/chars/dropped:"),
                ("echo phase14-ok", "phase14-ok"),
            ]

            for shell_command, marker in cases:
                before = serial_text(serial)
                prompt = before.count("axiom> ")
                send_command(monitor, shell_command)
                if marker is not None:
                    wait_for_text(process, serial, marker, time.monotonic() + 20)
                wait_for_prompt_count(process, serial, prompt + 1, time.monotonic() + 20)

            final = serial_text(serial)
            require("shell: command not found" not in final,
                    "phase14: a tested command was not recognized")
            require("Phase 14 shell task creation: FAILED" not in final,
                    "phase14: shell task creation failed")
            require("KERNEL PANIC" not in final, "phase14: kernel panicked during shell test")
            require(re.search(r"\[process \d+ exited 11\]", final) is not None,
                    "phase14: spawned ELF did not return exit status 11")

            stats = re.search(
                r"Keyboard IRQs/scancodes/chars/dropped: (\d+)/(\d+)/(\d+)/(\d+)",
                final,
            )
            require(stats is not None, "phase14: kbdstats output missing")
            require(int(stats.group(1)) > 0 and int(stats.group(3)) > 0,
                    "phase14: keyboard counters did not advance")
            require(int(stats.group(4)) == 0,
                    "phase14: keyboard input overflowed during shell test")

            print("PASS: Phase 14 Ring-3 interactive shell")
            print("PASS: Phase 14 command parser and built-ins")
            print("PASS: Phase 14 VFS file/directory commands")
            print("PASS: Phase 14 persistent /disk access")
            print("PASS: Phase 14 spawn/waitpid ELF execution")
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
    test_phase14()
    print("PASS: all Phase 14 shell tests.")
```

## `tests/phase7_devices.py`

```python
#!/usr/bin/env python3
"""Boot AxiomOS and validate Phase-7 APIC timer + PS/2 keyboard interrupts."""

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

        require(process.poll() is None,
                f"QEMU exited before {marker!r}")

        if time.monotonic() >= deadline:
            tail = "\n".join(text.replace("\r", "").splitlines()[-30:])
            raise AssertionError(
                f"boot/input timed out waiting for {marker!r}; "
                f"see {serial}\n--- serial tail ---\n{tail}"
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

        # Consume the HMP greeting when available; its contents are irrelevant.
        try:
            connection.recv(4096)
        except socket.timeout:
            pass

        for key in keys:
            connection.sendall(f"sendkey {key} 30\n".encode("ascii"))
            time.sleep(0.08)


def parse_u64(text, label):
    match = re.search(rf"^{re.escape(label)}: (\d+)$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


def test_phase7():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase7-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = directory / "phase7-serial.log"
    qemu_log = directory / "phase7-qemu.log"
    monitor = directory / "phase7-monitor.sock"

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
                "AxiomOS shell ready. Type 'help' for commands.",
                time.monotonic() + 35,
            )

            before_input = serial.read_text(errors="replace").replace("\r", "")

            for expected in [
                "AxiomOS kernel booted successfully.",
                "Phase 3 CPU initialization complete.",
                "Phase 4 physical memory manager complete.",
                "Phase 5 virtual memory manager complete.",
                "Phase 6 kernel heap complete.",
                "AxiomOS Phase 7 timer + keyboard online.",
                "APIC timer frequency: 100 Hz",
                "Interrupt controller: Local APIC + I/O APIC",
                "Phase 7 timer test: OK",
                "Phase 7 timer + keyboard drivers complete.",
                "AxiomOS shell ready. Type 'help' for commands.",
            ]:
                require(expected in before_input,
                        f"phase7: missing output: {expected}")

            require("KERNEL PANIC" not in before_input,
                    "phase7: kernel panicked before keyboard test")
            require("Phase 7 timer initialization: FAILED" not in before_input,
                    "phase7: APIC timer initialization failed")
            require("Phase 7 keyboard initialization: FAILED" not in before_input,
                    "phase7: PS/2 keyboard initialization failed")

            timer_ticks = parse_u64(before_input, "Timer self-test ticks")
            require(timer_ticks >= 10,
                    "phase7: timer did not deliver ten APIC ticks")

            # HMP injects actual virtual key events into QEMU's PS/2 keyboard.
            send_keys(monitor, ["k", "b", "d", "s", "t", "a", "t", "s", "ret"])

            wait_for_text(
                process,
                serial,
                "Keyboard IRQs/scancodes/chars/dropped:",
                time.monotonic() + 8,
            )

            text = serial.read_text(errors="replace").replace("\r", "")

            require("kbdstats" in text,
                    "phase7: injected PS/2 command was not decoded/buffered")
            require("KERNEL PANIC" not in text,
                    "phase7: keyboard IRQ path panicked")

            stats_match = re.search(
                r"Keyboard IRQs/scancodes/chars/dropped: "
                r"(\d+)/(\d+)/(\d+)/(\d+)",
                text,
            )
            require(stats_match is not None,
                    "phase7: keyboard statistics were not printed")

            irqs, scancodes, characters, dropped = (
                int(value) for value in stats_match.groups()
            )

            require(irqs > 0, "phase7: no IRQ1 interrupts were observed")
            require(scancodes >= 6,
                    "phase7: too few keyboard scancodes were observed")
            require(characters >= 6,
                    "phase7: decoder did not produce the expected characters")
            require(dropped == 0,
                    "phase7: keyboard ring buffer unexpectedly overflowed")

            print("PASS: Phase 7 APIC timer")
            print("  frequency: 100 Hz")
            print(f"  self-test ticks: {timer_ticks}")
            print("PASS: Phase 7 PS/2 keyboard")
            print(f"  IRQs:       {irqs}")
            print(f"  scancodes:  {scancodes}")
            print(f"  characters: {characters}")
            print(f"  dropped:    {dropped}")
            print("  shell command: kbdstats")
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
    test_phase7()
    print("PASS: all Phase 7 timer/keyboard tests.")
```

## `tests/phase8_scheduler.py`

```python
#!/usr/bin/env python3
"""Boot AxiomOS and prove Phase-8 timer-driven preemptive multitasking."""

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

        require(process.poll() is None,
                f"QEMU exited before {marker!r}")

        if time.monotonic() >= deadline:
            tail = "\n".join(text.replace("\r", "").splitlines()[-40:])
            raise AssertionError(
                f"phase8 timed out waiting for {marker!r}; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )

        time.sleep(0.05)


def parse_u64(text, label):
    match = re.search(rf"^{re.escape(label)}: (\d+)$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


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


def test_phase8():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase8-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = directory / "phase8-serial.log"
    qemu_log = directory / "phase8-qemu.log"
    monitor = directory / "phase8-monitor.sock"

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
                "Phase 8 multitasking complete.",
                time.monotonic() + 45,
            )

            wait_for_text(
                process,
                serial,
                "AxiomOS shell ready. Type 'help' for commands.",
                time.monotonic() + 5,
            )

            text = serial.read_text(errors="replace").replace("\r", "")

            for expected in [
                "Phase 7 timer + keyboard drivers complete.",
                "AxiomOS Phase 8 preemptive scheduler online.",
                "Scheduler policy: preemptive round-robin",
                "Scheduler quantum: 5 timer ticks",
                "Phase 8 preemptive multitasking test: OK",
                "Phase 8 multitasking complete.",
            ]:
                require(expected in text, f"phase8: missing output: {expected}")

            require("KERNEL PANIC" not in text, "phase8: kernel panicked")
            require("Phase 8 preemptive multitasking test: FAILED" not in text,
                    "phase8: kernel self-test reported failure")

            task_count = parse_u64(text, "Phase 8 task count")
            worker_a = parse_u64(text, "Phase 8 worker A count")
            worker_b = parse_u64(text, "Phase 8 worker B count")
            switches = parse_u64(text, "Phase 8 context switches")
            preemptions = parse_u64(text, "Phase 8 timer preemptions")

            require(task_count == 3,
                    f"phase8: expected 3 schedulable tasks, got {task_count}")
            require(worker_a >= 1000,
                    "phase8: worker A did not make independent progress")
            require(worker_b >= 1000,
                    "phase8: worker B did not make independent progress")
            require(switches >= 3,
                    "phase8: too few context switches observed")
            require(preemptions >= 3,
                    "phase8: scheduler was not timer-preemptive")

            # The scheduler must coexist with the Phase-7 IRQ-driven keyboard.
            send_keys(monitor, ["k", "b", "d", "s", "t", "a", "t", "s", "ret"])
            wait_for_text(
                process,
                serial,
                "Keyboard IRQs/scancodes/chars/dropped:",
                time.monotonic() + 10,
            )

            final_text = serial.read_text(errors="replace").replace("\r", "")
            require("Keyboard IRQs/scancodes/chars/dropped:" in final_text,
                    "phase8: keyboard input broke after scheduler start")
            require("KERNEL PANIC" not in final_text,
                    "phase8: kernel panicked during scheduler/keyboard coexistence test")

            print("PASS: Phase 8 preemptive round-robin scheduler")
            print(f"  tasks:            {task_count}")
            print(f"  worker A count:   {worker_a}")
            print(f"  worker B count:   {worker_b}")
            print(f"  context switches: {switches}")
            print(f"  preemptions:      {preemptions}")
            print("PASS: Phase 8 scheduler + keyboard coexistence")
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
    test_phase8()
    print("PASS: all Phase 8 multitasking tests.")
```

## `tests/phase9_userspace.py`

```python
#!/usr/bin/env python3
"""Boot AxiomOS and prove Phase-9 Ring-3 isolation and privilege transitions."""

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
            tail = "\n".join(text.replace("\r", "").splitlines()[-50:])
            raise AssertionError(
                f"phase9 timed out waiting for {marker!r}; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )

        time.sleep(0.05)


def parse_decimal(text, label):
    match = re.search(rf"^{re.escape(label)}: (\d+)$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


def parse_hex(text, label):
    match = re.search(rf"^{re.escape(label)}: 0x([0-9A-Fa-f]+)$", text, re.MULTILINE)
    require(match is not None, f"Missing hex field: {label}")
    return int(match.group(1), 16)


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


def test_phase9():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase9-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = directory / "phase9-serial.log"
    qemu_log = directory / "phase9-qemu.log"
    monitor = directory / "phase9-monitor.sock"

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
                "Phase 9 userspace complete.",
                time.monotonic() + 50,
            )

            wait_for_text(
                process,
                serial,
                "AxiomOS shell ready. Type 'help' for commands.",
                time.monotonic() + 5,
            )

            text = serial.read_text(errors="replace").replace("\r", "")

            for expected in [
                "Phase 8 preemptive multitasking test: OK",
                "AxiomOS Phase 9 Ring 3 userspace online.",
                "Phase 9 user privilege: Ring 3",
                "Phase 9 separate address spaces: OK",
                "Hello from AxiomOS userspace!",
                "Phase 9 kernel protection fault: OK",
                "Phase 9 userspace isolation test: OK",
                "Phase 9 userspace complete.",
            ]:
                require(expected in text, f"phase9: missing output: {expected}")

            require("KERNEL PANIC" not in text, "phase9: kernel panicked")
            require("Phase 9 userspace isolation test: FAILED" not in text,
                    "phase9: userspace isolation self-test failed")

            kernel_match = re.search(
                r"^VMM root PML4: physical 0x([0-9A-Fa-f]+)$",
                text,
                re.MULTILINE,
            )
            require(kernel_match is not None, "phase9: missing kernel CR3 field")
            kernel_space = int(kernel_match.group(1), 16)
            hello_space = parse_hex(text, "Phase 9 hello address space")
            protection_space = parse_hex(text, "Phase 9 protection address space")
            fault_vector = parse_decimal(text, "Phase 9 protection fault vector")
            fault_address = parse_hex(text, "Phase 9 protection fault address")
            fault_error = parse_hex(text, "Phase 9 protection fault error")
            fault_terminations = parse_decimal(text, "Phase 9 user fault terminations")

            require(hello_space != kernel_space,
                    "phase9: hello task reused the kernel address space")
            require(protection_space != kernel_space,
                    "phase9: protection task reused the kernel address space")
            require(hello_space != protection_space,
                    "phase9: user tasks did not receive separate address spaces")
            require(fault_vector == 14,
                    f"phase9: expected #PF vector 14, got {fault_vector}")
            require(fault_address == 0xFFFFFFFF80000000,
                    f"phase9: unexpected protected address 0x{fault_address:X}")
            require((fault_error & 0x1) != 0,
                    "phase9: kernel access did not fault as a protection violation")
            require((fault_error & 0x4) != 0,
                    "phase9: fault was not generated from user mode")
            require(fault_terminations >= 1,
                    "phase9: faulting user task was not terminated")

            # IRQ input should still work while Ring-3 and kernel tasks coexist.
            send_keys(monitor, ["k", "b", "d", "s", "t", "a", "t", "s", "ret"])
            wait_for_text(
                process,
                serial,
                "Keyboard IRQs/scancodes/chars/dropped:",
                time.monotonic() + 10,
            )

            final_text = serial.read_text(errors="replace").replace("\r", "")
            require("Keyboard IRQs/scancodes/chars/dropped:" in final_text,
                    "phase9: keyboard input broke after entering Ring 3")
            require("KERNEL PANIC" not in final_text,
                    "phase9: kernel panicked during userspace/keyboard coexistence")

            print("PASS: Phase 9 Ring 3 userspace")
            print(f"  kernel CR3:      0x{kernel_space:X}")
            print(f"  hello CR3:       0x{hello_space:X}")
            print(f"  protection CR3:  0x{protection_space:X}")
            print("PASS: Phase 9 hardware memory isolation")
            print(f"  fault vector:    {fault_vector}")
            print(f"  fault address:   0x{fault_address:X}")
            print(f"  fault error:     0x{fault_error:X}")
            print("PASS: Phase 9 userspace + scheduler + keyboard coexistence")
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
    test_phase9()
    print("PASS: all Phase 9 userspace/isolation tests.")
```

## `userspace/phase14_shell.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>
#include <axiom/abi/fs.h>
#include <axiom/abi/input.h>
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

    if (command[0] == '/' || command[0] == '.') {
        if (!normalize_path(command, path, sizeof(path))) {
            print("shell: program path too long\n");
            return;
        }
    } else {
        static const char prefix[] = "/bin/";
        size_t length = text_length(prefix);
        const size_t command_length = text_length(command);
        if (length + command_length + 1u > sizeof(path)) {
            print("shell: program name too long\n");
            return;
        }
        copy_text(path, prefix, sizeof(path));
        for (size_t i = 0u; i < command_length; ++i) path[length + i] = command[i];
        path[length + command_length] = '\0';
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

## `userspace/phase14_shell_start.S`

```asm
#include <axiom/abi/syscall.h>
.section .text.start,"ax",@progbits
.code64
.global _start
.type _start, @function
.extern shell_main
_start:
    movw $0x1B, %ax
    movw %ax, %ds
    movw %ax, %es
    call shell_main
    movl $0, %edi
    movl $AXIOM_SYS_EXIT, %eax
    syscall
    ud2
.size _start, .-_start

.section .note.GNU-stack,"",@progbits
```

