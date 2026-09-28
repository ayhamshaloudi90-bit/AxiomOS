# AxiomOS handoff — Phase 26 desktop GUI

Development environment: Linux/Distrobox on Steam Deck. Run commands from the
directory containing `Makefile`. The original Phase 0–25 roadmap is complete;
Phase 26 is the first post-roadmap extension.

## Accepted foundation

- Phases 0–24: kernel, Ring-3, storage, shell/processes, synchronization, SMP,
  networking, libc, security, advanced scheduling, IPC, graphics, and network apps.
- Phase 25: read-only `/proc` developer tooling and `/bin/sysinfo`.

## Phase 26 implementation

Phase 26 adds `/bin/desktop`, a normal Ring-3 graphical shell built entirely on
the existing Phase-23 framebuffer syscall API. The UI provides a branded
wallpaper, top bar, taskbar, launch cards, hover feedback, and launchers for the
Terminal, System telemetry, Network status, and the graphics demo.

A nested `/bin/axiomsh` is used for Terminal. The shell now has an `exit` built-in,
so the lifecycle is `outer shell -> desktop -> nested shell -> exit -> desktop ->
Q -> outer shell`. This preserves the exact terminal workflow used throughout
development while adding a friendly GUI around it.

Phase 26 also adds a best-effort PS/2 mouse driver on IRQ12, syscall 54
(`mousestate`), and syscall 55 (`gfx_cursor`). The cursor uses a save-under
12x19 patch so normal pointer movement does not repaint the whole desktop. The
desktop uses mouse movement/clicks when available and always
supports keyboard shortcuts (T/Enter, S, N, G, Q) as a fallback.

## Compatibility policy

The kernel still launches `/bin/axiomsh` after boot. This keeps all original
Phase 1–25 tests and the proven panic/debug console path intact. Enter the GUI
with:

```text
axiom> desktop
```

## Static validation

- all kernel C translation units compile with the freestanding
  `-Wall -Wextra -Werror` flags;
- `/bin/desktop` and the updated `/bin/axiomsh` compile/link against the current
  `libaxiom.a`;
- existing userspace programs remain ABI-compatible with syscalls 54-55 appended
  after the existing Phase-24 server call;
- `tests/phase26_desktop.py` syntax-checks and is included in `make test`.

Final runtime acceptance remains local because the packaging environment lacks
QEMU/NASM:

```bash
make clean
make
make test-phase26
make test
```
