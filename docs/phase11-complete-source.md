# AxiomOS Phase 11 — complete new/modified source

This handoff contains every file added or modified for Phase 11, including the ELF loader and `SYS_exec` image-replacement path.

## Files

- `Makefile`
- `README.md`
- `boot/limine/limine.conf`
- `arch/x86_64/boot/limine_requests.c`
- `include/axiom/boot/limine.h`
- `include/axiom/abi/syscall.h`
- `include/axiom/abi/user_layout.h`
- `include/axiom/elf/elf64.h`
- `kernel/elf/elf64.c`
- `include/axiom/memory/vmm.h`
- `memory/vmm.c`
- `include/axiom/process/task.h`
- `include/axiom/process/scheduler.h`
- `process/scheduler.c`
- `include/axiom/kernel/syscall.h`
- `kernel/syscall/syscall.c`
- `libc/include/axiom/unistd.h`
- `kernel/core/kernel.c`
- `userspace/phase11_launcher.S`
- `userspace/phase11_program.S`
- `userspace/x86_64_user.ld`
- `tests/phase11_elf.py`
- `docs/phase11.md`
- `docs/architecture.md`
- `docs/project-state.md`
- `docs/validation.md`
- `docs/processes.md`
- `docs/syscalls.md`

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
	@echo "AxiomOS Phase 11 targets:"
	@echo
	@echo "  make          Build kernel and bootable ISO"
	@echo "  make run      Boot AxiomOS using QEMU"
	@echo "  make debug    Boot QEMU paused for GDB"
	@echo "  make test     Run Phase 1 through Phase 11 tests"
	@echo "  make test-phase6 Run heap + corruption tests"
	@echo "  make test-phase7 Run timer + keyboard IRQ/input tests"
	@echo "  make test-phase8 Run preemptive scheduler/context-switch tests"
	@echo "  make test-phase9 Run Ring 3 userspace/isolation tests"
	@echo "  make test-phase10 Run SYSCALL/SYSRET ABI and user-copy tests"
	@echo "  make test-phase11 Run ELF64 loader/executable tests"
	@echo "  make MODE=heap_double_free run  Trigger double-free panic"
	@echo "  make MODE=heap_guard run        Trigger tail-guard panic"
	@echo "  make inspect  Inspect generated ELF64 kernel"
	@echo "  make clean    Remove generated build files"
	@echo "  make distclean Remove build files and Limine"

.PHONY: test-phase3 test-phase4 test-phase5 test-phase6 test-phase7 test-phase8 test-phase9 test-phase10 test-phase11

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

Phases 0–10 provide boot, terminal output, exceptions, PMM/VMM, a kernel heap,
APIC timer/keyboard IRQs, preemptive multitasking, isolated Ring-3 address
spaces, and an x86-64 `SYSCALL/SYSRETQ` ABI.

Phase 11 adds real ELF64 executable loading:

- a separately linked x86-64 `ET_EXEC` userspace binary;
- Limine-module delivery of the ELF file from the boot ISO;
- ELF header/program-header validation;
- `PT_LOAD` mapping with R/W/X-to-page-permission translation;
- BSS zero-fill from `p_memsz > p_filesz`;
- execution beginning at ELF `e_entry`;
- Ring-3 execution through the existing syscall ABI;
- `SYS_exec` image replacement for `/bin/phase11-demo` through a temporary boot-module executable registry;
- tests for entry point, segment permissions, BSS, PID preservation and exit status.

Implemented foundation now includes:

- bootable higher-half x86-64 kernel;
- framebuffer terminal, serial mirror, custom `kprintf()`;
- GDT/TSS, IDT, exceptions and panic diagnostics;
- Local APIC timer and I/O APIC keyboard routing;
- physical/virtual memory managers and dynamic kernel heap;
- preemptive round-robin scheduler and private task stacks;
- Ring-3 tasks with separate CR3 roots and hardware isolation;
- validated x86-64 syscall boundary;
- standalone ELF64 executable loader;
- automated regression tests through Phase 11.

## Common commands

```bash
make
make run
make test
make test-phase9
make test-phase10
make test-phase11
make MODE=page_fault run
make MODE=heap_double_free run
make MODE=heap_guard run
make debug
```

`make test` runs the complete Phase 1 through Phase 11 regression suite.

The next milestone is Phase 12: a virtual filesystem, beginning with an
in-memory filesystem and path/file-descriptor abstractions. Phase 12 can replace
the temporary Phase-11 boot-module executable registry behind `SYS_exec` with
real path resolution while keeping the ELF loader and image-replacement code.

[Architecture](docs/architecture.md) | [Memory](docs/memory.md) |
[Tasks](docs/processes.md) | [Syscalls](docs/syscalls.md) |
[Phase 11](docs/phase11.md) | [Project state](docs/project-state.md) |
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


int limine_get_module(
    const char *module_string,
    const void **address,
    uint64_t *size,
    const char **path
);


#endif
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
#define AXIOM_ENOENT  2
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

#define AXIOM_PHASE11_TEXT_BASE     0x0000000000400000
#define AXIOM_PHASE11_RODATA_BASE   0x0000000000401000
#define AXIOM_PHASE11_DATA_BASE     0x0000000000402000
#define AXIOM_PHASE11_PID_OFFSET    0
#define AXIOM_PHASE11_MAGIC_OFFSET  8
#define AXIOM_PHASE11_BSS_ZERO_OFFSET 16
#define AXIOM_PHASE11_BSS_PROBE_OFFSET 24
#define AXIOM_PHASE11_MAGIC         0x4158494F4D533131
#define AXIOM_PHASE11_EXIT_STATUS   11

#endif
```

## `include/axiom/elf/elf64.h`

```c
#ifndef AXIOM_ELF_ELF64_H
#define AXIOM_ELF_ELF64_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/memory/address.h>

#define ELF64_MAX_LOAD_PAGES 32u

struct elf64_loaded_page {
    vaddr_t virtual_address;
    paddr_t physical_address;
    uint64_t vmm_flags;
};

struct elf64_load_result {
    vaddr_t entry;
    uint16_t program_headers;
    uint16_t load_segments;
    size_t page_count;
    uint64_t file_bytes;
    uint64_t memory_bytes;
    struct elf64_loaded_page pages[ELF64_MAX_LOAD_PAGES];
};

/*
 * Parse an x86-64 little-endian ET_EXEC image already resident in kernel
 * memory and map each PT_LOAD segment into an existing user address space.
 * All mapped pages are recorded in result so task teardown can release them.
 */
int elf64_load_executable(
    paddr_t address_space,
    const void *image,
    size_t image_size,
    struct elf64_load_result *result
);

const char *elf64_last_error(void);

#endif
```

## `kernel/elf/elf64.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/elf/elf64.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>

#define EI_NIDENT 16u
#define EI_CLASS   4u
#define EI_DATA    5u
#define EI_VERSION 6u

#define ELFCLASS64 2u
#define ELFDATA2LSB 1u
#define EV_CURRENT 1u
#define ET_EXEC 2u
#define EM_X86_64 62u
#define PT_LOAD 1u

#define PF_X 0x1u
#define PF_W 0x2u
#define PF_R 0x4u

#define USER_MIN_ADDRESS 0x0000000000010000ULL
#define USER_MAX_ADDRESS 0x00007FFFFFFFFFFFULL

struct elf64_ehdr {
    unsigned char e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed));

struct elf64_phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed));

static const char *last_error = "no error";

static void bytes_clear(void *destination, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    size_t index;

    for (index = 0u; index < count; ++index) {
        out[index] = 0u;
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

static int add_overflow_u64(uint64_t a, uint64_t b, uint64_t *result)
{
    if (UINT64_MAX - a < b) {
        return 1;
    }

    *result = a + b;
    return 0;
}

static uint64_t align_down(uint64_t value)
{
    return value & ~(VMM_PAGE_SIZE - 1ULL);
}

static int align_up(uint64_t value, uint64_t *result)
{
    if (value > UINT64_MAX - (VMM_PAGE_SIZE - 1ULL)) {
        return 0;
    }

    *result = (value + VMM_PAGE_SIZE - 1ULL) & ~(VMM_PAGE_SIZE - 1ULL);
    return 1;
}

static int power_of_two(uint64_t value)
{
    return value != 0ULL && (value & (value - 1ULL)) == 0ULL;
}

static int range_inside_image(uint64_t offset, uint64_t length, size_t image_size)
{
    uint64_t end;

    if (add_overflow_u64(offset, length, &end)) {
        return 0;
    }

    return end <= (uint64_t)image_size;
}

static int page_already_planned(
    const struct elf64_load_result *result,
    vaddr_t virtual_address
)
{
    size_t index;

    for (index = 0u; index < result->page_count; ++index) {
        if (result->pages[index].virtual_address == virtual_address) {
            return 1;
        }
    }

    return 0;
}

static const struct elf64_phdr *program_header_at(
    const uint8_t *image,
    const struct elf64_ehdr *header,
    uint16_t index
)
{
    return (const struct elf64_phdr *)(const void *)(
        image + header->e_phoff + (uint64_t)index * header->e_phentsize
    );
}

static int validate_header(
    const uint8_t *image,
    size_t image_size,
    const struct elf64_ehdr **header_out
)
{
    const struct elf64_ehdr *header;
    uint64_t ph_size;

    if (image == 0 || image_size < sizeof(struct elf64_ehdr)) {
        last_error = "ELF image is smaller than the ELF64 header";
        return 0;
    }

    header = (const struct elf64_ehdr *)(const void *)image;

    if (header->e_ident[0] != 0x7Fu ||
        header->e_ident[1] != 'E' ||
        header->e_ident[2] != 'L' ||
        header->e_ident[3] != 'F') {
        last_error = "ELF magic is invalid";
        return 0;
    }

    if (header->e_ident[EI_CLASS] != ELFCLASS64 ||
        header->e_ident[EI_DATA] != ELFDATA2LSB ||
        header->e_ident[EI_VERSION] != EV_CURRENT) {
        last_error = "ELF is not 64-bit little-endian version 1";
        return 0;
    }

    if (header->e_type != ET_EXEC || header->e_machine != EM_X86_64 ||
        header->e_version != EV_CURRENT) {
        last_error = "ELF is not an x86-64 ET_EXEC image";
        return 0;
    }

    if (header->e_ehsize != sizeof(struct elf64_ehdr) ||
        header->e_phentsize != sizeof(struct elf64_phdr) ||
        header->e_phnum == 0u || header->e_phnum > 64u) {
        last_error = "ELF program-header table shape is unsupported";
        return 0;
    }

    ph_size = (uint64_t)header->e_phentsize * header->e_phnum;
    if (!range_inside_image(header->e_phoff, ph_size, image_size)) {
        last_error = "ELF program-header table is outside the file";
        return 0;
    }

    if (header->e_entry < USER_MIN_ADDRESS ||
        header->e_entry > USER_MAX_ADDRESS) {
        last_error = "ELF entry point is outside the user canonical range";
        return 0;
    }

    *header_out = header;
    return 1;
}

static int validate_load_segments(
    const uint8_t *image,
    size_t image_size,
    const struct elf64_ehdr *header,
    struct elf64_load_result *result
)
{
    uint16_t index;
    int entry_in_executable_segment = 0;

    (void)image;

    result->program_headers = header->e_phnum;
    result->load_segments = 0u;
    result->page_count = 0u;
    result->file_bytes = 0u;
    result->memory_bytes = 0u;
    result->entry = header->e_entry;

    for (index = 0u; index < header->e_phnum; ++index) {
        const struct elf64_phdr *program =
            program_header_at(image, header, index);
        uint64_t segment_end;
        uint64_t page_end;
        uint64_t page;

        if (program->p_type != PT_LOAD) {
            continue;
        }

        if (program->p_memsz == 0ULL) {
            continue;
        }

        if (program->p_filesz > program->p_memsz ||
            !range_inside_image(program->p_offset, program->p_filesz, image_size)) {
            last_error = "ELF PT_LOAD file range is invalid";
            return 0;
        }

        if (add_overflow_u64(program->p_vaddr, program->p_memsz, &segment_end) ||
            program->p_vaddr < USER_MIN_ADDRESS ||
            segment_end == 0ULL || segment_end - 1ULL > USER_MAX_ADDRESS) {
            last_error = "ELF PT_LOAD virtual range is invalid";
            return 0;
        }

        if (program->p_align > 1ULL) {
            if (!power_of_two(program->p_align) ||
                (program->p_vaddr & (program->p_align - 1ULL)) !=
                (program->p_offset & (program->p_align - 1ULL))) {
                last_error = "ELF PT_LOAD alignment is invalid";
                return 0;
            }
        }

        if (!align_up(segment_end, &page_end)) {
            last_error = "ELF PT_LOAD page range overflowed";
            return 0;
        }

        for (page = align_down(program->p_vaddr);
             page < page_end;
             page += VMM_PAGE_SIZE) {
            if (page_already_planned(result, page)) {
                last_error = "ELF PT_LOAD segments overlap the same page";
                return 0;
            }

            if (result->page_count >= ELF64_MAX_LOAD_PAGES) {
                last_error = "ELF needs more load pages than Phase 11 allows";
                return 0;
            }

            result->pages[result->page_count].virtual_address = page;
            result->pages[result->page_count].physical_address = PADDR_INVALID;
            result->pages[result->page_count].vmm_flags = 0ULL;
            ++result->page_count;
        }

        if ((program->p_flags & PF_X) != 0u &&
            header->e_entry >= program->p_vaddr &&
            header->e_entry < segment_end) {
            entry_in_executable_segment = 1;
        }

        ++result->load_segments;
        result->file_bytes += program->p_filesz;
        result->memory_bytes += program->p_memsz;
    }

    if (result->load_segments == 0u) {
        last_error = "ELF has no PT_LOAD segments";
        return 0;
    }

    if (!entry_in_executable_segment) {
        last_error = "ELF entry point is not inside an executable PT_LOAD";
        return 0;
    }

    return 1;
}

static struct elf64_loaded_page *result_page(
    struct elf64_load_result *result,
    vaddr_t virtual_address
)
{
    size_t index;

    for (index = 0u; index < result->page_count; ++index) {
        if (result->pages[index].virtual_address == virtual_address) {
            return &result->pages[index];
        }
    }

    return 0;
}

static void release_allocated_pages(struct elf64_load_result *result)
{
    size_t index;

    for (index = 0u; index < result->page_count; ++index) {
        if (result->pages[index].physical_address != PADDR_INVALID) {
            (void)pmm_free_page(result->pages[index].physical_address);
            result->pages[index].physical_address = PADDR_INVALID;
        }
    }
}

int elf64_load_executable(
    paddr_t address_space,
    const void *image_pointer,
    size_t image_size,
    struct elf64_load_result *result
)
{
    const uint8_t *image = (const uint8_t *)image_pointer;
    const struct elf64_ehdr *header;
    uint16_t index;

    if (result == 0 || address_space == PADDR_INVALID) {
        last_error = "ELF loader received invalid arguments";
        return 0;
    }

    bytes_clear(result, sizeof(*result));
    last_error = "no error";

    if (!validate_header(image, image_size, &header) ||
        !validate_load_segments(image, image_size, header, result)) {
        return 0;
    }

    for (index = 0u; index < header->e_phnum; ++index) {
        const struct elf64_phdr *program =
            program_header_at(image, header, index);
        uint64_t segment_end;
        uint64_t page_end;
        uint64_t page;
        uint64_t mapping_flags;

        if (program->p_type != PT_LOAD || program->p_memsz == 0ULL) {
            continue;
        }

        segment_end = program->p_vaddr + program->p_memsz;
        (void)align_up(segment_end, &page_end);

        mapping_flags = VMM_FLAG_USER;
        if ((program->p_flags & PF_W) != 0u) {
            mapping_flags |= VMM_FLAG_WRITABLE;
        }
        if ((program->p_flags & PF_X) == 0u) {
            mapping_flags |= VMM_FLAG_NO_EXECUTE;
        }

        for (page = align_down(program->p_vaddr);
             page < page_end;
             page += VMM_PAGE_SIZE) {
            struct elf64_loaded_page *loaded = result_page(result, page);
            paddr_t physical;
            uint8_t *destination;
            uint64_t file_start;
            uint64_t file_end;
            uint64_t page_start_in_segment;
            uint64_t page_end_in_segment;

            if (loaded == 0 || loaded->physical_address != PADDR_INVALID) {
                last_error = "ELF loader internal page-plan mismatch";
                release_allocated_pages(result);
                return 0;
            }

            physical = pmm_alloc_page();
            if (physical == PADDR_INVALID) {
                last_error = "ELF loader ran out of physical pages";
                release_allocated_pages(result);
                return 0;
            }

            destination = (uint8_t *)pmm_phys_to_hhdm(physical);
            if (destination == 0) {
                (void)pmm_free_page(physical);
                last_error = "ELF loader could not access a physical page";
                release_allocated_pages(result);
                return 0;
            }

            bytes_clear(destination, VMM_PAGE_SIZE);
            loaded->physical_address = physical;
            loaded->vmm_flags = mapping_flags;

            page_start_in_segment =
                page > program->p_vaddr ? page : program->p_vaddr;
            page_end_in_segment =
                page + VMM_PAGE_SIZE < program->p_vaddr + program->p_filesz ?
                page + VMM_PAGE_SIZE : program->p_vaddr + program->p_filesz;

            if (page_end_in_segment > page_start_in_segment) {
                file_start = program->p_offset +
                    (page_start_in_segment - program->p_vaddr);
                file_end = program->p_offset +
                    (page_end_in_segment - program->p_vaddr);

                bytes_copy(
                    destination + (page_start_in_segment - page),
                    image + file_start,
                    (size_t)(file_end - file_start)
                );
            }

            if (!vmm_map_page_in_address_space(
                    address_space,
                    page,
                    physical,
                    mapping_flags
                )) {
                last_error = "ELF loader could not map a PT_LOAD page";
                release_allocated_pages(result);
                return 0;
            }
        }
    }

    return 1;
}

const char *elf64_last_error(void)
{
    return last_error;
}
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

/* Phase 11: inspect the effective leaf flags for an existing 4 KiB mapping. */
int vmm_mapping_flags_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address,
    uint64_t *flags_out
);

struct vmm_stats vmm_get_stats(void);
int phase5_vmm_selftest(void);

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


int vmm_mapping_flags_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address,
    uint64_t *flags_out
)
{
    uint64_t *pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;
    uint64_t entry;
    uint64_t flags = 0ULL;

    if (!vmm.initialized || root_table == PADDR_INVALID || flags_out == 0 ||
        !is_canonical(virtual_address)) {
        return 0;
    }

    pml4 = owned_table(root_table);
    if (pml4 == 0) {
        return 0;
    }

    entry = pml4[pml4_index(virtual_address)];
    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return 0;
    }
    if ((entry & PAGE_ENTRY_USER) != 0ULL) {
        flags |= VMM_FLAG_USER;
    }
    if ((entry & PAGE_ENTRY_WRITABLE) == 0ULL) {
        /* A read-only parent makes the whole path read-only. */
    }

    pdpt = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pdpt == 0) {
        return 0;
    }

    entry = pdpt[pdpt_index(virtual_address)];
    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL ||
        (entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return 0;
    }
    if ((entry & PAGE_ENTRY_USER) != 0ULL) {
        flags |= VMM_FLAG_USER;
    }

    pd = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pd == 0) {
        return 0;
    }

    entry = pd[pd_index(virtual_address)];
    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL ||
        (entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return 0;
    }
    if ((entry & PAGE_ENTRY_USER) != 0ULL) {
        flags |= VMM_FLAG_USER;
    }

    pt = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pt == 0) {
        return 0;
    }

    entry = pt[pt_index(virtual_address)];
    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return 0;
    }

    flags = entry & (
        VMM_FLAG_WRITABLE |
        VMM_FLAG_USER |
        VMM_FLAG_WRITE_THROUGH |
        VMM_FLAG_CACHE_DISABLE |
        VMM_FLAG_GLOBAL |
        VMM_FLAG_NO_EXECUTE
    );

    *flags_out = flags;
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

const struct task *scheduler_current_task(void);

struct scheduler_stats scheduler_get_stats(void);
size_t scheduler_task_count(void);
const struct task *scheduler_task_at(size_t index);
const struct task *scheduler_task_by_id(uint64_t id);

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
    uint64_t exec_calls;
    uint64_t exec_successes;
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

## `kernel/syscall/syscall.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/syscall.h>
#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/boot/limine.h>
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

static int copy_user_path(
    const struct task *task,
    vaddr_t user_path,
    char *destination,
    size_t capacity
)
{
    size_t index;

    if (task == 0 || destination == 0 || capacity < 2u) {
        return 0;
    }

    for (index = 0u; index + 1u < capacity; ++index) {
        if (!vmm_copy_from_user(
                task->address_space,
                &destination[index],
                user_path + (vaddr_t)index,
                1u
            )) {
            return 0;
        }

        if (destination[index] == '\0') {
            return 1;
        }
    }

    destination[capacity - 1u] = '\0';
    return 0;
}

static int64_t sys_exec(vaddr_t user_path, struct syscall_frame *frame)
{
    const struct task *task = current_user_task();
    char path[64];
    const void *image;
    uint64_t image_size;
    const char *module_path;
    vaddr_t entry;
    vaddr_t stack;

    ++stats.exec_calls;

    if (task == 0 || frame == 0 ||
        !copy_user_path(task, user_path, path, sizeof(path))) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    /*
     * Phase 11 has no VFS yet. This is a deliberately tiny executable
     * registry backed by boot modules. Phase 12 can replace this lookup with
     * real path resolution while keeping the ELF replacement primitive.
     */
    if (!strings_equal(path, "/bin/phase11-demo")) {
        return syscall_error(AXIOM_ENOENT);
    }

    if (!limine_get_module(
            "phase11-demo",
            &image,
            &image_size,
            &module_path
        )) {
        return syscall_error(AXIOM_ENOENT);
    }
    (void)module_path;

    if (image_size > (uint64_t)SIZE_MAX ||
        !scheduler_exec_current_elf(
            image,
            (size_t)image_size,
            &entry,
            &stack
        )) {
        return syscall_error(AXIOM_EINVAL);
    }

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
    stats.exec_calls = 0u;
    stats.exec_successes = 0u;
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

        case AXIOM_SYS_EXEC:
            result = sys_exec((vaddr_t)frame->rdi, frame);
            break;

        case AXIOM_SYS_OPEN:
        case AXIOM_SYS_CLOSE:
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
long exec(const char *path);

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
#include <axiom/elf/elf64.h>
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
     * Phase 11: start from a standalone ELF launcher, then prove SYS_exec by
     * replacing that process with /bin/phase11-demo, another standalone ELF.
     * Until Phase 12 provides a VFS, the exec path resolves through a tiny
     * boot-module executable registry in the syscall layer.
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

    /* Keep the interactive kernel input demonstration alive. */
    phase7_input_loop();
}
```

## `userspace/phase11_launcher.S`

```asm
#include <axiom/abi/syscall.h>

.section .text.start,"ax",@progbits
.code64
.global _start
.type _start, @function
_start:
    movw $0x1B, %ax
    movw %ax, %ds
    movw %ax, %es

    leaq phase11_exec_path(%rip), %rdi
    call exec

    /* Successful exec never returns to this instruction stream. */
    movl $1, %edi
    leaq phase11_exec_failed(%rip), %rsi
    movl $(phase11_exec_failed_end - phase11_exec_failed), %edx
    call write

    movl $99, %edi
    call _exit
    ud2
.size _start, .-_start

.section .text.syswrap,"ax",@progbits
.global exec
exec:
    movl $AXIOM_SYS_EXEC, %eax
    syscall
    ret

.global write
write:
    movl $AXIOM_SYS_WRITE, %eax
    syscall
    ret

.global _exit
_exit:
    movl $AXIOM_SYS_EXIT, %eax
    syscall
    ud2

.section .rodata,"a",@progbits
phase11_exec_path:
    .asciz "/bin/phase11-demo"

phase11_exec_failed:
    .ascii "Phase 11 exec launcher: FAILED\n"
phase11_exec_failed_end:

.section .data,"aw",@progbits
.quad 0

.section .bss,"aw",@nobits
.skip 8

.section .note.GNU-stack,"",@progbits
```

## `userspace/phase11_program.S`

```asm
#include <axiom/abi/syscall.h>
#include <axiom/abi/user_layout.h>

.section .text.start,"ax",@progbits
.code64
.global _start
.type _start, @function
_start:
    movw $0x1B, %ax
    movw %ax, %ds
    movw %ax, %es

    call getpid
    movq %rax, phase11_pid(%rip)

    cmpq $0, phase11_bss_probe(%rip)
    jne .Lbss_bad
    movq $1, phase11_bss_was_zero(%rip)
.Lbss_bad:

    movl $1, %edi
    leaq phase11_message(%rip), %rsi
    movl $(phase11_message_end - phase11_message), %edx
    call write

    call yield
    movl $20, %edi
    call sleep

    movabs $AXIOM_PHASE11_MAGIC, %rax
    movq %rax, phase11_magic(%rip)

    movl $AXIOM_PHASE11_EXIT_STATUS, %edi
    call _exit
    ud2
.size _start, .-_start

.section .text.syswrap,"ax",@progbits
.global write
write:
    movl $AXIOM_SYS_WRITE, %eax
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

.section .rodata,"a",@progbits
.global phase11_message
phase11_message:
    .ascii "Hello from an ELF64 executable in AxiomOS!\n"
phase11_message_end:

.section .data,"aw",@progbits
.p2align 3
.global phase11_pid
phase11_pid:
    .quad 0
.global phase11_magic
phase11_magic:
    .quad 0
.global phase11_bss_was_zero
phase11_bss_was_zero:
    .quad 0

.section .bss,"aw",@nobits
.p2align 3
.global phase11_bss_probe
phase11_bss_probe:
    .skip 8

.section .note.GNU-stack,"",@progbits
```

## `userspace/x86_64_user.ld`

```ld
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

PHDRS
{
    text   PT_LOAD FLAGS(5); /* R-X */
    rodata PT_LOAD FLAGS(4); /* R-- */
    data   PT_LOAD FLAGS(6); /* RW- */
}

SECTIONS
{
    . = 0x0000000000400000;
    .text : ALIGN(0x1000)
    {
        *(.text.start)
        *(.text .text.*)
    } :text

    . = ALIGN(0x1000);
    .rodata :
    {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(0x1000);
    .data :
    {
        *(.data .data.*)
    } :data

    .bss (NOLOAD) :
    {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame*)
        *(.note*)
    }
}
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
            print("PASS: Phase 11 exec() image replacement through boot-module registry")
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

## `docs/phase11.md`

```markdown
# Phase 11 — ELF64 executable loader

## Goal

Replace the one-page raw userspace-image assumption with a real ELF64 loader.
AxiomOS now accepts a standalone x86-64 `ET_EXEC` file, parses its ELF header
and program headers, maps `PT_LOAD` segments into a fresh Ring-3 address space,
and starts the task at the ELF `e_entry` address.

Phase 11 builds two standalone ELF files: `phase11_launcher.elf` and
`phase11_demo.elf`. Both are copied into `/boot` and delivered as Limine
modules. The kernel initially creates the launcher as a Ring-3 ELF task; the
launcher then calls `exec("/bin/phase11-demo")`. Until Phase 12 supplies a VFS,
that path resolves through a deliberately tiny boot-module executable registry.
The scheduler replaces the current user CR3/image/stack with the demo ELF while
preserving the task ID.

## ELF validation

The loader rejects images unless they are:

- ELF magic `0x7F 'E' 'L' 'F'`;
- ELFCLASS64;
- little-endian;
- ELF version 1;
- `ET_EXEC`;
- machine `EM_X86_64`;
- equipped with a bounded, correctly sized program-header table;
- composed of valid lower-half user `PT_LOAD` ranges;
- free of overlapping load pages in this first implementation;
- entered through an address inside an executable `PT_LOAD` segment.

All file offsets and virtual ranges are overflow/bounds checked before pages are
allocated.

## Loading PT_LOAD segments

For every loadable segment AxiomOS:

1. rounds the segment virtual range to 4 KiB pages;
2. allocates fresh PMM pages;
3. clears every page first;
4. copies only the file-backed `p_filesz` bytes;
5. therefore leaves `p_memsz - p_filesz` (BSS) zero-filled;
6. maps the page USER;
7. applies WRITABLE only for `PF_W`;
8. applies NX whenever `PF_X` is absent.

The demo ELF intentionally contains three load segments:

```text
0x400000  text    R-X
0x401000  rodata  R-- + NX
0x402000  data    RW- + NX   (also contains BSS)
```

The ordinary four-page Ring-3 stack remains RW + NX near the top of the lower
canonical half.

## Execution

The ELF program uses the Phase-10 syscall ABI. It calls `getpid()`, verifies its
BSS began as zero, prints:

```text
Hello from an ELF64 executable in AxiomOS!
```

then yields, sleeps, writes a completion magic value into its data segment, and
exits with status 11.

The kernel verifies:

- RIP came from ELF `e_entry` (`0x400000`);
- program-header and `PT_LOAD` counts;
- text/rodata/data mapping permissions;
- BSS zero-fill;
- `getpid()` matches the scheduled task ID;
- exit status is 11.

## `exec("/bin/phase11-demo")`

`SYS_exec` is now functional for the Phase-11 executable registry. The kernel
validates/copies the user path, resolves `/bin/phase11-demo` to the separate
Limine module, builds a fresh address space, loads the ELF, switches CR3, frees
the old user image/stack, resets user RIP/RSP/register state, and returns with
`SYSRETQ` into the new ELF entry point. The PID/task ID is preserved.

This is intentionally **not** a VFS implementation. Phase 12 can replace the
small path-to-module registry with real path lookup and file reads while keeping
the same ELF loading and process-image replacement mechanisms.

## Current limits

- `ET_DYN`/PIE and dynamic linking are not supported.
- ELF relocations/interpreters are not supported.
- `PT_LOAD` segments that share a 4 KiB page are rejected.
- Maximum executable image footprint is 32 mapped pages for this phase.
- ASLR is not implemented.
- Executables currently arrive as boot modules because the VFS is still future
  work.

## Acceptance

```bash
make clean
make
make test-phase11
make test
```

The dedicated test also inspects the standalone ELF with `llvm-readelf` (or
`readelf`) before booting it.
```

## `docs/architecture.md`

```markdown
# Architecture at Phase 11

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
- Phase 10 provides `SYSCALL/SYSRETQ`, validated user copies, write/read/exit/
  sleep/getpid/yield and errno-style failures.

## Phase 11 executable path

```text
userspace/phase11_program.S
        |
        v
Clang + LLD + x86_64_user.ld
        |
        v
phase11_launcher.elf + phase11_demo.elf   (ELF64 ET_EXEC)
        |
        v
ISO: /boot/*.elf
        |
        v
Limine module response
        |
        v
AxiomOS ELF parser
        |
        +-- validate ELF/program headers
        +-- allocate PMM pages
        +-- copy p_filesz
        +-- zero remaining p_memsz (BSS)
        +-- map R/W/X permissions
        |
        v
fresh Ring-3 CR3 + stack
        |
        v
RIP = e_entry
        |
        v
Phase-10 syscall ABI
```

The Phase-11 demo has three page-aligned `PT_LOAD` segments: R-X text, R--
rodata, and RW- data/BSS. Non-executable segments are mapped NX.

`SYS_exec` already replaces the current process image. In Phase 11 its only
registered path is `/bin/phase11-demo`, backed by a Limine module. Phase 12 can
replace that temporary registry with VFS lookup while reusing the same ELF and
address-space replacement machinery.
```

## `docs/project-state.md`

```markdown
# AxiomOS handoff — Phase 11 implementation

Development environment: Linux/Distrobox on Steam Deck. Run commands from the
directory containing `Makefile`. Roadmap: Phase 0 through Phase 25.

Accepted foundation before Phase 11:

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
- Phase 10: x86-64 `SYSCALL/SYSRETQ`, validated user copies, basic syscalls.

Phase 11 adds:

- `kernel/elf/elf64.c` + `include/axiom/elf/elf64.h`;
- a standalone Phase-11 userspace `ET_EXEC` linked by
  `userspace/x86_64_user.ld`;
- `/boot/phase11_launcher.elf` and `/boot/phase11_demo.elf` in the boot ISO;
- Limine module request/lookup for separately delivered executables;
- strict ELF64/x86-64 header and program-header validation;
- bounded `PT_LOAD` parsing and mapping;
- ELF `PF_W`/`PF_X` translation to VMM WRITABLE/NX flags;
- BSS zero-fill by clearing frames before copying `p_filesz`;
- `user_task_create_elf()` using a fresh user CR3 and ordinary user stack;
- working `SYS_exec` for `/bin/phase11-demo` via a temporary boot-module registry;
- in-place process-image replacement with a new CR3/ELF/user stack while preserving PID;
- task metadata for ELF entry/segments/pages/file-vs-memory bytes;
- VMM leaf mapping-flag inspection used by the acceptance test;
- `tests/phase11_elf.py`, which inspects the ELF and then executes it in QEMU.

Important limits:

- `ET_DYN`, PIE, relocations, interpreters and dynamic linking are unsupported;
- overlapping `PT_LOAD` pages are rejected;
- executable images are capped at 32 mapped image pages for now;
- ELF files arrive through Limine because VFS/disk are not built yet;
- Phase-11 `exec()` path lookup is limited to the boot-module executable registry;
  Phase 12 should replace that lookup with the VFS, not replace the ELF loader;
- no normal terminated-task resource reaper yet;
- single CPU only.

Acceptance commands:

```bash
make clean
make
make test-phase11
make test
```

Next milestone: Phase 12 — virtual filesystem and in-memory filesystem.
```

## `docs/validation.md`

```markdown
# Validation status — Phase 11

Static validation performed before packaging:

- every C translation unit compiles with the project's freestanding Clang flags
  and `-Wall -Wextra -Werror`;
- all deliberate fault/corruption build modes compile at the C layer;
- both Phase-11 userspace sources assemble for `x86_64-unknown-none-elf`;
- LLD produces standalone launcher and demo ELF64 `ET_EXEC` files at entry `0x400000`;
- `readelf` shows exactly three `PT_LOAD` segments: R-X, R--, RW-;
- the RW segment has `p_memsz > p_filesz`, exercising BSS zero-fill;
- all Python tests byte-compile and shell tests pass `bash -n`.

Runtime acceptance must be performed in the user's development container:

```bash
make clean
make
make test-phase11
make test
```

`make test-phase11` must prove the standalone ELF is built, delivered as a
Limine module, parsed, mapped with expected permissions, entered through
`e_entry`, able to execute `SYS_exec` from launcher to demo while preserving
PID, able to use Phase-10 syscalls, given zeroed BSS, and terminated with the
expected status without panicking the kernel.
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

## Phase 11 and `exec`

`SYS_exec` now performs real process-image replacement. The Phase-11 launcher
passes `/bin/phase11-demo`; the syscall safely copies that path from userspace,
resolves it through a temporary boot-module executable registry, loads the
standalone ELF into a fresh CR3, replaces the current user image/stack, and
returns to the new ELF entry point while preserving the task ID. Phase 12 should
replace only the temporary registry/path source with the VFS.
```
