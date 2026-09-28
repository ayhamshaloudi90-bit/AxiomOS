#!/usr/bin/env python3
"""Boot AxiomOS and validate the Phase-6 kernel heap and corruption panics."""

from pathlib import Path
import re
import subprocess
import time


ROOT = Path(__file__).resolve().parents[1]


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def run_image(mode, marker):
    directory = ROOT / ("build" if mode == "normal" else f"build-{mode}")
    directory.mkdir(exist_ok=True)

    with (directory / "phase6-build.log").open("w") as output:
        subprocess.run(
            ["make", f"MODE={mode}", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = directory / "phase6-serial.log"
    qemu_log = directory / "phase6-qemu.log"
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
            deadline = time.monotonic() + 30

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


def parse_u64(text, label):
    match = re.search(rf"^{re.escape(label)}: (\d+)$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


def parse_hex(text, label):
    match = re.search(rf"^{re.escape(label)}: 0x([0-9A-F]+)$", text, re.MULTILINE)
    require(match is not None, f"Missing hexadecimal field: {label}")
    return int(match.group(1), 16)


def test_normal():
    text = run_image("normal", "Phase 6 kernel heap complete.")

    for expected in [
        "AxiomOS kernel booted successfully.",
        "Phase 2 terminal test complete.",
        "Phase 3 CPU initialization complete.",
        "Phase 4 physical memory manager complete.",
        "Phase 5 virtual memory manager complete.",
        "AxiomOS Phase 6 kernel heap online.",
        "Phase 6 heap self-test: OK",
        "Phase 6 kernel heap complete.",
    ]:
        require(expected in text, f"normal: missing output: {expected}")

    require("KERNEL PANIC" not in text, "normal: heap boot panicked")
    require("FAILED (" not in text, "normal: heap self-test reported failure")
    require("Phase 6 heap initialization: FAILED" not in text,
            "normal: heap initialization failed")

    heap_base = parse_hex(text, "Heap base")
    initial_pages = parse_u64(text, "Heap mapped pages")
    mapped_pages = parse_u64(text, "Heap mapped pages after test")
    active = parse_u64(text, "Heap active allocations")
    in_use = parse_u64(text, "Heap bytes in use")
    free_bytes = parse_u64(text, "Heap free bytes")
    largest_free = parse_u64(text, "Heap largest free block")
    allocations = parse_u64(text, "Heap total allocations")
    frees = parse_u64(text, "Heap total frees")
    reallocations = parse_u64(text, "Heap reallocations")
    failed = parse_u64(text, "Heap failed allocations")

    require(heap_base == 0xFFFFC00000000000,
            "normal: unexpected kernel heap base")
    require(initial_pages == 4, "normal: heap did not start with four pages")
    require(mapped_pages > initial_pages,
            "normal: stress allocation did not grow the heap")
    require(active == 0, "normal: self-test left live allocations")
    require(in_use == 0, "normal: self-test left bytes in use")
    require(free_bytes > 0, "normal: heap reports no free memory")
    require(largest_free == free_bytes,
            "normal: final free space did not fully coalesce")
    require(allocations == frees and allocations > 0,
            "normal: successful allocation/free counters are unbalanced")
    require(reallocations >= 2,
            "normal: realloc growth/shrink paths were not both exercised")
    require(failed >= 1,
            "normal: overflow/failure path was not exercised")

    print("PASS: Phase 6 kernel heap")
    print(f"  mapped pages:       {mapped_pages}")
    print(f"  final free bytes:   {free_bytes}")
    print(f"  allocations/frees:  {allocations}/{frees}")
    print(f"  reallocations:      {reallocations}")
    print(f"  failed allocations: {failed}")


def test_double_free():
    text = run_image("heap_double_free", "CPU halted.")

    for expected in [
        "Phase 6 heap self-test: OK",
        "Phase 6 kernel heap complete.",
        "Test: triggering heap double free.",
        "KERNEL PANIC",
        "Reason: Heap double free detected",
        "CPU halted.",
    ]:
        require(expected in text, f"heap_double_free: missing output: {expected}")

    require(text.count("KERNEL PANIC") == 1,
            "heap_double_free: expected exactly one panic")
    require("Phase 6 double-free test: FAILED to panic" not in text,
            "heap_double_free: second free unexpectedly returned")

    print("PASS: Phase 6 double-free protection")


def test_guard_corruption():
    text = run_image("heap_guard", "CPU halted.")

    for expected in [
        "Phase 6 heap self-test: OK",
        "Phase 6 kernel heap complete.",
        "Test: triggering heap tail-guard corruption.",
        "KERNEL PANIC",
        "Reason: Heap tail guard corrupted",
        "CPU halted.",
    ]:
        require(expected in text, f"heap_guard: missing output: {expected}")

    require(text.count("KERNEL PANIC") == 1,
            "heap_guard: expected exactly one panic")
    require("Phase 6 guard-corruption test: FAILED to panic" not in text,
            "heap_guard: corrupted allocation unexpectedly freed")

    print("PASS: Phase 6 tail-guard corruption detection")


if __name__ == "__main__":
    test_normal()
    test_double_free()
    test_guard_corruption()
    print("PASS: all Phase 6 kernel-heap tests.")
