# Phase 14 — Ring-3 interactive shell

Phase 14 replaces the old kernel input demonstration with a real userspace
shell, `/bin/axiomsh`.

## Architecture

The shell is a standalone ELF64 `ET_EXEC` program. It runs at CPL3 and uses the
existing Phase-10 syscall boundary rather than calling kernel/VFS functions
directly.

To make command execution possible without destroying the shell process,
Phase 14 extends the syscall ABI with:

- `readdir(path, index, entry)` — enumerate VFS directories;
- `mkdir(path)` — create a directory through the mounted filesystem;
- `spawn(path)` — load a VFS-backed ELF into a new Ring-3 task;
- `waitpid(pid, status)` — wait for a spawned child and retrieve its exit code;
- `clear()` — clear the framebuffer terminal;
- `kbdstats()` — expose the existing PS/2 diagnostic counters.

`spawn()` intentionally accepts only an executable path in this phase. `argv`,
environment variables, pipes, redirection, job control and signals are later
userspace/process-management work.

## Commands

```text
help
clear
echo <text>
pwd
cd <dir>
ls [dir]
cat <file>
stat <path>
touch <file>
write <file> <text>
mkdir <dir>
kbdstats
<program>
/path/program
```

An unqualified external command is resolved under `/bin`. For example:

```text
axiom> phase11-demo
```

spawns `/bin/phase11-demo`, waits for it, then returns to the shell prompt.

## Filesystem behavior

The shell can interact with every currently mounted VFS backend:

- `/`, `/tmp` — RAMFS;
- `/disk` — persistent Phase-13 diskfs when an AHCI disk is attached.

The shell keeps its current working directory in userspace and converts
relative names to absolute VFS paths before issuing syscalls.

## Acceptance

```bash
make clean
make
make test-phase14
make test
```

The dedicated test injects real QEMU PS/2 key events and verifies built-ins,
VFS reads/writes/directories, `/disk` access, keyboard diagnostics, and
spawn/wait of another Ring-3 ELF.
