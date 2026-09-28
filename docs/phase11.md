# Phase 11 — ELF64 executable loader

## Goal

Replace the one-page raw userspace-image assumption with a real ELF64 loader.
AxiomOS now accepts a standalone x86-64 `ET_EXEC` file, parses its ELF header
and program headers, maps `PT_LOAD` segments into a fresh Ring-3 address space,
and starts the task at the ELF `e_entry` address.

Phase 11 builds two standalone ELF files: `phase11_launcher.elf` and
`phase11_demo.elf`. Both are copied into `/boot` and delivered as Limine
modules. The kernel initially creates the launcher as a Ring-3 ELF task; the
launcher then calls `exec("/bin/phase11-demo")`. Until Phase 12 supplies a VFS,
that path resolves through a deliberately tiny boot-module executable registry.
The scheduler replaces the current user CR3/image/stack with the demo ELF while
preserving the task ID.

## ELF validation

The loader rejects images unless they are:

- ELF magic `0x7F 'E' 'L' 'F'`;
- ELFCLASS64;
- little-endian;
- ELF version 1;
- `ET_EXEC`;
- machine `EM_X86_64`;
- equipped with a bounded, correctly sized program-header table;
- composed of valid lower-half user `PT_LOAD` ranges;
- free of overlapping load pages in this first implementation;
- entered through an address inside an executable `PT_LOAD` segment.

All file offsets and virtual ranges are overflow/bounds checked before pages are
allocated.

## Loading PT_LOAD segments

For every loadable segment AxiomOS:

1. rounds the segment virtual range to 4 KiB pages;
2. allocates fresh PMM pages;
3. clears every page first;
4. copies only the file-backed `p_filesz` bytes;
5. therefore leaves `p_memsz - p_filesz` (BSS) zero-filled;
6. maps the page USER;
7. applies WRITABLE only for `PF_W`;
8. applies NX whenever `PF_X` is absent.

The demo ELF intentionally contains three load segments:

```text
0x400000  text    R-X
0x401000  rodata  R-- + NX
0x402000  data    RW- + NX   (also contains BSS)
```

The ordinary four-page Ring-3 stack remains RW + NX near the top of the lower
canonical half.

## Execution

The ELF program uses the Phase-10 syscall ABI. It calls `getpid()`, verifies its
BSS began as zero, prints:

```text
Hello from an ELF64 executable in AxiomOS!
```

then yields, sleeps, writes a completion magic value into its data segment, and
exits with status 11.

The kernel verifies:

- RIP came from ELF `e_entry` (`0x400000`);
- program-header and `PT_LOAD` counts;
- text/rodata/data mapping permissions;
- BSS zero-fill;
- `getpid()` matches the scheduled task ID;
- exit status is 11.

## `exec("/bin/phase11-demo")`

`SYS_exec` is now functional for the Phase-11 executable registry. The kernel
validates/copies the user path, resolves `/bin/phase11-demo` to the separate
Limine module, builds a fresh address space, loads the ELF, switches CR3, frees
the old user image/stack, resets user RIP/RSP/register state, and returns with
`SYSRETQ` into the new ELF entry point. The PID/task ID is preserved.

This is intentionally **not** a VFS implementation. Phase 12 can replace the
small path-to-module registry with real path lookup and file reads while keeping
the same ELF loading and process-image replacement mechanisms.

## Current limits

- `ET_DYN`/PIE and dynamic linking are not supported.
- ELF relocations/interpreters are not supported.
- `PT_LOAD` segments that share a 4 KiB page are rejected.
- Maximum executable image footprint is 32 mapped pages for this phase.
- ASLR is not implemented.
- Executables currently arrive as boot modules because the VFS is still future
  work.

## Acceptance

```bash
make clean
make
make test-phase11
make test
```

The dedicated test also inspects the standalone ELF with `llvm-readelf` (or
`readelf`) before booting it.
