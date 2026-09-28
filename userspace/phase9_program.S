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
