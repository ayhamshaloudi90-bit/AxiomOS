# Phase 23 — Graphical framebuffer

## Goal

Turn the framebuffer that already backs the Phase-2 terminal into a reusable
2D graphics subsystem while keeping the terminal and the Phase-20 security
boundary intact.

Phase 23 implements the graphics milestone from the original roadmap:

```c
draw_pixel()
draw_line()
draw_rectangle()
draw_bitmap()
draw_text()
```

The kernel implementation lives in `kernel/graphics/framebuffer.c`. Userspace
programs use the `libaxiom.a` interface in `<axiom/graphics.h>` rather than
receiving an unrestricted raw framebuffer mapping.

## Renderer

The graphics subsystem reuses Limine's 32-bit RGB framebuffer and VGA-style
8-pixel-wide bitmap font. RGB values use the portable `0xRRGGBB` format and are
packed according to the framebuffer's red/green/blue mask fields.

Implemented primitives:

- clipped single-pixel writes;
- Bresenham integer line drawing;
- filled rectangles;
- RGB32 bitmap blitting;
- bitmap-font text with newline support;
- whole-screen clears;
- framebuffer mode and cumulative graphics statistics.

The existing framebuffer terminal remains authoritative for shell output and
panic diagnostics. `axiom_clear()` still restores a clean terminal surface and
resets its cursor after graphical programs finish.

## Ring-3 graphics ABI

Phase 23 adds syscalls 45–52:

```text
45 gfx_info
46 gfx_clear
47 gfx_pixel
48 gfx_line
49 gfx_rect
50 gfx_bitmap
51 gfx_text
52 gfx_stats
```

Pointer-bearing operations use the existing VMM copy/validation helpers. Bitmap
uploads are capped at 16,384 pixels and text at 128 characters. Rectangle and
line requests are bounded so an untrusted process cannot turn one graphics
syscall into an unbounded kernel loop.

`libaxiom.a` exposes:

```c
axiom_gfx_info()
axiom_gfx_clear()
axiom_draw_pixel()
axiom_draw_line()
axiom_draw_rectangle()
axiom_draw_bitmap()
axiom_draw_text()
axiom_gfx_stats()
```

## Demo and shell observability

`/bin/gfxdemo` draws a centered Phase-23 panel using every primitive, verifies
that graphics statistics changed, leaves the scene visible briefly, and then
restores the normal terminal.

The shell adds:

```text
axiom> gfxinfo
axiom> gfxdemo
```

`gfxinfo` reports framebuffer dimensions, pitch, bpp, font dimensions, drawing
counters, and clipping activity.

## Deliberate limits

The original roadmap lists mouse support, windows, a compositor, and a graphical
terminal as later/evolutionary items. Phase 23 intentionally does **not** claim
those are complete. It establishes the renderer and protected userspace API
those features can build on.

Other current limits:

- one boot framebuffer, 32-bit RGB only;
- no hardware-accelerated drawing;
- no alpha blending, scaling, image decoding, double buffering, or vsync;
- drawing syscalls are synchronous and serialized by the current BSP-only task
  execution model;
- applications do not own persistent surfaces or windows yet.

## Acceptance

```bash
make clean
make
make test-phase23
make test
```

Expected boot markers include:

```text
AxiomOS Phase 23 graphical framebuffer online.
Phase 23 primitives: pixel + line + rectangle + bitmap + text
Phase 23 framebuffer read/write self-test: OK
Phase 23 clipping protection: OK
Phase 23 graphics library initialization complete.
```

The userspace demo must report:

```text
Phase 23 draw_pixel: OK
Phase 23 draw_line: OK
Phase 23 draw_rectangle: OK
Phase 23 draw_bitmap: OK
Phase 23 draw_text: OK
Phase 23 userspace graphics library: OK
Phase 23 framebuffer graphics demo complete.
```
