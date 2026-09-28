#!/usr/bin/env python3
"""Boot AxiomOS and validate Phase-5 paging plus a real #PF path."""

from pathlib import Path
import re
import subprocess
import time


ROOT = Path(__file__).resolve().parents[1]
FAULT_ADDRESS = 0x0000612345600000


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def run_image(mode, marker):
    directory = ROOT / ("build" if mode == "normal" else f"build-{mode}")
    directory.mkdir(exist_ok=True)

    with (directory / "phase5-build.log").open("w") as output:
        subprocess.run(
            ["make", f"MODE={mode}", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = directory / "phase5-serial.log"
    qemu_log = directory / "phase5-qemu.log"
    serial.write_text("")

    command = [
        "qemu-system-x86_64",
        "-machine", "q35",
        "-accel", "tcg",
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
        process = subprocess.Popen(command, stdout=output, stderr=subprocess.STDOUT)

        try:
            deadline = time.monotonic() + 25

            while marker not in serial.read_text(errors="replace"):
                require(process.poll() is None,
                        f"{mode}: QEMU exited before {marker!r}")
                require(time.monotonic() < deadline,
                        f"{mode}: boot timed out; see {serial}")
                time.sleep(0.05)
        finally:
            if process.poll() is None:
                process.terminate()

            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()

    return serial.read_text(errors="replace").replace("\r", "")


def parse_hex(text, label):
    match = re.search(rf"^{re.escape(label)}: (?:physical )?0x([0-9A-F]+)$",
                      text, re.MULTILINE)
    require(match is not None, f"Missing hexadecimal field: {label}")
    return int(match.group(1), 16)


def parse_u64(text, label):
    match = re.search(rf"^{re.escape(label)}: (\d+)$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


def test_normal():
    text = run_image("normal", "Phase 5 virtual memory manager complete.")

    for expected in [
        "AxiomOS kernel booted successfully.",
        "Phase 2 terminal test complete.",
        "Phase 3 CPU initialization complete.",
        "Phase 4 physical memory manager complete.",
        "AxiomOS Phase 5 virtual memory manager online.",
        "Phase 5 virtual memory test: OK",
        "Phase 5 virtual memory manager complete.",
    ]:
        require(expected in text, f"normal: missing output: {expected}")

    require("FAILED" not in text, "normal: kernel reported a failure")
    require("KERNEL PANIC" not in text, "normal: paging boot panicked")

    root = parse_hex(text, "VMM root PML4")
    hhdm = parse_hex(text, "VMM HHDM offset")
    kernel_phys = parse_hex(text, "kernel_main physical address")
    page_tables = parse_u64(text, "VMM owned page-table pages")

    require(root != 0 and root % 4096 == 0,
            "normal: root PML4 is not a valid page-aligned physical address")
    require(page_tables > 0, "normal: VMM owns no page-table pages")
    require(hhdm != 0, "normal: HHDM offset is zero")
    require(kernel_phys != 0xFFFFFFFFFFFFFFFF,
            "normal: kernel_main failed virtual-to-physical translation")

    print("PASS: Phase 5 virtual memory manager")
    print(f"  root PML4:       0x{root:X}")
    print(f"  page-table pages: {page_tables}")
    print(f"  HHDM offset:     0x{hhdm:X}")
    print(f"  kernel_main phys: 0x{kernel_phys:X}")


def test_page_fault():
    text = run_image("page_fault", "CPU halted.")

    for expected in [
        "Phase 5 virtual memory test: OK",
        "Phase 5 virtual memory manager complete.",
        "Test: triggering unmapped page read.",
        "KERNEL PANIC",
        "Exception: Page Fault",
        "Vector: 14  Error: 0x0",
        f"CR2: 0x{FAULT_ADDRESS:X}",
        "Cause: Non-present page",
        "Access: Read",
        "Privilege: Supervisor",
        "Reserved-bit violation: no",
        "Instruction fetch: no",
        "CPU halted.",
    ]:
        require(expected in text, f"page_fault: missing output: {expected}")

    require(text.count("KERNEL PANIC") == 1,
            "page_fault: expected exactly one panic")
    require("NESTED KERNEL PANIC" not in text,
            "page_fault: nested panic occurred")
    require("Phase 5 page-fault test: FAILED to fault" not in text,
            "page_fault: unmapped access unexpectedly succeeded")

    print("PASS: Phase 5 page-fault handler")
    print(f"  CR2: 0x{FAULT_ADDRESS:X}")
    print("  error code: 0x0 (supervisor read of non-present page)")


if __name__ == "__main__":
    test_normal()
    test_page_fault()
    print("PASS: all Phase 5 virtual-memory tests.")
