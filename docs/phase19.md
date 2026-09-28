# Phase 19 — Userspace libc

Phase 19 turns the ad-hoc C helpers used by earlier Ring-3 programs into a reusable static userspace library, `libaxiom.a`.

## Library layout

- `libc/crt0.S`: common ELF entry point calling `main()` then `_exit()`.
- `libc/include/stdio.h`: `printf`, `snprintf`, `puts`, `putchar`.
- `libc/include/stdlib.h`: `malloc`, `free`, `calloc`, `realloc`, `atoi`, `strtol`.
- `libc/include/string.h`: common string/memory operations.
- `libc/include/ctype.h`: character classification/case conversion.
- `libc/include/unistd.h`: read/write/process wrappers.
- `libc/include/fcntl.h`, `sys/stat.h`: small filesystem-facing interfaces.
- `libc/include/axiom/syscalls.h`: AxiomOS-specific directory, process, network, and mmap wrappers.

The shell and the Phase-15 C programs now link against `libaxiom.a`. Assembly-only historical demos remain standalone because they intentionally demonstrate the raw syscall/ELF layers from their original phases.

## Anonymous mmap and malloc

`AXIOM_SYS_MMAP`, reserved since Phase 10, is now implemented. It allocates zero-filled, writable, NX user pages from a per-process anonymous mapping region beginning at `0x0000100000000000`.

The first allocator is a first-fit userspace allocator with 16-byte alignment, block splitting, block coalescing, and page growth through `AXIOM_SYS_MMAP`. `free()` recycles blocks inside the process; it does not yet return individual pages to the kernel. All mapped pages are reclaimed when the process exits or execs.

Anonymous mapping pages participate in Phase-15 eager `fork()`: physical pages are copied into the child, preserving process isolation. `exec()` discards old anonymous mappings with the old address space.

Current per-process anonymous mapping ceiling: 128 pages (512 KiB).

## printf subset

Supported conversions: `%c`, `%s`, `%d`, `%i`, `%u`, `%x`, `%X`, `%p`, `%%`, including `l`/`ll` integer length modifiers. Width, precision, floating point, and locale are deliberately not implemented yet.

## Acceptance

Run:

```bash
make test-phase19
make test
```

`test-phase19` launches `/bin/phase19-demo` from the real Ring-3 shell and verifies stdio, strings/memory, ctype/numeric conversion, dynamic allocation, malloc-backed fork isolation, exec cleanup of anonymous mappings, files, process wrappers, and the network-info wrapper.
