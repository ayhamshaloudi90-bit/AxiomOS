# Phase 7 — Timer and keyboard drivers

## Goal

Turn the Phase-3 interrupt framework into continuously enabled hardware input on
AxiomOS's q35 target:

- Local APIC periodic timer at 100 Hz;
- I/O APIC routing for the PS/2 keyboard;
- scan-code decoding and a kernel input ring buffer.

Phases 1–6 deliberately kept IF clear. Phase 7 is the first milestone that executes
`sti` and leaves external interrupts enabled during normal execution.

## Why APIC instead of relying on the legacy PIC

The first Phase-7 attempt programmed PIT IRQ0 through the 8259 PIC. On the q35
machine that path reached timer setup but no IRQ0 reached the CPU. q35 is an
APIC-era platform, so AxiomOS now uses its native interrupt architecture instead
of forcing legacy PIC delivery.

The 8259 PIC is still initialized/remapped for the earlier architecture work, but
it remains fully masked once Phase 7 starts.

## Local APIC timer

AxiomOS maps the Local APIC MMIO page by reading `IA32_APIC_BASE`. The timer is
calibrated without requiring a PIT interrupt:

1. LAPIC timer starts as a masked one-shot counter.
2. PIT channel 2 runs for approximately 10 ms in mode 0.
3. AxiomOS polls PIT channel-2 OUT through port `0x61`.
4. The elapsed LAPIC count over those 10 ms is measured.
5. That count becomes the basis for the 100 Hz periodic LAPIC timer.

The periodic timer uses IDT vector 32. Its handler increments a monotonic 64-bit
tick counter; the common interrupt dispatcher sends the LAPIC EOI.

## I/O APIC keyboard route

For the pinned q35 target, AxiomOS maps the standard q35 I/O APIC MMIO page at
physical `0xFEC00000`. Every redirection entry starts masked. The first PS/2
keyboard line (ISA IRQ/GSI 1) is routed to vector 33 on the bootstrap CPU.

Future platform-general work should discover I/O APICs and interrupt-source
overrides from the ACPI MADT instead of relying on the pinned q35 topology.

## PS/2 keyboard

The i8042 first port is configured while IF is still clear. The driver enables
scan-code translation and scanning, then routes the keyboard interrupt through the
I/O APIC. The decoder supports normal set-1 text keys, Shift, Caps Lock, digits,
punctuation, Space, Enter, Tab and Backspace.

Decoded characters are pushed into a 128-character SPSC ring buffer. The IRQ
handler never prints or allocates memory; the normal kernel input loop drains and
echoes the buffer.

## Interactive behavior

After the timer self-test succeeds:

```text
Phase 7 input ready. Type into AxiomOS.
axiom>
```

Typing a line and pressing Enter prints:

```text
Keyboard line: hello
Keyboard IRQs/scancodes/chars/dropped: ...
```

This is an input demonstration, not the later shell.

## Validation

Run:

```bash
make test-phase7
```

The test boots q35 under TCG, requires at least ten Local APIC timer ticks, then
uses QEMU's monitor to inject the real virtual key sequence `axiom<Enter>`. AxiomOS
must reconstruct `Keyboard line: axiom` from the IRQ-driven PS/2 path.

Then run:

```bash
make test
```

for the complete Phase 1–7 regression suite.

## Current limitations

- single CPU and one address space;
- I/O APIC location/ISA GSI topology are pinned to the q35 development target;
- no ACPI MADT parser yet;
- fixed US scan-code-set-1 text mapping;
- navigation/function keys and mouse are not exposed;
- keyboard buffer is kernel-only and non-blocking;
- heap has no interrupt/SMP locking and IRQ handlers deliberately do not allocate.
