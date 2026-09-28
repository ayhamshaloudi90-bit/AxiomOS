#!/usr/bin/env python3
"""Validate Phase 13 AHCI block I/O and persistent disk-backed VFS storage."""

from pathlib import Path
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def wait_for_text(process, serial, marker, deadline):
    while True:
        text = serial.read_text(errors="replace")
        if marker in text:
            return
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.replace("\r", "").splitlines()[-100:])
            raise AssertionError(
                f"phase13 timed out waiting for {marker!r}; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )
        time.sleep(0.05)


def boot_once(directory, disk, label):
    serial = directory / f"phase13-{label}-serial.log"
    qemu_log = directory / f"phase13-{label}-qemu.log"
    serial.write_text("")

    command = [
        "qemu-system-x86_64",
        "-machine", "q35",
        "-accel", "tcg",
        "-m", "256M",
        "-cdrom", str(directory / "AxiomOS.iso"),
        "-boot", "d",
        "-drive", f"file={disk},format=raw,if=none,id=axiomdisk",
        "-device", "ide-hd,drive=axiomdisk,bus=ide.0",
        "-display", "none",
        "-serial", f"file:{serial}",
        "-monitor", "none",
        "-no-reboot",
        "-no-shutdown",
    ]

    with qemu_log.open("w") as output:
        process = subprocess.Popen(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)
        try:
            wait_for_text(
                process,
                serial,
                "Phase 13 persistent storage complete.",
                time.monotonic() + 120,
            )
            text = serial.read_text(errors="replace").replace("\r", "")
            require("KERNEL PANIC" not in text, f"phase13 {label}: kernel panicked")
            require("Phase 13 AHCI initialization: FAILED" not in text,
                    f"phase13 {label}: AHCI initialization failed")
            require("Phase 13 diskfs mount: FAILED" not in text,
                    f"phase13 {label}: diskfs mount failed")
            require("Phase 13 userspace disk demo: FAILED" not in text,
                    f"phase13 {label}: userspace disk demo failed")
            return text
        finally:
            if process.poll() is None:
                process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


def parse_number(text, label):
    match = re.search(rf"^{re.escape(label)}: (\d+)$", text, re.MULTILINE)
    require(match is not None, f"phase13: missing numeric field {label}")
    return int(match.group(1))


def test_phase13():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase13-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    disk = directory / "phase13-test-disk.img"
    with disk.open("wb") as handle:
        handle.truncate(16 * 1024 * 1024)

    first = boot_once(directory, disk, "first")
    second = boot_once(directory, disk, "second")

    expected = [
        "AxiomOS Phase 13 persistent storage online.",
        "Block device: ahci0 (512-byte sectors)",
        "VFS disk mount: diskfs mounted at /disk",
        "Phase 13 diskfs readback: Persistent storage works across AxiomOS reboots!",
        "Phase 13 disk round-trip: OK",
        "Phase 13 persistent storage complete.",
    ]
    for marker in expected:
        require(marker in first, f"phase13 first boot missing: {marker}")
        require(marker in second, f"phase13 second boot missing: {marker}")

    require("Phase 13 disk freshly formatted: yes" in first,
            "phase13: first boot did not format blank disk")
    require("Phase 13 persistent file before user: absent" in first,
            "phase13: blank disk unexpectedly contained persistent file")
    require("Phase 13 disk freshly formatted: no" in second,
            "phase13: second boot reformatted persistent disk")
    require("Phase 13 persistent file before user: present" in second,
            "phase13: file did not persist into second boot")

    sectors = parse_number(first, "Phase 13 disk sectors")
    reads = parse_number(second, "Phase 13 block sectors read")
    writes = parse_number(first, "Phase 13 block sectors written")
    require(sectors == (16 * 1024 * 1024) // 512,
            f"phase13: unexpected disk capacity {sectors} sectors")
    require(reads > 0, "phase13: no sectors were read")
    require(writes > 0, "phase13: no sectors were written")

    print("PASS: Phase 13 PCI/AHCI SATA discovery and IDENTIFY")
    print(f"  capacity:           {sectors} sectors")
    print("PASS: Phase 13 512-byte block reads/writes")
    print(f"  second-boot reads:  {reads}")
    print(f"  first-boot writes:  {writes}")
    print("PASS: Phase 13 diskfs mounted through VFS")
    print("PASS: Phase 13 file persisted across two QEMU boots")


if __name__ == "__main__":
    test_phase13()
    print("PASS: all Phase 13 disk/persistence tests.")
