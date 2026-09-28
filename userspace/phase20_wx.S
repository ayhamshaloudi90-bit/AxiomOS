.section .phase20wx,"ax",@progbits
.global _start
_start:
    mov $3, %rax
    mov $20, %rdi
    syscall
1:  hlt
    jmp 1b
.section .note.GNU-stack,"",@progbits
