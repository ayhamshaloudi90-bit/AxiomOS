#include <axiom/abi/syscall.h>
.section .text.start,"ax",@progbits
.code64
.global _start
.type _start, @function
.extern phase15_main
_start:
    movw $0x1B, %ax
    movw %ax, %ds
    movw %ax, %es
    call phase15_main
    movq %rax, %rdi
    movl $AXIOM_SYS_EXIT, %eax
    syscall
    ud2
.size _start, .-_start
.section .note.GNU-stack,"",@progbits
