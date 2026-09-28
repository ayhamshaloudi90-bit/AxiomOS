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

    /* Read a boot-created file through open/read/stat/close. */
    leaq motd_path(%rip), %rdi
    movl $AXIOM_O_RDONLY, %esi
    call open
    testq %rax, %rax
    js phase12_fail
    movl %eax, %r12d

    leaq motd_path(%rip), %rdi
    leaq phase12_stat_buffer(%rip), %rsi
    call stat
    testq %rax, %rax
    js phase12_fail
    cmpq $0, phase12_stat_buffer(%rip)
    je phase12_fail

    movl %r12d, %edi
    leaq phase12_read_buffer(%rip), %rsi
    movl $96, %edx
    call read
    testq %rax, %rax
    jle phase12_fail
    movq %rax, %r14

    movl %r12d, %edi
    call close
    testq %rax, %rax
    js phase12_fail

    movl $1, %edi
    leaq motd_prefix(%rip), %rsi
    movl $(motd_prefix_end - motd_prefix), %edx
    call write

    movl $1, %edi
    leaq phase12_read_buffer(%rip), %rsi
    movq %r14, %rdx
    call write

    /* Create a file on the separately mounted /tmp RAM filesystem. */
    leaq temp_path(%rip), %rdi
    movl $(AXIOM_O_CREAT | AXIOM_O_RDWR | AXIOM_O_TRUNC), %esi
    call open
    testq %rax, %rax
    js phase12_fail
    movl %eax, %r13d

    movl %r13d, %edi
    leaq roundtrip_message(%rip), %rsi
    movl $(roundtrip_message_end - roundtrip_message), %edx
    call write
    cmpq $(roundtrip_message_end - roundtrip_message), %rax
    jne phase12_fail

    movl %r13d, %edi
    xorq %rsi, %rsi
    movl $AXIOM_SEEK_SET, %edx
    call lseek
    testq %rax, %rax
    jne phase12_fail

    movl %r13d, %edi
    leaq phase12_roundtrip_buffer(%rip), %rsi
    movl $(roundtrip_message_end - roundtrip_message), %edx
    call read
    cmpq $(roundtrip_message_end - roundtrip_message), %rax
    jne phase12_fail
    movq %rax, %r15

    movl %r13d, %edi
    call close
    testq %rax, %rax
    js phase12_fail

    leaq temp_path(%rip), %rdi
    leaq phase12_temp_stat(%rip), %rsi
    call stat
    testq %rax, %rax
    js phase12_fail
    cmpq $(roundtrip_message_end - roundtrip_message), phase12_temp_stat(%rip)
    jne phase12_fail

    movl $1, %edi
    leaq roundtrip_prefix(%rip), %rsi
    movl $(roundtrip_prefix_end - roundtrip_prefix), %edx
    call write

    movl $1, %edi
    leaq phase12_roundtrip_buffer(%rip), %rsi
    movq %r15, %rdx
    call write

    /* Prove exec() now obtains the target through the VFS path. */
    leaq exec_path(%rip), %rdi
    call exec

    /* A successful exec replaces this image and never returns here. */
phase12_fail:
    movl $2, %edi
    leaq failure_message(%rip), %rsi
    movl $(failure_message_end - failure_message), %edx
    call write

    movl $92, %edi
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

.global exec
exec:
    movl $AXIOM_SYS_EXEC, %eax
    syscall
    ret

.global _exit
_exit:
    movl $AXIOM_SYS_EXIT, %eax
    syscall
    ud2

.section .rodata,"a",@progbits
motd_path:
    .asciz "/etc/motd"
temp_path:
    .asciz "/tmp/phase12.txt"
exec_path:
    .asciz "/bin/phase11-demo"

motd_prefix:
    .ascii "Phase 12 VFS motd: "
motd_prefix_end:

roundtrip_prefix:
    .ascii "Phase 12 VFS readback: "
roundtrip_prefix_end:

roundtrip_message:
    .ascii "RAM filesystem round-trip works!\n"
roundtrip_message_end:

failure_message:
    .ascii "Phase 12 userspace VFS demo: FAILED\n"
failure_message_end:

.section .data,"aw",@progbits
.quad 0

.section .bss,"aw",@nobits
.p2align 3
phase12_stat_buffer:
    .skip 16
phase12_temp_stat:
    .skip 16
phase12_read_buffer:
    .skip 128
phase12_roundtrip_buffer:
    .skip 128

.section .note.GNU-stack,"",@progbits
