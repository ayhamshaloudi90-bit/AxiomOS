.text
.global smp_ap_entry_asm
.type smp_ap_entry_asm,@function
.extern smp_ap_entry_c

/*
 * Limine enters here with RDI = struct limine_mp_info *.
 * extra_argument (offset 24) points at AxiomOS struct smp_cpu.
 * struct smp_cpu offsets are asserted in smp.c:
 *   kernel_cr3 = 16
 *   stack_top  = 24
 */
smp_ap_entry_asm:
    cli
    movq 24(%rdi), %rax
    testq %rax, %rax
    jz .Lpark

    movq 16(%rax), %rcx
    movq %rcx, %cr3

    movq 24(%rax), %rsp
    andq $-16, %rsp
    xorq %rbp, %rbp

    movq %rax, %rdi
    call smp_ap_entry_c

.Lpark:
    cli
.Lhalt:
    hlt
    jmp .Lhalt
.size smp_ap_entry_asm, .-smp_ap_entry_asm

.section .note.GNU-stack,"",@progbits
