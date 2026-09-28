# AxiomOS Phase 8 — complete new/modified source
This document records the complete contents of every file added or materially modified for Phase 8.

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
	drivers/timer/apic_timer.c \
	drivers/keyboard/ps2_keyboard.c \
	process/task.c \
	process/scheduler.c \
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
	$(MAKE) test-phase7
	$(MAKE) test-phase8


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
	@echo "AxiomOS Phase 8 targets:"
	@echo
	@echo "  make          Build kernel and bootable ISO"
	@echo "  make run      Boot AxiomOS using QEMU"
	@echo "  make debug    Boot QEMU paused for GDB"
	@echo "  make test     Run Phase 1 through Phase 8 tests"
	@echo "  make test-phase6 Run heap + corruption tests"
	@echo "  make test-phase7 Run timer + keyboard IRQ/input tests"
	@echo "  make test-phase8 Run preemptive scheduler/context-switch tests"
	@echo "  make MODE=heap_double_free run  Trigger double-free panic"
	@echo "  make MODE=heap_guard run        Trigger tail-guard panic"
	@echo "  make inspect  Inspect generated ELF64 kernel"
	@echo "  make clean    Remove generated build files"
	@echo "  make distclean Remove build files and Limine"

.PHONY: test-phase3 test-phase4 test-phase5 test-phase6 test-phase7 test-phase8

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

# Track C header changes too; each fault mode has its own object directory.
-include $(C_OBJECTS:.o=.d)

```

## `arch/x86_64/boot/entry.asm`

```nasm
bits 64
default rel

section .text

global _start
global kernel_stack_bottom
global kernel_stack_top
extern kernel_main

_start:
    ; CPU tables are not installed yet.
    ; Interrupts must therefore remain disabled.
    cli
    cld

    ; Limine initially gives us a temporary stack.
    ; Switch to AxiomOS's own 64 KiB bootstrap stack.
    lea rsp, [kernel_stack_top]

    ; The System V AMD64 ABI requires 16-byte stack alignment
    ; before calling another function.
    and rsp, -16

    ; There is no previous stack frame.
    xor rbp, rbp

    ; Enter the C portion of our kernel.
    call kernel_main

.halt:
    ; kernel_main should never need to return, but if it does,
    ; stop the CPU safely rather than executing random memory.
    hlt
    jmp .halt


section .bss

align 16

kernel_stack_bottom:
    resb 65536

kernel_stack_top:

```

## `drivers/timer/apic_timer.c`

```c
#include <stdint.h>

#include <axiom/arch/apic.h>
#include <axiom/arch/interrupts.h>
#include <axiom/drivers/timer.h>
#include <axiom/process/scheduler.h>

#define TIMER_IRQ    0u
#define TIMER_VECTOR 32u

static volatile uint64_t tick_count;
static uint32_t configured_frequency;

static void timer_irq_handler(struct interrupt_frame *frame)
{
    ++tick_count;
    scheduler_on_timer_interrupt(frame);
}

int timer_init(uint32_t frequency_hz)
{
    if (frequency_hz == 0u || interrupts_enabled()) {
        return 0;
    }

    if (!apic_init()) {
        return 0;
    }

    if (irq_register(TIMER_IRQ, timer_irq_handler) != 0) {
        return 0;
    }

    tick_count = 0u;

    if (!apic_timer_start(TIMER_VECTOR, frequency_hz)) {
        return 0;
    }

    configured_frequency = apic_timer_frequency();
    return configured_frequency != 0u;
}

uint64_t timer_ticks(void)
{
    return tick_count;
}

uint32_t timer_frequency(void)
{
    return configured_frequency;
}

void timer_wait_ticks(uint64_t count)
{
    const uint64_t start = timer_ticks();

    while ((timer_ticks() - start) < count) {
        __asm__ volatile ("hlt" ::: "memory");
    }
}

```

## `kernel/core/kernel.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>

#include <axiom/boot/limine.h>
#include <axiom/drivers/keyboard.h>
#include <axiom/drivers/serial.h>
#include <axiom/drivers/timer.h>
#include <axiom/memory/heap.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>
#include <axiom/process/scheduler.h>

#include <axiom/terminal/kprintf.h>
#include <axiom/terminal/terminal.h>



#define PHASE7_LINE_CAPACITY 128u
#define PHASE8_WORKER_TARGET 1000ULL
#define PHASE8_TEST_TIMEOUT_TICKS 300ULL

static volatile uint64_t phase8_worker_a_count;
static volatile uint64_t phase8_worker_b_count;

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
     * Keep the Phase-7 interactive input demonstration alive as the bootstrap
     * task while the two Phase-8 worker threads continue to be preempted.
     */
    phase7_input_loop();
}

```

## `include/axiom/process/task.h`

```c
#ifndef AXIOM_PROCESS_TASK_H
#define AXIOM_PROCESS_TASK_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/interrupts.h>
#include <axiom/memory/address.h>

typedef void (*task_entry_t)(void *argument);

enum task_state {
    TASK_RUNNING = 0,
    TASK_READY,
    TASK_BLOCKED,
    TASK_SLEEPING,
    TASK_TERMINATED,
};

/*
 * Phase-8 kernel task. The saved interrupt frame is the complete CPU context
 * restored by the common ISR epilogue: all GPRs plus RIP/CS/RFLAGS/RSP/SS.
 * Kernel threads currently share one CR3/address space; Phase 9 will change
 * that when user processes receive isolated address spaces.
 */
struct task {
    uint64_t id;
    const char *name;
    enum task_state state;

    struct interrupt_frame context;

    void *stack_base;
    size_t stack_size;
    paddr_t address_space;

    task_entry_t entry;
    void *argument;

    uint64_t quantum_ticks;
    uint64_t ticks_in_slice;
    uint64_t runtime_ticks;
    uint64_t context_switches;
};

const char *task_state_name(enum task_state state);

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
    uint64_t current_task_id;
};

/* Initialise the bootstrap task. Must be called with IF=0 after heap/VMM. */
int scheduler_init(void);

/* Create a ring-0 kernel thread with a private stack. Must be called with IF=0. */
int task_create(
    const char *name,
    task_entry_t entry,
    void *argument,
    uint64_t *task_id_out
);

/* Enable round-robin decisions on subsequent timer interrupts. */
int scheduler_start(void);
int scheduler_running(void);

/* Called from the APIC timer ISR with interrupts disabled by the CPU. */
void scheduler_on_timer_interrupt(struct interrupt_frame *frame);

/* Called automatically if a kernel-thread entry function returns. */
_Noreturn void task_exit_current(void);

struct scheduler_stats scheduler_get_stats(void);
size_t scheduler_task_count(void);
const struct task *scheduler_task_at(size_t index);

#endif

```

## `process/task.c`

```c
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

```

## `process/scheduler.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/kernel/panic.h>
#include <axiom/memory/heap.h>
#include <axiom/memory/vmm.h>
#include <axiom/process/scheduler.h>

static struct task tasks[SCHEDULER_MAX_TASKS];
static size_t task_count_value;
static size_t current_index;
static uint64_t next_task_id;
static uint64_t total_context_switches;
static uint64_t total_preemptions;
static uint64_t total_scheduling_ticks;
static int initialized;
static int running;

extern uint8_t kernel_stack_bottom[];
extern uint8_t kernel_stack_top[];

static void context_clear(struct interrupt_frame *context)
{
    context->r15 = 0u;
    context->r14 = 0u;
    context->r13 = 0u;
    context->r12 = 0u;
    context->r11 = 0u;
    context->r10 = 0u;
    context->r9 = 0u;
    context->r8 = 0u;
    context->rbp = 0u;
    context->rdi = 0u;
    context->rsi = 0u;
    context->rdx = 0u;
    context->rcx = 0u;
    context->rbx = 0u;
    context->rax = 0u;
    context->vector = 0u;
    context->error_code = 0u;
    context->rip = 0u;
    context->cs = 0u;
    context->rflags = 0u;
    context->rsp = 0u;
    context->ss = 0u;
}

static void context_copy(
    struct interrupt_frame *destination,
    const struct interrupt_frame *source
)
{
    destination->r15 = source->r15;
    destination->r14 = source->r14;
    destination->r13 = source->r13;
    destination->r12 = source->r12;
    destination->r11 = source->r11;
    destination->r10 = source->r10;
    destination->r9 = source->r9;
    destination->r8 = source->r8;
    destination->rbp = source->rbp;
    destination->rdi = source->rdi;
    destination->rsi = source->rsi;
    destination->rdx = source->rdx;
    destination->rcx = source->rcx;
    destination->rbx = source->rbx;
    destination->rax = source->rax;
    destination->vector = source->vector;
    destination->error_code = source->error_code;
    destination->rip = source->rip;
    destination->cs = source->cs;
    destination->rflags = source->rflags;
    destination->rsp = source->rsp;
    destination->ss = source->ss;
}

static void task_reset(struct task *task)
{
    task->id = 0u;
    task->name = 0;
    task->state = TASK_TERMINATED;
    context_clear(&task->context);
    task->stack_base = 0;
    task->stack_size = 0u;
    task->address_space = PADDR_INVALID;
    task->entry = 0;
    task->argument = 0;
    task->quantum_ticks = SCHEDULER_DEFAULT_QUANTUM_TICKS;
    task->ticks_in_slice = 0u;
    task->runtime_ticks = 0u;
    task->context_switches = 0u;
}

static _Noreturn void task_bootstrap(struct task *task)
{
    if (task == 0 || task->entry == 0) {
        kernel_panic("scheduler entered an invalid task bootstrap");
    }

    task->entry(task->argument);
    task_exit_current();
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

int scheduler_init(void)
{
    size_t index;
    struct task *bootstrap;
    const struct vmm_stats virtual_memory = vmm_get_stats();

    if (initialized || interrupts_enabled() ||
        virtual_memory.root_table == PADDR_INVALID) {
        return 0;
    }

    for (index = 0u; index < SCHEDULER_MAX_TASKS; ++index) {
        task_reset(&tasks[index]);
    }

    bootstrap = &tasks[0];
    bootstrap->id = 0u;
    bootstrap->name = "bootstrap";
    bootstrap->state = TASK_RUNNING;
    bootstrap->stack_base = kernel_stack_bottom;
    bootstrap->stack_size =
        (size_t)(kernel_stack_top - kernel_stack_bottom);
    bootstrap->address_space = virtual_memory.root_table;
    bootstrap->quantum_ticks = SCHEDULER_DEFAULT_QUANTUM_TICKS;

    task_count_value = 1u;
    current_index = 0u;
    next_task_id = 1u;
    total_context_switches = 0u;
    total_preemptions = 0u;
    total_scheduling_ticks = 0u;
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
    uintptr_t stack_top;
    uintptr_t initial_rsp;
    void *stack;

    if (!initialized || running || interrupts_enabled() ||
        entry == 0 || task_count_value >= SCHEDULER_MAX_TASKS) {
        return 0;
    }

    stack = kmalloc(SCHEDULER_TASK_STACK_SIZE);
    if (stack == 0) {
        return 0;
    }

    task = &tasks[task_count_value];
    task_reset(task);

    stack_top =
        ((uintptr_t)stack + SCHEDULER_TASK_STACK_SIZE) & ~(uintptr_t)0xFu;

    /*
     * A normal SysV C function sees RSP == 8 (mod 16) at entry because CALL
     * pushed a return address. IRETQ does not do that, so reserve one dummy
     * quadword to present the same ABI shape to task_bootstrap().
     */
    initial_rsp = stack_top - sizeof(uint64_t);
    *(uint64_t *)initial_rsp = 0u;

    task->id = next_task_id++;
    task->name = name != 0 ? name : "kernel-thread";
    task->state = TASK_READY;
    task->stack_base = stack;
    task->stack_size = SCHEDULER_TASK_STACK_SIZE;
    task->address_space = vmm_get_stats().root_table;
    task->entry = entry;
    task->argument = argument;
    task->quantum_ticks = SCHEDULER_DEFAULT_QUANTUM_TICKS;

    task->context.rip = (uint64_t)(uintptr_t)&task_bootstrap;
    task->context.cs = GDT_KERNEL_CODE;
    task->context.rflags = 0x202ULL; /* Reserved bit + IF. */
    task->context.rsp = (uint64_t)initial_rsp;
    task->context.ss = GDT_KERNEL_DATA;
    task->context.rdi = (uint64_t)(uintptr_t)task;

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

void scheduler_on_timer_interrupt(struct interrupt_frame *frame)
{
    struct task *current;
    struct task *next;
    size_t next_index;

    if (!running || frame == 0 || task_count_value == 0u) {
        return;
    }

    current = &tasks[current_index];
    context_copy(&current->context, frame);

    ++total_scheduling_ticks;
    ++current->runtime_ticks;
    ++current->ticks_in_slice;

    if (current->state == TASK_RUNNING &&
        current->ticks_in_slice < current->quantum_ticks) {
        return;
    }

    next_index = find_next_ready(current_index);

    if (next_index == SCHEDULER_MAX_TASKS) {
        if (current->state == TASK_RUNNING) {
            current->ticks_in_slice = 0u;
            return;
        }

        kernel_panic("scheduler has no runnable task");
    }

    if (current->state == TASK_RUNNING) {
        current->state = TASK_READY;
    }
    current->ticks_in_slice = 0u;

    next = &tasks[next_index];
    next->state = TASK_RUNNING;
    next->ticks_in_slice = 0u;
    ++next->context_switches;

    current_index = next_index;
    ++total_context_switches;
    ++total_preemptions;

    /*
     * The common ISR epilogue will POP/IRETQ this replacement frame. Replacing
     * the frame changes GPRs, RIP and RSP in one atomic interrupt return, which
     * is the actual kernel-thread context switch.
     */
    context_copy(frame, &next->context);
}

_Noreturn void task_exit_current(void)
{
    if (!running || !initialized || current_index >= task_count_value) {
        kernel_panic("task_exit_current called without a running scheduler");
    }

    interrupts_disable();
    tasks[current_index].state = TASK_TERMINATED;
    tasks[current_index].ticks_in_slice = tasks[current_index].quantum_ticks;

    /*
     * Do not free this stack while executing on it. Phase 8 deliberately keeps
     * terminated stacks allocated; a later reaper can reclaim them safely.
     */
    for (;;) {
        __asm__ volatile ("sti; hlt" ::: "memory");
    }
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
                "Phase 7 input ready. Type into AxiomOS.",
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
            send_keys(monitor, ["s", "c", "h", "e", "d", "ret"])
            wait_for_text(
                process,
                serial,
                "Keyboard line: sched",
                time.monotonic() + 10,
            )

            final_text = serial.read_text(errors="replace").replace("\r", "")
            require("Keyboard line: sched" in final_text,
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

## `README.md`

```markdown
# AxiomOS

AxiomOS is a freestanding x86-64 hobby operating-system kernel written in C and
NASM, built with Clang/LLD and booted by Limine v12.9.0 under QEMU. It is a
higher-half ELF64 kernel and does not use the host libc.

## Current milestone

Phases 0–7 provide boot, framebuffer/serial output, CPU exception handling,
physical and virtual memory management, a kernel heap, and APIC-driven timer and
PS/2 keyboard interrupts. Phase 8 adds the first preemptive multitasking system:
ring-0 kernel threads with private stacks, saved CPU contexts, task states, and a
timer-driven round-robin scheduler.

Implemented foundation:

- bootable x86-64 ELF64 kernel;
- framebuffer terminal, serial mirror, and custom `kprintf()`;
- GDT/TSS, 256-entry IDT, exception stubs and panic diagnostics;
- Local APIC timer at 100 Hz and I/O APIC keyboard routing on q35;
- PS/2 scan-code decoding and buffered keyboard input;
- 4 KiB bitmap physical memory manager;
- AxiomOS-owned four-level page tables and decoded page faults;
- PMM/VMM-backed kernel heap with corruption checks;
- `struct task` with CPU context, state, private stack and address-space field;
- preemptive fixed-quantum round-robin scheduler;
- multiple non-yielding kernel threads making independent progress;
- automated regression tests through Phase 8.

## Common commands

```bash
make
make run
make test
make test-phase5
make test-phase6
make test-phase7
make test-phase8
make MODE=page_fault run
make MODE=heap_double_free run
make MODE=heap_guard run
make debug
```

`make test` runs the complete Phase 1 through Phase 8 regression suite.

Current scope remains single-CPU/ring-0. Ring-3 userspace and isolated process
address spaces are Phase 9.

[Architecture](docs/architecture.md) | [Memory](docs/memory.md) |
[Tasks](docs/processes.md) | [Phase 8](docs/phase8.md) |
[Project state](docs/project-state.md) | [Original roadmap](docs/roadmap.md)

```

## `docs/phase8.md`

```markdown
# Phase 8 — Preemptive multitasking

## Goal

Turn the Phase-7 100 Hz Local APIC timer into a real preemptive scheduler. AxiomOS
now runs multiple ring-0 kernel threads with independent stacks and CPU contexts.
The policy is intentionally simple: fixed-quantum round robin.

## Task model

`struct task` contains:

- task ID and human-readable name;
- state: `RUNNING`, `READY`, `BLOCKED`, `SLEEPING`, or `TERMINATED`;
- a complete x86-64 `interrupt_frame` CPU context;
- private kernel stack base/size;
- address-space/root-page-table identifier;
- entry function and argument;
- quantum, ticks consumed, runtime ticks, and switch count.

Phase 8 implements and exercises `RUNNING`, `READY`, and the termination path.
`BLOCKED` and `SLEEPING` are represented now so later synchronization/sleep APIs do
not need to redesign the task structure.

## Why kernel threads first

All Phase-8 tasks execute in ring 0 and share the current AxiomOS page tables. This
separates two hard problems:

1. prove CPU context switching and preemptive scheduling;
2. later add ring-3 privilege changes and per-process address spaces in Phase 9.

Mixing both into one phase would make failures much harder to isolate.

## Context switch

The Local APIC timer still fires at 100 Hz. The scheduler gives each task a
five-tick quantum (about 50 ms at the configured frequency).

The CPU already creates a complete interrupt frame on every timer interrupt. The
common ISR stub then saves all general-purpose registers. Phase 8 uses that frame
as the task context:

```text
running task
   |
   v
Local APIC timer interrupt
   |
   v
ISR saves GPRs + CPU saved RIP/RSP/RFLAGS/etc.
   |
   v
scheduler copies frame -> current task context
   |
   v
round-robin chooses next READY task
   |
   v
scheduler copies next task context -> ISR frame
   |
   v
IRETQ restores the next task
```

There is no host thread API and the worker threads do not voluntarily yield.

## New-task bootstrap

A new task receives a 16 KiB stack from the Phase-6 heap while interrupts are
disabled. A synthetic interrupt context points RIP at `task_bootstrap()` and RSP
at the new stack. Its RFLAGS has IF set, so once `IRETQ` selects it the task runs
like any normally interrupted kernel thread.

If a task entry returns, the bootstrap marks it `TERMINATED`. Phase 8 deliberately
does not free its stack while executing on it; stack reaping/reclamation is later
scheduler work.

## Round robin

The scheduler scans the fixed task table after the current task has consumed its
quantum and chooses the next `READY` task. With the Phase-8 demo:

```text
bootstrap -> worker A -> worker B -> bootstrap -> ...
```

The bootstrap task is the original `kernel_main()` execution context.

## Acceptance demonstration

Two worker threads run infinite loops and increment separate volatile counters.
They contain no `yield()`, `hlt`, blocking call, or scheduler call. Therefore both
counters can advance and the bootstrap task can regain control only if timer
preemption and context restoration work.

The normal boot requires:

- exactly three schedulable tasks for the demo;
- both worker counters to exceed 1000;
- at least three context switches/preemptions;
- the Phase-7 keyboard input loop to continue working after the scheduler starts.

Run:

```bash
make test-phase8
```

Then run all regressions:

```bash
make test
```

## Current limitations

- single CPU only;
- kernel threads only; no ring-3 processes yet;
- every Phase-8 task shares the same CR3/address space;
- fixed five-tick quantum, no priorities;
- `BLOCKED`/`SLEEPING` states exist but wait queues and sleeping APIs are later;
- terminated task stacks are not reclaimed yet;
- no FPU/SIMD context exists because AxiomOS still builds with those facilities
  disabled;
- heap allocation remains forbidden from interrupt context.

```

## `docs/processes.md`

```markdown
# Tasks and scheduling — Phase 8

AxiomOS now has preemptively scheduled ring-0 kernel threads.

The implementation lives in:

```text
process/task.c
process/scheduler.c
include/axiom/process/task.h
include/axiom/process/scheduler.h
```

Each task records an ID, task state, complete saved x86-64 interrupt/register
context, private stack, address-space identifier, entry point/argument, fixed
round-robin quantum, runtime ticks, and scheduling statistics.

Current states are:

```text
RUNNING
READY
BLOCKED
SLEEPING
TERMINATED
```

Phase 8 actively uses RUNNING/READY and supports termination. Blocking, sleep
queues, priorities, per-process address spaces, and user processes are future
work.

The scheduler is driven by the Phase-7 Local APIC timer at 100 Hz. A five-tick
quantum produces a nominal 50 ms time slice. Context switching is performed by
saving the timer interrupt frame into the current task and replacing that frame
with the next task's saved CPU state before `IRETQ`.

```

## `docs/architecture.md`

```markdown
# Architecture at Phase 8

- Target: x86-64, one bootstrap CPU, freestanding C/NASM.
- Limine v12.9.0 boots a higher-half ELF64 kernel.
- Serial output mirrors the framebuffer terminal/custom `kprintf()`.
- GDT/TSS plus a 256-entry IDT provide exception/interrupt infrastructure.
- The legacy 8259 PIC remains masked on q35.
- Local APIC supplies the 100 Hz periodic timer; I/O APIC routes PS/2 keyboard IRQ1.
- PMM manages usable RAM as 4 KiB physical frames with a bitmap.
- VMM owns PML4/PDPT/PD/PT pages and exposes map/unmap/translate operations.
- Kernel heap provides `kmalloc`/`kcalloc`/`krealloc`/`kfree`.
- Phase 8 adds preemptive ring-0 kernel threads with 16 KiB private stacks.
- Scheduler policy: fixed five-tick preemptive round robin.
- Timer interrupt frames are the saved CPU contexts used for switching tasks.
- Phase-8 tasks share one CR3/address space; process isolation begins in Phase 9.
- Keyboard input continues to use an IRQ-owned ring buffer and the bootstrap task
  consumes/echoes it while worker tasks are scheduled in the background.

```

## `docs/project-state.md`

```markdown
# AxiomOS handoff — Phase 8 implementation

Development environment: Linux/Distrobox on Steam Deck. Run commands from the
directory containing `Makefile`. Do not restart from earlier phases.

Roadmap: Phase 0 through Phase 25, 26 phases including setup.

Accepted foundation before Phase 8:

- Phase 0: development environment.
- Phase 1: bootable x86-64 ELF64 kernel and serial output.
- Phase 2: framebuffer terminal and custom formatter.
- Phase 3: GDT/TSS, IDT, exceptions/interrupts and panic diagnostics.
- Phase 4: tested 4 KiB bitmap physical page allocator.
- Phase 5: AxiomOS-owned four-level page tables, mapping and page faults.
- Phase 6: PMM/VMM-backed first-fit/coalescing kernel heap.
- Phase 7: q35 Local APIC 100 Hz timer plus I/O-APIC-routed PS/2 keyboard input.

Phase 8 adds:

- `struct task` with ID, state, full CPU context, stack, address space, and
  scheduling information;
- task states RUNNING/READY/BLOCKED/SLEEPING/TERMINATED;
- fixed task table, maximum 8 tasks for this stage;
- 16 KiB heap-backed private kernel-thread stacks;
- synthetic initial interrupt context for new tasks;
- timer-driven preemptive round-robin scheduling;
- five-tick time quantum at the Phase-7 100 Hz timer rate;
- context switch by replacing the timer ISR's saved register/IRETQ frame;
- bootstrap task representing the existing kernel_main execution context;
- task-return termination handling without freeing the active stack;
- two non-yielding worker tasks proving actual preemption;
- scheduler statistics and `tests/phase8_scheduler.py`;
- Phase-7 keyboard input retained while scheduler/worker tasks run.

Important limitations:

- one CPU only;
- kernel threads only, no Ring 3;
- all tasks share the same page-table root;
- fixed quantum and no task priorities;
- BLOCKED/SLEEPING are represented but wait queues/sleep APIs are not implemented;
- terminated stacks are not reclaimed yet;
- no FPU/SSE context is needed because the kernel build disables those units;
- heap remains non-interrupt-safe and task creation occurs with IF=0.

Acceptance commands:

```bash
make clean
make
make test-phase7
make test-phase8
make test
```

Phase 8 is complete only after the local QEMU scheduler test and full Phase 1-8
regression suite pass.

Next milestone: Phase 9 — Ring-3 userspace, separate user stacks/address spaces,
privilege transitions, and memory protection between user processes and kernel.

```

## `docs/validation.md`

```markdown
# Validation status — Phase 8

## User-accepted baseline

The user has accepted Phase 7 on the APIC-based q35 implementation. The active
baseline therefore includes the Phase-7 Local APIC timer and I/O APIC keyboard
routing, not the earlier legacy-PIC timer attempt.

## Phase-8 validation performed before packaging

- Every C translation unit is compiled/syntax-checked with the project's actual
  freestanding Clang target and `-Wall -Wextra -Werror` policy.
- The scheduler does not call host threading, libc, or Linux services.
- The timer ISR invokes the scheduler only after incrementing the monotonic timer.
- Task creation occurs only with interrupts disabled and before scheduling starts.
- New task stacks satisfy the SysV x86-64 entry alignment expected by C code.
- Worker tasks deliberately contain no voluntary yield operation.
- Phase-8 Python test syntax is validated.
- Existing deliberate CPU/page/heap fault-mode C builds are revalidated before
  final packaging.

## Required local QEMU acceptance

This packaging environment does not provide NASM/QEMU, so runtime acceptance must
occur in the user's kernel-development distrobox:

```bash
make clean
make
make test-phase8
make test
```

`make test-phase8` must prove both non-yielding workers make progress, at least
three timer-driven context switches occur, and keyboard input still works after
the scheduler is running.

```
