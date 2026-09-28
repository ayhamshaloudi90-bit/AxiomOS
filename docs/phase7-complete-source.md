# AxiomOS Phase 7 — APIC revision complete source
This document contains every file added or modified by the corrected Phase-7 APIC implementation.
The first Phase-7 attempt used legacy PIT/PIC delivery on q35 and stalled waiting for IRQ0. This revision uses a Local APIC periodic timer and I/O APIC keyboard routing while leaving the 8259 PIC masked.

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
	@echo "AxiomOS Phase 7 targets:"
	@echo
	@echo "  make          Build kernel and bootable ISO"
	@echo "  make run      Boot AxiomOS using QEMU"
	@echo "  make debug    Boot QEMU paused for GDB"
	@echo "  make test     Run Phase 1 through Phase 7 tests"
	@echo "  make test-phase6 Run heap + corruption tests"
	@echo "  make test-phase7 Run timer + keyboard IRQ/input tests"
	@echo "  make MODE=heap_double_free run  Trigger double-free panic"
	@echo "  make MODE=heap_guard run        Trigger tail-guard panic"
	@echo "  make inspect  Inspect generated ELF64 kernel"
	@echo "  make clean    Remove generated build files"
	@echo "  make distclean Remove build files and Limine"

.PHONY: test-phase3 test-phase4 test-phase5 test-phase6 test-phase7

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

Phases 0–4 have been locally accepted. Phase 5 added AxiomOS-owned page tables,
4 KiB mapping primitives, address translation, TLB invalidation, and decoded
page-fault diagnostics. Phase 6 added a PMM/VMM-backed dynamic kernel heap.
Phase 7 adds the first continuously enabled hardware interrupts: a 100 Hz
Local APIC timer plus I/O-APIC-routed PS/2 keyboard input with scan-code decoding
and buffering.
Phase 5–7 remain accepted only after their local QEMU regression tests pass.

Implemented foundation:

- bootable x86-64 ELF64 kernel;
- serial output and framebuffer terminal with custom `kprintf()`;
- GDT/TSS, 256-entry IDT, exception stubs and panic diagnostics;
- remapped 8259 PIC with explicit per-IRQ masking;
- 4 KiB bitmap physical memory manager;
- AxiomOS-owned PML4/PDPT/PD/PT hierarchy;
- `map_page()`, `unmap_page()`, `virt_to_phys()`, and page-fault diagnostics;
- higher-half PMM/VMM-backed kernel heap;
- `kmalloc()`, `kcalloc()`, `krealloc()`, and `kfree()` with corruption checks;
- Local APIC periodic timer at 100 Hz, calibrated against PIT channel 2;
- first-port PS/2 keyboard routed through I/O APIC vector 33;
- scan-code-set-1 text decoding with Shift/Caps Lock;
- 128-character interrupt-to-kernel input ring buffer;
- interactive Phase-7 input demonstration;
- automated regression tests through timer/keyboard input injection.

## Common commands

```bash
make
make run
make test
make test-phase5
make test-phase6
make test-phase7
make MODE=page_fault run
make MODE=heap_double_free run
make MODE=heap_guard run
make debug
```

`make test` runs the complete Phase 1 through Phase 7 regression suite. Fault
builds use separate directories such as `build-divide/`, `build-page_fault/`,
`build-heap_double_free/`, and `build-heap_guard/`.

Normal Phase-7 execution leaves IF enabled with the Local APIC timer and I/O APIC
keyboard route active. The legacy 8259 PIC remains masked. The kernel still has no
scheduler, processes/userspace, filesystem, disk driver, or network stack.

[Architecture](docs/architecture.md) | [Memory](docs/memory.md) |
[Phase 5](docs/phase5.md) | [Phase 6](docs/phase6.md) |
[Phase 7](docs/phase7.md) | [Project state](docs/project-state.md) |
[Original roadmap](docs/roadmap.md)

```

## `include/axiom/arch/apic.h`

```c
#ifndef AXIOM_ARCH_APIC_H
#define AXIOM_ARCH_APIC_H

#include <stdint.h>

/*
 * Phase 7 uses the xAPIC MMIO interface on the q35 machine model.
 * The local APIC handles the periodic timer; the I/O APIC routes ISA IRQs.
 */
int apic_init(void);
int apic_active(void);
uint8_t apic_local_id(void);

/* Route one legacy ISA IRQ/GSI to an IDT vector on the bootstrap CPU. */
int apic_route_isa_irq(uint8_t irq, uint8_t vector);
int apic_mask_isa_irq(uint8_t irq);

/* Signal completion of a non-spurious local/I/O APIC interrupt. */
void apic_eoi(void);

/* Configure the local APIC timer in periodic mode. */
int apic_timer_start(uint8_t vector, uint32_t frequency_hz);
uint32_t apic_timer_frequency(void);

#endif

```

## `include/axiom/drivers/timer.h`

```c
#ifndef AXIOM_DRIVERS_TIMER_H
#define AXIOM_DRIVERS_TIMER_H

#include <stdint.h>

/* Initialise the local APIC periodic timer on vector 32. Call while IF=0. */
int timer_init(uint32_t frequency_hz);

/* Monotonic count of periodic vector-32 timer deliveries since timer_init(). */
uint64_t timer_ticks(void);

/* Configured Local APIC timer frequency. */
uint32_t timer_frequency(void);

/* Sleep this CPU until at least count more timer interrupts have arrived. */
void timer_wait_ticks(uint64_t count);

#endif

```

## `arch/x86_64/interrupts/apic.c`

```c
#include <stdint.h>

#include <axiom/arch/apic.h>
#include <axiom/arch/io.h>
#include <axiom/memory/address.h>
#include <axiom/memory/vmm.h>

#define IA32_APIC_BASE_MSR 0x1Bu
#define IA32_APIC_BASE_ENABLE (1ULL << 11)
#define IA32_APIC_BASE_X2APIC (1ULL << 10)
#define IA32_APIC_BASE_MASK   0x000FFFFFFFFFF000ULL

#define LAPIC_MMIO_VA  0xFFFFD00000000000ULL
#define IOAPIC_MMIO_VA 0xFFFFD00000001000ULL

#define Q35_IOAPIC_PHYSICAL 0xFEC00000ULL

#define LAPIC_REG_ID                 0x020u
#define LAPIC_REG_EOI                0x0B0u
#define LAPIC_REG_SPURIOUS           0x0F0u
#define LAPIC_REG_LVT_TIMER          0x320u
#define LAPIC_REG_TIMER_INITIAL      0x380u
#define LAPIC_REG_TIMER_CURRENT      0x390u
#define LAPIC_REG_TIMER_DIVIDE       0x3E0u

#define LAPIC_SOFTWARE_ENABLE        (1u << 8)
#define LAPIC_TIMER_MASKED           (1u << 16)
#define LAPIC_TIMER_PERIODIC         (1u << 17)
#define LAPIC_SPURIOUS_VECTOR        0xFFu
#define LAPIC_TIMER_DIVIDE_BY_16     0x3u

#define IOAPIC_REG_ID                0x00u
#define IOAPIC_REG_VERSION           0x01u
#define IOAPIC_REG_REDIR_BASE        0x10u
#define IOAPIC_REDIR_MASKED          (1u << 16)

#define PIT_CHANNEL2_DATA            0x42u
#define PIT_COMMAND                  0x43u
#define PIT_SPEAKER_CONTROL          0x61u
#define PIT_INPUT_HZ                 1193182u
#define PIT_CHANNEL2_ONESHOT         0xB0u
#define PIT_SPEAKER_GATE2            0x01u
#define PIT_SPEAKER_ENABLE           0x02u
#define PIT_SPEAKER_OUT2             0x20u

#define APIC_CALIBRATION_HZ          100u
#define PIT_POLL_LIMIT               10000000u

static volatile uint32_t *lapic;
static volatile uint32_t *ioapic_select;
static volatile uint32_t *ioapic_window;
static uint8_t local_id;
static uint8_t ioapic_max_redirection;
static uint32_t timer_frequency_hz;
static int initialized;

static uint64_t read_msr(uint32_t msr)
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

static void write_msr(uint32_t msr, uint64_t value)
{
    __asm__ volatile (
        "wrmsr"
        :
        : "c"(msr), "a"((uint32_t)value), "d"((uint32_t)(value >> 32))
        : "memory"
    );
}

static int cpu_has_apic(void)
{
    uint32_t eax = 1u;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;

    __asm__ volatile (
        "cpuid"
        : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        :
    );

    (void)ebx;
    (void)ecx;
    return (edx & (1u << 9)) != 0u;
}

static uint32_t lapic_read(uint32_t offset)
{
    return lapic[offset / sizeof(uint32_t)];
}

static void lapic_write(uint32_t offset, uint32_t value)
{
    lapic[offset / sizeof(uint32_t)] = value;
    (void)lapic[offset / sizeof(uint32_t)];
}

static uint32_t ioapic_read(uint8_t reg)
{
    *ioapic_select = reg;
    return *ioapic_window;
}

static void ioapic_write(uint8_t reg, uint32_t value)
{
    *ioapic_select = reg;
    *ioapic_window = value;
}

static int map_mmio_page(vaddr_t virtual_address, paddr_t physical_address)
{
    paddr_t current = virt_to_phys(virtual_address);

    if (current != PADDR_INVALID) {
        return (current & ~(VMM_PAGE_SIZE - 1ULL)) == physical_address;
    }

    return map_page(
        virtual_address,
        physical_address,
        VMM_FLAG_WRITABLE |
        VMM_FLAG_WRITE_THROUGH |
        VMM_FLAG_CACHE_DISABLE |
        VMM_FLAG_NO_EXECUTE
    );
}

static int pit_wait_10ms(void)
{
    const uint16_t count = (uint16_t)(PIT_INPUT_HZ / APIC_CALIBRATION_HZ);
    const uint8_t original = inb(PIT_SPEAKER_CONTROL);
    uint32_t remaining = PIT_POLL_LIMIT;

    /* Gate channel 2 off while loading a fresh mode-0 one-shot count. */
    outb(
        PIT_SPEAKER_CONTROL,
        (uint8_t)(original & ~(PIT_SPEAKER_GATE2 | PIT_SPEAKER_ENABLE))
    );

    outb(PIT_COMMAND, PIT_CHANNEL2_ONESHOT);
    outb(PIT_CHANNEL2_DATA, (uint8_t)(count & 0xFFu));
    outb(PIT_CHANNEL2_DATA, (uint8_t)(count >> 8));

    /* Gate high starts the countdown; leave the speaker itself disabled. */
    outb(
        PIT_SPEAKER_CONTROL,
        (uint8_t)((original & ~PIT_SPEAKER_ENABLE) | PIT_SPEAKER_GATE2)
    );

    while ((inb(PIT_SPEAKER_CONTROL) & PIT_SPEAKER_OUT2) == 0u) {
        if (remaining-- == 0u) {
            outb(PIT_SPEAKER_CONTROL, original);
            return 0;
        }

        __asm__ volatile ("pause");
    }

    outb(PIT_SPEAKER_CONTROL, original);
    return 1;
}

int apic_init(void)
{
    uint64_t apic_base;
    paddr_t lapic_physical;
    uint32_t version;
    uint32_t index;

    if (initialized) {
        return 1;
    }

    if (!cpu_has_apic()) {
        return 0;
    }

    apic_base = read_msr(IA32_APIC_BASE_MSR);

    /* Phase 7 deliberately uses the xAPIC MMIO interface, not x2APIC. */
    if ((apic_base & IA32_APIC_BASE_X2APIC) != 0ULL) {
        return 0;
    }

    if ((apic_base & IA32_APIC_BASE_ENABLE) == 0ULL) {
        apic_base |= IA32_APIC_BASE_ENABLE;
        write_msr(IA32_APIC_BASE_MSR, apic_base);
    }

    lapic_physical = apic_base & IA32_APIC_BASE_MASK;

    if (!map_mmio_page(LAPIC_MMIO_VA, lapic_physical) ||
        !map_mmio_page(IOAPIC_MMIO_VA, Q35_IOAPIC_PHYSICAL)) {
        return 0;
    }

    lapic = (volatile uint32_t *)(uintptr_t)LAPIC_MMIO_VA;
    ioapic_select = (volatile uint32_t *)(uintptr_t)IOAPIC_MMIO_VA;
    ioapic_window = (volatile uint32_t *)(uintptr_t)(IOAPIC_MMIO_VA + 0x10u);

    local_id = (uint8_t)(lapic_read(LAPIC_REG_ID) >> 24);
    version = ioapic_read(IOAPIC_REG_VERSION);
    ioapic_max_redirection = (uint8_t)((version >> 16) & 0xFFu);

    /* Mask every I/O APIC input until a driver explicitly claims it. */
    for (index = 0u; index <= ioapic_max_redirection; ++index) {
        const uint8_t low = (uint8_t)(IOAPIC_REG_REDIR_BASE + index * 2u);
        const uint8_t high = (uint8_t)(low + 1u);

        ioapic_write(high, 0u);
        ioapic_write(low, IOAPIC_REDIR_MASKED);
    }

    /* Enable the local APIC and reserve vector 0xFF for spurious interrupts. */
    lapic_write(
        LAPIC_REG_SPURIOUS,
        LAPIC_SOFTWARE_ENABLE | LAPIC_SPURIOUS_VECTOR
    );

    /* Mask the local APIC timer until timer_init() calibrates it. */
    lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_MASKED | 32u);
    lapic_write(LAPIC_REG_TIMER_INITIAL, 0u);

    /* Reading the IOAPIC ID proves the MMIO window responds. */
    (void)ioapic_read(IOAPIC_REG_ID);

    initialized = 1;
    return 1;
}

int apic_active(void)
{
    return initialized;
}

uint8_t apic_local_id(void)
{
    return local_id;
}

int apic_route_isa_irq(uint8_t irq, uint8_t vector)
{
    uint8_t low;
    uint8_t high;

    if (!initialized || irq > ioapic_max_redirection || vector < 32u) {
        return 0;
    }

    low = (uint8_t)(IOAPIC_REG_REDIR_BASE + irq * 2u);
    high = (uint8_t)(low + 1u);

    /* Physical destination, fixed delivery, edge-triggered, active-high. */
    ioapic_write(high, (uint32_t)local_id << 24);
    ioapic_write(low, vector);
    return 1;
}

int apic_mask_isa_irq(uint8_t irq)
{
    uint8_t low;
    uint32_t value;

    if (!initialized || irq > ioapic_max_redirection) {
        return 0;
    }

    low = (uint8_t)(IOAPIC_REG_REDIR_BASE + irq * 2u);
    value = ioapic_read(low);
    ioapic_write(low, value | IOAPIC_REDIR_MASKED);
    return 1;
}

void apic_eoi(void)
{
    if (initialized) {
        lapic_write(LAPIC_REG_EOI, 0u);
    }
}

int apic_timer_start(uint8_t vector, uint32_t frequency_hz)
{
    uint32_t current;
    uint32_t elapsed_10ms;
    uint64_t initial_count;

    if (!initialized || frequency_hz == 0u || vector < 32u) {
        return 0;
    }

    lapic_write(LAPIC_REG_TIMER_DIVIDE, LAPIC_TIMER_DIVIDE_BY_16);
    lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_MASKED | vector);
    lapic_write(LAPIC_REG_TIMER_INITIAL, 0xFFFFFFFFu);

    if (!pit_wait_10ms()) {
        lapic_write(LAPIC_REG_TIMER_INITIAL, 0u);
        return 0;
    }

    current = lapic_read(LAPIC_REG_TIMER_CURRENT);
    elapsed_10ms = 0xFFFFFFFFu - current;

    if (elapsed_10ms < 100u) {
        lapic_write(LAPIC_REG_TIMER_INITIAL, 0u);
        return 0;
    }

    /*
     * elapsed_10ms is the LAPIC count for 1/100 second. Scale that to the
     * requested period. Phase 7 requests exactly 100 Hz.
     */
    initial_count =
        ((uint64_t)elapsed_10ms * APIC_CALIBRATION_HZ) /
        frequency_hz;

    if (initial_count == 0ULL || initial_count > UINT32_MAX) {
        lapic_write(LAPIC_REG_TIMER_INITIAL, 0u);
        return 0;
    }

    timer_frequency_hz = frequency_hz;
    lapic_write(LAPIC_REG_LVT_TIMER, LAPIC_TIMER_PERIODIC | vector);
    lapic_write(LAPIC_REG_TIMER_INITIAL, (uint32_t)initial_count);
    return 1;
}

uint32_t apic_timer_frequency(void)
{
    return timer_frequency_hz;
}

```

## `arch/x86_64/interrupts/idt.c`

```c
#include <stdint.h>

#include <axiom/arch/apic.h>
#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/arch/pic.h>
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


void interrupts_init(void)
{
    uint16_t vector;
    struct idt_pointer pointer;


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


    pointer.limit = sizeof(idt) - 1u;
    pointer.base = (uint64_t)(uintptr_t)idt;


    __asm__ volatile ("lidt %0" : : "m"(pointer) : "memory");

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


    if (irq >= 16 ||
        handler == 0 ||
        (flags & (1ULL << 9)) != 0) {

        return -1;
    }


    irq_handlers[irq] = handler;
    return 0;
}


void interrupt_dispatch(struct interrupt_frame *frame)
{
    if (frame->vector == 3) {
        kprintf("Breakpoint: resumed safely.\n");
        return;
    }


    if (frame->vector == 14) {
        page_fault_panic(frame);
    }


    if (frame->vector < 32) {
        exception_panic(frame);
    }


    if (frame->vector < 48) {
        const uint8_t irq = (uint8_t)(frame->vector - 32);


        if (apic_active()) {
            if (irq_handlers[irq] != 0) {
                irq_handlers[irq](frame);
            }

            apic_eoi();
            return;
        }


        if (pic_is_spurious(irq)) {
            return;
        }


        if (irq_handlers[irq] != 0) {
            irq_handlers[irq](frame);
        }


        pic_eoi(irq);
        return;
    }


    if (frame->vector == 0x80) {
        kprintf("Software interrupt 0x80: returned safely.\n");
        return;
    }


    if (frame->vector == 0xFF) {
        /* Local APIC spurious vector: no EOI is required. */
        return;
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

## `drivers/timer/apic_timer.c`

```c
#include <stdint.h>

#include <axiom/arch/apic.h>
#include <axiom/arch/interrupts.h>
#include <axiom/drivers/timer.h>

#define TIMER_IRQ    0u
#define TIMER_VECTOR 32u

static volatile uint64_t tick_count;
static uint32_t configured_frequency;

static void timer_irq_handler(struct interrupt_frame *frame)
{
    (void)frame;
    ++tick_count;
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

## `drivers/keyboard/ps2_keyboard.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/apic.h>
#include <axiom/arch/interrupts.h>
#include <axiom/arch/io.h>
#include <axiom/drivers/keyboard.h>

#define PS2_DATA_PORT    0x60u
#define PS2_STATUS_PORT  0x64u
#define PS2_COMMAND_PORT 0x64u

#define PS2_STATUS_OUTPUT_FULL 0x01u
#define PS2_STATUS_INPUT_FULL  0x02u
#define PS2_STATUS_AUX_DATA    0x20u

#define PS2_CMD_DISABLE_FIRST  0xADu
#define PS2_CMD_DISABLE_SECOND 0xA7u
#define PS2_CMD_ENABLE_FIRST   0xAEu
#define PS2_CMD_READ_CONFIG   0x20u
#define PS2_CMD_WRITE_CONFIG  0x60u

#define PS2_CONFIG_IRQ1          0x01u
#define PS2_CONFIG_IRQ12         0x02u
#define PS2_CONFIG_FIRST_CLOCK   0x10u
#define PS2_CONFIG_TRANSLATION   0x40u

#define KEYBOARD_CMD_ENABLE_SCANNING 0xF4u
#define KEYBOARD_ACK                 0xFAu
#define KEYBOARD_RESEND              0xFEu

#define PS2_WAIT_LIMIT 1000000u

static volatile char input_buffer[KEYBOARD_BUFFER_CAPACITY];
static volatile size_t buffer_head;
static volatile size_t buffer_tail;

static volatile uint64_t irq_count;
static volatile uint64_t scancode_count;
static volatile uint64_t character_count;
static volatile uint64_t dropped_characters;

static uint8_t left_shift;
static uint8_t right_shift;
static uint8_t caps_lock;
static uint8_t extended_prefix;

static const char normal_map[128] = {
    [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4',
    [0x06] = '5', [0x07] = '6', [0x08] = '7', [0x09] = '8',
    [0x0A] = '9', [0x0B] = '0', [0x0C] = '-', [0x0D] = '=',
    [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r',
    [0x14] = 't', [0x15] = 'y', [0x16] = 'u', [0x17] = 'i',
    [0x18] = 'o', [0x19] = 'p', [0x1A] = '[', [0x1B] = ']',
    [0x1C] = '\n',
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f',
    [0x22] = 'g', [0x23] = 'h', [0x24] = 'j', [0x25] = 'k',
    [0x26] = 'l', [0x27] = ';', [0x28] = '\'', [0x29] = '`',
    [0x2B] = '\\',
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v',
    [0x30] = 'b', [0x31] = 'n', [0x32] = 'm', [0x33] = ',',
    [0x34] = '.', [0x35] = '/', [0x39] = ' '
};

static const char shifted_map[128] = {
    [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$',
    [0x06] = '%', [0x07] = '^', [0x08] = '&', [0x09] = '*',
    [0x0A] = '(', [0x0B] = ')', [0x0C] = '_', [0x0D] = '+',
    [0x1A] = '{', [0x1B] = '}', [0x27] = ':', [0x28] = '"',
    [0x29] = '~', [0x2B] = '|', [0x33] = '<', [0x34] = '>',
    [0x35] = '?'
};

static int ps2_wait_input_clear(void)
{
    uint32_t remaining = PS2_WAIT_LIMIT;

    while (remaining-- != 0u) {
        if ((inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_FULL) == 0u) {
            return 1;
        }
    }

    return 0;
}

static int ps2_wait_output_full(void)
{
    uint32_t remaining = PS2_WAIT_LIMIT;

    while (remaining-- != 0u) {
        if ((inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) != 0u) {
            return 1;
        }
    }

    return 0;
}

static int ps2_write_command(uint8_t command)
{
    if (!ps2_wait_input_clear()) {
        return 0;
    }

    outb(PS2_COMMAND_PORT, command);
    return 1;
}

static int ps2_write_data(uint8_t data)
{
    if (!ps2_wait_input_clear()) {
        return 0;
    }

    outb(PS2_DATA_PORT, data);
    return 1;
}

static int ps2_read_data(uint8_t *data)
{
    if (data == 0 || !ps2_wait_output_full()) {
        return 0;
    }

    *data = inb(PS2_DATA_PORT);
    return 1;
}

static void ps2_flush_output(void)
{
    uint32_t remaining = 256u;

    while (remaining-- != 0u &&
           (inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_FULL) != 0u) {
        (void)inb(PS2_DATA_PORT);
    }
}

static int keyboard_device_command(uint8_t command)
{
    uint8_t response;
    unsigned int attempt;

    for (attempt = 0; attempt < 2u; ++attempt) {
        if (!ps2_write_data(command) || !ps2_read_data(&response)) {
            return 0;
        }

        if (response == KEYBOARD_ACK) {
            return 1;
        }

        if (response != KEYBOARD_RESEND) {
            return 0;
        }
    }

    return 0;
}

static int is_ascii_letter(char character)
{
    return character >= 'a' && character <= 'z';
}

static void enqueue_character(char character)
{
    const size_t next = (buffer_head + 1u) % KEYBOARD_BUFFER_CAPACITY;

    if (next == buffer_tail) {
        ++dropped_characters;
        return;
    }

    input_buffer[buffer_head] = character;
    buffer_head = next;
    ++character_count;
}

static void decode_scancode(uint8_t scancode)
{
    uint8_t code;
    int released;
    int shifted;
    char character;

    ++scancode_count;

    if (scancode == 0xE0u) {
        extended_prefix = 1u;
        return;
    }

    code = scancode & 0x7Fu;
    released = (scancode & 0x80u) != 0u;

    if (extended_prefix != 0u) {
        /*
         * Phase 7 only turns normal text keys into characters. Consume the
         * second byte of an extended set-1 sequence so it cannot be mistaken
         * for an ordinary key. Arrow/navigation keys can be exposed later.
         */
        extended_prefix = 0u;
        return;
    }

    if (code == 0x2Au) {
        left_shift = released ? 0u : 1u;
        return;
    }

    if (code == 0x36u) {
        right_shift = released ? 0u : 1u;
        return;
    }

    if (released) {
        return;
    }

    if (code == 0x3Au) {
        caps_lock ^= 1u;
        return;
    }

    character = normal_map[code];

    if (character == '\0') {
        return;
    }

    shifted = (left_shift != 0u || right_shift != 0u);

    if (is_ascii_letter(character)) {
        if (shifted != (caps_lock != 0u)) {
            character = (char)(character - 'a' + 'A');
        }
    } else if (shifted && shifted_map[code] != '\0') {
        character = shifted_map[code];
    }

    enqueue_character(character);
}

static void keyboard_irq_handler(struct interrupt_frame *frame)
{
    uint8_t status;

    (void)frame;
    ++irq_count;

    /*
     * A controller may already contain more than one byte when IRQ1 runs.
     * Drain keyboard bytes now, but do not consume second-port (mouse) bytes.
     */
    for (;;) {
        status = inb(PS2_STATUS_PORT);

        if ((status & PS2_STATUS_OUTPUT_FULL) == 0u) {
            break;
        }

        if ((status & PS2_STATUS_AUX_DATA) != 0u) {
            break;
        }

        decode_scancode(inb(PS2_DATA_PORT));
    }
}

int keyboard_init(void)
{
    uint8_t config;

    if (interrupts_enabled()) {
        return 0;
    }

    if (irq_register(1u, keyboard_irq_handler) != 0) {
        return 0;
    }

    if (!ps2_write_command(PS2_CMD_DISABLE_FIRST) ||
        !ps2_write_command(PS2_CMD_DISABLE_SECOND)) {
        return 0;
    }

    ps2_flush_output();

    if (!ps2_write_command(PS2_CMD_READ_CONFIG) ||
        !ps2_read_data(&config)) {
        return 0;
    }

    config |= PS2_CONFIG_IRQ1;
    config &= (uint8_t)~PS2_CONFIG_IRQ12;
    config |= PS2_CONFIG_TRANSLATION;
    config &= (uint8_t)~PS2_CONFIG_FIRST_CLOCK;

    if (!ps2_write_command(PS2_CMD_WRITE_CONFIG) ||
        !ps2_write_data(config) ||
        !ps2_write_command(PS2_CMD_ENABLE_FIRST)) {
        return 0;
    }

    ps2_flush_output();

    if (!keyboard_device_command(KEYBOARD_CMD_ENABLE_SCANNING)) {
        return 0;
    }

    buffer_head = 0u;
    buffer_tail = 0u;
    irq_count = 0u;
    scancode_count = 0u;
    character_count = 0u;
    dropped_characters = 0u;
    left_shift = 0u;
    right_shift = 0u;
    caps_lock = 0u;
    extended_prefix = 0u;

    return apic_route_isa_irq(1u, 33u);
}

int keyboard_read_char(char *character)
{
    size_t tail;

    if (character == 0) {
        return 0;
    }

    tail = buffer_tail;

    if (tail == buffer_head) {
        return 0;
    }

    *character = input_buffer[tail];
    buffer_tail = (tail + 1u) % KEYBOARD_BUFFER_CAPACITY;
    return 1;
}

size_t keyboard_pending(void)
{
    const size_t head = buffer_head;
    const size_t tail = buffer_tail;

    if (head >= tail) {
        return head - tail;
    }

    return KEYBOARD_BUFFER_CAPACITY - tail + head;
}

struct keyboard_stats keyboard_get_stats(void)
{
    const struct keyboard_stats stats = {
        .irq_count = irq_count,
        .scancode_count = scancode_count,
        .character_count = character_count,
        .dropped_characters = dropped_characters,
    };

    return stats;
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

#include <axiom/terminal/kprintf.h>
#include <axiom/terminal/terminal.h>



#define PHASE7_LINE_CAPACITY 128u


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

    phase7_input_loop();
}

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
                "Phase 7 input ready. Type into AxiomOS.",
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
                "Phase 7 input ready. Type into AxiomOS.",
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
            send_keys(monitor, ["a", "x", "i", "o", "m", "ret"])

            wait_for_text(
                process,
                serial,
                "Keyboard line: axiom",
                time.monotonic() + 8,
            )

            text = serial.read_text(errors="replace").replace("\r", "")

            require("Keyboard line: axiom" in text,
                    "phase7: injected PS/2 text was not decoded/buffered")
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
            print("  line:       axiom")
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

## `docs/phase7.md`

```markdown
# Phase 7 — Timer and keyboard drivers

## Goal

Turn the Phase-3 interrupt framework into continuously enabled hardware input on
AxiomOS's q35 target:

- Local APIC periodic timer at 100 Hz;
- I/O APIC routing for the PS/2 keyboard;
- scan-code decoding and a kernel input ring buffer.

Phases 1–6 deliberately kept IF clear. Phase 7 is the first milestone that executes
`sti` and leaves external interrupts enabled during normal execution.

## Why APIC instead of relying on the legacy PIC

The first Phase-7 attempt programmed PIT IRQ0 through the 8259 PIC. On the q35
machine that path reached timer setup but no IRQ0 reached the CPU. q35 is an
APIC-era platform, so AxiomOS now uses its native interrupt architecture instead
of forcing legacy PIC delivery.

The 8259 PIC is still initialized/remapped for the earlier architecture work, but
it remains fully masked once Phase 7 starts.

## Local APIC timer

AxiomOS maps the Local APIC MMIO page by reading `IA32_APIC_BASE`. The timer is
calibrated without requiring a PIT interrupt:

1. LAPIC timer starts as a masked one-shot counter.
2. PIT channel 2 runs for approximately 10 ms in mode 0.
3. AxiomOS polls PIT channel-2 OUT through port `0x61`.
4. The elapsed LAPIC count over those 10 ms is measured.
5. That count becomes the basis for the 100 Hz periodic LAPIC timer.

The periodic timer uses IDT vector 32. Its handler increments a monotonic 64-bit
tick counter; the common interrupt dispatcher sends the LAPIC EOI.

## I/O APIC keyboard route

For the pinned q35 target, AxiomOS maps the standard q35 I/O APIC MMIO page at
physical `0xFEC00000`. Every redirection entry starts masked. The first PS/2
keyboard line (ISA IRQ/GSI 1) is routed to vector 33 on the bootstrap CPU.

Future platform-general work should discover I/O APICs and interrupt-source
overrides from the ACPI MADT instead of relying on the pinned q35 topology.

## PS/2 keyboard

The i8042 first port is configured while IF is still clear. The driver enables
scan-code translation and scanning, then routes the keyboard interrupt through the
I/O APIC. The decoder supports normal set-1 text keys, Shift, Caps Lock, digits,
punctuation, Space, Enter, Tab and Backspace.

Decoded characters are pushed into a 128-character SPSC ring buffer. The IRQ
handler never prints or allocates memory; the normal kernel input loop drains and
echoes the buffer.

## Interactive behavior

After the timer self-test succeeds:

```text
Phase 7 input ready. Type into AxiomOS.
axiom>
```

Typing a line and pressing Enter prints:

```text
Keyboard line: hello
Keyboard IRQs/scancodes/chars/dropped: ...
```

This is an input demonstration, not the later shell.

## Validation

Run:

```bash
make test-phase7
```

The test boots q35 under TCG, requires at least ten Local APIC timer ticks, then
uses QEMU's monitor to inject the real virtual key sequence `axiom<Enter>`. AxiomOS
must reconstruct `Keyboard line: axiom` from the IRQ-driven PS/2 path.

Then run:

```bash
make test
```

for the complete Phase 1–7 regression suite.

## Current limitations

- single CPU and one address space;
- I/O APIC location/ISA GSI topology are pinned to the q35 development target;
- no ACPI MADT parser yet;
- fixed US scan-code-set-1 text mapping;
- navigation/function keys and mouse are not exposed;
- keyboard buffer is kernel-only and non-blocking;
- heap has no interrupt/SMP locking and IRQ handlers deliberately do not allocate.

```

## `docs/architecture.md`

```markdown
# Architecture at Phase 7

- Target: x86-64, one bootstrap CPU, ring 0, freestanding C/NASM.
- Toolchain: Clang `x86_64-unknown-none-elf`, LLD, NASM, Make, QEMU.
- Limine v12.9.0 loads the ELF64 higher-half kernel at `0xffffffff80000000`.
- Entry disables IRQs, selects the AxiomOS bootstrap stack, and calls C.
- Serial output is mirrored with the framebuffer terminal/custom `kprintf()`.
- GDT contains kernel code/data and a 64-bit TSS; #DF/NMI/#MC have IST stacks.
- IDT contains 256 ring-0 interrupt gates; stubs normalize error-code frames.
- #BP and software INT 0x80 return; fatal exceptions panic and halt.
- Vector 14 has a dedicated decoded page-fault panic path using CR2.
- The 8259 PIC is remapped to vectors 32–47 and remains fully masked once APIC mode is active.
- PMM manages 4 KiB frames from Limine `USABLE` regions using two bitmaps.
- VMM owns a cloned four-level page-table hierarchy and exposes 4 KiB
  `map_page()`, `unmap_page()`, and `virt_to_phys()` APIs.
- The kernel heap begins at `0xFFFFC00000000000`, grows through PMM/VMM pages,
  and provides `kmalloc()`, `kcalloc()`, `krealloc()`, and `kfree()`.
- Heap blocks use first-fit splitting/coalescing plus header cookies and tail
  guards. Heap pages are grow-only in this milestone.
- Phase 7 enables the Local APIC, calibrates its periodic timer against polled PIT channel 2, and uses vector 32 for 100 Hz ticks.
- Phase 7 initializes the first PS/2 controller port and installs an IRQ1
  keyboard handler.
- The legacy PIC remains masked; the I/O APIC routes PS/2 keyboard GSI 1 to vector 33 on the bootstrap CPU.
- IF is enabled only after timer and keyboard handlers are registered.
- IRQ handlers do minimal work: the timer increments a tick counter; the
  keyboard decodes scan codes into a fixed 128-character ring buffer.
- Terminal/framebuffer printing happens outside interrupt context.
- The normal kernel idles with `hlt` and wakes on hardware interrupts.
- The kernel remains single-address-space, single-CPU, ring-0-only.
- No scheduler, ring 3, filesystem, disk driver, mouse, or networking exists.

Phase 8 can use the periodic timer interrupt as the hardware foundation for a
round-robin scheduler and context switching.

```

## `docs/project-state.md`

```markdown
# AxiomOS handoff — Phase 7 implementation

Development environment: Linux/Distrobox on Steam Deck. Run commands from the
directory containing `Makefile`. Do not restart from earlier phases.

Roadmap: Phase 0 through Phase 25, 26 phases including setup.

Earlier accepted foundation:

- Phase 0: development environment.
- Phase 1: bootable x86-64 ELF64 kernel and serial output.
- Phase 2: framebuffer terminal and custom formatter.
- Phase 3: GDT/TSS, IDT, exception/interrupt state and PIC baseline.
- Phase 4: tested 4 KiB bitmap physical page allocator.

Phase 5 provides PMM-owned four-level page tables, CR3 ownership,
`map_page()`/`unmap_page()`/`virt_to_phys()`, TLB invalidation, and decoded page
fault diagnostics.

Phase 6 provides a higher-half PMM/VMM-backed heap with first-fit splitting,
immediate coalescing, `kmalloc()`/`kcalloc()`/`krealloc()`/`kfree()`, and
header/tail corruption checks.

Phase 7 adds:

- `drivers/timer/apic_timer.c` and `include/axiom/drivers/timer.h`;
- `drivers/keyboard/ps2_keyboard.c` and `include/axiom/drivers/keyboard.h`;
- explicit `pic_mask_irq()` / `pic_unmask_irq()` APIs;
- explicit interrupt enable/disable/status helpers;
- Local APIC periodic timer at 100 Hz, calibrated with polled PIT channel 2;
- monotonic 64-bit APIC timer tick count and `timer_wait_ticks()`;
- xAPIC MMIO initialization plus I/O APIC routing for the q35 platform;
- i8042 first-port initialization with PS/2 scanning enabled;
- IRQ1 scan-code-set-1 decoding for normal text keys;
- Shift and Caps Lock handling;
- 128-character SPSC keyboard ring buffer plus diagnostic counters;
- terminal backspace erase support;
- an interrupt-driven kernel input loop using `hlt` while idle;
- `tests/phase7_devices.py`, which injects real QEMU virtual key events and
  requires AxiomOS to reconstruct `axiom` from IRQ1 input.

Important current limitations:

- Single address space and single CPU.
- q35 uses Local APIC + I/O APIC now; ACPI MADT discovery and SMP-aware routing remain later.
- Keyboard layout is fixed US set-1 text; navigation keys and mouse are absent.
- Heap has no interrupt/SMP locking. Current IRQ handlers never allocate.
- Heap remains first-fit O(n) and grow-only.
- `map_page()` does not split existing huge-page mappings.
- No bootloader-reclaimable memory reclamation yet.
- No userspace, scheduler, filesystem, disk driver, or networking.

Acceptance commands:

```bash
make clean
make
make test-phase5
make test-phase6
make test-phase7
make test
```

Phase 7 is complete only after the local QEMU timer/keyboard test and the full
Phase 1–7 regression suite pass.

Next milestone: Phase 8 — multitasking, task state, context switching, and a
first round-robin scheduler driven by the Phase-7 timer interrupt.

```

## `docs/validation.md`

```markdown
# Validation status — Phase 7

## Previously observed local baseline

The user's earlier QEMU regression accepted Phases 1–4. The Phase-4 PMM test
reported 65,094 usable pages, 4 metadata/allocated pages, and 65,090 free pages
with 256 MiB configured for QEMU. All five Phase-3 CPU cases continued to pass.

Phase 5 and Phase 6 packages added paging and kernel-heap functionality with
local QEMU acceptance commands included in their phase documentation.

## Phase-7 static validation performed before packaging

- Every C translation unit compiles with the project's freestanding Clang
  options and `-Wall -Wextra -Werror`.
- Timer/keyboard driver headers and sources compile without host libc.
- Existing deliberate fault-mode kernel C variants compile cleanly.
- Phase 3 through Phase 7 Python tests pass Python syntax/bytecode checks.
- Phase 1/2 shell tests pass `bash -n` syntax validation.
- The Phase-7 QEMU test injects virtual key events through the QEMU monitor,
  rather than directly calling the decoder in kernel code.

## Required local acceptance

This packaging environment does not provide QEMU or NASM, so runtime acceptance
must occur in the user's kernel-development container:

```bash
make clean
make
make test-phase7
make test
```

`make test-phase7` must demonstrate:

1. the Local APIC timer delivers at least ten ticks at the configured 100 Hz rate;
2. IRQ1 receives QEMU PS/2 keyboard events;
3. scan codes decode into buffered characters;
4. `axiom<Enter>` becomes `Keyboard line: axiom`;
5. the input ring buffer reports zero dropped characters in this test;
6. no kernel panic occurs.

Phase 7 is accepted only after that real QEMU boot passes and the complete
Phase 1–7 regression remains green.

```
