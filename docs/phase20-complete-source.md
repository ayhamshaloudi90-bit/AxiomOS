# AxiomOS Phase 20 — changed source manifest

This document identifies the Phase-20 security changes. The authoritative full
source is the accompanying `AxiomOS_Phase20_source.zip`.

## New files

- `userspace/phase20_demo.c` — Ring-3 attack/acceptance program.
- `userspace/phase20_wx.S` — deliberately malicious W+X ELF payload.
- `userspace/phase20_wx.ld` — emits a single RWE PT_LOAD segment.
- `tests/phase20_security.py` — QEMU integration test.
- `docs/phase20.md` — design, limits, and acceptance notes.

## Modified files

- `Makefile`
- `boot/limine/limine.conf`
- `filesystem/bootstrap.c`
- `filesystem/vfs.c`
- `include/axiom/abi/fs.h`
- `include/axiom/abi/syscall.h`
- `include/axiom/abi/user_layout.h`
- `include/axiom/filesystem/vfs.h`
- `include/axiom/process/task.h`
- `kernel/core/kernel.c`
- `kernel/elf/elf64.c`
- `kernel/syscall/syscall.c`
- `libc/include/unistd.h`
- `libc/src/unistd.c`
- `process/scheduler.c`
- `userspace/phase14_shell.c`
- `README.md`
- `docs/project-state.md`
- `docs/validation.md`

See `docs/phase20.md` for the security model and deliberate limits.
