bits 64
default rel
section .text
extern interrupt_dispatch

%macro ISR 1
global isr_%1
isr_%1:
    ; These exceptions push a hardware error code. All others need a zero.
    ; Includes #CP (21), AMD #VC (29), and #SX (30).
%if %1 != 8 && %1 != 10 && %1 != 11 && %1 != 12 && %1 != 13 && %1 != 14 && %1 != 17 && %1 != 21 && %1 != 29 && %1 != 30
    push qword 0
%endif
    push qword %1
    jmp isr_common
%endmacro

%assign vector 0
%rep 256
    ISR vector
%assign vector vector+1
%endrep

isr_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    cld                     ; C ABI requires DF=0; IRET restores original flags.
    mov rdi, rsp            ; First C argument: interrupt_frame pointer.
    mov rbx, rsp            ; Preserve the original frame while aligning CALL.
    and rsp, -16            ; System V alignment immediately before CALL.
    call interrupt_dispatch ; RAX = frame/stack context to restore.
    mov rsp, rax            ; May switch to a different task's kernel stack.
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    add rsp, 16             ; Discard vector and normalized error code.
    iretq                   ; Restore RIP, CS, RFLAGS, RSP, and SS.

section .rodata
align 8
global isr_stub_table
isr_stub_table:
%assign vector 0
%rep 256
    dq isr_%+vector
%assign vector vector+1
%endrep
section .note.GNU-stack noalloc noexec nowrite progbits
