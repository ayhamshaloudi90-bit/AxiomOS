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
