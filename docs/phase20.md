# Phase 20 — Security hardening

Phase 20 hardens the existing Ring-3/process/VFS/ELF design instead of adding a
separate security subsystem.

## Implemented protections

- **Ring-3/kernel isolation remains mandatory.** User address spaces inherit the
  upper-half kernel mappings with the page-table USER bit cleared, so Ring 3
  cannot traverse kernel/HHDM/heap/MMIO mappings.
- **W^X ELF policy.** Any PT_LOAD segment that is both writable and executable is
  rejected before mapping. Executable pages are read/execute; writable pages are
  NX.
- **NX userspace data.** User data pages, anonymous mmap/malloc pages, and user
  stacks are writable but non-executable.
- **User stack guard page.** One page immediately below the four mapped stack
  pages is deliberately left unmapped. Downward stack overflow reaches the guard
  before adjacent user mappings.
- **Kernel stack canary.** Dynamically allocated kernel task stacks contain a
  fixed canary at the low end. The scheduler validates the canary when scheduling
  a task and when handling timer preemption; corruption panics the kernel instead
  of continuing with a silently damaged stack.
- **Syscall pointer validation.** Existing page-table-based copy_from_user /
  copy_to_user checks remain the only supported way for Ring 0 to access user
  buffers. Phase-20 tests explicitly pass a kernel-half address and require
  `-EFAULT`.
- **File permissions.** VFS mode bits are now enforced for read/write open and
  execute permission is required by both `exec` and `spawn`. Boot-seeded `/bin`
  programs are mode `0555`; normal new RAMFS/diskfs files remain `0644`.
- **Credentials.** Normal userspace tasks receive UID 1000 / GID 1000 and fork
  inherits those credentials. `getuid()` and `getgid()` are exposed through the
  syscall ABI and libc. The shell adds `id`.

## Deliberate attack tests

`/bin/phase20-demo` verifies:

1. UID/GID identity.
2. `/bin` executable mode and lack of write permission.
3. A normal `/tmp` file cannot be executed (`-EACCES`).
4. A kernel-half syscall pointer is rejected (`-EFAULT`).
5. A child attempting to read kernel memory dies with a user page fault.
6. A child attempting to execute malloc-backed memory dies because the heap is
   NX.
7. A child touching the stack guard page dies with a user page fault.
8. A deliberately built ELF with an RWE PT_LOAD segment is rejected by W^X.

The destructive checks run in forked children so the parent survives to verify
that the protection fired.

## Limits

This is not a complete Unix security model. Phase 20 intentionally does **not**
claim:

- file ownership or per-owner/group/other access selection;
- `chmod`, `chown`, setuid/setgid or privilege escalation;
- ACLs/capabilities;
- ASLR/PIE (current user executables are fixed-address ET_EXEC images);
- stack guard pages for dynamically allocated kernel stacks (the kernel uses a
  canary there instead because the current heap API does not expose guard-page
  virtual reservations);
- cryptographic code signing or measured boot.

Those require additional VFS metadata, process policy, or relocation support.

## Acceptance

```bash
make clean
make
make test-phase20
make test
```

A successful Phase-20 runtime test reaches the shell, runs `id`, executes
`phase20-demo`, observes every `Phase 20 ...: OK` marker, and returns to the
shell without `KERNEL PANIC`.
