# Phase 3 — complete new and modified implementation files

These are complete files, compared with Ayham's uploaded Phase 2 ZIP.
Concepts, installation, expected output, and debugging: see phase3.md.

## .gitignore

```text
build/

third_party/limine/

*.o
*.iso
*.hdd

*.log

/build-*/
__pycache__/

```

## Makefile

```makefile
SHELL := /bin/bash


PROJECT := AxiomOS

MODE ?= normal
BUILD_DIR := build$(if $(filter-out normal,$(MODE)),-$(MODE))
VALID_MODES := normal divide invalid gp double_fault
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
	rm -rf $(BUILD_DIR)


distclean: clean
	rm -rf $(LIMINE_DIR)


help:
	@echo "AxiomOS Phase 3 targets:"
	@echo
	@echo "  make          Build kernel and bootable ISO"
	@echo "  make run      Boot AxiomOS using QEMU"
	@echo "  make debug    Boot QEMU paused for GDB"
	@echo "  make test     Run Phase 1 + Phase 2 + Phase 3 tests"
	@echo "  make inspect  Inspect generated ELF64 kernel"
	@echo "  make clean    Remove generated build files"
	@echo "  make distclean Remove build files and Limine"

.PHONY: test-phase3

test-phase3:
	python3 tests/phase3_cpu.py

# Track C header changes too; each fault mode has its own object directory.
-include $(C_OBJECTS:.o=.d)

```

## README.md

````markdown
# AxiomOS

A freestanding x86-64 hobby kernel in C and NASM, built with Clang/LLD and
booted by Limine v12.9.0 under QEMU. Higher-half ELF64; no host libc.

Phases 0–2 are complete. Phase 3 source adds GDT/TSS/IST, a 256-entry IDT,
exception diagnostics, PIC remapping/masking, and CPU tests. See
[the Phase 3 walkthrough](docs/phase3.md), [complete changed source](docs/phase3-complete-source.md),
and [validation record](docs/validation.md).

```bash
make
make run
make test
make MODE=divide run
make MODE=invalid debug
```

Python 3 is needed for Phase 3 tests. `make run` stays the normal build;
`MODE=divide`, `invalid`, `gp` and `double_fault` use separate build directories.
No keyboard input or shell yet. Hardware IRQs stay masked and IF stays clear.

[Architecture](docs/architecture.md) | [Project state](docs/project-state.md) |
[Original roadmap](docs/roadmap.md)

````

## arch/x86_64/boot/entry.asm

```nasm
bits 64
default rel

section .text

global _start
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

## arch/x86_64/cpu/gdt.c

```c
#include <stddef.h>
#include <stdint.h>
#include <axiom/arch/gdt.h>

/* Long-mode TSS: stack pointers, not a hardware task-switch context. */
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

/* Null, ring-0 code, ring-0 data, and a two-slot TSS descriptor. */
static uint64_t gdt[5] __attribute__((aligned(16)));
static struct task_state_segment tss;
static uint8_t emergency_stacks[3][16384] __attribute__((aligned(16)));
extern uint8_t kernel_stack_top[];
extern void gdt_load(const struct descriptor_pointer *pointer);

void gdt_init(void)
{
    const uint64_t base = (uint64_t)(uintptr_t)&tss;
    const uint64_t limit = sizeof(tss) - 1u;
    gdt[0] = 0;
    gdt[1] = 0x00AF9A000000FFFFULL; /* Present, executable, L=1, D=0. */
    gdt[2] = 0x00CF92000000FFFFULL; /* Present, writable data, L=0. */
    gdt[3] = (limit & 0xFFFFu)
           | ((base & 0xFFFFFFu) << 16)
           | (0x89ULL << 40) /* Present available 64-bit TSS. */
           | (((limit >> 16) & 0xFu) << 48)
           | (((base >> 24) & 0xFFu) << 56);
    gdt[4] = base >> 32;
    tss.rsp[0] = (uint64_t)(uintptr_t)kernel_stack_top;
    for (uint8_t i = 0; i < 3; ++i) {
        tss.ist[i] = (uint64_t)(uintptr_t)&emergency_stacks[i][16384];
    }
    /* No I/O permission bitmap; its offset is beyond the descriptor limit. */
    tss.iomap_base = sizeof(tss);
    const struct descriptor_pointer pointer = {
        .limit = sizeof(gdt) - 1u,
        .base = (uint64_t)(uintptr_t)gdt
    };
    gdt_load(&pointer);
}

int gdt_ist_contains(uint8_t index, uintptr_t address)
{
    if (index < 1 || index > 3) return 0;
    const uintptr_t begin = (uintptr_t)emergency_stacks[index - 1];
    return address >= begin && address < begin + 16384u;
}

```

## arch/x86_64/cpu/gdt_load.asm

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
    mov ax, 0x18
    ltr ax
    ret
section .note.GNU-stack noalloc noexec nowrite progbits

```

## arch/x86_64/cpu/register_probe.asm

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
    mov eax, 0x28             ; Index 5 lies beyond our five-slot GDT.
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

## arch/x86_64/cpu/selftest.c

```c
#include <stdint.h>
#include <axiom/arch/interrupts.h>
#include <axiom/terminal/kprintf.h>

extern uint64_t phase3_register_probe(void);
extern void phase3_trigger_divide(void);
extern void phase3_trigger_invalid(void);
extern void phase3_trigger_gp(void);

void phase3_selftest(void)
{
    /* Assembly checks all 15 saved GPRs, RSP and RFLAGS after IRETQ. */
    if (phase3_register_probe() != 1) {
        kprintf("Phase 3 register preservation: FAILED\n");
        __asm__ volatile ("ud2");
    }
    kprintf("Phase 3 register preservation: OK\n");
    __asm__ volatile ("int $0x80" ::: "memory");
    kprintf("Phase 3 CPU initialization complete.\n");
#if defined(AXIOM_TEST_DIVIDE)
    kprintf("Test: triggering real divide-by-zero.\n");
    phase3_trigger_divide();
#elif defined(AXIOM_TEST_INVALID)
    kprintf("Test: triggering UD2.\n");
    phase3_trigger_invalid();
#elif defined(AXIOM_TEST_GP)
    kprintf("Test: triggering invalid segment selector.\n");
    phase3_trigger_gp();
#elif defined(AXIOM_TEST_DOUBLE_FAULT)
    extern void phase3_disable_gp_gate(void);
    kprintf("Test: triggering double fault.\n");
    phase3_disable_gp_gate();
    phase3_trigger_gp();
#endif
}

```

## arch/x86_64/interrupts/idt.c

```c
#include <stdint.h>
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
    __asm__ volatile ("cli" ::: "memory");
    for (uint16_t vector = 0; vector < 256; ++vector) {
        const uintptr_t address = (uintptr_t)isr_stub_table[vector];
        struct idt_entry *entry = &idt[vector];
        entry->offset_low = (uint16_t)address;
        entry->selector = GDT_KERNEL_CODE;
        entry->ist = vector == 8 ? 1 : vector == 2 ? 2 : vector == 18 ? 3 : 0;
        entry->attributes = 0x8E; /* P=1, DPL=0, 64-bit interrupt gate. */
        entry->offset_middle = (uint16_t)(address >> 16);
        entry->offset_high = (uint32_t)(address >> 32);
        entry->reserved = 0;
    }
    const struct idt_pointer pointer = {
        .limit = sizeof(idt) - 1u,
        .base = (uint64_t)(uintptr_t)idt
    };
    __asm__ volatile ("lidt %0" : : "m"(pointer) : "memory");
    pic_init_masked();
}

int irq_register(uint8_t irq, irq_handler_t handler)
{
    uint64_t flags;
    __asm__ volatile ("pushfq; popq %0" : "=r"(flags));
    if (irq >= 16 || handler == 0 || (flags & (1ULL << 9)) != 0) return -1;
    irq_handlers[irq] = handler;
    return 0;
}

void interrupt_dispatch(struct interrupt_frame *frame)
{
    if (frame->vector == 3) {
        /* INT3 is a trap: saved RIP already points after the instruction. */
        kprintf("Breakpoint: resumed safely.\n");
        return;
    }
    if (frame->vector < 32) exception_panic(frame);
    if (frame->vector < 48) {
        const uint8_t irq = (uint8_t)(frame->vector - 32);
        if (pic_is_spurious(irq)) return;
        if (irq_handlers[irq] != 0) irq_handlers[irq](frame);
        pic_eoi(irq);
        return;
    }
    /* A dedicated kernel-only software test vector; no PIC EOI. */
    if (frame->vector == 0x80) {
        kprintf("Software interrupt 0x80: returned safely.\n");
        return;
    }
    exception_panic(frame);
}

#ifdef AXIOM_TEST_DOUBLE_FAULT
/* Test-only: make #GP delivery fail with #NP, provoking a genuine #DF. */
void phase3_disable_gp_gate(void)
{
    idt[13].attributes &= (uint8_t)~0x80u;
}
#endif

```

## arch/x86_64/interrupts/isr_stubs.asm

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
    mov rbx, rsp            ; Callee-saved register holds unaligned frame base.
    and rsp, -16            ; System V alignment immediately before CALL.
    call interrupt_dispatch
    mov rsp, rbx
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

## arch/x86_64/interrupts/pic.c

```c
#include <axiom/arch/io.h>
#include <axiom/arch/pic.h>

#define PIC1_COMMAND 0x20u
#define PIC1_DATA 0x21u
#define PIC2_COMMAND 0xA0u
#define PIC2_DATA 0xA1u

static void pic_write(uint16_t port, uint8_t value)
{
    outb(port, value);
    outb(0x80, 0); /* Traditional I/O delay for the legacy controller. */
}

void pic_init_masked(void)
{
    pic_write(PIC1_DATA, 0xFF);
    pic_write(PIC2_DATA, 0xFF);
    pic_write(PIC1_COMMAND, 0x11); /* ICW1: initialize, ICW4 follows. */
    pic_write(PIC2_COMMAND, 0x11);
    pic_write(PIC1_DATA, 0x20);    /* ICW2: master vectors 32..39. */
    pic_write(PIC2_DATA, 0x28);    /* Slave vectors 40..47. */
    pic_write(PIC1_DATA, 0x04);    /* ICW3: slave wired to master IRQ2. */
    pic_write(PIC2_DATA, 0x02);
    pic_write(PIC1_DATA, 0x01);    /* ICW4: 8086 mode, explicit EOI. */
    pic_write(PIC2_DATA, 0x01);
    pic_write(PIC1_DATA, 0xFF);    /* No device drivers yet. */
    pic_write(PIC2_DATA, 0xFF);
}

int pic_is_spurious(uint8_t irq)
{
    if (irq == 7) {
        outb(PIC1_COMMAND, 0x0B); /* OCW3: read in-service register. */
        return (inb(PIC1_COMMAND) & 0x80u) == 0;
    }
    if (irq == 15) {
        outb(PIC2_COMMAND, 0x0B);
        if ((inb(PIC2_COMMAND) & 0x80u) == 0) {
            /* Slave did not accept IRQ15, but master serviced cascade. */
            outb(PIC1_COMMAND, 0x20);
            return 1;
        }
    }
    return 0;
}

void pic_eoi(uint8_t irq)
{
    if (irq >= 8) outb(PIC2_COMMAND, 0x20);
    outb(PIC1_COMMAND, 0x20);
}

```

## include/axiom/arch/gdt.h

```c
#ifndef AXIOM_ARCH_GDT_H
#define AXIOM_ARCH_GDT_H
#include <stdint.h>
#define GDT_KERNEL_CODE 0x08u
#define GDT_KERNEL_DATA 0x10u
#define GDT_TSS_SELECTOR 0x18u
void gdt_init(void);
int gdt_ist_contains(uint8_t index, uintptr_t address);
#endif

```

## include/axiom/arch/interrupts.h

```c
#ifndef AXIOM_ARCH_INTERRUPTS_H
#define AXIOM_ARCH_INTERRUPTS_H
#include <stddef.h>
#include <stdint.h>

/* Exact layout made by isr_stubs.asm, followed by the long-mode CPU frame.
 * In 64-bit mode SS/RSP are pushed even without a privilege change.
 * General registers are saved by us; vector/error are normalized by stubs.
 */
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

typedef void (*irq_handler_t)(struct interrupt_frame *frame);
void interrupts_init(void);
/* Install while IF=0. Registration does not unmask a hardware IRQ. */
int irq_register(uint8_t irq, irq_handler_t handler);
void interrupt_dispatch(struct interrupt_frame *frame);
void phase3_selftest(void);
_Noreturn void exception_panic(const struct interrupt_frame *frame);
#endif

```

## include/axiom/arch/pic.h

```c
#ifndef AXIOM_ARCH_PIC_H
#define AXIOM_ARCH_PIC_H
#include <stdint.h>
void pic_init_masked(void);
int pic_is_spurious(uint8_t irq);
void pic_eoi(uint8_t irq);
#endif

```

## kernel/core/kernel.c

```c
#include <stdint.h>
#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>

#include <axiom/boot/limine.h>
#include <axiom/drivers/serial.h>

#include <axiom/terminal/kprintf.h>
#include <axiom/terminal/terminal.h>


void kernel_main(void)
{
    uint64_t line;
    uint64_t scroll_lines;


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
     * Keep this exact line so the Phase-1 regression
     * test continues to work.
     */
    serial_write_string(
        "AxiomOS kernel booted successfully.\n"
    );


    /*
     * Initialise our new Phase-2 framebuffer terminal.
     */
    if (!terminal_init()) {
        serial_write_string(
            "AxiomOS terminal error: "
            "framebuffer terminal unavailable.\n"
        );

        return;
    }


    /*
     * Exercise scrolling.
     *
     * Print slightly more than one screen of text.
     */
    scroll_lines =
        (uint64_t)terminal_rows()
        +
        3ULL;


    for (
        line = 1;
        line <= scroll_lines;
        ++line
    ) {

        kprintf(
            "Scroll exercise line %llu\n",
            (unsigned long long)line
        );
    }


    /*
     * Scrolling was exercised.
     * Clear the display and show the proper Phase-2
     * boot screen.
     */
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

        (unsigned long long)
            terminal_columns(),

        (unsigned long long)
            terminal_rows()
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
    /* Phase 3 uses the Phase 2 terminal for CPU diagnostics. */
    gdt_init();
    interrupts_init();
    kprintf("GDT/TSS loaded. IDT: 256 gates installed.\n");
    kprintf("PIC remapped: IRQs masked; IF=0.\n");
    phase3_selftest();
}

```

## kernel/core/panic.c

```c
#include <stdint.h>
#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/drivers/serial.h>
#include <axiom/terminal/kprintf.h>
#include <axiom/terminal/terminal.h>

static const char *const exception_names[32] = {
    "Divide by Zero", "Debug", "Non-Maskable Interrupt", "Breakpoint",
    "Overflow", "BOUND Range Exceeded", "Invalid Opcode", "Device Not Available",
    "Double Fault", "Coprocessor Segment Overrun", "Invalid TSS", "Segment Not Present",
    "Stack-Segment Fault", "General Protection Fault", "Page Fault", "Reserved",
    "x87 Floating-Point Exception", "Alignment Check", "Machine Check", "SIMD Exception",
    "Virtualization Exception", "Control Protection Exception", "Reserved", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Hypervisor Injection Exception",
    "VMM Communication Exception", "Security Exception", "Reserved"
};
static volatile uint32_t panicking;

static _Noreturn void halt_cpu(void)
{
    for (;;) __asm__ volatile ("cli; hlt" ::: "memory");
}

_Noreturn void exception_panic(const struct interrupt_frame *frame)
{
    uint64_t cr2;
    __asm__ volatile ("cli; mov %%cr2, %0" : "=r"(cr2) : : "memory");
    /* Single-core only. Avoid recursively re-entering framebuffer output. */
    if (panicking) {
        serial_write_string("NESTED KERNEL PANIC: CPU halted.\n");
        halt_cpu();
    }
    panicking = 1;
    terminal_clear();
    terminal_set_color(TERMINAL_COLOR_LIGHT_RED, TERMINAL_COLOR_BLACK);
    kprintf("\nKERNEL PANIC\n");
    kprintf("Exception: %s\n", frame->vector < 32 ? exception_names[frame->vector]
                                              : "Unexpected Interrupt");
    kprintf("Vector: %llu  Error: 0x%llX\n",
            (unsigned long long)frame->vector, (unsigned long long)frame->error_code);
#define REG(label, field) kprintf(label ": 0x%llX\n", (unsigned long long)frame->field)
    REG("RIP", rip);
    REG("RSP", rsp);
    REG("RFLAGS", rflags);
    kprintf("CS: 0x%llX  SS: 0x%llX\n", (unsigned long long)frame->cs,
            (unsigned long long)frame->ss);
    REG("RAX", rax); REG("RBX", rbx); REG("RCX", rcx); REG("RDX", rdx);
    REG("RSI", rsi); REG("RDI", rdi); REG("RBP", rbp);
    REG("R8", r8); REG("R9", r9); REG("R10", r10); REG("R11", r11);
    REG("R12", r12); REG("R13", r13); REG("R14", r14); REG("R15", r15);
#undef REG
    if (frame->vector == 14) kprintf("CR2: 0x%llX\n", (unsigned long long)cr2);
    if (frame->vector == 8) {
        kprintf("Double-fault IST stack: %s\n",
                gdt_ist_contains(1, (uintptr_t)frame) ? "OK" : "FAILED");
    }
    kprintf("CPU halted.\n");
    halt_cpu();
}

```

## tests/phase3_cpu.py

```python
#!/usr/bin/env python3
"""Boot real ISOs; check return-state tests and independently located fault RIPs."""
from pathlib import Path
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
CASES = [
    ('normal', None, None, None, None),
    ('divide', 'Divide by Zero', 0, 0, 'phase3_divide_fault'),
    ('invalid', 'Invalid Opcode', 6, 0, 'phase3_invalid_fault'),
    ('gp', 'General Protection Fault', 13, 0x28, 'phase3_gp_fault'),
    ('double_fault', 'Double Fault', 8, 0, None),
]


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def symbol_address(elf, name):
    output = subprocess.check_output(['llvm-nm', str(elf)], text=True)
    for line in output.splitlines():
        fields = line.split()
        if len(fields) == 3 and fields[2] == name:
            return int(fields[0], 16)
    raise AssertionError(f'Missing ELF symbol {name}')


def run_case(mode, name, vector, error, fault_symbol):
    directory = ROOT / ('build' if mode == 'normal' else f'build-{mode}')
    directory.mkdir(exist_ok=True)
    with (directory / 'phase3-build.log').open('w') as output:
        subprocess.run(['make', f'MODE={mode}', 'all'], cwd=ROOT,
                       stdout=output, stderr=subprocess.STDOUT, check=True)
    serial = directory / 'phase3-serial.log'
    serial.write_text('')
    marker = 'Phase 3 CPU initialization complete.' if name is None else 'CPU halted.'
    command = ['qemu-system-x86_64', '-machine', 'q35', '-accel', 'tcg',
               '-m', '256M', '-cdrom', str(directory / 'AxiomOS.iso'),
               '-boot', 'd', '-display', 'none', '-serial', f'file:{serial}',
               '-monitor', 'none', '-no-reboot', '-no-shutdown']
    with (directory / 'phase3-qemu.log').open('w') as output:
        process = subprocess.Popen(command, stdout=output, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic() + 20
            while marker not in serial.read_text(errors='replace'):
                require(process.poll() is None, f'{mode}: QEMU exited before completion')
                require(time.monotonic() < deadline, f'{mode}: boot timed out; see {serial}')
                time.sleep(0.05)
        finally:
            if process.poll() is None:
                process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
    text = serial.read_text().replace('\r', '')
    for expected in ['Phase 2 terminal test complete.',
                     'GDT/TSS loaded. IDT: 256 gates installed.',
                     'Breakpoint: resumed safely.',
                     'Phase 3 register preservation: OK',
                     'Software interrupt 0x80: returned safely.',
                     'Phase 3 CPU initialization complete.']:
        require(expected in text, f'{mode}: missing {expected}')
    require('FAILED' not in text and 'NESTED KERNEL PANIC' not in text,
            f'{mode}: internal failure')
    if name is None:
        require('KERNEL PANIC' not in text, 'Normal boot panicked')
    else:
        require(text.count('KERNEL PANIC') == 1, f'{mode}: missing/repeated panic')
        require(f'Exception: {name}\n' in text, f'{mode}: wrong exception')
        require(f'Vector: {vector}  Error: 0x{error:X}\n' in text,
                f'{mode}: wrong vector/error code')
        registers = {key: int(value, 16) for key, value in
                     re.findall(r'^(R[A-Z0-9]+): 0x([0-9A-F]+)$', text, re.M)}
        for key in ['RIP', 'RSP', 'RFLAGS', 'RAX', 'RBX', 'RCX', 'RDX', 'RSI',
                    'RDI', 'RBP', 'R8', 'R9', 'R10', 'R11', 'R12', 'R13', 'R14', 'R15']:
            require(key in registers, f'{mode}: missing {key}')
        if fault_symbol:
            expected_rip = symbol_address(directory / 'AxiomOS.elf', fault_symbol)
            require(registers['RIP'] == expected_rip, f'{mode}: incorrect saved RIP')
        bottom = symbol_address(directory / 'AxiomOS.elf', 'kernel_stack_bottom')
        top = symbol_address(directory / 'AxiomOS.elf', 'kernel_stack_top')
        if mode != 'double_fault':
            require(bottom <= registers['RSP'] < top, f'{mode}: invalid interrupted RSP')
        require(registers['RFLAGS'] & 2, f'{mode}: invalid RFLAGS reserved bit')
        require((registers['RFLAGS'] & 0x200) == 0, f'{mode}: IF unexpectedly set')
        require('CS: 0x8  SS: 0x10' in text, f'{mode}: wrong segment state')
        if mode == 'divide':
            require(registers['RAX'] == 123 and registers['RCX'] == 0
                    and registers['RDX'] == 0, 'Divide operands not preserved')
        if mode == 'double_fault':
            require('Double-fault IST stack: OK' in text, 'Double fault did not use IST1')
    print(f'PASS: Phase 3 {mode}', flush=True)


if __name__ == '__main__':
    for case in CASES:
        run_case(*case)
    print('PASS: all Phase 3 CPU tests. No Phase 4 functionality added.')

```

