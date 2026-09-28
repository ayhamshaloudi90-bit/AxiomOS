#!/usr/bin/env python3
"""Boot AxiomOS and prove Phase-9 Ring-3 isolation and privilege transitions."""

from pathlib import Path
import re
import socket
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
            tail = "\n".join(text.replace("\r", "").splitlines()[-50:])
            raise AssertionError(
                f"phase9 timed out waiting for {marker!r}; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )

        time.sleep(0.05)


def parse_decimal(text, label):
    match = re.search(rf"^{re.escape(label)}: (\d+)$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


def parse_hex(text, label):
    match = re.search(rf"^{re.escape(label)}: 0x([0-9A-Fa-f]+)$", text, re.MULTILINE)
    require(match is not None, f"Missing hex field: {label}")
    return int(match.group(1), 16)


def send_keys(monitor_path, keys):
    deadline = time.monotonic() + 5

    while not monitor_path.exists():
        require(time.monotonic() < deadline,
                "QEMU monitor socket was not created")
        time.sleep(0.05)

    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
        connection.settimeout(2)
        connection.connect(str(monitor_path))

        try:
            connection.recv(4096)
        except socket.timeout:
            pass

        for key in keys:
            connection.sendall(f"sendkey {key} 30\n".encode("ascii"))
            time.sleep(0.08)


def test_phase9():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase9-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = directory / "phase9-serial.log"
    qemu_log = directory / "phase9-qemu.log"
    monitor = directory / "phase9-monitor.sock"

    serial.write_text("")
    monitor.unlink(missing_ok=True)

    command = [
        "qemu-system-x86_64",
        "-machine", "q35",
        "-accel", "tcg",
        "-m", "256M",
        "-cdrom", str(directory / "AxiomOS.iso"),
        "-boot", "d",
        "-display", "none",
        "-serial", f"file:{serial}",
        "-monitor", f"unix:{monitor},server=on,wait=off",
        "-no-reboot",
        "-no-shutdown",
    ]

    with qemu_log.open("w") as output:
        process = subprocess.Popen(command, stdout=output, stderr=subprocess.STDOUT)

        try:
            wait_for_text(
                process,
                serial,
                "Phase 9 userspace complete.",
                time.monotonic() + 50,
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
                "Phase 8 preemptive multitasking test: OK",
                "AxiomOS Phase 9 Ring 3 userspace online.",
                "Phase 9 user privilege: Ring 3",
                "Phase 9 separate address spaces: OK",
                "Hello from AxiomOS userspace!",
                "Phase 9 kernel protection fault: OK",
                "Phase 9 userspace isolation test: OK",
                "Phase 9 userspace complete.",
            ]:
                require(expected in text, f"phase9: missing output: {expected}")

            require("KERNEL PANIC" not in text, "phase9: kernel panicked")
            require("Phase 9 userspace isolation test: FAILED" not in text,
                    "phase9: userspace isolation self-test failed")

            kernel_match = re.search(
                r"^VMM root PML4: physical 0x([0-9A-Fa-f]+)$",
                text,
                re.MULTILINE,
            )
            require(kernel_match is not None, "phase9: missing kernel CR3 field")
            kernel_space = int(kernel_match.group(1), 16)
            hello_space = parse_hex(text, "Phase 9 hello address space")
            protection_space = parse_hex(text, "Phase 9 protection address space")
            fault_vector = parse_decimal(text, "Phase 9 protection fault vector")
            fault_address = parse_hex(text, "Phase 9 protection fault address")
            fault_error = parse_hex(text, "Phase 9 protection fault error")
            fault_terminations = parse_decimal(text, "Phase 9 user fault terminations")

            require(hello_space != kernel_space,
                    "phase9: hello task reused the kernel address space")
            require(protection_space != kernel_space,
                    "phase9: protection task reused the kernel address space")
            require(hello_space != protection_space,
                    "phase9: user tasks did not receive separate address spaces")
            require(fault_vector == 14,
                    f"phase9: expected #PF vector 14, got {fault_vector}")
            require(fault_address == 0xFFFFFFFF80000000,
                    f"phase9: unexpected protected address 0x{fault_address:X}")
            require((fault_error & 0x1) != 0,
                    "phase9: kernel access did not fault as a protection violation")
            require((fault_error & 0x4) != 0,
                    "phase9: fault was not generated from user mode")
            require(fault_terminations >= 1,
                    "phase9: faulting user task was not terminated")

            # IRQ input should still work while Ring-3 and kernel tasks coexist.
            send_keys(monitor, ["k", "b", "d", "s", "t", "a", "t", "s", "ret"])
            wait_for_text(
                process,
                serial,
                "Keyboard IRQs/scancodes/chars/dropped:",
                time.monotonic() + 10,
            )

            final_text = serial.read_text(errors="replace").replace("\r", "")
            require("Keyboard IRQs/scancodes/chars/dropped:" in final_text,
                    "phase9: keyboard input broke after entering Ring 3")
            require("KERNEL PANIC" not in final_text,
                    "phase9: kernel panicked during userspace/keyboard coexistence")

            print("PASS: Phase 9 Ring 3 userspace")
            print(f"  kernel CR3:      0x{kernel_space:X}")
            print(f"  hello CR3:       0x{hello_space:X}")
            print(f"  protection CR3:  0x{protection_space:X}")
            print("PASS: Phase 9 hardware memory isolation")
            print(f"  fault vector:    {fault_vector}")
            print(f"  fault address:   0x{fault_address:X}")
            print(f"  fault error:     0x{fault_error:X}")
            print("PASS: Phase 9 userspace + scheduler + keyboard coexistence")
        finally:
            if process.poll() is None:
                process.terminate()

            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()

            monitor.unlink(missing_ok=True)


if __name__ == "__main__":
    test_phase9()
    print("PASS: all Phase 9 userspace/isolation tests.")
