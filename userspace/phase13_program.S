#include <axiom/abi/fs.h>
#include <axiom/abi/syscall.h>

.section .text.start,"ax",@progbits
.code64
.global _start
.type _start, @function
_start:
    movw $0x1B, %ax
    movw %ax, %ds
    movw %ax, %es

    leaq persist_path(%rip), %rdi
    movl $(AXIOM_O_CREAT | AXIOM_O_RDWR), %esi
    call open
    testq %rax, %rax
    js phase13_fail
    movl %eax, %r12d

    leaq persist_path(%rip), %rdi
    leaq persist_stat(%rip), %rsi
    call stat
    testq %rax, %rax
    js phase13_fail

    cmpq $0, persist_stat(%rip)
    jne phase13_read

    movl %r12d, %edi
    leaq persistent_message(%rip), %rsi
    movl $(persistent_message_end - persistent_message), %edx
    call write
    cmpq $(persistent_message_end - persistent_message), %rax
    jne phase13_fail

phase13_read:
    movl %r12d, %edi
    xorq %rsi, %rsi
    movl $AXIOM_SEEK_SET, %edx
    call lseek
    testq %rax, %rax
    jne phase13_fail

    movl %r12d, %edi
    leaq persist_buffer(%rip), %rsi
    movl $(persistent_message_end - persistent_message), %edx
    call read
    cmpq $(persistent_message_end - persistent_message), %rax
    jne phase13_fail

    leaq persist_buffer(%rip), %rsi
    leaq persistent_message(%rip), %rdi
    movl $(persistent_message_end - persistent_message), %ecx
phase13_compare:
    movb (%rsi), %al
    cmpb (%rdi), %al
    jne phase13_fail
    incq %rsi
    incq %rdi
    decl %ecx
    jne phase13_compare

    movl %r12d, %edi
    call close
    testq %rax, %rax
    js phase13_fail

    movl $1, %edi
    leaq readback_prefix(%rip), %rsi
    movl $(readback_prefix_end - readback_prefix), %edx
    call write

    movl $1, %edi
    leaq persist_buffer(%rip), %rsi
    movl $(persistent_message_end - persistent_message), %edx
    call write

    movl $13, %edi
    call _exit
    ud2

phase13_fail:
    movl $2, %edi
    leaq failure_message(%rip), %rsi
    movl $(failure_message_end - failure_message), %edx
    call write
    movl $93, %edi
    call _exit
    ud2
.size _start, .-_start

.section .text.syswrap,"ax",@progbits
.global write
write:
    movl $AXIOM_SYS_WRITE, %eax
    syscall
    ret
.global read
read:
    movl $AXIOM_SYS_READ, %eax
    syscall
    ret
.global open
open:
    movl $AXIOM_SYS_OPEN, %eax
    syscall
    ret
.global close
close:
    movl $AXIOM_SYS_CLOSE, %eax
    syscall
    ret
.global lseek
lseek:
    movl $AXIOM_SYS_LSEEK, %eax
    syscall
    ret
.global stat
stat:
    movl $AXIOM_SYS_STAT, %eax
    syscall
    ret
.global _exit
_exit:
    movl $AXIOM_SYS_EXIT, %eax
    syscall
    ud2

.section .rodata,"a",@progbits
persist_path:
    .asciz "/disk/persist.txt"
readback_prefix:
    .ascii "Phase 13 diskfs readback: "
readback_prefix_end:
persistent_message:
    .ascii "Persistent storage works across AxiomOS reboots!\n"
persistent_message_end:
failure_message:
    .ascii "Phase 13 userspace disk demo: FAILED\n"
failure_message_end:

.section .data,"aw",@progbits
.quad 0

.section .bss,"aw",@nobits
.p2align 3
persist_stat:
    .skip 16
persist_buffer:
    .skip 128

.section .note.GNU-stack,"",@progbits
