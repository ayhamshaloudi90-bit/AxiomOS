.section .text.syscall,"ax",@progbits
.code64

.extern syscall_dispatch
.extern syscall_kernel_stack_top

.section .bss.syscall,"aw",@nobits
.p2align 3
syscall_scratch_user_rsp:
    .quad 0

.section .text.syscall,"ax",@progbits
.global syscall_entry
.type syscall_entry, @function
syscall_entry:
    /*
     * SYSCALL enters CPL0 without changing RSP. IA32_FMASK clears IF before
     * this instruction stream runs, so the single-CPU scratch slot is safe
     * until the user RSP has been moved onto the task's private kernel stack.
     */
    movq %rsp, syscall_scratch_user_rsp(%rip)
    movq syscall_kernel_stack_top(%rip), %rsp

    /* Build struct syscall_frame from high addresses downward. */
    pushq syscall_scratch_user_rsp(%rip) /* user_rsp */
    pushq %r11                          /* user_rflags */
    pushq %rcx                          /* user_rip */
    pushq %rax
    pushq %rbx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    cld
    movq %rsp, %rdi
    call syscall_dispatch

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rbx
    popq %rax
    popq %rcx                          /* SYSRET RIP */
    popq %r11                          /* SYSRET RFLAGS */
    movq (%rsp), %rsp                  /* restore user RSP */
    sysretq
.size syscall_entry, .-syscall_entry

.section .note.GNU-stack,"",@progbits
