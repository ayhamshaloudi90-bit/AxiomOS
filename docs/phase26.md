# Phase 26 — Desktop GUI (post-roadmap extension)

## Goal

Give AxiomOS a polished graphical desktop without sacrificing the interactive
Ring-3 shell that has been the primary interface since Phase 14.

Phase 26 is intentionally a **post-roadmap extension**. The original Phase 0–25
roadmap remains complete; this phase builds on the Phase-23 graphics layer and
adds a usable desktop shell rather than changing the kernel architecture.

## User experience

From the ordinary AxiomOS shell:

```text
axiom> desktop
```

The `/bin/desktop` Ring-3 application switches the framebuffer to a desktop UI
with:

- AxiomOS branding and wallpaper;
- top/status bar and bottom taskbar;
- Terminal, System, Network, and Graphics launch cards;
- mouse hover/click interaction when a PS/2 mouse is available;
- keyboard shortcuts as a guaranteed fallback.

Keyboard controls:

```text
T or Enter   open Terminal
S            open sysinfo
N            open ifconfig
G            open gfxdemo
Q            leave Desktop and return to the original shell
```

The Terminal launcher starts a nested `/bin/axiomsh`. The shell now implements
an `exit` built-in. Therefore:

```text
Desktop -> Terminal -> axiomsh -> exit -> Desktop
```

leaves the graphical desktop alive, while `Q` from the desktop returns to the
original shell that launched it.

## Architecture

```text
/bin/desktop (Ring 3)
      |
      +---- libaxiom graphics -> gfx syscalls 45..52 -> framebuffer renderer
      |
      +---- mouse state ------> syscall 54 ----------> PS/2 mouse driver
      +---- cursor position ---> syscall 55 ----------> save-under cursor
      |
      +---- keyboard stdin ---> SYS_read ------------> PS/2 keyboard buffer
      |
      +---- launchers --------> spawn/waitpid -------> normal Ring-3 ELFs
                                        |
                         +--------------+--------------+
                         |              |              |
                     axiomsh         sysinfo        ifconfig/gfxdemo
```

The desktop does not receive or map the raw framebuffer. It uses the protected
Phase-23 drawing API exactly like any other userspace program.

## Mouse input

Phase 26 adds an optional PS/2 second-port mouse driver:

- controller second-port enable;
- IRQ12 routed through the existing I/O APIC to vector 44;
- standard 3-byte PS/2 packet decoding;
- cumulative X/Y motion and button state;
- syscall-mediated read-only userspace state.

The mouse is **best effort**. Failure to initialize it is non-fatal; the desktop
remains completely usable through keyboard shortcuts.

## Cursor performance

The first Phase-26 desktop prototype repainted the entire framebuffer whenever
the mouse moved. That made a relative PS/2 pointer feel buffered because a
1024x768 desktop could be redrawn for every tiny packet.

The optimized desktop uses a kernel **save-under software cursor** instead:

1. restore the 12x19 pixels beneath the old cursor;
2. save the 12x19 pixels beneath the new cursor;
3. draw the cursor at the new location.

The desktop only performs a full redraw when hover state actually changes, such
as entering or leaving a launcher card. Rectangle filling is also row-oriented
and packs the RGB color once per rectangle rather than once per pixel. Mouse
deltas are scaled 2x for more natural QEMU/Steam-Deck pointer travel.

Because the PS/2 device is a **relative** pointer, QEMU should have input grabbed
while using the desktop. In QEMU's graphical frontend, `Ctrl+Alt+G` toggles the
mouse/keyboard grab.

## Compatibility choice

AxiomOS still boots to the normal text shell. This is deliberate:

1. Phases 1–25 retain their exact accepted test/boot workflow.
2. Kernel panic/logging always has the proven framebuffer terminal available.
3. The graphical desktop remains a normal userspace application, so a desktop
   crash cannot replace the system's recovery/debug interface.

A later release may choose to make `/bin/desktop` the login/session manager once
windowing and input isolation are mature.

## Acceptance

```bash
make test-phase26
make test
```

The Phase-26 test launches `desktop`, opens a nested Terminal with `T`, executes
an ordinary shell command, types `exit` to return to the desktop, then types `Q`
to return to the original shell. This verifies the full UI/terminal lifecycle
without requiring visual screenshot comparison.
