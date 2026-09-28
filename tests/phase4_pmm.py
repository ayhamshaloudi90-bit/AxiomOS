#!/usr/bin/env python3
"""Boot the normal AxiomOS image and validate the Phase-4 physical allocator."""

from pathlib import Path
import re
import subprocess
import time


ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build"
ISO = BUILD / "AxiomOS.iso"
SERIAL = BUILD / "phase4-serial.log"
QEMU_LOG = BUILD / "phase4-qemu.log"


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def extract_u64(text, label):
    match = re.search(rf"^{re.escape(label)}: (\d+)$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


def main():
    subprocess.run(["make", "MODE=normal", "all"], cwd=ROOT, check=True)

    SERIAL.write_text("")

    command = [
        "qemu-system-x86_64",
        "-machine", "q35",
        "-accel", "tcg",
        "-m", "256M",
        "-cdrom", str(ISO),
        "-boot", "d",
        "-display", "none",
        "-serial", f"file:{SERIAL}",
        "-monitor", "none",
        "-no-reboot",
        "-no-shutdown",
    ]

    with QEMU_LOG.open("w") as output:
        process = subprocess.Popen(command, stdout=output, stderr=subprocess.STDOUT)

        try:
            deadline = time.monotonic() + 20
            marker = "Phase 4 physical memory manager complete."

            while marker not in SERIAL.read_text(errors="replace"):
                require(process.poll() is None, "QEMU exited before Phase 4 completed")
                require(time.monotonic() < deadline,
                        f"Phase 4 boot timed out; see {SERIAL}")
                time.sleep(0.05)
        finally:
            if process.poll() is None:
                process.terminate()

            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()

    text = SERIAL.read_text(errors="replace").replace("\r", "")

    for expected in [
        "AxiomOS kernel booted successfully.",
        "Phase 2 terminal test complete.",
        "Phase 3 CPU initialization complete.",
        "AxiomOS Phase 4 physical memory manager online.",
        "Phase 4 PMM allocation/free test: OK",
        "Phase 4 physical memory manager complete.",
    ]:
        require(expected in text, f"Missing output: {expected}")

    require("FAILED" not in text, "Kernel reported a Phase 4 failure")
    require("KERNEL PANIC" not in text, "Normal Phase 4 boot panicked")

    usable_match = re.search(
        r"^PMM usable memory: (\d+) MiB \((\d+) pages\)$",
        text,
        re.MULTILINE,
    )
    require(usable_match is not None, "Missing PMM usable-memory diagnostics")

    usable_mib = int(usable_match.group(1))
    usable_pages = int(usable_match.group(2))
    metadata_match = re.search(
        r"^PMM metadata: (\d+) pages at physical 0x([0-9A-F]+)$",
        text,
        re.MULTILINE,
    )
    require(metadata_match is not None, "Missing PMM metadata diagnostics")
    metadata_pages = int(metadata_match.group(1))
    metadata_base = int(metadata_match.group(2), 16)

    allocated = extract_u64(text, "PMM allocated pages")
    free = extract_u64(text, "PMM free pages")
    post_allocated = extract_u64(text, "PMM post-test allocated pages")
    post_free = extract_u64(text, "PMM post-test free pages")

    require(usable_pages > 0 and usable_mib > 0, "No usable RAM reported")
    require(metadata_pages > 0, "Allocator reserved no bitmap pages")
    require(metadata_base % 4096 == 0, "PMM metadata is not page aligned")
    require(allocated == metadata_pages,
            "Initial allocated count should equal allocator metadata pages")
    require(allocated + free == usable_pages,
            "Allocated + free pages does not equal usable pages")
    require(post_allocated == allocated and post_free == free,
            "Self-test did not restore allocator accounting")

    print("PASS: Phase 4 physical memory manager")
    print(f"  usable pages:    {usable_pages}")
    print(f"  metadata pages:  {metadata_pages}")
    print(f"  allocated pages: {allocated}")
    print(f"  free pages:      {free}")


if __name__ == "__main__":
    main()
