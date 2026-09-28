# AxiomOS Phase 9 — complete new/modified source

This document contains the complete contents of every file added or modified for Phase 9.

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
	-m64


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

GAS_SOURCES := \
	userspace/phase9_program.S

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
	@echo "AxiomOS Phase 9 targets:"
	@echo
	@echo "  make          Build kernel and bootable ISO"
	@echo "  make run      Boot AxiomOS using QEMU"
	@echo "  make debug    Boot QEMU paused for GDB"
	@echo "  make test     Run Phase 1 through Phase 9 tests"
	@echo "  make test-phase6 Run heap + corruption tests"
	@echo "  make test-phase7 Run timer + keyboard IRQ/input tests"
	@echo "  make test-phase8 Run preemptive scheduler/context-switch tests"
	@echo "  make test-phase9 Run Ring 3 userspace/isolation tests"
	@echo "  make MODE=heap_double_free run  Trigger double-free panic"
	@echo "  make MODE=heap_guard run        Trigger tail-guard panic"
	@echo "  make inspect  Inspect generated ELF64 kernel"
	@echo "  make clean    Remove generated build files"
	@echo "  make distclean Remove build files and Limine"

.PHONY: test-phase3 test-phase4 test-phase5 test-phase6 test-phase7 test-phase8 test-phase9

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

Phases 0–8 provide boot, terminal/serial output, CPU exception handling, physical
and virtual memory management, a kernel heap, APIC-driven timer/keyboard IRQs,
and preemptive round-robin kernel-thread scheduling.

Phase 9 adds the first protected userspace:

- ring-3 code and data segments;
- TSS `RSP0` kernel-stack transitions;
- separate PML4/CR3 roots for user tasks;
- supervisor-only shared higher-half kernel mappings;
- user code/data/stack mappings with the x86 U/S permission bit;
- scheduler CR3 switching between isolated tasks;
- a first Ring-3 program that produces `Hello from AxiomOS userspace!` through a
  user-memory mailbox (syscalls intentionally remain Phase 10);
- termination of a Ring-3 task that attempts to read kernel memory;
- corrected task-stack switching through the ISR return path.

Implemented foundation now includes:

- bootable x86-64 ELF64 kernel;
- framebuffer terminal, serial mirror, and custom `kprintf()`;
- GDT/TSS, 256-entry IDT, exception stubs and panic diagnostics;
- Local APIC timer at 100 Hz and I/O APIC keyboard routing on q35;
- PS/2 scan-code decoding and buffered keyboard input;
- 4 KiB bitmap physical memory manager;
- AxiomOS-owned four-level page tables and decoded page faults;
- PMM/VMM-backed kernel heap with corruption checks;
- preemptive fixed-quantum round-robin scheduler;
- real per-task kernel stacks and saved interrupt contexts;
- Ring-3 tasks with separate user stacks and address spaces;
- hardware-enforced user/kernel memory isolation;
- automated regression tests through Phase 9.

## Common commands

```bash
make
make run
make test
make test-phase7
make test-phase8
make test-phase9
make MODE=page_fault run
make MODE=heap_double_free run
make MODE=heap_guard run
make debug
```

`make test` runs the complete Phase 1 through Phase 9 regression suite.

The system is still single-CPU. Phase 10 will add a real syscall ABI so Ring-3
programs can request kernel services instead of communicating through the
Phase-9 validation mailbox.

[Architecture](docs/architecture.md) | [Memory](docs/memory.md) |
[Tasks](docs/processes.md) | [Phase 9](docs/phase9.md) |
[Project state](docs/project-state.md) | [Original roadmap](docs/roadmap.md)

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

```

## `arch/x86_64/cpu/gdt_load.asm`

```nasm
bits 64
default rel
section .text
global gdt_load
gdt_load:
    lgdt [rdi]
    ; A far return reloads CS from our new GDT (selector 0x08).
    push qword 0x08
    lea rax, [rel .reload_cs]
    push rax
    retfq
.reload_cs:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    ; No FS/GS-based per-CPU or TLS data exists in this phase.
    xor eax, eax
    mov fs, ax
    mov gs, ax
    mov ax, 0x28
    ltr ax
    ret
section .note.GNU-stack noalloc noexec nowrite progbits

```

## `arch/x86_64/cpu/register_probe.asm`

```nasm
bits 64
default rel
section .text
global phase3_register_probe
phase3_register_probe:
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    mov [rel probe_rsp], rsp
    mov rax, 257
    mov rbx, 258
    mov rcx, 259
    mov rdx, 260
    mov rsi, 261
    mov rdi, 262
    mov rbp, 263
    mov r8, 264
    mov r9, 265
    mov r10, 266
    mov r11, 267
    mov r12, 268
    mov r13, 269
    mov r14, 270
    mov r15, 271
    ; Exercise DF restoration; the ISR must clear it before calling C.
    std
    pushfq
    pop qword [rel probe_flags]
    int3
    pushfq
    pop qword [rel returned_flags]
    cld
    cmp rax, 257
    jne .failed
    cmp rbx, 258
    jne .failed
    cmp rcx, 259
    jne .failed
    cmp rdx, 260
    jne .failed
    cmp rsi, 261
    jne .failed
    cmp rdi, 262
    jne .failed
    cmp rbp, 263
    jne .failed
    cmp r8, 264
    jne .failed
    cmp r9, 265
    jne .failed
    cmp r10, 266
    jne .failed
    cmp r11, 267
    jne .failed
    cmp r12, 268
    jne .failed
    cmp r13, 269
    jne .failed
    cmp r14, 270
    jne .failed
    cmp r15, 271
    jne .failed
    cmp rsp, [rel probe_rsp]
    jne .failed
    mov rax, [rel probe_flags]
    cmp rax, [rel returned_flags]
    jne .failed
    mov eax, 1
    jmp .done
.failed:
    xor eax, eax
.done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

global phase3_trigger_divide, phase3_divide_fault
phase3_trigger_divide:
    mov eax, 123
    xor edx, edx
    xor ecx, ecx
phase3_divide_fault:
    div rcx
    ret

global phase3_trigger_invalid, phase3_invalid_fault
phase3_trigger_invalid:
phase3_invalid_fault:
    ud2
    ret

global phase3_trigger_gp, phase3_gp_fault
phase3_trigger_gp:
    mov eax, 0x38             ; Index 7 lies beyond our seven-slot GDT.
phase3_gp_fault:
    mov ds, ax
    ret

section .bss
align 8
probe_rsp: resq 1
probe_flags: resq 1
returned_flags: resq 1
section .note.GNU-stack noalloc noexec nowrite progbits

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

## `arch/x86_64/interrupts/isr_stubs.asm`

```nasm
bits 64
default rel
section .text
extern interrupt_dispatch

%macro ISR 1
global isr_%1
isr_%1:
    ; These exceptions push a hardware error code. All others need a zero.
    ; Includes #CP (21), AMD #VC (29), and #SX (30).
%if %1 != 8 && %1 != 10 && %1 != 11 && %1 != 12 && %1 != 13 && %1 != 14 && %1 != 17 && %1 != 21 && %1 != 29 && %1 != 30
    push qword 0
%endif
    push qword %1
    jmp isr_common
%endmacro

%assign vector 0
%rep 256
    ISR vector
%assign vector vector+1
%endrep

isr_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    cld                     ; C ABI requires DF=0; IRET restores original flags.
    mov rdi, rsp            ; First C argument: interrupt_frame pointer.
    mov rbx, rsp            ; Preserve the original frame while aligning CALL.
    and rsp, -16            ; System V alignment immediately before CALL.
    call interrupt_dispatch ; RAX = frame/stack context to restore.
    mov rsp, rax            ; May switch to a different task's kernel stack.
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 16             ; Discard vector and normalized error code.
    iretq                   ; Restore RIP, CS, RFLAGS, RSP, and SS.

section .rodata
align 8
global isr_stub_table
isr_stub_table:
%assign vector 0
%rep 256
    dq isr_%+vector
%assign vector vector+1
%endrep
section .note.GNU-stack noalloc noexec nowrite progbits

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

static struct interrupt_frame *keyboard_irq_handler(struct interrupt_frame *frame)
{
    uint8_t status;

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

    return frame;
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

static struct interrupt_frame *timer_irq_handler(struct interrupt_frame *frame)
{
    ++tick_count;
    return scheduler_on_timer_interrupt(frame);
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
void gdt_set_kernel_stack(uintptr_t stack_top);
uintptr_t gdt_kernel_stack(void);
int gdt_ist_contains(uint8_t index, uintptr_t address);

#endif

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

## `include/axiom/memory/vmm.h`

```c
#ifndef AXIOM_MEMORY_VMM_H
#define AXIOM_MEMORY_VMM_H

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

_Noreturn void task_exit_current(void);

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

#include <axiom/arch/interrupts.h>
#include <axiom/memory/address.h>

#define TASK_USER_CODE_BASE   0x0000000000400000ULL
#define TASK_USER_DATA_BASE   0x0000000000401000ULL
#define TASK_USER_STACK_TOP   0x00007FFFFFF00000ULL
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
#define PHASE9_TEST_TIMEOUT_TICKS 500ULL
#define PHASE9_KERNEL_PROBE_ADDRESS 0xFFFFFFFF80000000ULL

static volatile uint64_t phase8_worker_a_count;
static volatile uint64_t phase8_worker_b_count;


extern const uint8_t phase9_user_hello_start[];
extern const uint8_t phase9_user_hello_end[];
extern const uint8_t phase9_user_fault_start[];
extern const uint8_t phase9_user_fault_end[];

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

    /* Keep the interactive kernel input demonstration alive. */
    phase7_input_loop();
}

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
    task_exit_current();
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

_Noreturn void task_exit_current(void)
{
    if (!running || !initialized || current_index >= task_count_value) {
        kernel_panic("task_exit_current called without a running scheduler");
    }

    interrupts_disable();
    tasks[current_index].state = TASK_TERMINATED;
    tasks[current_index].ticks_in_slice = tasks[current_index].quantum_ticks;

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

```

## `userspace/phase9_program.S`

```asm
.section .rodata.phase9_user,"a",@progbits
.code64

.equ USER_DATA, 0x0000000000401000
.equ USER_MESSAGE_MAGIC_OFFSET, 64
.equ USER_MESSAGE_MAGIC, 0x4158494F4D555345
.equ KERNEL_PROBE_ADDRESS, 0xFFFFFFFF80000000

.global phase9_user_hello_start
.global phase9_user_hello_end
phase9_user_hello_start:
    movw $0x1B, %ax
    movw %ax, %ds
    movw %ax, %es
    movabs $USER_DATA, %rdi
    leaq .Lhello_message(%rip), %rsi
    movl $30, %ecx
    rep movsb

    movabs $(USER_DATA + USER_MESSAGE_MAGIC_OFFSET), %rdi
    movabs $USER_MESSAGE_MAGIC, %rax
    movq %rax, (%rdi)

.Lhello_spin:
    pause
    jmp .Lhello_spin

.Lhello_message:
    .asciz "Hello from AxiomOS userspace!"
phase9_user_hello_end:

.p2align 4
.global phase9_user_fault_start
.global phase9_user_fault_end
phase9_user_fault_start:
    movw $0x1B, %ax
    movw %ax, %ds
    movw %ax, %es
    movabs $KERNEL_PROBE_ADDRESS, %rax
    movq (%rax), %rbx
.Lfault_unexpected:
    pause
    jmp .Lfault_unexpected
phase9_user_fault_end:

.section .note.GNU-stack,"",@progbits

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
                "Phase 7 input ready. Type into AxiomOS.",
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
            send_keys(monitor, ["u", "s", "e", "r", "ret"])
            wait_for_text(
                process,
                serial,
                "Keyboard line: user",
                time.monotonic() + 10,
            )

            final_text = serial.read_text(errors="replace").replace("\r", "")
            require("Keyboard line: user" in final_text,
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

## Phase 9 user address spaces

Phase 9 introduces per-user-task CR3 roots. Each user root starts with an empty
lower canonical half and shares the kernel's upper-half PML4 entries with USER
cleared. User code/data/stack mappings are created only in the lower half with
`VMM_FLAG_USER`.

New VMM interfaces include:

```c
paddr_t vmm_create_user_address_space(void);
int vmm_destroy_user_address_space(paddr_t root_table);
int vmm_map_page_in_address_space(...);
paddr_t vmm_virt_to_phys_in_address_space(...);
int vmm_activate_address_space(paddr_t root_table);
paddr_t vmm_current_address_space(void);
paddr_t vmm_kernel_address_space(void);
```

The scheduler changes CR3 when switching between tasks with different address
spaces. Because the higher-half kernel mapping is present in every user CR3,
interrupt handling can continue safely after a Ring-3 interrupt switches to the
per-task TSS kernel stack.

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

## Phase-9 context-switch refinement

Phase 9 refined the stack-switch mechanism used by the Phase-8 scheduler. In
64-bit mode, the interrupt frame includes saved `SS:RSP` even for same-CPL
interrupts, and `IRETQ` restores them. The ISR contract now returns the selected
task's saved-frame pointer; each synthetic kernel-thread frame carries a real
private-stack `RSP` and kernel `SS`. The round-robin policy remains unchanged.

```

## `docs/phase9.md`

```markdown
# Phase 9 — Ring 3 userspace and isolation

## Goal

Move AxiomOS from kernel-only multitasking to hardware-protected userspace.
Phase 9 deliberately does **not** introduce a syscall ABI; that is Phase 10.
Instead, it proves privilege transitions, isolated address spaces, separate user
stacks, and kernel protection.

## GDT/TSS privilege setup

The GDT now contains ring-0 code/data, ring-3 data/code, and a 64-bit TSS.
Selectors are:

```text
0x08  kernel code
0x10  kernel data
0x1B  user data (index 3, RPL 3)
0x23  user code (index 4, RPL 3)
0x28  TSS
```

Before returning to a Ring-3 task, the scheduler sets the TSS `RSP0` field to
that task's private kernel-stack top. If a timer interrupt or exception occurs in
Ring 3, the CPU automatically switches to that trusted Ring-0 stack before
entering the interrupt handler.

## Address spaces

Every user task receives a fresh PML4. Its lower canonical half begins empty.
The upper canonical half copies the kernel's PML4 entries so the kernel can keep
executing after CR3 changes, but the copied entries have the x86 USER bit clear.
Ring 3 therefore cannot traverse the kernel mappings.

Each Phase-9 user task maps:

```text
0x0000000000400000  user code, readable/executable, not writable
0x0000000000401000  user data, writable, NX
near 0x00007FFFFFF00000  four-page user stack, writable, NX
```

Page-table parent entries for those lower-half mappings carry the USER bit.
Kernel/HHDM/heap/APIC mappings remain supervisor-only.

## Correct task-stack switching

Phase 9 also makes the Phase-8 stack-switch path explicit and auditable. In
64-bit mode, interrupt frames include saved `SS:RSP` even without a privilege
change, and `IRETQ` restores them. Each synthetic kernel-thread frame therefore
contains a real private-stack `RSP` plus the kernel data selector in `SS`.

The ISR dispatcher now returns a pointer to the exact saved frame that should be
restored. The assembly epilogue executes:

```text
current interrupt frame
        |
        v
scheduler chooses next task
        |
        v
RAX = next task saved-frame pointer
        |
        v
RSP = RAX
        |
        v
POP registers + IRETQ
```

That means the epilogue restores the selected task's complete interrupt frame,
including its saved stack pointer. For a Ring-3 task, `IRETQ` additionally
changes CPL from 0 to 3 using the user CS/SS selectors.

## First userspace program

`userspace/phase9_program.S` contains a tiny position-independent Ring-3 image.
The scheduler copies it into a user-owned physical page and maps that page at
`0x400000`.

Because syscalls do not exist yet, the program proves execution by writing:

```text
Hello from AxiomOS userspace!
```

into its own data page and then writing a completion magic value. The kernel
reads the physical page through the HHDM after the task has run and prints the
message. This keeps the phase boundary honest: user execution exists, but the
kernel-service interface does not exist until Phase 10.

## Protection test

A second isolated Ring-3 task deliberately reads:

```text
0xFFFFFFFF80000000
```

which is inside the higher-half kernel mapping. The mapping is present but
supervisor-only, so x86 raises page fault vector 14 with the user-mode bit set in
the page-fault error code.

Instead of panicking AxiomOS, the Phase-9 exception path records the fault in the
current user task, marks that task TERMINATED, selects another READY task,
switches CR3/TSS state, and resumes scheduling.

This demonstrates that a bad user program can be stopped without taking down the
kernel.

## Acceptance

Run:

```bash
make clean
make
make test-phase9
make test
```

A successful Phase-9 test must prove:

- the first program actually enters Ring 3 and writes the expected message;
- the two user tasks have distinct CR3 roots;
- neither user CR3 equals the kernel CR3;
- the protection task receives #PF vector 14 at the kernel probe address;
- the fault error code says the access came from user mode and hit a present
  supervisor page;
- the faulting user task terminates without a kernel panic;
- scheduler and keyboard IRQ input still work afterward.

## Current limitations

- single CPU;
- fixed user virtual layout and one code page per Phase-9 program;
- no ELF loader yet;
- no syscall ABI yet;
- no user libc;
- no process-owned heap/mmap interface;
- no safe task reaper/resource reclamation for normally terminated tasks yet;
- no FPU/SSE user context because those units remain disabled by the build.

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

```

## `docs/project-state.md`

```markdown
# AxiomOS handoff — Phase 9 implementation

Development environment: Linux/Distrobox on Steam Deck. Run commands from the
directory containing `Makefile`. Do not restart from earlier phases.

Roadmap: Phase 0 through Phase 25, 26 phases including setup.

Accepted foundation:

- Phase 0: development environment.
- Phase 1: bootable x86-64 ELF64 kernel and serial output.
- Phase 2: framebuffer terminal and custom formatter.
- Phase 3: GDT/TSS, IDT, exceptions/interrupts and panic diagnostics.
- Phase 4: tested 4 KiB bitmap physical page allocator.
- Phase 5: AxiomOS-owned four-level page tables, mapping and page faults.
- Phase 6: PMM/VMM-backed first-fit/coalescing kernel heap.
- Phase 7: q35 Local APIC 100 Hz timer plus I/O-APIC-routed PS/2 keyboard.
- Phase 8: preemptive round-robin task scheduler and kernel threads.

Phase 9 adds/refines:

- ring-3 code and data GDT descriptors;
- TSS `RSP0` updates per scheduled task;
- corrected saved-frame pointer context switching so kernel threads truly switch
  private stacks;
- fresh PML4 roots for user tasks;
- private lower-half code/data/stack mappings with USER permissions;
- shared supervisor-only upper-half kernel mappings;
- CR3 switching in the scheduler;
- 16 KiB user stacks plus separate 16 KiB trusted kernel stacks;
- first Ring-3 program writing `Hello from AxiomOS userspace!` into a mailbox;
- a Ring-3 protection task that intentionally reads kernel memory;
- user #PF termination without kernel panic;
- `tests/phase9_userspace.py` validating privilege/isolation and keyboard
  coexistence.

Important limitations:

- one CPU only;
- fixed round-robin quantum and no priorities;
- Phase-9 user images use a fixed raw one-page layout, not ELF;
- no syscalls yet (Phase 10);
- no userspace libc;
- no filesystem-backed executables yet;
- BLOCKED/SLEEPING states exist but full wait/sleep queues are later work;
- heap remains non-interrupt-safe, so task creation still occurs with IF=0;
- normal terminated-task resource reaping is not implemented yet.

Acceptance commands:

```bash
make clean
make
make test-phase8
make test-phase9
make test
```

Next milestone: Phase 10 — x86-64 syscall interface and small userspace wrappers.

```

## `docs/validation.md`

```markdown
# Validation status — Phase 9

## User-accepted baseline

The user has accepted Phase 8 on the APIC-based q35 implementation. Phase 9 is
built directly on that exact source tree.

## Static validation performed before packaging

- Every C translation unit compiles with the project's freestanding Clang target
  and `-Wall -Wextra -Werror` policy.
- Every existing deliberate fault-mode C variant still compiles.
- The new Ring-3 assembly image compiles with Clang's x86-64 assembler.
- A relocatable partial link of all C objects plus the userspace image succeeds.
- All Python regression tests through Phase 9 pass syntax/bytecode validation.
- Phase 1/2 shell tests pass `bash -n`.
- User address spaces use a private lower half and supervisor-only shared kernel
  upper half.
- User code is mapped executable/non-writable; user data and stack are writable
  and NX.
- User #PF handling terminates a Ring-3 task instead of panicking the kernel.

## Runtime acceptance required on the Steam Deck distrobox

This packaging environment does not provide working NASM/QEMU, so final runtime
acceptance must happen locally:

```bash
make clean
make
make test-phase9
make test
```

`make test-phase9` must show distinct kernel/hello/protection CR3 values, print
the userspace message, observe vector 14 for a Ring-3 read of
`0xFFFFFFFF80000000`, confirm the page-fault user/protection bits, and prove the
keyboard still works afterward.

```
