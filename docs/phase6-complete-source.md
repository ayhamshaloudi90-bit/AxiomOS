# AxiomOS Phase 6 — complete new and modified source
This document contains the complete contents of every file added or modified for Phase 6.

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
	arch/x86_64/interrupts/pic.c


ASM_SOURCES := \
	arch/x86_64/boot/entry.asm \
	arch/x86_64/cpu/gdt_load.asm \
	arch/x86_64/cpu/register_probe.asm \
	arch/x86_64/interrupts/isr_stubs.asm


C_OBJECTS := \
	$(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SOURCES))


ASM_OBJECTS := \
	$(patsubst %.asm,$(OBJ_DIR)/%.o,$(ASM_SOURCES))


OBJECTS := \
	$(ASM_OBJECTS) \
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
	@echo "AxiomOS Phase 6 targets:"
	@echo
	@echo "  make          Build kernel and bootable ISO"
	@echo "  make run      Boot AxiomOS using QEMU"
	@echo "  make debug    Boot QEMU paused for GDB"
	@echo "  make test     Run Phase 1 through Phase 6 tests"
	@echo "  make test-phase6 Run heap + corruption tests"
	@echo "  make MODE=heap_double_free run  Trigger double-free panic"
	@echo "  make MODE=heap_guard run        Trigger tail-guard panic"
	@echo "  make inspect  Inspect generated ELF64 kernel"
	@echo "  make clean    Remove generated build files"
	@echo "  make distclean Remove build files and Limine"

.PHONY: test-phase3 test-phase4 test-phase5 test-phase6

test-phase3:
	python3 tests/phase3_cpu.py

test-phase4:
	python3 tests/phase4_pmm.py

test-phase5:
	python3 tests/phase5_vmm.py

test-phase6:
	python3 tests/phase6_heap.py

# Track C header changes too; each fault mode has its own object directory.
-include $(C_OBJECTS:.o=.d)
```

## `README.md`

```markdown
# AxiomOS

AxiomOS is a freestanding x86-64 hobby operating-system kernel written in C and
NASM, built with Clang/LLD and booted by Limine v12.9.0 under QEMU. It is a
higher-half ELF64 kernel and does not use the host libc.

## Current milestone

Phases 0–4 have been locally accepted. Phase 5 adds AxiomOS-owned page tables,
4 KiB mapping primitives, address translation, TLB invalidation, and decoded
page-fault diagnostics. Phase 6 builds a dynamic kernel heap on top of the PMM
and VMM. Phase 5 and Phase 6 are accepted only after their local QEMU regression
tests pass.

Implemented foundation:

- bootable x86-64 ELF64 kernel;
- serial output and framebuffer terminal with custom `kprintf()`;
- GDT/TSS, 256-entry IDT, exception stubs and panic diagnostics;
- masked/remapped PIC and Phase-3 CPU exception tests;
- 4 KiB bitmap physical memory manager;
- AxiomOS-owned PML4/PDPT/PD/PT hierarchy;
- `map_page()`, `unmap_page()`, `virt_to_phys()`, and page-fault diagnostics;
- higher-half kernel heap at `0xFFFFC00000000000`;
- first-fit allocation with splitting and immediate coalescing;
- `kmalloc()`, `kcalloc()`, `krealloc()`, and `kfree()`;
- heap header integrity cookies, tail guards, double-free detection;
- automated normal, double-free, and guard-corruption Phase-6 tests.

## Common commands

```bash
make
make run
make test
make test-phase5
make test-phase6
make MODE=page_fault run
make MODE=heap_double_free run
make MODE=heap_guard run
make debug
```

`make test` runs the complete Phase 1 through Phase 6 regression suite. Fault
builds use separate directories such as `build-divide/`, `build-page_fault/`,
`build-heap_double_free/`, and `build-heap_guard/`.

Hardware IRQs remain masked and IF remains clear. There is no scheduler,
keyboard/timer driver, userspace, filesystem, or networking yet.

[Architecture](docs/architecture.md) | [Memory](docs/memory.md) |
[Phase 5](docs/phase5.md) | [Phase 6](docs/phase6.md) |
[Project state](docs/project-state.md) | [Original roadmap](docs/roadmap.md)
```

## `include/axiom/kernel/panic.h`

```c
#ifndef AXIOM_KERNEL_PANIC_H
#define AXIOM_KERNEL_PANIC_H

_Noreturn void kernel_panic(const char *reason);

#endif
```

## `include/axiom/memory/heap.h`

```c
#ifndef AXIOM_MEMORY_HEAP_H
#define AXIOM_MEMORY_HEAP_H

#include <stddef.h>
#include <stdint.h>

#define HEAP_ALIGNMENT 16ULL
#define HEAP_BASE_ADDRESS 0xFFFFC00000000000ULL
#define HEAP_MAX_SIZE (64ULL * 1024ULL * 1024ULL)

struct heap_stats {
    uint64_t mapped_pages;
    uint64_t mapped_bytes;

    uint64_t active_allocations;
    uint64_t total_allocations;
    uint64_t total_frees;
    uint64_t total_reallocations;
    uint64_t failed_allocations;

    uint64_t bytes_in_use;
    uint64_t peak_bytes_in_use;

    uint64_t block_count;
    uint64_t free_block_count;
    uint64_t free_bytes;
    uint64_t largest_free_block;
};

/* Initialise the higher-half kernel heap. Returns 1 on success. */
int heap_init(void);

/* Freestanding kernel allocation API. */
void *kmalloc(size_t size);
void *kcalloc(size_t count, size_t size);
void *krealloc(void *pointer, size_t new_size);
void kfree(void *pointer);

/* Diagnostic helpers. */
int heap_validate(void);
struct heap_stats heap_get_stats(void);

/* Phase-6 destructive allocator self-test; leaves no live allocations. */
int phase6_heap_selftest(void);

#endif
```

## `kernel/core/kernel.c`

```c
#include <stdint.h>

#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>

#include <axiom/boot/limine.h>
#include <axiom/drivers/serial.h>
#include <axiom/memory/heap.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>

#include <axiom/terminal/kprintf.h>
#include <axiom/terminal/terminal.h>


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

}
```

## `kernel/core/panic.c`

```c
#include <stdint.h>

#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/drivers/serial.h>
#include <axiom/kernel/panic.h>
#include <axiom/terminal/kprintf.h>
#include <axiom/terminal/terminal.h>


static const char *const exception_names[32] = {
    "Divide by Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved"
};


static volatile uint32_t panicking;


static _Noreturn void halt_cpu(void)
{
    for (;;) {
        __asm__ volatile ("cli; hlt" ::: "memory");
    }
}


static void begin_panic(void)
{
    __asm__ volatile ("cli" ::: "memory");


    if (panicking) {
        serial_write_string("NESTED KERNEL PANIC: CPU halted.\n");
        halt_cpu();
    }


    panicking = 1;

    terminal_clear();

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_RED,
        TERMINAL_COLOR_BLACK
    );

    kprintf("\nKERNEL PANIC\n");
}


static void print_frame(const struct interrupt_frame *frame)
{
#define REG(label, field) \
    kprintf(label ": 0x%llX\n", (unsigned long long)frame->field)

    REG("RIP", rip);
    REG("RSP", rsp);
    REG("RFLAGS", rflags);

    kprintf(
        "CS: 0x%llX  SS: 0x%llX\n",
        (unsigned long long)frame->cs,
        (unsigned long long)frame->ss
    );

    REG("RAX", rax);
    REG("RBX", rbx);
    REG("RCX", rcx);
    REG("RDX", rdx);
    REG("RSI", rsi);
    REG("RDI", rdi);
    REG("RBP", rbp);
    REG("R8", r8);
    REG("R9", r9);
    REG("R10", r10);
    REG("R11", r11);
    REG("R12", r12);
    REG("R13", r13);
    REG("R14", r14);
    REG("R15", r15);

#undef REG
}


_Noreturn void kernel_panic(const char *reason)
{
    begin_panic();

    kprintf("Reason: %s\n", reason != 0 ? reason : "unspecified kernel panic");
    kprintf("CPU halted.\n");
    halt_cpu();
}


_Noreturn void page_fault_panic(const struct interrupt_frame *frame)
{
    uint64_t cr2;
    const uint64_t error = frame->error_code;


    __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));

    begin_panic();


    kprintf("Exception: Page Fault\n");

    kprintf(
        "Vector: 14  Error: 0x%llX\n",
        (unsigned long long)error
    );

    kprintf(
        "CR2: 0x%llX\n",
        (unsigned long long)cr2
    );

    kprintf(
        "Cause: %s\n",
        (error & (1ULL << 0)) != 0ULL
            ? "Protection violation"
            : "Non-present page"
    );

    kprintf(
        "Access: %s\n",
        (error & (1ULL << 1)) != 0ULL
            ? "Write"
            : "Read"
    );

    kprintf(
        "Privilege: %s\n",
        (error & (1ULL << 2)) != 0ULL
            ? "User"
            : "Supervisor"
    );

    kprintf(
        "Reserved-bit violation: %s\n",
        (error & (1ULL << 3)) != 0ULL ? "yes" : "no"
    );

    kprintf(
        "Instruction fetch: %s\n",
        (error & (1ULL << 4)) != 0ULL ? "yes" : "no"
    );

    kprintf(
        "Protection key: %s\n",
        (error & (1ULL << 5)) != 0ULL ? "yes" : "no"
    );

    kprintf(
        "Shadow stack: %s\n",
        (error & (1ULL << 6)) != 0ULL ? "yes" : "no"
    );

    kprintf(
        "SGX: %s\n",
        (error & (1ULL << 15)) != 0ULL ? "yes" : "no"
    );


    print_frame(frame);

    kprintf("CPU halted.\n");
    halt_cpu();
}


_Noreturn void exception_panic(const struct interrupt_frame *frame)
{
    begin_panic();


    kprintf(
        "Exception: %s\n",
        frame->vector < 32
            ? exception_names[frame->vector]
            : "Unexpected Interrupt"
    );

    kprintf(
        "Vector: %llu  Error: 0x%llX\n",
        (unsigned long long)frame->vector,
        (unsigned long long)frame->error_code
    );


    print_frame(frame);


    if (frame->vector == 8) {
        kprintf(
            "Double-fault IST stack: %s\n",
            gdt_ist_contains(1, (uintptr_t)frame)
                ? "OK"
                : "FAILED"
        );
    }


    kprintf("CPU halted.\n");
    halt_cpu();
}
```

## `memory/heap.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/kernel/panic.h>
#include <axiom/memory/heap.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>

#define HEAP_INITIAL_PAGES 4ULL
#define HEAP_MAX_PAGES (HEAP_MAX_SIZE / VMM_PAGE_SIZE)
#define HEAP_GUARD_BYTES 16ULL
#define HEAP_MIN_FREE_CAPACITY 32ULL

#define HEAP_BLOCK_FREE (1ULL << 0)

#define HEAP_HEADER_MAGIC 0xA610B10C4B484541ULL
#define HEAP_COOKIE_MAGIC 0xC001C0DE5EEDB10CULL
#define HEAP_GUARD_MAGIC  0xA6106A7DDEADC0DEULL


struct heap_block {
    uint64_t magic;
    uint64_t capacity;
    uint64_t requested_size;

    struct heap_block *prev;
    struct heap_block *next;

    uint64_t flags;
    uint64_t cookie;
    uint64_t cookie_inverse;
};


_Static_assert(
    sizeof(struct heap_block) == 64,
    "heap block header must remain 64 bytes"
);

_Static_assert(
    (sizeof(struct heap_block) % HEAP_ALIGNMENT) == 0,
    "heap block header alignment"
);


static struct {
    struct heap_block *first;

    uintptr_t mapped_end;
    uint64_t mapped_pages;

    uint64_t active_allocations;
    uint64_t total_allocations;
    uint64_t total_frees;
    uint64_t total_reallocations;
    uint64_t failed_allocations;

    uint64_t bytes_in_use;
    uint64_t peak_bytes_in_use;

    int initialized;
} heap;


static uintptr_t block_address(const struct heap_block *block)
{
    return (uintptr_t)block;
}


static uint8_t *block_payload(struct heap_block *block)
{
    return (uint8_t *)(void *)(block + 1);
}


static const uint8_t *block_payload_const(const struct heap_block *block)
{
    return (const uint8_t *)(const void *)(block + 1);
}


static uintptr_t block_end_address(const struct heap_block *block)
{
    return block_address(block) + sizeof(*block) + block->capacity;
}


static int block_is_free(const struct heap_block *block)
{
    return (block->flags & HEAP_BLOCK_FREE) != 0ULL;
}


static uint64_t block_magic_value(const struct heap_block *block)
{
    return HEAP_HEADER_MAGIC ^ (uint64_t)block_address(block);
}


static uint64_t block_cookie_value(const struct heap_block *block)
{
    return HEAP_COOKIE_MAGIC ^
           (uint64_t)block_address(block) ^
           block->capacity ^
           block->requested_size ^
           (uint64_t)(uintptr_t)block->prev ^
           (uint64_t)(uintptr_t)block->next ^
           block->flags;
}


static void refresh_block_integrity(struct heap_block *block)
{
    const uint64_t cookie = block_cookie_value(block);

    block->magic = block_magic_value(block);
    block->cookie = cookie;
    block->cookie_inverse = ~cookie;
}


static int block_header_valid(const struct heap_block *block)
{
    uint64_t expected_cookie;


    if (block == 0 || block->magic != block_magic_value(block)) {
        return 0;
    }


    expected_cookie = block_cookie_value(block);

    return block->cookie == expected_cookie &&
           block->cookie_inverse == ~expected_cookie;
}


static void write_u64_unaligned(uint8_t *destination, uint64_t value)
{
    uint64_t index;

    for (index = 0; index < 8ULL; ++index) {
        destination[index] = (uint8_t)(value >> (index * 8ULL));
    }
}


static uint64_t read_u64_unaligned(const uint8_t *source)
{
    uint64_t value;
    uint64_t index;

    value = 0ULL;

    for (index = 0; index < 8ULL; ++index) {
        value |= (uint64_t)source[index] << (index * 8ULL);
    }

    return value;
}


static uint64_t guard_value(const struct heap_block *block)
{
    return HEAP_GUARD_MAGIC ^
           (uint64_t)block_address(block) ^
           block->capacity ^
           block->requested_size;
}


static void write_guard(struct heap_block *block)
{
    uint8_t *guard;
    const uint64_t first = guard_value(block);

    guard = block_payload(block) + block->requested_size;

    write_u64_unaligned(guard, first);
    write_u64_unaligned(guard + 8ULL, ~first);
}


static int guard_valid(const struct heap_block *block)
{
    const uint8_t *guard;
    const uint64_t expected = guard_value(block);


    if (block_is_free(block) ||
        block->requested_size == 0ULL ||
        block->requested_size > block->capacity ||
        HEAP_GUARD_BYTES > block->capacity - block->requested_size) {

        return 0;
    }


    guard = block_payload_const(block) + block->requested_size;

    return read_u64_unaligned(guard) == expected &&
           read_u64_unaligned(guard + 8ULL) == ~expected;
}


static int add_overflow_u64(uint64_t left, uint64_t right, uint64_t *result)
{
    if (UINT64_MAX - left < right) {
        return 1;
    }

    *result = left + right;
    return 0;
}


static int required_capacity(size_t requested_size, uint64_t *capacity)
{
    uint64_t value;
    uint64_t remainder;


    if (requested_size == 0) {
        *capacity = 0ULL;
        return 1;
    }


    if (add_overflow_u64(
            (uint64_t)requested_size,
            HEAP_GUARD_BYTES,
            &value)) {

        return 0;
    }


    remainder = value & (HEAP_ALIGNMENT - 1ULL);

    if (remainder != 0ULL) {
        if (add_overflow_u64(
                value,
                HEAP_ALIGNMENT - remainder,
                &value)) {

            return 0;
        }
    }


    *capacity = value;
    return 1;
}


static void zero_page(paddr_t physical)
{
    uint8_t *page;
    uint64_t index;


    page = (uint8_t *)pmm_phys_to_hhdm(physical);

    if (page == 0) {
        kernel_panic("Heap could not access a newly allocated physical page");
    }


    for (index = 0; index < VMM_PAGE_SIZE; ++index) {
        page[index] = 0u;
    }
}


static void rollback_mapped_pages(uintptr_t start, uint64_t count)
{
    uint64_t index;


    for (index = 0; index < count; ++index) {
        const vaddr_t virtual_address =
            (vaddr_t)(start + index * VMM_PAGE_SIZE);

        const paddr_t physical = virt_to_phys(virtual_address);


        if (physical == PADDR_INVALID) {
            continue;
        }


        if (!unmap_page(virtual_address)) {
            kernel_panic("Heap mapping rollback could not unmap a page");
        }


        if (!pmm_free_page(physical & ~(PMM_PAGE_SIZE - 1ULL))) {
            kernel_panic("Heap mapping rollback could not free a page");
        }
    }
}


static struct heap_block *last_block(void)
{
    struct heap_block *block;


    block = heap.first;

    if (block == 0) {
        return 0;
    }


    while (block->next != 0) {
        block = block->next;
    }


    return block;
}


static int heap_validate_internal(int check_guards)
{
    struct heap_block *block;
    struct heap_block *previous;
    uintptr_t expected_address;
    uint64_t visited;


    if (!heap.initialized || heap.first == 0) {
        return 0;
    }


    block = heap.first;
    previous = 0;
    expected_address = (uintptr_t)HEAP_BASE_ADDRESS;
    visited = 0ULL;


    while (block != 0) {
        uintptr_t end;


        if (++visited > (HEAP_MAX_SIZE / sizeof(struct heap_block))) {
            return 0;
        }


        if (block_address(block) != expected_address ||
            (block_address(block) & (HEAP_ALIGNMENT - 1ULL)) != 0ULL ||
            block_address(block) < (uintptr_t)HEAP_BASE_ADDRESS ||
            block_address(block) + sizeof(*block) > heap.mapped_end ||
            !block_header_valid(block) ||
            block->prev != previous ||
            block->capacity == 0ULL ||
            (block->capacity & (HEAP_ALIGNMENT - 1ULL)) != 0ULL) {

            return 0;
        }


        end = block_end_address(block);

        if (end <= block_address(block) || end > heap.mapped_end) {
            return 0;
        }


        if (block_is_free(block)) {
            if (block->requested_size != 0ULL) {
                return 0;
            }
        } else {
            if (block->requested_size == 0ULL ||
                block->requested_size > block->capacity ||
                HEAP_GUARD_BYTES > block->capacity - block->requested_size) {

                return 0;
            }

            if (check_guards && !guard_valid(block)) {
                return 0;
            }
        }


        if (block->next != 0) {
            if ((uintptr_t)block->next != end) {
                return 0;
            }
        } else if (end != heap.mapped_end) {
            return 0;
        }


        expected_address = end;
        previous = block;
        block = block->next;
    }


    return expected_address == heap.mapped_end;
}


static void assert_heap_structure_valid(void)
{
    if (!heap_validate_internal(0)) {
        kernel_panic("Kernel heap metadata is corrupted");
    }
}


static void assert_heap_valid(void)
{
    if (!heap_validate_internal(1)) {
        kernel_panic("Kernel heap metadata or allocation guard is corrupted");
    }
}


static void initialise_free_block(
    struct heap_block *block,
    uint64_t capacity,
    struct heap_block *previous,
    struct heap_block *next
)
{
    block->capacity = capacity;
    block->requested_size = 0ULL;
    block->prev = previous;
    block->next = next;
    block->flags = HEAP_BLOCK_FREE;

    refresh_block_integrity(block);
}


static int map_heap_pages(uint64_t page_count)
{
    const uintptr_t start = heap.mapped_end;
    uint64_t mapped;


    if (page_count == 0ULL ||
        page_count > HEAP_MAX_PAGES - heap.mapped_pages) {

        return 0;
    }


    mapped = 0ULL;

    while (mapped < page_count) {
        const vaddr_t virtual_address =
            (vaddr_t)(start + mapped * VMM_PAGE_SIZE);

        paddr_t physical;


        if (virt_to_phys(virtual_address) != PADDR_INVALID) {
            rollback_mapped_pages(start, mapped);
            return 0;
        }


        physical = pmm_alloc_page();

        if (physical == PADDR_INVALID) {
            rollback_mapped_pages(start, mapped);
            return 0;
        }


        zero_page(physical);


        if (!map_page(
                virtual_address,
                physical,
                VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE)) {

            (void)pmm_free_page(physical);
            rollback_mapped_pages(start, mapped);
            return 0;
        }


        ++mapped;
    }


    heap.mapped_end += page_count * VMM_PAGE_SIZE;
    heap.mapped_pages += page_count;

    return 1;
}


static int grow_heap(uint64_t required)
{
    struct heap_block *tail;
    uint64_t additional_bytes;
    uint64_t pages;
    uintptr_t old_end;


    tail = last_block();


    if (tail != 0 && block_is_free(tail)) {
        if (tail->capacity >= required) {
            return 1;
        }

        additional_bytes = required - tail->capacity;
    } else {
        if (add_overflow_u64(
                required,
                (uint64_t)sizeof(struct heap_block),
                &additional_bytes)) {

            return 0;
        }
    }


    pages =
        (additional_bytes + VMM_PAGE_SIZE - 1ULL) /
        VMM_PAGE_SIZE;


    if (pages == 0ULL) {
        pages = 1ULL;
    }


    old_end = heap.mapped_end;


    if (!map_heap_pages(pages)) {
        return 0;
    }


    if (tail == 0) {
        heap.first = (struct heap_block *)(uintptr_t)HEAP_BASE_ADDRESS;

        initialise_free_block(
            heap.first,
            pages * VMM_PAGE_SIZE - sizeof(struct heap_block),
            0,
            0
        );

        return 1;
    }


    if (block_is_free(tail)) {
        tail->capacity += pages * VMM_PAGE_SIZE;
        refresh_block_integrity(tail);
        return 1;
    }


    {
        struct heap_block *new_tail =
            (struct heap_block *)(void *)old_end;

        initialise_free_block(
            new_tail,
            pages * VMM_PAGE_SIZE - sizeof(struct heap_block),
            tail,
            0
        );

        tail->next = new_tail;
        refresh_block_integrity(tail);
    }


    return 1;
}


static struct heap_block *find_fit(uint64_t required)
{
    struct heap_block *block;


    for (block = heap.first; block != 0; block = block->next) {
        if (block_is_free(block) && block->capacity >= required) {
            return block;
        }
    }


    return 0;
}


static struct heap_block *split_block(
    struct heap_block *block,
    uint64_t first_capacity
)
{
    const uint64_t original_capacity = block->capacity;
    struct heap_block *old_next;
    struct heap_block *remainder;


    if (original_capacity < first_capacity ||
        original_capacity - first_capacity <
            sizeof(struct heap_block) + HEAP_MIN_FREE_CAPACITY) {

        return 0;
    }


    old_next = block->next;

    remainder = (struct heap_block *)(void *)(
        block_address(block) +
        sizeof(struct heap_block) +
        first_capacity
    );


    initialise_free_block(
        remainder,
        original_capacity - first_capacity - sizeof(struct heap_block),
        block,
        old_next
    );


    block->capacity = first_capacity;
    block->next = remainder;
    refresh_block_integrity(block);


    if (old_next != 0) {
        old_next->prev = remainder;
        refresh_block_integrity(old_next);
    }


    return remainder;
}


static void absorb_next_block(struct heap_block *block)
{
    struct heap_block *next;
    struct heap_block *after;


    next = block->next;

    if (next == 0 || !block_is_free(next)) {
        return;
    }


    after = next->next;

    block->capacity += sizeof(struct heap_block) + next->capacity;
    block->next = after;
    refresh_block_integrity(block);


    if (after != 0) {
        after->prev = block;
        refresh_block_integrity(after);
    }


    next->magic = 0ULL;
    next->cookie = 0ULL;
    next->cookie_inverse = 0ULL;
}


static void mark_allocated(struct heap_block *block, size_t requested_size)
{
    block->requested_size = (uint64_t)requested_size;
    block->flags &= ~HEAP_BLOCK_FREE;
    refresh_block_integrity(block);
    write_guard(block);
}


static void mark_free(struct heap_block *block)
{
    uint64_t index;
    uint8_t *payload;


    payload = block_payload(block);

    for (index = 0; index < block->requested_size; ++index) {
        payload[index] = 0xDDu;
    }


    block->requested_size = 0ULL;
    block->flags |= HEAP_BLOCK_FREE;
    refresh_block_integrity(block);
}


static struct heap_block *find_block_by_payload(const void *pointer)
{
    struct heap_block *block;


    for (block = heap.first; block != 0; block = block->next) {
        if ((const void *)block_payload(block) == pointer) {
            return block;
        }
    }


    return 0;
}


int heap_init(void)
{
    struct vmm_stats paging;


    if (heap.initialized) {
        return 1;
    }


    paging = vmm_get_stats();

    if (paging.root_table == 0ULL || paging.root_table == PADDR_INVALID) {
        return 0;
    }


    if (virt_to_phys((vaddr_t)HEAP_BASE_ADDRESS) != PADDR_INVALID) {
        return 0;
    }


    heap.first = 0;
    heap.mapped_end = (uintptr_t)HEAP_BASE_ADDRESS;
    heap.mapped_pages = 0ULL;

    heap.active_allocations = 0ULL;
    heap.total_allocations = 0ULL;
    heap.total_frees = 0ULL;
    heap.total_reallocations = 0ULL;
    heap.failed_allocations = 0ULL;

    heap.bytes_in_use = 0ULL;
    heap.peak_bytes_in_use = 0ULL;


    if (!map_heap_pages(HEAP_INITIAL_PAGES)) {
        return 0;
    }


    heap.first = (struct heap_block *)(uintptr_t)HEAP_BASE_ADDRESS;

    initialise_free_block(
        heap.first,
        HEAP_INITIAL_PAGES * VMM_PAGE_SIZE - sizeof(struct heap_block),
        0,
        0
    );


    heap.initialized = 1;


    if (!heap_validate_internal(1)) {
        heap.initialized = 0;
        return 0;
    }


    return 1;
}


void *kmalloc(size_t size)
{
    uint64_t required;
    struct heap_block *block;


    if (!heap.initialized || size == 0) {
        return 0;
    }


    assert_heap_valid();


    if (!required_capacity(size, &required) || required > HEAP_MAX_SIZE) {
        ++heap.failed_allocations;
        return 0;
    }


    block = find_fit(required);

    if (block == 0) {
        if (!grow_heap(required)) {
            ++heap.failed_allocations;
            return 0;
        }

        block = find_fit(required);

        if (block == 0) {
            kernel_panic("Heap growth succeeded but no fitting block exists");
        }
    }


    (void)split_block(block, required);
    mark_allocated(block, size);

    ++heap.active_allocations;
    ++heap.total_allocations;
    heap.bytes_in_use += (uint64_t)size;

    if (heap.bytes_in_use > heap.peak_bytes_in_use) {
        heap.peak_bytes_in_use = heap.bytes_in_use;
    }


    assert_heap_valid();

    return block_payload(block);
}


void *kcalloc(size_t count, size_t size)
{
    size_t total;
    uint8_t *memory;
    size_t index;


    if (count == 0 || size == 0) {
        return 0;
    }


    if (count > SIZE_MAX / size) {
        ++heap.failed_allocations;
        return 0;
    }


    total = count * size;
    memory = (uint8_t *)kmalloc(total);

    if (memory == 0) {
        return 0;
    }


    for (index = 0; index < total; ++index) {
        memory[index] = 0u;
    }


    return memory;
}


void kfree(void *pointer)
{
    struct heap_block *block;
    uint64_t released;


    if (pointer == 0) {
        return;
    }


    if (!heap.initialized) {
        kernel_panic("kfree called before heap initialization");
    }


    assert_heap_structure_valid();

    block = find_block_by_payload(pointer);

    if (block == 0) {
        kernel_panic("Heap free received an invalid pointer");
    }


    if (block_is_free(block)) {
        kernel_panic("Heap double free detected");
    }


    if (!guard_valid(block)) {
        kernel_panic("Heap tail guard corrupted");
    }


    released = block->requested_size;
    mark_free(block);


    if (heap.active_allocations == 0ULL || heap.bytes_in_use < released) {
        kernel_panic("Heap allocation accounting underflow");
    }


    --heap.active_allocations;
    ++heap.total_frees;
    heap.bytes_in_use -= released;


    if (block->next != 0 && block_is_free(block->next)) {
        absorb_next_block(block);
    }


    if (block->prev != 0 && block_is_free(block->prev)) {
        block = block->prev;
        absorb_next_block(block);
    }


    assert_heap_valid();
}


void *krealloc(void *pointer, size_t new_size)
{
    struct heap_block *block;
    uint64_t required;
    uint64_t old_requested;


    if (pointer == 0) {
        return kmalloc(new_size);
    }


    if (new_size == 0) {
        kfree(pointer);
        return 0;
    }


    if (!heap.initialized) {
        return 0;
    }


    assert_heap_structure_valid();

    block = find_block_by_payload(pointer);

    if (block == 0 || block_is_free(block)) {
        kernel_panic("krealloc received an invalid pointer");
    }


    if (!guard_valid(block)) {
        kernel_panic("Heap tail guard corrupted before krealloc");
    }


    if (!required_capacity(new_size, &required) || required > HEAP_MAX_SIZE) {
        ++heap.failed_allocations;
        return 0;
    }


    old_requested = block->requested_size;
    ++heap.total_reallocations;


    if (required <= block->capacity) {
        struct heap_block *remainder;


        remainder = split_block(block, required);

        if (remainder != 0 &&
            remainder->next != 0 &&
            block_is_free(remainder->next)) {

            absorb_next_block(remainder);
        }


        block->requested_size = (uint64_t)new_size;
        refresh_block_integrity(block);
        write_guard(block);


        if ((uint64_t)new_size >= old_requested) {
            heap.bytes_in_use += (uint64_t)new_size - old_requested;
        } else {
            heap.bytes_in_use -= old_requested - (uint64_t)new_size;
        }


        if (heap.bytes_in_use > heap.peak_bytes_in_use) {
            heap.peak_bytes_in_use = heap.bytes_in_use;
        }


        assert_heap_valid();
        return pointer;
    }


    if (block->next != 0 && block_is_free(block->next)) {
        const uint64_t combined =
            block->capacity +
            sizeof(struct heap_block) +
            block->next->capacity;


        if (combined >= required) {
            struct heap_block *remainder;


            absorb_next_block(block);
            remainder = split_block(block, required);

            if (remainder != 0 &&
                remainder->next != 0 &&
                block_is_free(remainder->next)) {

                absorb_next_block(remainder);
            }


            block->requested_size = (uint64_t)new_size;
            refresh_block_integrity(block);
            write_guard(block);

            heap.bytes_in_use += (uint64_t)new_size - old_requested;

            if (heap.bytes_in_use > heap.peak_bytes_in_use) {
                heap.peak_bytes_in_use = heap.bytes_in_use;
            }


            assert_heap_valid();
            return pointer;
        }
    }


    {
        uint8_t *replacement;
        const uint8_t *old_bytes;
        size_t index;


        replacement = (uint8_t *)kmalloc(new_size);

        if (replacement == 0) {
            return 0;
        }


        old_bytes = (const uint8_t *)pointer;

        for (index = 0; index < old_requested; ++index) {
            replacement[index] = old_bytes[index];
        }


        kfree(pointer);
        return replacement;
    }
}


int heap_validate(void)
{
    if (!heap.initialized) {
        return 0;
    }

    return heap_validate_internal(1);
}


struct heap_stats heap_get_stats(void)
{
    struct heap_stats stats;
    struct heap_block *block;


    assert_heap_valid();


    stats.mapped_pages = heap.mapped_pages;
    stats.mapped_bytes = heap.mapped_pages * VMM_PAGE_SIZE;

    stats.active_allocations = heap.active_allocations;
    stats.total_allocations = heap.total_allocations;
    stats.total_frees = heap.total_frees;
    stats.total_reallocations = heap.total_reallocations;
    stats.failed_allocations = heap.failed_allocations;

    stats.bytes_in_use = heap.bytes_in_use;
    stats.peak_bytes_in_use = heap.peak_bytes_in_use;

    stats.block_count = 0ULL;
    stats.free_block_count = 0ULL;
    stats.free_bytes = 0ULL;
    stats.largest_free_block = 0ULL;


    for (block = heap.first; block != 0; block = block->next) {
        ++stats.block_count;

        if (block_is_free(block)) {
            ++stats.free_block_count;
            stats.free_bytes += block->capacity;

            if (block->capacity > stats.largest_free_block) {
                stats.largest_free_block = block->capacity;
            }
        }
    }


    return stats;
}
```

## `memory/heap_selftest.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/memory/heap.h>
#include <axiom/terminal/kprintf.h>


static int selftest_fail(const char *reason)
{
    kprintf("Phase 6 heap self-test: FAILED (%s)\n", reason);
    return 0;
}


static int pointer_is_aligned(const void *pointer)
{
    return ((uintptr_t)pointer & (HEAP_ALIGNMENT - 1ULL)) == 0ULL;
}


int phase6_heap_selftest(void)
{
    uint8_t *small;
    uint8_t *medium;
    uint8_t *reuse;
    uint8_t *large;
    uint64_t *zeroed;

    uint8_t *resized;
    uint8_t *grown;
    uint8_t *shrunk;

    uint8_t *first;
    uint8_t *second;
    uint8_t *third;
    uint8_t *merged;

    struct heap_stats midpoint;
    struct heap_stats after;
    size_t index;


    if (!heap_validate()) {
        return selftest_fail("initial heap validation failed");
    }


    /* Basic allocation, alignment, data integrity, and heap growth. */
    small = (uint8_t *)kmalloc(24);
    medium = (uint8_t *)kmalloc(1000);
    large = (uint8_t *)kmalloc(20000);
    zeroed = (uint64_t *)kcalloc(128, sizeof(uint64_t));


    if (small == 0 || medium == 0 || large == 0 || zeroed == 0) {
        return selftest_fail("basic allocation failed");
    }


    if (!pointer_is_aligned(small) ||
        !pointer_is_aligned(medium) ||
        !pointer_is_aligned(large) ||
        !pointer_is_aligned(zeroed)) {

        return selftest_fail("allocation alignment is not 16 bytes");
    }


    if (small == medium || small == large || medium == large) {
        return selftest_fail("distinct allocations overlapped");
    }


    for (index = 0; index < 24; ++index) {
        small[index] = (uint8_t)(0xA0u + (uint8_t)index);
    }


    for (index = 0; index < 1000; ++index) {
        medium[index] = (uint8_t)(index ^ 0x5Au);
    }


    for (index = 0; index < 20000; ++index) {
        large[index] = (uint8_t)(index * 13u + 7u);
    }


    for (index = 0; index < 128; ++index) {
        if (zeroed[index] != 0ULL) {
            return selftest_fail("kcalloc did not zero memory");
        }
    }


    for (index = 0; index < 24; ++index) {
        if (small[index] != (uint8_t)(0xA0u + (uint8_t)index)) {
            return selftest_fail("small allocation data changed");
        }
    }


    for (index = 0; index < 1000; ++index) {
        if (medium[index] != (uint8_t)(index ^ 0x5Au)) {
            return selftest_fail("medium allocation data changed");
        }
    }


    for (index = 0; index < 20000; ++index) {
        if (large[index] != (uint8_t)(index * 13u + 7u)) {
            return selftest_fail("large allocation data changed");
        }
    }


    /* Overflow must fail rather than wrap into a tiny allocation. */
    if (kcalloc(SIZE_MAX, 2) != 0) {
        return selftest_fail("kcalloc overflow was accepted");
    }


    /* First-fit reuse: freeing 1000 bytes should service a 512-byte request. */
    kfree(medium);
    reuse = (uint8_t *)kmalloc(512);

    if (reuse == 0 || reuse != medium) {
        return selftest_fail("first-fit allocator did not reuse a free block");
    }


    for (index = 0; index < 512; ++index) {
        reuse[index] = 0x3Cu;
    }


    /* Reallocation must preserve the old payload while growing and shrinking. */
    resized = (uint8_t *)kmalloc(64);

    if (resized == 0) {
        return selftest_fail("realloc setup allocation failed");
    }


    for (index = 0; index < 64; ++index) {
        resized[index] = (uint8_t)(0xD0u + (uint8_t)index);
    }


    grown = (uint8_t *)krealloc(resized, 4096);

    if (grown == 0) {
        return selftest_fail("krealloc growth failed");
    }


    for (index = 0; index < 64; ++index) {
        if (grown[index] != (uint8_t)(0xD0u + (uint8_t)index)) {
            return selftest_fail("krealloc growth did not preserve data");
        }
    }


    shrunk = (uint8_t *)krealloc(grown, 32);

    if (shrunk == 0) {
        return selftest_fail("krealloc shrink failed");
    }


    for (index = 0; index < 32; ++index) {
        if (shrunk[index] != (uint8_t)(0xD0u + (uint8_t)index)) {
            return selftest_fail("krealloc shrink did not preserve data");
        }
    }


    kfree(small);
    kfree(reuse);
    kfree(large);
    kfree(zeroed);
    kfree(shrunk);


    if (!heap_validate()) {
        return selftest_fail("heap failed after basic free/realloc tests");
    }


    midpoint = heap_get_stats();

    if (midpoint.active_allocations != 0ULL ||
        midpoint.bytes_in_use != 0ULL ||
        midpoint.free_block_count != 1ULL) {

        return selftest_fail("basic allocations did not coalesce back to one block");
    }


    /*
     * Deterministic coalescing test. With a single free block at this point,
     * first and second are adjacent. Freeing both must produce enough room for
     * a 500-byte request at first's original address.
     */
    first = (uint8_t *)kmalloc(256);
    second = (uint8_t *)kmalloc(256);
    third = (uint8_t *)kmalloc(256);

    if (first == 0 || second == 0 || third == 0) {
        return selftest_fail("coalescing setup allocation failed");
    }


    kfree(first);
    kfree(second);

    merged = (uint8_t *)kmalloc(500);

    if (merged == 0 || merged != first) {
        return selftest_fail("adjacent free blocks were not coalesced");
    }


    kfree(merged);
    kfree(third);


    if (!heap_validate()) {
        return selftest_fail("final heap validation failed");
    }


    after = heap_get_stats();


    if (after.active_allocations != 0ULL || after.bytes_in_use != 0ULL) {
        return selftest_fail("live allocations remained after self-test");
    }


    if (after.total_allocations != after.total_frees) {
        return selftest_fail("allocation/free counters are unbalanced");
    }


    if (after.mapped_pages <= 4ULL) {
        return selftest_fail("large allocation did not grow the heap");
    }


    if (after.free_block_count != 1ULL ||
        after.largest_free_block != after.free_bytes) {

        return selftest_fail("free blocks did not fully coalesce");
    }


    kprintf("Phase 6 heap self-test: OK\n");
    return 1;
}
```

## `tests/phase6_heap.py`

```python
#!/usr/bin/env python3
"""Boot AxiomOS and validate the Phase-6 kernel heap and corruption panics."""

from pathlib import Path
import re
import subprocess
import time


ROOT = Path(__file__).resolve().parents[1]


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def run_image(mode, marker):
    directory = ROOT / ("build" if mode == "normal" else f"build-{mode}")
    directory.mkdir(exist_ok=True)

    with (directory / "phase6-build.log").open("w") as output:
        subprocess.run(
            ["make", f"MODE={mode}", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = directory / "phase6-serial.log"
    qemu_log = directory / "phase6-qemu.log"
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
            deadline = time.monotonic() + 30

            while marker not in serial.read_text(errors="replace"):
                require(process.poll() is None,
                        f"{mode}: QEMU exited before {marker!r}")
                require(time.monotonic() < deadline,
                        f"{mode}: boot timed out; see {serial}")
                time.sleep(0.05)
        finally:
            if process.poll() is None:
                process.terminate()

            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()

    return serial.read_text(errors="replace").replace("\r", "")


def parse_u64(text, label):
    match = re.search(rf"^{re.escape(label)}: (\d+)$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


def parse_hex(text, label):
    match = re.search(rf"^{re.escape(label)}: 0x([0-9A-F]+)$", text, re.MULTILINE)
    require(match is not None, f"Missing hexadecimal field: {label}")
    return int(match.group(1), 16)


def test_normal():
    text = run_image("normal", "Phase 6 kernel heap complete.")

    for expected in [
        "AxiomOS kernel booted successfully.",
        "Phase 2 terminal test complete.",
        "Phase 3 CPU initialization complete.",
        "Phase 4 physical memory manager complete.",
        "Phase 5 virtual memory manager complete.",
        "AxiomOS Phase 6 kernel heap online.",
        "Phase 6 heap self-test: OK",
        "Phase 6 kernel heap complete.",
    ]:
        require(expected in text, f"normal: missing output: {expected}")

    require("KERNEL PANIC" not in text, "normal: heap boot panicked")
    require("FAILED (" not in text, "normal: heap self-test reported failure")
    require("Phase 6 heap initialization: FAILED" not in text,
            "normal: heap initialization failed")

    heap_base = parse_hex(text, "Heap base")
    initial_pages = parse_u64(text, "Heap mapped pages")
    mapped_pages = parse_u64(text, "Heap mapped pages after test")
    active = parse_u64(text, "Heap active allocations")
    in_use = parse_u64(text, "Heap bytes in use")
    free_bytes = parse_u64(text, "Heap free bytes")
    largest_free = parse_u64(text, "Heap largest free block")
    allocations = parse_u64(text, "Heap total allocations")
    frees = parse_u64(text, "Heap total frees")
    reallocations = parse_u64(text, "Heap reallocations")
    failed = parse_u64(text, "Heap failed allocations")

    require(heap_base == 0xFFFFC00000000000,
            "normal: unexpected kernel heap base")
    require(initial_pages == 4, "normal: heap did not start with four pages")
    require(mapped_pages > initial_pages,
            "normal: stress allocation did not grow the heap")
    require(active == 0, "normal: self-test left live allocations")
    require(in_use == 0, "normal: self-test left bytes in use")
    require(free_bytes > 0, "normal: heap reports no free memory")
    require(largest_free == free_bytes,
            "normal: final free space did not fully coalesce")
    require(allocations == frees and allocations > 0,
            "normal: successful allocation/free counters are unbalanced")
    require(reallocations >= 2,
            "normal: realloc growth/shrink paths were not both exercised")
    require(failed >= 1,
            "normal: overflow/failure path was not exercised")

    print("PASS: Phase 6 kernel heap")
    print(f"  mapped pages:       {mapped_pages}")
    print(f"  final free bytes:   {free_bytes}")
    print(f"  allocations/frees:  {allocations}/{frees}")
    print(f"  reallocations:      {reallocations}")
    print(f"  failed allocations: {failed}")


def test_double_free():
    text = run_image("heap_double_free", "CPU halted.")

    for expected in [
        "Phase 6 heap self-test: OK",
        "Phase 6 kernel heap complete.",
        "Test: triggering heap double free.",
        "KERNEL PANIC",
        "Reason: Heap double free detected",
        "CPU halted.",
    ]:
        require(expected in text, f"heap_double_free: missing output: {expected}")

    require(text.count("KERNEL PANIC") == 1,
            "heap_double_free: expected exactly one panic")
    require("Phase 6 double-free test: FAILED to panic" not in text,
            "heap_double_free: second free unexpectedly returned")

    print("PASS: Phase 6 double-free protection")


def test_guard_corruption():
    text = run_image("heap_guard", "CPU halted.")

    for expected in [
        "Phase 6 heap self-test: OK",
        "Phase 6 kernel heap complete.",
        "Test: triggering heap tail-guard corruption.",
        "KERNEL PANIC",
        "Reason: Heap tail guard corrupted",
        "CPU halted.",
    ]:
        require(expected in text, f"heap_guard: missing output: {expected}")

    require(text.count("KERNEL PANIC") == 1,
            "heap_guard: expected exactly one panic")
    require("Phase 6 guard-corruption test: FAILED to panic" not in text,
            "heap_guard: corrupted allocation unexpectedly freed")

    print("PASS: Phase 6 tail-guard corruption detection")


if __name__ == "__main__":
    test_normal()
    test_double_free()
    test_guard_corruption()
    print("PASS: all Phase 6 kernel-heap tests.")
```

## `docs/architecture.md`

```markdown
# Architecture at Phase 6

- Target: x86-64, one bootstrap CPU, ring 0, freestanding C/NASM.
- Toolchain: Clang `x86_64-unknown-none-elf`, LLD, NASM, Make, QEMU.
- Limine v12.9.0 loads the ELF64 higher-half kernel at `0xffffffff80000000`.
- Entry disables IRQs, selects the AxiomOS bootstrap stack, and calls C.
- Serial output is mirrored with the framebuffer terminal/custom `kprintf()`.
- GDT contains kernel code/data and a 64-bit TSS; #DF/NMI/#MC have IST stacks.
- IDT contains 256 ring-0 interrupt gates; stubs normalize error-code frames.
- #BP and software INT 0x80 return; fatal exceptions panic and halt.
- Vector 14 has a dedicated decoded page-fault panic path using CR2.
- PIC is remapped and fully masked; IF remains 0.
- PMM manages 4 KiB frames from Limine `USABLE` regions using two bitmaps.
- VMM clones Limine's active 4-level hierarchy into PMM-owned table pages.
- CR3 is switched to the AxiomOS-owned PML4 during `vmm_init()`.
- VMM exposes 4 KiB `map_page()`, `unmap_page()`, and `virt_to_phys()` APIs.
- The kernel heap occupies a reserved higher-half region beginning at
  `0xFFFFC00000000000` and grows by mapping PMM frames through the VMM.
- Heap blocks use a first-fit, address-ordered doubly linked list.
- Free blocks split on allocation and coalesce immediately on free.
- Allocations are 16-byte aligned and protected by header cookies plus 16-byte
  tail guards.
- `kmalloc()`, `kcalloc()`, `krealloc()`, and `kfree()` are available to future
  kernel subsystems.
- Heap pages are currently grow-only: free blocks are reused, but top pages are
  not returned to the PMM yet.
- No scheduler, ring 3, filesystem, disk driver, or networking exists yet.

Phase 7 can now build timer and keyboard drivers without relying on static-only
buffers for every kernel data structure.
```

## `docs/memory.md`

```markdown
# AxiomOS memory management — Phase 6

AxiomOS now has three memory-management layers:

```text
physical RAM
    ↓
Phase 4 PMM — allocates 4 KiB physical frames
    ↓
Phase 5 VMM — maps frames into virtual address space
    ↓
Phase 6 heap — allocates arbitrary-sized kernel objects
```

## Physical memory

The PMM manages only Limine `USABLE` regions with 4 KiB frames. It distinguishes
physical addresses with `paddr_t` and exposes HHDM aliases explicitly.
Bootloader-reclaimable memory is still not reclaimed.

## Virtual memory

AxiomOS owns the active 4-level PML4/PDPT/PD/PT hierarchy. The VMM can map and
unmap 4 KiB pages and translate 4 KiB plus inherited 2 MiB/1 GiB leaves.
Changed mappings invalidate the appropriate TLB entry.

## Kernel heap

The Phase-6 heap begins at:

```text
0xFFFFC00000000000
```

and is limited to 64 MiB in this phase. The heap starts with four mapped pages.
When no free block is large enough, the heap allocates additional PMM frames and
maps them writable + NX at consecutive heap virtual addresses.

The mapped heap is partitioned into contiguous variable-sized blocks. A 64-byte
header precedes each payload. Allocated blocks are aligned to 16 bytes and keep
a 16-byte tail guard immediately after the requested payload.

The allocator uses first-fit search, splitting, and immediate coalescing.
`kcalloc()` checks multiplication overflow and zero-fills. `krealloc()` shrinks
in place, grows into the next free block when possible, otherwise moves and
copies the allocation.

The heap is grow-only for now. `kfree()` returns space to the allocator but not
to the PMM. A later optimization may trim completely unused top pages.

See [Phase 6](phase6.md) for block layout, corruption checks, fragmentation,
and the full test plan.
```

## `docs/phase6.md`

```markdown
# Phase 6 — Kernel heap

## Goal

Build a freestanding dynamic allocator on top of the Phase-4 physical memory
manager and Phase-5 virtual memory manager.

Phase 6 implements:

```c
void *kmalloc(size_t size);
void *kcalloc(size_t count, size_t size);
void *krealloc(void *pointer, size_t new_size);
void kfree(void *pointer);
```

The allocator does not call host Linux, libc, `malloc()`, or a bootloader
allocator.

## Virtual heap region

The kernel heap begins at the canonical higher-half address:

```text
0xFFFFC00000000000
```

The Phase-6 heap is capped at 64 MiB. It starts with four 4 KiB pages and grows
on demand. Growth works by:

1. allocating a physical frame with `pmm_alloc_page()`;
2. zeroing it through its HHDM alias;
3. mapping it at the next heap virtual page with `map_page()`;
4. adding the newly mapped bytes to the free-list allocator.

Heap mappings are writable and NX.

Phase 6 is intentionally grow-only: `kfree()` makes memory reusable inside the
heap, but does not yet trim top pages back to the PMM. That keeps page-return
policy separate from the first allocator implementation.

## Allocator design

AxiomOS uses a doubly linked, address-ordered block list with a first-fit search.
Every mapped byte in the heap belongs to exactly one block.

Conceptually:

```text
+---------+------------------+---------+----------------+---------+
| header  | allocation A     | header  | free space     | ...     |
+---------+------------------+---------+----------------+---------+
```

Each block has a 64-byte header containing:

- integrity magic;
- payload capacity;
- currently requested byte count;
- previous/next block links;
- free/allocated state;
- an integrity cookie and its inverse.

Allocated payloads are 16-byte aligned.

## Splitting

If a free block is substantially larger than a request, it is split:

```text
Before:

+-----------------------------------------------+
|                  free block                   |
+-----------------------------------------------+

After allocation:

+--------------------+--------------------------+
| allocated block    |       free remainder     |
+--------------------+--------------------------+
```

A split is performed only when the remainder is large enough to hold another
64-byte header plus a useful minimum payload.

## Coalescing

When a block is freed, adjacent free blocks are merged immediately:

```text
+---------+---------+---------+
| free A  | free B  | used C  |
+---------+---------+---------+

          becomes

+-------------------+---------+
|      free A+B     | used C  |
+-------------------+---------+
```

This directly reduces external fragmentation and is tested in the Phase-6
self-test.

## `krealloc()`

`krealloc()` uses three paths:

1. **Shrink in place** when the current block is already large enough. A useful
   remainder is split into a new free block.
2. **Grow in place** when the immediately following block is free and combining
   the two creates enough capacity.
3. **Move** when neither in-place option works: allocate a new block, copy the
   old payload byte-for-byte, and free the old block.

`krealloc(NULL, n)` behaves like `kmalloc(n)`. `krealloc(ptr, 0)` frees the
allocation and returns `NULL`.

## `kcalloc()`

`kcalloc()` rejects integer multiplication overflow before allocating. A valid
allocation is zero-filled explicitly by the kernel.

## Corruption checks

The heap performs intentionally expensive validation in Phase 6 because
correctness is more important than allocation speed at this stage.

### Header integrity

Each header contains an address-dependent magic value plus a cookie derived
from its size, links, requested size, and flags. The cookie inverse must also
match.

The allocator validates that blocks:

- are contiguous and 16-byte aligned;
- remain inside the currently mapped heap;
- have consistent previous/next links;
- have sane capacities;
- partition the entire mapped heap without gaps or overlap.

### Tail guards

Each live allocation reserves 16 bytes immediately after the caller-requested
payload and stores a two-word guard there. `kfree()` and `krealloc()` verify the
guard before modifying the block.

A one-byte write past the requested size therefore corrupts the guard and causes
an intentional kernel panic.

### Double free

A second `kfree()` of a block that is still represented as free is detected and
panics rather than corrupting allocator state.

## Fragmentation

Two different forms matter:

### Internal fragmentation

The caller may request 17 bytes, but AxiomOS must align the allocation and
reserve guard space. The block therefore consumes more than exactly 17 bytes.
That unused space inside an allocated block is internal fragmentation.

### External fragmentation

The heap might contain 8 KiB total free space, but split into four separate 2
KiB holes. A 5 KiB request cannot use those holes even though the total free
space is larger than 5 KiB. That is external fragmentation.

Phase 6 fights external fragmentation using immediate coalescing. First-fit is
simple and easy to audit, but may still fragment over long-running workloads.
Later kernels could add segregated free lists or slab allocators for common
object sizes.

## Phase-6 self-test

The normal self-test verifies:

1. 16-byte allocation alignment;
2. distinct blocks do not overlap;
3. write/read integrity for small, medium, and 20 KiB allocations;
4. dynamic heap page growth;
5. `kcalloc()` zero-fill;
6. `kcalloc()` multiplication-overflow rejection;
7. first-fit reuse of a freed block;
8. `krealloc()` growth with data preservation;
9. `krealloc()` shrink with data preservation;
10. adjacent-free-block coalescing;
11. no live allocations remain afterward;
12. allocation/free counters balance;
13. the final heap collapses back into one free block.

Two additional QEMU modes validate corruption protection:

```bash
make MODE=heap_double_free run
make MODE=heap_guard run
```

The first intentionally frees an allocation twice. The second writes one byte
past a 32-byte allocation and then frees it. Both must panic and halt.
```

## `docs/project-state.md`

```markdown
# AxiomOS handoff — Phase 6 implementation

Development environment: Linux/Distrobox on Steam Deck. Run commands from the
directory containing `Makefile`. Do not restart from earlier phases.

Roadmap: Phase 0 through Phase 25, 26 phases including setup.

Locally accepted before the Phase-5/6 packaged updates:

- Phase 0: development environment.
- Phase 1: bootable x86-64 ELF64 kernel and serial output.
- Phase 2: framebuffer terminal and custom formatter.
- Phase 3: GDT/TSS, IDT, exception/interrupt state and PIC baseline.
- Phase 4: tested 4 KiB bitmap physical page allocator.

Phase 5 implementation provides:

- PMM-owned clone of the active x86-64 paging hierarchy;
- CR3 switch to the AxiomOS PML4;
- `map_page()`, `unmap_page()`, and `virt_to_phys()`;
- TLB invalidation and decoded vector-14 page-fault panic;
- `MODE=page_fault` QEMU validation.

Phase 6 adds:

- `include/axiom/memory/heap.h`;
- `memory/heap.c`;
- `memory/heap_selftest.c`;
- higher-half heap base `0xFFFFC00000000000`;
- on-demand PMM/VMM-backed heap growth up to 64 MiB;
- first-fit free-list search, block splitting, and immediate coalescing;
- `kmalloc()`, `kcalloc()`, `krealloc()`, and `kfree()`;
- 16-byte payload alignment;
- header integrity cookies and 16-byte tail guards;
- invalid-pointer/double-free/guard-corruption panic paths;
- `MODE=heap_double_free` and `MODE=heap_guard` deliberate corruption builds;
- `tests/phase6_heap.py` normal + corruption validation.

Important current limitations:

- Single address space and single CPU.
- No page-table or heap locking; interrupts remain disabled.
- Heap is first-fit O(n) and grow-only; it does not trim pages back to the PMM.
- No slab/object caches yet.
- `map_page()` does not split existing huge-page mappings.
- No bootloader-reclaimable memory reclamation yet.
- No userspace, scheduler, filesystem, or networking.

Acceptance commands:

```bash
make clean
make
make test-phase5
make test-phase6
make test
```

Phase 6 is complete only after the real local QEMU Phase-6 normal, double-free,
and tail-guard tests pass and the full regression suite remains green.
```

## `docs/validation.md`

```markdown
# Validation status — Phase 6

## Previously observed local baseline

The user's earlier QEMU regression accepted Phases 1–4. The Phase-4 PMM test
reported 65,094 usable pages, 4 metadata/allocated pages, and 65,090 free pages
with 256 MiB configured for QEMU. All five Phase-3 CPU cases continued to pass.

## Phase-5 implementation

The Phase-5 source added AxiomOS-owned page tables, virtual mapping primitives,
and a deliberate page-fault test. It still requires the user's local QEMU
acceptance if that has not already been run.

## Phase-6 static validation performed before packaging

- Every C translation unit compiles with the project's freestanding Clang
  options and `-Wall -Wextra -Werror`.
- Kernel variants for page fault, divide, invalid opcode, GP, double fault,
  heap double free, and heap guard corruption all compile cleanly.
- Phase 3 through Phase 6 Python tests pass Python syntax/bytecode checks.
- Phase 1/2 shell tests pass `bash -n` syntax validation.
- Heap self-test source exercises allocation, first-fit reuse, growth, calloc,
  realloc, coalescing, overflow rejection, and final accounting.

## Required local acceptance

This packaging environment does not provide QEMU or NASM, so runtime acceptance
must occur in the user's kernel-development container:

```bash
make clean
make
make test-phase6
make test
```

`make test-phase6` boots:

1. the normal kernel heap test;
2. `MODE=heap_double_free`, which must panic;
3. `MODE=heap_guard`, which must panic after a one-byte overrun.

Phase 6 is complete only after those QEMU boots pass and the complete Phase 1–6
regression remains green.
```
