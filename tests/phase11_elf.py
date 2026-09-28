#!/usr/bin/env python3
"""Boot AxiomOS and validate the Phase-11 ELF64 executable loader."""

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
            tail = "\n".join(text.replace("\r", "").splitlines()[-70:])
            raise AssertionError(
                f"phase11 timed out waiting for {marker!r}; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )

        time.sleep(0.05)


def parse_decimal(text, label, signed=False):
    pattern = r"(-?\d+)" if signed else r"(\d+)"
    match = re.search(rf"^{re.escape(label)}: {pattern}$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


def parse_hex(text, label):
    match = re.search(rf"^{re.escape(label)}: 0x([0-9A-Fa-f]+)$", text, re.MULTILINE)
    require(match is not None, f"Missing hex field: {label}")
    return int(match.group(1), 16)


def inspect_userspace_elf(path):
    readelf = shutil.which("llvm-readelf") or shutil.which("readelf")
    require(readelf is not None, "phase11: llvm-readelf/readelf is unavailable")

    result = subprocess.run(
        [readelf, "-h", "-l", str(path)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=True,
    )
    text = result.stdout

    require("ELF64" in text, "phase11: user executable is not ELF64")
    require("EXEC (Executable file)" in text or "Type:                              EXEC" in text,
            "phase11: user executable is not ET_EXEC")
    require("Advanced Micro Devices X86-64" in text or "AMD x86-64" in text,
            "phase11: user executable is not x86-64")
    require("Entry point address:               0x400000" in text,
            "phase11: unexpected ELF entry point")

    load_lines = [line for line in text.splitlines() if line.strip().startswith("LOAD")]
    require(len(load_lines) == 3,
            f"phase11: expected 3 PT_LOAD entries, saw {len(load_lines)}")


def test_phase11():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase11-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    launcher_elf = directory / "userspace" / "phase11_launcher.elf"
    user_elf = directory / "userspace" / "phase11_demo.elf"
    require(launcher_elf.exists(), "phase11: exec launcher ELF was not built")
    require(user_elf.exists(), "phase11: standalone userspace ELF was not built")
    inspect_userspace_elf(launcher_elf)
    inspect_userspace_elf(user_elf)

    serial = directory / "phase11-serial.log"
    qemu_log = directory / "phase11-qemu.log"
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
                "Phase 11 ELF loader complete.",
                time.monotonic() + 80,
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

            for expected in [
                "Phase 10 system calls complete.",
                "AxiomOS Phase 11 ELF64 loader online.",
                "ELF launcher source: /boot/phase11_launcher.elf (Limine module)",
                "ELF exec target: /bin/phase11-demo -> /boot/phase11_demo.elf",
                "ELF type: ET_EXEC x86-64",
                "Phase 11 segment permissions: RX/R/RW+NX OK",
                "Hello from an ELF64 executable in AxiomOS!",
                "Phase 11 exec preserved task ID: OK",
                "Phase 11 BSS zero-fill: OK",
                "Phase 11 ELF executable test: OK",
                "Phase 11 ELF loader complete.",
            ]:
                require(expected in text, f"phase11: missing output: {expected}")

            require("KERNEL PANIC" not in text, "phase11: kernel panicked")
            require("Phase 11 ELF task creation: FAILED" not in text,
                    "phase11: ELF task creation failed")
            require("Phase 11 ELF mapping validation: FAILED" not in text,
                    "phase11: ELF page-permission validation failed")
            require("Phase 11 ELF executable test: FAILED" not in text,
                    "phase11: ELF executable self-test failed")

            entry = parse_hex(text, "ELF entry point")
            phnum = parse_decimal(text, "ELF program headers")
            segments = parse_decimal(text, "ELF PT_LOAD segments")
            pages = parse_decimal(text, "ELF mapped pages")
            task_id = parse_decimal(text, "Phase 11 ELF task ID")
            pid = parse_decimal(text, "Phase 11 getpid result")
            exit_status = parse_decimal(text, "Phase 11 exit status", signed=True)
            exec_match = re.search(
                r"^Phase 11 exec transitions: (\d+)/(\d+)$",
                text,
                re.MULTILINE,
            )
            require(exec_match is not None,
                    "phase11: missing exec transition diagnostics")
            exec_successes, exec_calls = (int(value) for value in exec_match.groups())

            byte_match = re.search(
                r"^ELF file/memory bytes: (\d+)/(\d+)$",
                text,
                re.MULTILINE,
            )
            require(byte_match is not None,
                    "phase11: missing ELF file/memory byte diagnostics")
            file_bytes, memory_bytes = (int(value) for value in byte_match.groups())

            require(entry == 0x400000,
                    f"phase11: unexpected loaded entry 0x{entry:X}")
            require(phnum == 3, f"phase11: expected 3 program headers, got {phnum}")
            require(segments == 3,
                    f"phase11: expected 3 PT_LOAD segments, got {segments}")
            require(pages == 3,
                    f"phase11: expected 3 image pages, got {pages}")
            require(pid == task_id,
                    f"phase11: getpid returned {pid}, task id is {task_id}")
            require(exit_status == 11,
                    f"phase11: exit status {exit_status}, expected 11")
            require(memory_bytes > file_bytes,
                    "phase11: BSS did not make p_memsz larger than p_filesz")
            require(exec_calls >= 1 and exec_successes >= 1,
                    "phase11: SYS_exec did not replace the launcher image")

            print("PASS: Phase 11 standalone ELF64 executable")
            print(f"  entry:            0x{entry:X}")
            print(f"  program headers:  {phnum}")
            print(f"  PT_LOAD segments: {segments}")
            print(f"  mapped pages:     {pages}")
            print("PASS: Phase 11 ELF segment permissions + BSS zero-fill")
            print(f"  file/memory:      {file_bytes}/{memory_bytes} bytes")
            print("PASS: Phase 11 exec() image replacement (current kernel resolves target through VFS)")
            print(f"  exec success/calls: {exec_successes}/{exec_calls}")
            print("PASS: Phase 11 ELF process executed through existing syscall ABI")
            print(f"  task id/getpid:   {task_id}/{pid}")
            print(f"  exit status:      {exit_status}")
        finally:
            if process.poll() is None:
                process.terminate()

            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


if __name__ == "__main__":
    test_phase11()
    print("PASS: all Phase 11 ELF-loader tests.")
