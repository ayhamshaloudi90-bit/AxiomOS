bits 64
default rel
section .text
global phase3_register_probe
phase3_register_probe:
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    mov [rel probe_rsp], rsp
    mov rax, 257
    mov rbx, 258
    mov rcx, 259
    mov rdx, 260
    mov rsi, 261
    mov rdi, 262
    mov rbp, 263
    mov r8, 264
    mov r9, 265
    mov r10, 266
    mov r11, 267
    mov r12, 268
    mov r13, 269
    mov r14, 270
    mov r15, 271
    ; Exercise DF restoration; the ISR must clear it before calling C.
    std
    pushfq
    pop qword [rel probe_flags]
    int3
    pushfq
    pop qword [rel returned_flags]
    cld
    cmp rax, 257
    jne .failed
    cmp rbx, 258
    jne .failed
    cmp rcx, 259
    jne .failed
    cmp rdx, 260
    jne .failed
    cmp rsi, 261
    jne .failed
    cmp rdi, 262
    jne .failed
    cmp rbp, 263
    jne .failed
    cmp r8, 264
    jne .failed
    cmp r9, 265
    jne .failed
    cmp r10, 266
    jne .failed
    cmp r11, 267
    jne .failed
    cmp r12, 268
    jne .failed
    cmp r13, 269
    jne .failed
    cmp r14, 270
    jne .failed
    cmp r15, 271
    jne .failed
    cmp rsp, [rel probe_rsp]
    jne .failed
    mov rax, [rel probe_flags]
    cmp rax, [rel returned_flags]
    jne .failed
    mov eax, 1
    jmp .done
.failed:
    xor eax, eax
.done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

global phase3_trigger_divide, phase3_divide_fault
phase3_trigger_divide:
    mov eax, 123
    xor edx, edx
    xor ecx, ecx
phase3_divide_fault:
    div rcx
    ret

global phase3_trigger_invalid, phase3_invalid_fault
phase3_trigger_invalid:
phase3_invalid_fault:
    ud2
    ret

global phase3_trigger_gp, phase3_gp_fault
phase3_trigger_gp:
    mov eax, 0x38             ; Index 7 lies beyond our seven-slot GDT.
phase3_gp_fault:
    mov ds, ax
    ret

section .bss
align 8
probe_rsp: resq 1
probe_flags: resq 1
returned_flags: resq 1
section .note.GNU-stack noalloc noexec nowrite progbits
