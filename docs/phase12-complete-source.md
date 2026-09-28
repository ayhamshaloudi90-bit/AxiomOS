# AxiomOS Phase 12 — complete new/modified source

This handoff contains the complete contents of every file added or modified for Phase 12.

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
	kernel/elf/elf64.c \
	filesystem/vfs.c \
	filesystem/ramfs.c \
	filesystem/bootstrap.c \
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
	$(MAKE) test-phase11
	$(MAKE) test-phase12


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
	@echo "AxiomOS Phase 12 targets:"
	@echo
	@echo "  make          Build kernel and bootable ISO"
	@echo "  make run      Boot AxiomOS using QEMU"
	@echo "  make debug    Boot QEMU paused for GDB"
	@echo "  make test     Run Phase 1 through Phase 12 tests"
	@echo "  make test-phase6 Run heap + corruption tests"
	@echo "  make test-phase7 Run timer + keyboard IRQ/input tests"
	@echo "  make test-phase8 Run preemptive scheduler/context-switch tests"
	@echo "  make test-phase9 Run Ring 3 userspace/isolation tests"
	@echo "  make test-phase10 Run SYSCALL/SYSRET ABI and user-copy tests"
	@echo "  make test-phase11 Run ELF64 loader/executable tests"
	@echo "  make test-phase12 Run VFS/RAM filesystem/file-descriptor tests"
	@echo "  make MODE=heap_double_free run  Trigger double-free panic"
	@echo "  make MODE=heap_guard run        Trigger tail-guard panic"
	@echo "  make inspect  Inspect generated ELF64 kernel"
	@echo "  make clean    Remove generated build files"
	@echo "  make distclean Remove build files and Limine"

.PHONY: test-phase3 test-phase4 test-phase5 test-phase6 test-phase7 test-phase8 test-phase9 test-phase10 test-phase11 test-phase12

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

Phases 0–11 provide boot, terminal output, exceptions, PMM/VMM, a kernel heap,
APIC timer/keyboard IRQs, preemptive multitasking, isolated Ring-3 processes,
an x86-64 `SYSCALL/SYSRETQ` ABI, and standalone ELF64 executable loading.

Phase 12 adds a real virtual filesystem layer:

- normalized absolute paths and longest-prefix mount resolution;
- generic vnode operations suitable for future disk filesystems;
- writable heap-backed RAM filesystems;
- rootfs mounted at `/` and a second ramfs mounted at `/tmp`;
- `/bin`, `/etc`, `/dev`, `/home`, and `/tmp` hierarchy;
- per-process descriptor tables with ordinary files starting at fd 3;
- functional `open`, file `read/write`, `close`, `lseek`, and `stat` syscalls;
- automatic descriptor cleanup on process exit/fault;
- ELF `exec()` resolved through VFS file reads instead of a hardcoded registry;
- automated regression tests through Phase 12.

Current boot files are copied into ramfs from Limine modules because persistent
block storage is deliberately Phase 13. The VFS and ELF layers are already
independent of that source.

## Common commands

```bash
make
make run
make test
make test-phase10
make test-phase11
make test-phase12
make MODE=page_fault run
make MODE=heap_double_free run
make MODE=heap_guard run
make debug
```

`make test` runs the complete Phase 1 through Phase 12 regression suite.

The next milestone is Phase 13: a QEMU block-device driver and real sector I/O,
which can then feed a persistent filesystem backend into the existing VFS.

[Architecture](docs/architecture.md) | [Memory](docs/memory.md) |
[Tasks](docs/processes.md) | [Syscalls](docs/syscalls.md) |
[Filesystem](docs/filesystem.md) | [Phase 12](docs/phase12.md) |
[Project state](docs/project-state.md) | [Original roadmap](docs/roadmap.md)

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

```

## `include/axiom/abi/errno.h`

```c
#ifndef AXIOM_ABI_ERRNO_H
#define AXIOM_ABI_ERRNO_H

/* Small errno set shared by kernel and userspace. Syscalls return -errno. */
#define AXIOM_ENOENT       2
#define AXIOM_EIO          5
#define AXIOM_EBADF        9
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

/* open(2)-style access and creation flags used by the Phase-12 ABI. */
#define AXIOM_O_RDONLY 0x0000
#define AXIOM_O_WRONLY 0x0001
#define AXIOM_O_RDWR   0x0002
#define AXIOM_O_ACCMODE 0x0003
#define AXIOM_O_CREAT  0x0040
#define AXIOM_O_TRUNC  0x0200
#define AXIOM_O_APPEND 0x0400

#define AXIOM_SEEK_SET 0
#define AXIOM_SEEK_CUR 1
#define AXIOM_SEEK_END 2

#define AXIOM_DT_FILE 1
#define AXIOM_DT_DIR  2

#ifndef __ASSEMBLER__
#include <stdint.h>

struct axiom_stat {
    uint64_t size;
    uint32_t type;
    uint32_t mode;
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

/* Stable syscall numbers shared by kernel and userspace. */
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
#define AXIOM_SYS_LSEEK   12
#define AXIOM_SYS_STAT    13

#define AXIOM_SYSCALL_MAX_NUMBER AXIOM_SYS_STAT

#endif

```

## `include/axiom/filesystem/bootstrap.h`

```c
#ifndef AXIOM_FILESYSTEM_BOOTSTRAP_H
#define AXIOM_FILESYSTEM_BOOTSTRAP_H

/* Build the Phase-12 root RAM filesystem and seed boot-module executables. */
int filesystem_phase12_bootstrap(void);

#endif

```

## `include/axiom/filesystem/ramfs.h`

```c
#ifndef AXIOM_FILESYSTEM_RAMFS_H
#define AXIOM_FILESYSTEM_RAMFS_H

#include <axiom/filesystem/vfs.h>

/* Allocate an empty writable in-memory filesystem. */
struct vfs_filesystem *ramfs_create(const char *name);

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
    uint64_t exec_calls;
    uint64_t exec_successes;
    uint64_t lseek_calls;
    uint64_t stat_calls;
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
     * Phase 13 will replace these boot-module copies with files read from a
     * block device. The VFS/ELF layers above this point do not need to change.
     */
    if (!seed_module_file("phase11-launcher", "/bin/phase11-launcher") ||
        !seed_module_file("phase11-demo", "/bin/phase11-demo") ||
        !seed_module_file("phase12-demo", "/bin/phase12-demo")) {
        return 0;
    }

    /* A second RAM filesystem proves that path resolution honors mounts. */
    if (vfs_mount("/tmp", tmpfs) < 0) {
        return 0;
    }

    return 1;
}

```

## `filesystem/ramfs.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>
#include <axiom/filesystem/ramfs.h>
#include <axiom/memory/heap.h>

#define RAMFS_NAME_CAPACITY 32u
#define RAMFS_FILE_MODE 0644u
#define RAMFS_DIRECTORY_MODE 0755u

struct ramfs_node {
    struct vfs_node vfs;
    char name[VFS_NAME_MAX + 1u];
    struct ramfs_node *parent;
    struct ramfs_node *first_child;
    struct ramfs_node *next_sibling;
    uint8_t *data;
    size_t capacity;
};

struct ramfs_instance {
    struct vfs_filesystem filesystem;
    char name[RAMFS_NAME_CAPACITY];
    struct ramfs_node *root;
};

static int ramfs_lookup(
    struct vfs_node *directory,
    const char *name,
    struct vfs_node **node_out
);
static int ramfs_create_node(
    struct vfs_node *directory,
    const char *name,
    enum vfs_node_type type,
    struct vfs_node **node_out
);
static int64_t ramfs_read(
    struct vfs_node *node,
    uint64_t offset,
    void *buffer,
    size_t count
);
static int64_t ramfs_write(
    struct vfs_node *node,
    uint64_t offset,
    const void *buffer,
    size_t count
);
static int ramfs_truncate(struct vfs_node *node, uint64_t size);
static int ramfs_readdir(
    struct vfs_node *directory,
    size_t index,
    struct vfs_dirent *entry_out
);

static const struct vfs_node_ops ramfs_ops = {
    .lookup = ramfs_lookup,
    .create = ramfs_create_node,
    .read = ramfs_read,
    .write = ramfs_write,
    .truncate = ramfs_truncate,
    .readdir = ramfs_readdir,
};

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

static struct ramfs_node *ramfs_from_vfs(struct vfs_node *node)
{
    return node != 0 ? (struct ramfs_node *)node->private_data : 0;
}

static struct ramfs_node *allocate_node(
    const char *name,
    enum vfs_node_type type,
    struct ramfs_node *parent
)
{
    struct ramfs_node *node;

    if (name == 0 || string_length(name) > VFS_NAME_MAX) {
        return 0;
    }

    node = (struct ramfs_node *)kmalloc(sizeof(*node));
    if (node == 0) {
        return 0;
    }

    string_copy(node->name, name, sizeof(node->name));
    node->parent = parent;
    node->first_child = 0;
    node->next_sibling = 0;
    node->data = 0;
    node->capacity = 0u;

    node->vfs.name = node->name;
    node->vfs.type = type;
    node->vfs.mode = type == VFS_NODE_DIRECTORY ?
        RAMFS_DIRECTORY_MODE : RAMFS_FILE_MODE;
    node->vfs.size = 0u;
    node->vfs.ops = &ramfs_ops;
    node->vfs.private_data = node;
    return node;
}

static int ramfs_lookup(
    struct vfs_node *directory,
    const char *name,
    struct vfs_node **node_out
)
{
    struct ramfs_node *parent;
    struct ramfs_node *child;

    if (directory == 0 || name == 0 || node_out == 0) {
        return -AXIOM_EINVAL;
    }
    if (directory->type != VFS_NODE_DIRECTORY) {
        return -AXIOM_ENOTDIR;
    }

    parent = ramfs_from_vfs(directory);
    if (parent == 0) {
        return -AXIOM_EIO;
    }

    for (child = parent->first_child; child != 0; child = child->next_sibling) {
        if (strings_equal(child->name, name)) {
            *node_out = &child->vfs;
            return 0;
        }
    }

    *node_out = 0;
    return -AXIOM_ENOENT;
}

static int ramfs_create_node(
    struct vfs_node *directory,
    const char *name,
    enum vfs_node_type type,
    struct vfs_node **node_out
)
{
    struct ramfs_node *parent;
    struct ramfs_node *node;
    struct vfs_node *existing = 0;

    if (directory == 0 || name == 0 || name[0] == '\0' ||
        string_length(name) > VFS_NAME_MAX ||
        (type != VFS_NODE_FILE && type != VFS_NODE_DIRECTORY)) {
        return -AXIOM_EINVAL;
    }
    if (directory->type != VFS_NODE_DIRECTORY) {
        return -AXIOM_ENOTDIR;
    }

    if (ramfs_lookup(directory, name, &existing) == 0) {
        return -AXIOM_EEXIST;
    }

    parent = ramfs_from_vfs(directory);
    if (parent == 0) {
        return -AXIOM_EIO;
    }

    node = allocate_node(name, type, parent);
    if (node == 0) {
        return -AXIOM_ENOMEM;
    }

    node->next_sibling = parent->first_child;
    parent->first_child = node;

    if (node_out != 0) {
        *node_out = &node->vfs;
    }
    return 0;
}

static int64_t ramfs_read(
    struct vfs_node *node,
    uint64_t offset,
    void *buffer,
    size_t count
)
{
    struct ramfs_node *ram_node;
    uint64_t available;
    size_t transfer;

    if (node == 0 || (count != 0u && buffer == 0)) {
        return -AXIOM_EINVAL;
    }
    if (node->type != VFS_NODE_FILE) {
        return -AXIOM_EISDIR;
    }
    if (offset >= node->size || count == 0u) {
        return 0;
    }

    ram_node = ramfs_from_vfs(node);
    if (ram_node == 0) {
        return -AXIOM_EIO;
    }

    available = node->size - offset;
    transfer = available < (uint64_t)count ? (size_t)available : count;

    if (transfer != 0u) {
        if (ram_node->data == 0) {
            return -AXIOM_EIO;
        }
        bytes_copy(buffer, ram_node->data + (size_t)offset, transfer);
    }

    return (int64_t)transfer;
}

static int ensure_capacity(struct ramfs_node *node, size_t required)
{
    size_t new_capacity;
    uint8_t *new_data;

    if (required <= node->capacity) {
        return 0;
    }

    new_capacity = node->capacity == 0u ? 64u : node->capacity;
    while (new_capacity < required) {
        if (new_capacity > SIZE_MAX / 2u) {
            new_capacity = required;
            break;
        }
        new_capacity *= 2u;
    }

    new_data = (uint8_t *)krealloc(node->data, new_capacity);
    if (new_data == 0) {
        return -AXIOM_ENOMEM;
    }

    bytes_clear(new_data + node->capacity, new_capacity - node->capacity);
    node->data = new_data;
    node->capacity = new_capacity;
    return 0;
}

static int64_t ramfs_write(
    struct vfs_node *node,
    uint64_t offset,
    const void *buffer,
    size_t count
)
{
    struct ramfs_node *ram_node;
    uint64_t end_u64;
    size_t end;
    int result;

    if (node == 0 || (count != 0u && buffer == 0)) {
        return -AXIOM_EINVAL;
    }
    if (node->type != VFS_NODE_FILE) {
        return -AXIOM_EISDIR;
    }
    if (count == 0u) {
        return 0;
    }
    if (offset > (uint64_t)SIZE_MAX || UINT64_MAX - offset < (uint64_t)count) {
        return -AXIOM_ENOSPC;
    }

    end_u64 = offset + (uint64_t)count;
    if (end_u64 > (uint64_t)SIZE_MAX) {
        return -AXIOM_ENOSPC;
    }
    end = (size_t)end_u64;

    ram_node = ramfs_from_vfs(node);
    if (ram_node == 0) {
        return -AXIOM_EIO;
    }

    result = ensure_capacity(ram_node, end);
    if (result < 0) {
        return result;
    }

    if (offset > node->size) {
        bytes_clear(
            ram_node->data + (size_t)node->size,
            (size_t)(offset - node->size)
        );
    }

    bytes_copy(ram_node->data + (size_t)offset, buffer, count);
    if (end_u64 > node->size) {
        node->size = end_u64;
    }
    return (int64_t)count;
}

static int ramfs_truncate(struct vfs_node *node, uint64_t size)
{
    struct ramfs_node *ram_node;
    int result;

    if (node == 0 || node->type != VFS_NODE_FILE || size > (uint64_t)SIZE_MAX) {
        return -AXIOM_EINVAL;
    }

    ram_node = ramfs_from_vfs(node);
    if (ram_node == 0) {
        return -AXIOM_EIO;
    }

    result = ensure_capacity(ram_node, (size_t)size);
    if (result < 0) {
        return result;
    }

    if (size > node->size && ram_node->data != 0) {
        bytes_clear(
            ram_node->data + (size_t)node->size,
            (size_t)(size - node->size)
        );
    }

    node->size = size;
    return 0;
}

static int ramfs_readdir(
    struct vfs_node *directory,
    size_t index,
    struct vfs_dirent *entry_out
)
{
    struct ramfs_node *parent;
    struct ramfs_node *child;
    size_t current = 0u;

    if (directory == 0 || entry_out == 0) {
        return -AXIOM_EINVAL;
    }
    if (directory->type != VFS_NODE_DIRECTORY) {
        return -AXIOM_ENOTDIR;
    }

    parent = ramfs_from_vfs(directory);
    if (parent == 0) {
        return -AXIOM_EIO;
    }

    for (child = parent->first_child; child != 0; child = child->next_sibling) {
        if (current == index) {
            string_copy(entry_out->name, child->name, sizeof(entry_out->name));
            entry_out->type = (uint32_t)child->vfs.type;
            return 1;
        }
        ++current;
    }

    return 0;
}

struct vfs_filesystem *ramfs_create(const char *name)
{
    struct ramfs_instance *instance;
    struct ramfs_node *root;

    instance = (struct ramfs_instance *)kmalloc(sizeof(*instance));
    if (instance == 0) {
        return 0;
    }

    root = allocate_node("/", VFS_NODE_DIRECTORY, 0);
    if (root == 0) {
        kfree(instance);
        return 0;
    }

    string_copy(
        instance->name,
        name != 0 ? name : "ramfs",
        sizeof(instance->name)
    );
    instance->root = root;
    instance->filesystem.name = instance->name;
    instance->filesystem.root = &root->vfs;
    instance->filesystem.private_data = instance;
    return &instance->filesystem;
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
    *file_out = file;
    ++stats.opens;
    return 0;
}

int vfs_close(struct vfs_file *file)
{
    if (file == 0) {
        return -AXIOM_EBADF;
    }

    kfree(file);
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
#include <axiom/elf/elf64.h>
#include <axiom/filesystem/bootstrap.h>
#include <axiom/filesystem/vfs.h>
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
#define PHASE11_TEST_TIMEOUT_TICKS 1000ULL
#define PHASE12_TEST_TIMEOUT_TICKS 1000ULL
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

    interrupts_enable();

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

    if (!initialized || interrupts_enabled() || elf_image == 0 ||
        elf_size == 0u || task_count_value >= SCHEDULER_MAX_TASKS) {
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

    ++task_count_value;
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

```

## `libc/include/axiom/unistd.h`

```c
#ifndef AXIOM_LIBC_UNISTD_H
#define AXIOM_LIBC_UNISTD_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/fs.h>

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

#endif

```

## `userspace/phase12_program.S`

```asm
#include <axiom/abi/fs.h>
#include <axiom/abi/syscall.h>

.section .text.start,"ax",@progbits
.code64
.global _start
.type _start, @function
_start:
    movw $0x1B, %ax
    movw %ax, %ds
    movw %ax, %es

    /* Read a boot-created file through open/read/stat/close. */
    leaq motd_path(%rip), %rdi
    movl $AXIOM_O_RDONLY, %esi
    call open
    testq %rax, %rax
    js phase12_fail
    movl %eax, %r12d

    leaq motd_path(%rip), %rdi
    leaq phase12_stat_buffer(%rip), %rsi
    call stat
    testq %rax, %rax
    js phase12_fail
    cmpq $0, phase12_stat_buffer(%rip)
    je phase12_fail

    movl %r12d, %edi
    leaq phase12_read_buffer(%rip), %rsi
    movl $96, %edx
    call read
    testq %rax, %rax
    jle phase12_fail
    movq %rax, %r14

    movl %r12d, %edi
    call close
    testq %rax, %rax
    js phase12_fail

    movl $1, %edi
    leaq motd_prefix(%rip), %rsi
    movl $(motd_prefix_end - motd_prefix), %edx
    call write

    movl $1, %edi
    leaq phase12_read_buffer(%rip), %rsi
    movq %r14, %rdx
    call write

    /* Create a file on the separately mounted /tmp RAM filesystem. */
    leaq temp_path(%rip), %rdi
    movl $(AXIOM_O_CREAT | AXIOM_O_RDWR | AXIOM_O_TRUNC), %esi
    call open
    testq %rax, %rax
    js phase12_fail
    movl %eax, %r13d

    movl %r13d, %edi
    leaq roundtrip_message(%rip), %rsi
    movl $(roundtrip_message_end - roundtrip_message), %edx
    call write
    cmpq $(roundtrip_message_end - roundtrip_message), %rax
    jne phase12_fail

    movl %r13d, %edi
    xorq %rsi, %rsi
    movl $AXIOM_SEEK_SET, %edx
    call lseek
    testq %rax, %rax
    jne phase12_fail

    movl %r13d, %edi
    leaq phase12_roundtrip_buffer(%rip), %rsi
    movl $(roundtrip_message_end - roundtrip_message), %edx
    call read
    cmpq $(roundtrip_message_end - roundtrip_message), %rax
    jne phase12_fail
    movq %rax, %r15

    movl %r13d, %edi
    call close
    testq %rax, %rax
    js phase12_fail

    leaq temp_path(%rip), %rdi
    leaq phase12_temp_stat(%rip), %rsi
    call stat
    testq %rax, %rax
    js phase12_fail
    cmpq $(roundtrip_message_end - roundtrip_message), phase12_temp_stat(%rip)
    jne phase12_fail

    movl $1, %edi
    leaq roundtrip_prefix(%rip), %rsi
    movl $(roundtrip_prefix_end - roundtrip_prefix), %edx
    call write

    movl $1, %edi
    leaq phase12_roundtrip_buffer(%rip), %rsi
    movq %r15, %rdx
    call write

    /* Prove exec() now obtains the target through the VFS path. */
    leaq exec_path(%rip), %rdi
    call exec

    /* A successful exec replaces this image and never returns here. */
phase12_fail:
    movl $2, %edi
    leaq failure_message(%rip), %rsi
    movl $(failure_message_end - failure_message), %edx
    call write

    movl $92, %edi
    call _exit
    ud2
.size _start, .-_start

.section .text.syswrap,"ax",@progbits
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

.global lseek
lseek:
    movl $AXIOM_SYS_LSEEK, %eax
    syscall
    ret

.global stat
stat:
    movl $AXIOM_SYS_STAT, %eax
    syscall
    ret

.global exec
exec:
    movl $AXIOM_SYS_EXEC, %eax
    syscall
    ret

.global _exit
_exit:
    movl $AXIOM_SYS_EXIT, %eax
    syscall
    ud2

.section .rodata,"a",@progbits
motd_path:
    .asciz "/etc/motd"
temp_path:
    .asciz "/tmp/phase12.txt"
exec_path:
    .asciz "/bin/phase11-demo"

motd_prefix:
    .ascii "Phase 12 VFS motd: "
motd_prefix_end:

roundtrip_prefix:
    .ascii "Phase 12 VFS readback: "
roundtrip_prefix_end:

roundtrip_message:
    .ascii "RAM filesystem round-trip works!\n"
roundtrip_message_end:

failure_message:
    .ascii "Phase 12 userspace VFS demo: FAILED\n"
failure_message_end:

.section .data,"aw",@progbits
.quad 0

.section .bss,"aw",@nobits
.p2align 3
phase12_stat_buffer:
    .skip 16
phase12_temp_stat:
    .skip 16
phase12_read_buffer:
    .skip 128
phase12_roundtrip_buffer:
    .skip 128

.section .note.GNU-stack,"",@progbits

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
                "Phase 7 input ready. Type into AxiomOS.",
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
                "Phase 7 input ready. Type into AxiomOS.",
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

## `docs/architecture.md`

```markdown
# Architecture at Phase 12

- Target: x86-64, one bootstrap CPU, freestanding C plus x86-64 assembly.
- Limine v12.9.0 boots a higher-half ELF64 kernel.
- Serial output mirrors the framebuffer terminal/custom `kprintf()`.
- GDT contains Ring-0 code/data, Ring-3 data/code, and a 64-bit TSS.
- The TSS `RSP0` and syscall kernel-stack pointer follow the scheduled task.
- Local APIC supplies the 100 Hz timer; I/O APIC routes PS/2 keyboard IRQ1.
- PMM manages usable RAM as 4 KiB frames; VMM owns the kernel PML4 and creates
  separate user PML4 roots sharing supervisor-only upper-half kernel mappings.
- Kernel heap provides dynamic allocation.
- Scheduler is five-tick preemptive round robin with RUNNING/READY/BLOCKED/
  SLEEPING/TERMINATED states.
- Ring-3 tasks use private user memory, a four-page user stack, and a trusted
  kernel stack for interrupts/syscalls.
- The syscall boundary validates/copies every userspace pointer.
- The ELF64 loader maps ET_EXEC PT_LOAD segments with user R/W/X permissions.
- Phase 12 adds a VFS, RAM filesystem, mounts, paths, metadata and per-task file
  descriptors.

## Filesystem stack

```text
Ring-3 ELF program
        |
        v
SYSCALL / SYSRETQ
        |
        v
validated user-copy layer
        |
        v
per-task descriptor table
        |
        v
+------------------------------+
|             VFS              |
| paths | mounts | vnode ops   |
+------------------------------+
        |                 |
        v                 v
 rootfs ramfs          tmpfs ramfs
 mounted at /          mounted at /tmp
```

The VFS is backend-neutral. A future disk filesystem implements the same vnode
operations and mounts alongside or instead of ramfs.

## ELF execution path at Phase 12

```text
exec("/bin/program")
        |
        v
copy/validate userspace path
        |
        v
VFS path lookup + read
        |
        v
kernel heap ELF buffer
        |
        v
ELF parser / PT_LOAD mapper
        |
        v
fresh CR3 + user stack
        |
        v
RIP = e_entry
```

The current `/bin` executable bytes are seeded from Limine modules only because
Phase 13 has not provided persistent block storage yet. The syscall and ELF
layers no longer know about that boot source.

```

## `docs/filesystem.md`

```markdown
# AxiomOS filesystem architecture

Phase 12 introduces the virtual filesystem layer.

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

## RAM filesystem

`filesystem/ramfs.c` is the first backend. It stores directory nodes, file
metadata, and dynamically grown file data in the Phase-6 kernel heap.

At boot, `filesystem/bootstrap.c` creates:

```text
rootfs -> /
tmpfs  -> /tmp
```

Rootfs is populated with `/bin`, `/etc`, `/dev`, `/home`, and `/tmp`. Boot ELF
modules are copied into `/bin` so that the existing ELF loader and `exec()` can
use normal VFS reads.

## File descriptors

Every task now owns 16 descriptor slots. Slots 0, 1 and 2 retain stdin/stdout/
stderr semantics; VFS opens allocate from descriptor 3 upward.

Descriptors hold a `struct vfs_file`, which records the vnode, current offset,
and open flags. Task exit/fault cleanup closes all remaining regular file
descriptors.

## Next step

Phase 13 adds a QEMU block-device driver. A persistent disk filesystem can then
implement the same VFS node operations and be mounted without changing the
userspace syscall ABI or ELF loader.

```

## `docs/project-state.md`

```markdown
# AxiomOS handoff — Phase 12 implementation

Development environment: Linux/Distrobox on Steam Deck. Run commands from the
directory containing `Makefile`. Roadmap: Phase 0 through Phase 25.

Accepted foundation before Phase 12:

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
- Phase 10: x86-64 `SYSCALL/SYSRETQ`, validated user copies and basic syscalls.
- Phase 11: standalone ELF64 `ET_EXEC` loading and process-image replacement.

Phase 12 adds:

- `filesystem/vfs.c`: VFS path, mount, node and open-file abstraction;
- `filesystem/ramfs.c`: writable heap-backed in-memory filesystem;
- `filesystem/bootstrap.c`: rootfs + separate `/tmp` mount and boot file seeding;
- normalized absolute paths with `.`, `..`, repeated-slash handling;
- longest-prefix mount resolution;
- generic vnode operations: lookup/create/read/write/truncate/readdir;
- VFS operations: open/read/write/close/seek/stat/mkdir/readdir;
- root hierarchy `/bin`, `/etc`, `/dev`, `/home`, `/tmp`;
- a second ramfs mounted at `/tmp` to prove mount-point resolution;
- per-task 16-entry file descriptor tables; regular descriptors start at 3;
- automatic descriptor cleanup when a task exits or dies from a userspace fault;
- real file-backed `SYS_open`, `SYS_close`, `SYS_read`, `SYS_write`,
  `SYS_lseek`, and `SYS_stat`;
- `SYS_exec` now reads executable bytes from VFS instead of a hardcoded module
  registry;
- `/bin/phase11-demo`, `/bin/phase11-launcher`, and `/bin/phase12-demo` seeded
  into rootfs from boot modules until persistent storage exists;
- a Phase-12 ELF demo that reads `/etc/motd`, round-trips a file through `/tmp`,
  seeks/stats/closes it, then execs the Phase-11 ELF through VFS;
- `tests/phase12_vfs.py` plus complete Phase 1–12 regression integration.

Important limits:

- ramfs is volatile and disappears on reboot;
- no disk-backed filesystem yet because Phase 13 is the block-device phase;
- no current-working-directory/relative-path userspace API yet;
- no unlink/rename/mkdir userspace syscalls yet;
- permissions/mode bits are metadata only for now;
- descriptor table is fixed at 16 entries;
- no file locking or multiprocess VFS synchronization yet;
- no normal terminated-task resource reaper yet;
- single CPU only.

Acceptance commands:

```bash
make clean
make
make test-phase12
make test
```

Next milestone: Phase 13 — QEMU block-device driver and persistent sector I/O.

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

```

## `docs/phase12.md`

```markdown
# Phase 12 — Virtual filesystem and RAM filesystem

## Goal

Introduce a real filesystem abstraction without depending on a disk driver yet.
AxiomOS now has a VFS layer, path lookup, mount points, per-process file
descriptors, file metadata, and a writable in-memory filesystem.

The actual block-device-backed filesystem is intentionally deferred until Phase
13, because a filesystem cannot read persistent disk sectors before a block
driver exists. Phase 13 can attach a FAT-like or other on-disk filesystem to the
same VFS API instead of replacing the VFS.

## Architecture

```text
Ring-3 program
    |
    | open/read/write/close/lseek/stat/exec
    v
SYSCALL layer
    |
    v
per-process descriptor table
    |
    v
VFS
    |
    +-- path normalization
    +-- longest-prefix mount selection
    +-- vnode operations
    +-- file offsets/open flags
    |
    +-----------------------+
    |                       |
    v                       v
rootfs ramfs mounted /      tmpfs ramfs mounted /tmp
```

The VFS exposes generic nodes and node operations so future filesystems can
implement lookup/create/read/write/truncate/readdir without changing the
syscall layer.

## Paths and mount points

Phase 12 creates this initial hierarchy:

```text
/
├── bin/
│   ├── phase11-launcher
│   ├── phase11-demo
│   └── phase12-demo
├── etc/
│   └── motd
├── dev/
├── home/
│   └── readme.txt
└── tmp/        <- second ramfs mounted here
```

The executable files are initially seeded from Limine modules into rootfs.
That is a temporary boot-time source only. Once a disk driver/filesystem exists,
`/bin/*` can come from persistent storage while `SYS_exec` keeps using ordinary
VFS reads.

Paths are absolute and normalized. Repeated slashes, `.` and `..` are handled,
and mount resolution chooses the longest matching mount prefix.

## Per-process file descriptors

Each task owns a fixed 16-entry descriptor table.

- `0` is stdin;
- `1` is stdout;
- `2` is stderr;
- ordinary VFS files begin at descriptor `3`.

Descriptors keep an independent current file offset and open flags. They are
closed automatically when a task exits or is terminated by a userspace fault.
They survive `exec()` for now because close-on-exec flags are not implemented.

## System calls implemented in this phase

The previously reserved calls are now wired to the VFS:

- `open(path, flags)`;
- `read(fd, buffer, count)` for regular files as well as existing stdin;
- `write(fd, buffer, count)` for regular files as well as stdout/stderr;
- `close(fd)`;
- `lseek(fd, offset, whence)`;
- `stat(path, struct axiom_stat *)`.

`fork()` and `mmap()` remain `-ENOSYS` because their owning phases are still in
the future.

All userspace path/buffer/stat pointers are copied through the existing VMM
user-copy validation instead of being dereferenced directly in Ring 0.

## `exec()` now uses the VFS

Phase 11 used a small syscall-local registry that translated
`/bin/phase11-demo` directly to a Limine module. Phase 12 removes that special
lookup.

`SYS_exec` now does:

```text
copy path from Ring 3
        |
        v
vfs_read_all(path)
        |
        v
kernel heap buffer containing ELF file
        |
        v
scheduler_exec_current_elf()
        |
        v
fresh CR3 + ELF segments + user stack
```

The ELF loader itself did not need to be redesigned.

## Phase-12 userspace test

`phase12_demo.elf` is loaded from `/bin/phase12-demo` through the VFS. It:

1. opens `/etc/motd` read-only;
2. calls `stat()` on it;
3. reads and prints its contents;
4. creates `/tmp/phase12.txt` on the second mounted ramfs;
5. writes `RAM filesystem round-trip works!`;
6. seeks back to offset zero;
7. reads the bytes back and prints them;
8. stats and closes the file;
9. calls `exec("/bin/phase11-demo")`;
10. the Phase-11 ELF runs under the same PID and exits normally.

The kernel additionally reopens `/tmp/phase12.txt` after the process finishes
and byte-compares its contents.

## Current limitations

- RAM filesystem contents disappear on reboot.
- No disk-backed filesystem yet; that depends on Phase 13.
- No unlink/rename/mkdir syscalls yet.
- No permissions/users/groups enforcement yet; mode bits are metadata only.
- No file locking or VFS synchronization yet; AxiomOS is still single-CPU and
  synchronization is a later phase.
- Descriptor table size is fixed at 16 per task.
- Relative paths/current working directories are not exposed to userspace yet.

## Acceptance

```bash
make clean
make
make test-phase12
make test
```

The dedicated test verifies the standalone Phase-12 ELF, mount count, path
lookup, descriptor allocation, RAM-file write/read/seek/stat, and VFS-backed
`exec()` before the old interactive keyboard prompt is reached.

```
