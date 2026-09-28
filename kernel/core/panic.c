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
