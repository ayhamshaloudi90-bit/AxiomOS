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
