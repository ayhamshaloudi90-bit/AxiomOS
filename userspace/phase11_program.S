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
