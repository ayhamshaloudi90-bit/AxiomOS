# Phase 9 — Ring 3 userspace and isolation

## Goal

Move AxiomOS from kernel-only multitasking to hardware-protected userspace.
Phase 9 deliberately does **not** introduce a syscall ABI; that is Phase 10.
Instead, it proves privilege transitions, isolated address spaces, separate user
stacks, and kernel protection.

## GDT/TSS privilege setup

The GDT now contains ring-0 code/data, ring-3 data/code, and a 64-bit TSS.
Selectors are:

```text
0x08  kernel code
0x10  kernel data
0x1B  user data (index 3, RPL 3)
0x23  user code (index 4, RPL 3)
0x28  TSS
```

Before returning to a Ring-3 task, the scheduler sets the TSS `RSP0` field to
that task's private kernel-stack top. If a timer interrupt or exception occurs in
Ring 3, the CPU automatically switches to that trusted Ring-0 stack before
entering the interrupt handler.

## Address spaces

Every user task receives a fresh PML4. Its lower canonical half begins empty.
The upper canonical half copies the kernel's PML4 entries so the kernel can keep
executing after CR3 changes, but the copied entries have the x86 USER bit clear.
Ring 3 therefore cannot traverse the kernel mappings.

Each Phase-9 user task maps:

```text
0x0000000000400000  user code, readable/executable, not writable
0x0000000000401000  user data, writable, NX
near 0x00007FFFFFF00000  four-page user stack, writable, NX
```

Page-table parent entries for those lower-half mappings carry the USER bit.
Kernel/HHDM/heap/APIC mappings remain supervisor-only.

## Correct task-stack switching

Phase 9 also makes the Phase-8 stack-switch path explicit and auditable. In
64-bit mode, interrupt frames include saved `SS:RSP` even without a privilege
change, and `IRETQ` restores them. Each synthetic kernel-thread frame therefore
contains a real private-stack `RSP` plus the kernel data selector in `SS`.

The ISR dispatcher now returns a pointer to the exact saved frame that should be
restored. The assembly epilogue executes:

```text
current interrupt frame
        |
        v
scheduler chooses next task
        |
        v
RAX = next task saved-frame pointer
        |
        v
RSP = RAX
        |
        v
POP registers + IRETQ
```

That means the epilogue restores the selected task's complete interrupt frame,
including its saved stack pointer. For a Ring-3 task, `IRETQ` additionally
changes CPL from 0 to 3 using the user CS/SS selectors.

## First userspace program

`userspace/phase9_program.S` contains a tiny position-independent Ring-3 image.
The scheduler copies it into a user-owned physical page and maps that page at
`0x400000`.

Because syscalls do not exist yet, the program proves execution by writing:

```text
Hello from AxiomOS userspace!
```

into its own data page and then writing a completion magic value. The kernel
reads the physical page through the HHDM after the task has run and prints the
message. This keeps the phase boundary honest: user execution exists, but the
kernel-service interface does not exist until Phase 10.

## Protection test

A second isolated Ring-3 task deliberately reads:

```text
0xFFFFFFFF80000000
```

which is inside the higher-half kernel mapping. The mapping is present but
supervisor-only, so x86 raises page fault vector 14 with the user-mode bit set in
the page-fault error code.

Instead of panicking AxiomOS, the Phase-9 exception path records the fault in the
current user task, marks that task TERMINATED, selects another READY task,
switches CR3/TSS state, and resumes scheduling.

This demonstrates that a bad user program can be stopped without taking down the
kernel.

## Acceptance

Run:

```bash
make clean
make
make test-phase9
make test
```

A successful Phase-9 test must prove:

- the first program actually enters Ring 3 and writes the expected message;
- the two user tasks have distinct CR3 roots;
- neither user CR3 equals the kernel CR3;
- the protection task receives #PF vector 14 at the kernel probe address;
- the fault error code says the access came from user mode and hit a present
  supervisor page;
- the faulting user task terminates without a kernel panic;
- scheduler and keyboard IRQ input still work afterward.

## Current limitations

- single CPU;
- fixed user virtual layout and one code page per Phase-9 program;
- no ELF loader yet;
- no syscall ABI yet;
- no user libc;
- no process-owned heap/mmap interface;
- no safe task reaper/resource reclamation for normally terminated tasks yet;
- no FPU/SSE user context because those units remain disabled by the build.
