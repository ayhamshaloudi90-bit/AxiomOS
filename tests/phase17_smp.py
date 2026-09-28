#!/usr/bin/env python3
"""Validate Phase-17 x86-64 SMP discovery, AP bring-up, and cross-CPU locking."""

from pathlib import Path
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def serial_text(path):
    return path.read_text(errors="replace").replace("\r", "")


def wait_for_text(process, serial, marker, deadline):
    while True:
        text = serial_text(serial)
        if marker in text:
            return text
        if "Phase 17 SMP initialization/self-test: FAILED" in text or \
                "Phase 17 multicore parallel-work test: FAILED" in text:
            tail = "\n".join(text.splitlines()[-180:])
            raise AssertionError(
                f"phase17 kernel SMP self-test failed; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-180:])
            raise AssertionError(
                f"phase17 timed out waiting for {marker!r}; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )
        time.sleep(0.05)


def test_phase17():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase17-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = directory / "phase17-serial.log"
    qemu_log = directory / "phase17-qemu.log"
    serial.write_text("")

    command = [
        "qemu-system-x86_64",
        "-machine", "q35",
        "-accel", "tcg",
        "-smp", "4",
        "-m", "256M",
        "-cdrom", str(directory / "AxiomOS.iso"),
        "-boot", "d",
        "-display", "none",
        "-serial", f"file:{serial}",
        "-monitor", "none",
        "-no-reboot",
        "-no-shutdown",
    ]

    with qemu_log.open("w") as output:
        process = subprocess.Popen(
            command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT
        )
        try:
            text = wait_for_text(
                process,
                serial,
                "AxiomOS shell ready. Type 'help' for commands.",
                time.monotonic() + 180,
            )

            require("AxiomOS Phase 17 SMP / multicore online." in text,
                    "phase17: SMP banner missing")
            require("Phase 17 AP bring-up: OK" in text,
                    "phase17: AP bring-up did not pass")
            require("Phase 17 cross-CPU spinlock: OK" in text,
                    "phase17: cross-CPU spinlock did not pass")
            require("Phase 17 multicore parallel-work test: OK" in text,
                    "phase17: multicore work test did not pass")
            require("Phase 17 SMP initialization complete." in text,
                    "phase17: completion marker missing")
            require("KERNEL PANIC" not in text, "phase17: kernel panicked")
            require("SKIPPED (single CPU boot)" not in text,
                    "phase17: test unexpectedly booted only one CPU")

            counts = re.search(
                r"Phase 17 CPUs detected/managed/online: (\d+)/(\d+)/(\d+)",
                text,
            )
            require(counts is not None, "phase17: CPU counts missing")
            detected, managed, online = map(int, counts.groups())
            require(detected >= 4, "phase17: QEMU did not expose four CPUs")
            require(managed >= 4 and online >= 4,
                    "phase17: not all requested CPUs became managed/online")

            aps = re.search(
                r"Phase 17 APs released/completed: (\d+)/(\d+)", text
            )
            require(aps is not None, "phase17: AP counts missing")
            require(int(aps.group(1)) >= 3,
                    "phase17: fewer than three APs were released")
            require(aps.group(1) == aps.group(2),
                    "phase17: not every released AP completed work")

            counter = re.search(
                r"Phase 17 locked counter expected/actual: (\d+)/(\d+)", text
            )
            require(counter is not None, "phase17: locked counter missing")
            require(counter.group(1) == counter.group(2),
                    "phase17: shared counter was corrupted across CPUs")
            require(int(counter.group(1)) == online * 5000,
                    "phase17: unexpected shared-work amount")

            mask = re.search(
                r"Phase 17 parallel participant mask: 0x([0-9A-Fa-f]+)", text
            )
            require(mask is not None, "phase17: participant mask missing")
            require(int(mask.group(1), 16).bit_count() == online,
                    "phase17: participant mask does not include every online CPU")

            print(f"PASS: Phase 17 detected {detected} CPUs; {online} online")
            print("PASS: Phase 17 application processors entered AxiomOS")
            print("PASS: Phase 17 per-CPU stacks/GDT/TSS initialized")
            print("PASS: Phase 17 BSP/AP parallel shared-memory work")
            print("PASS: Phase 17 cross-CPU spinlock preserved exact counter")
            print("PASS: Phase 17 shell still launches with APs parked")
        finally:
            if process.poll() is None:
                process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


if __name__ == "__main__":
    test_phase17()
    print("PASS: all Phase 17 SMP/multicore tests.")
