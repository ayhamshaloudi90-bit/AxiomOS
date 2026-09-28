#include <axiom/abi/syscall.h>
#include <axiom/abi/user_layout.h>

.section .rodata.phase10_user,"a",@progbits
.code64

.equ USER_DATA, AXIOM_USER_DATA_BASE
.equ KERNEL_PROBE_ADDRESS, 0xFFFFFFFF80000000

.global phase10_user_start
.global phase10_user_end
phase10_user_start:
    movw $0x1B, %ax
    movw %ax, %ds
    movw %ax, %es

    movabs $USER_DATA, %r12

    call getpid
    movq %rax, AXIOM_PHASE10_PID_OFFSET(%r12)

    movl $1, %edi
    leaq .Lhello(%rip), %rsi
    movl $(.Lhello_end - .Lhello), %edx
    call write

    /* A kernel pointer must be rejected rather than faulting Ring 0. */
    movl $1, %edi
    movabs $KERNEL_PROBE_ADDRESS, %rsi
    movl $4, %edx
    call write
    movq %rax, AXIOM_PHASE10_BAD_WRITE_OFFSET(%r12)

    /* open() has a stable ABI number but the VFS does not exist until Phase 12. */
    leaq .Lfake_path(%rip), %rdi
    xorl %esi, %esi
    call open
    movq %rax, AXIOM_PHASE10_OPEN_RESULT_OFFSET(%r12)

    call yield

    movl $30, %edi
    call sleep

    movl $1, %edi
    leaq .Lwaiting(%rip), %rsi
    movl $(.Lwaiting_end - .Lwaiting), %edx
    call write

    movl $3, %r13d
.Lread_loop:
    movl $0, %edi
    leaq AXIOM_PHASE10_READ_CHAR_OFFSET(%r12), %rsi
    movl $1, %edx
    call read
    cmpq $1, %rax
    je .Lgot_input

    movl $10, %edi
    call sleep
    decl %r13d
    jnz .Lread_loop
    jmp .Lfinish

.Lgot_input:
    movl $1, %edi
    leaq .Lread_prefix(%rip), %rsi
    movl $(.Lread_prefix_end - .Lread_prefix), %edx
    call write

    movl $1, %edi
    leaq AXIOM_PHASE10_READ_CHAR_OFFSET(%r12), %rsi
    movl $1, %edx
    call write

    movl $1, %edi
    leaq .Lnewline(%rip), %rsi
    movl $1, %edx
    call write

.Lfinish:
    movabs $AXIOM_PHASE10_MAGIC, %rax
    movq %rax, AXIOM_PHASE10_MAGIC_OFFSET(%r12)

    movl $AXIOM_PHASE10_EXIT_STATUS, %edi
    call _exit
    ud2

/* Minimal Phase-10 libc-style syscall wrappers. */
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

.Lhello:
    .ascii "Hello via SYS_write from Ring 3!\n"
.Lhello_end:

.Lwaiting:
    .ascii "Phase 10 userspace waiting for keyboard input...\n"
.Lwaiting_end:

.Lread_prefix:
    .ascii "Phase 10 userspace read: "
.Lread_prefix_end:

.Lnewline:
    .ascii "\n"

.Lfake_path:
    .asciz "/not-yet"

phase10_user_end:

.section .note.GNU-stack,"",@progbits
