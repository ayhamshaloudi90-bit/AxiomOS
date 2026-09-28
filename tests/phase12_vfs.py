#!/usr/bin/env python3
"""Boot AxiomOS and validate the Phase-12 VFS/RAM filesystem."""

from pathlib import Path
import re
import shutil
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
            tail = "\n".join(text.replace("\r", "").splitlines()[-90:])
            raise AssertionError(
                f"phase12 timed out waiting for {marker!r}; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )

        time.sleep(0.05)


def parse_decimal(text, label):
    match = re.search(rf"^{re.escape(label)}: (\d+)$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


def inspect_userspace_elf(path):
    readelf = shutil.which("llvm-readelf") or shutil.which("readelf")
    require(readelf is not None, "phase12: llvm-readelf/readelf is unavailable")

    result = subprocess.run(
        [readelf, "-h", "-l", str(path)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=True,
    )
    text = result.stdout

    require("ELF64" in text, "phase12: VFS demo is not ELF64")
    require("EXEC (Executable file)" in text or
            "Type:                              EXEC" in text,
            "phase12: VFS demo is not ET_EXEC")
    require("Entry point address:               0x400000" in text,
            "phase12: unexpected VFS demo entry point")

    load_lines = [line for line in text.splitlines() if line.strip().startswith("LOAD")]
    require(len(load_lines) == 3,
            f"phase12: expected 3 PT_LOAD entries, saw {len(load_lines)}")


def test_phase12():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase12-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    demo_elf = directory / "userspace" / "phase12_demo.elf"
    require(demo_elf.exists(), "phase12: VFS demo ELF was not built")
    inspect_userspace_elf(demo_elf)

    serial = directory / "phase12-serial.log"
    qemu_log = directory / "phase12-qemu.log"
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
            wait_for_text(
                process,
                serial,
                "Phase 12 virtual filesystem complete.",
                time.monotonic() + 90,
            )
            # Later phases now run before the shell. Keep this regression
            # timeout large enough for the integrated modern boot path.
            wait_for_text(
                process,
                serial,
                "AxiomOS shell ready. Type 'help' for commands.",
                time.monotonic() + 90,
            )

            text = serial.read_text(errors="replace").replace("\r", "")

            expected_lines = [
                "Phase 11 ELF loader complete.",
                "AxiomOS Phase 12 virtual filesystem online.",
                "VFS root filesystem: ramfs mounted at /",
                "VFS secondary mount: ramfs mounted at /tmp",
                "VFS /bin type: DIRECTORY",
                "VFS /tmp type: DIRECTORY (mount point)",
                "Phase 12 ELF source: /bin/phase12-demo (VFS)",
                "Phase 12 VFS motd: Welcome to the AxiomOS virtual filesystem!",
                "Phase 12 VFS readback: RAM filesystem round-trip works!",
                "Phase 12 /tmp file round-trip: OK",
                "Phase 12 seek/stat operations: OK",
                "Phase 12 exec through VFS: OK",
                "Phase 12 VFS self-test: OK",
                "Phase 12 virtual filesystem complete.",
            ]

            for expected in expected_lines:
                require(expected in text, f"phase12: missing output: {expected}")

            require("KERNEL PANIC" not in text, "phase12: kernel panicked")
            require("Phase 12 VFS bootstrap: FAILED" not in text,
                    "phase12: VFS bootstrap failed")
            require("Phase 12 VFS task creation: FAILED" not in text,
                    "phase12: VFS ELF task creation failed")
            require("Phase 12 userspace VFS demo: FAILED" not in text,
                    "phase12: userspace VFS demo failed")
            require("Phase 12 VFS self-test: FAILED" not in text,
                    "phase12: VFS self-test failed")

            mounts = parse_decimal(text, "VFS mount count")
            task_id = parse_decimal(text, "Phase 12 ELF task ID")
            fd_base = parse_decimal(text, "Phase 12 file descriptor base")

            open_close = re.search(
                r"^Phase 12 VFS opens/closes: (\d+)/(\d+)$",
                text,
                re.MULTILINE,
            )
            require(open_close is not None,
                    "phase12: missing VFS open/close diagnostics")
            opens, closes = (int(value) for value in open_close.groups())

            byte_counts = re.search(
                r"^Phase 12 VFS bytes read/written: (\d+)/(\d+)$",
                text,
                re.MULTILINE,
            )
            require(byte_counts is not None,
                    "phase12: missing VFS byte diagnostics")
            bytes_read, bytes_written = (int(value) for value in byte_counts.groups())

            require(mounts == 2, f"phase12: expected two mounts, got {mounts}")
            require(task_id > 0, "phase12: invalid userspace task id")
            require(fd_base == 3, f"phase12: first regular fd should be 3, got {fd_base}")
            require(opens >= 4 and closes >= 4,
                    "phase12: too little VFS open/close activity")
            require(bytes_read > 0 and bytes_written > 0,
                    "phase12: VFS moved no file data")
            require(text.count("Hello from an ELF64 executable in AxiomOS!") >= 2,
                    "phase12: VFS-backed exec target did not execute")

            print("PASS: Phase 12 VFS path and mount resolution")
            print(f"  mounts:            {mounts}")
            print("PASS: Phase 12 per-process file descriptors")
            print(f"  first regular fd:  {fd_base}")
            print("PASS: Phase 12 RAM filesystem read/write/seek/stat")
            print(f"  VFS opens/closes:  {opens}/{closes}")
            print(f"  bytes read/written:{bytes_read}/{bytes_written}")
            print("PASS: Phase 12 exec() resolves through the VFS")
        finally:
            if process.poll() is None:
                process.terminate()

            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


if __name__ == "__main__":
    test_phase12()
    print("PASS: all Phase 12 VFS tests.")
