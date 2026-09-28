#!/usr/bin/env python3
"""Boot AxiomOS and validate Phase-7 APIC timer + PS/2 keyboard interrupts."""

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

        require(process.poll() is None,
                f"QEMU exited before {marker!r}")

        if time.monotonic() >= deadline:
            tail = "\n".join(text.replace("\r", "").splitlines()[-30:])
            raise AssertionError(
                f"boot/input timed out waiting for {marker!r}; "
                f"see {serial}\n--- serial tail ---\n{tail}"
            )

        time.sleep(0.05)


def send_keys(monitor_path, keys):
    deadline = time.monotonic() + 5

    while not monitor_path.exists():
        require(time.monotonic() < deadline,
                "QEMU monitor socket was not created")
        time.sleep(0.05)

    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
        connection.settimeout(2)
        connection.connect(str(monitor_path))

        # Consume the HMP greeting when available; its contents are irrelevant.
        try:
            connection.recv(4096)
        except socket.timeout:
            pass

        for key in keys:
            connection.sendall(f"sendkey {key} 30\n".encode("ascii"))
            time.sleep(0.08)


def parse_u64(text, label):
    match = re.search(rf"^{re.escape(label)}: (\d+)$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


def test_phase7():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase7-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = directory / "phase7-serial.log"
    qemu_log = directory / "phase7-qemu.log"
    monitor = directory / "phase7-monitor.sock"

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
            # Later phases now run before the shell. Keep this regression
            # timeout large enough for the integrated modern boot path.
            wait_for_text(
                process,
                serial,
                "AxiomOS shell ready. Type 'help' for commands.",
                time.monotonic() + 90,
            )

            before_input = serial.read_text(errors="replace").replace("\r", "")

            for expected in [
                "AxiomOS kernel booted successfully.",
                "Phase 3 CPU initialization complete.",
                "Phase 4 physical memory manager complete.",
                "Phase 5 virtual memory manager complete.",
                "Phase 6 kernel heap complete.",
                "AxiomOS Phase 7 timer + keyboard online.",
                "APIC timer frequency: 100 Hz",
                "Interrupt controller: Local APIC + I/O APIC",
                "Phase 7 timer test: OK",
                "Phase 7 timer + keyboard drivers complete.",
                "AxiomOS shell ready. Type 'help' for commands.",
            ]:
                require(expected in before_input,
                        f"phase7: missing output: {expected}")

            require("KERNEL PANIC" not in before_input,
                    "phase7: kernel panicked before keyboard test")
            require("Phase 7 timer initialization: FAILED" not in before_input,
                    "phase7: APIC timer initialization failed")
            require("Phase 7 keyboard initialization: FAILED" not in before_input,
                    "phase7: PS/2 keyboard initialization failed")

            timer_ticks = parse_u64(before_input, "Timer self-test ticks")
            require(timer_ticks >= 10,
                    "phase7: timer did not deliver ten APIC ticks")

            # HMP injects actual virtual key events into QEMU's PS/2 keyboard.
            send_keys(monitor, ["k", "b", "d", "s", "t", "a", "t", "s", "ret"])

            wait_for_text(
                process,
                serial,
                "Keyboard IRQs/scancodes/chars/dropped:",
                time.monotonic() + 8,
            )

            text = serial.read_text(errors="replace").replace("\r", "")

            require("kbdstats" in text,
                    "phase7: injected PS/2 command was not decoded/buffered")
            require("KERNEL PANIC" not in text,
                    "phase7: keyboard IRQ path panicked")

            stats_match = re.search(
                r"Keyboard IRQs/scancodes/chars/dropped: "
                r"(\d+)/(\d+)/(\d+)/(\d+)",
                text,
            )
            require(stats_match is not None,
                    "phase7: keyboard statistics were not printed")

            irqs, scancodes, characters, dropped = (
                int(value) for value in stats_match.groups()
            )

            require(irqs > 0, "phase7: no IRQ1 interrupts were observed")
            require(scancodes >= 6,
                    "phase7: too few keyboard scancodes were observed")
            require(characters >= 6,
                    "phase7: decoder did not produce the expected characters")
            require(dropped == 0,
                    "phase7: keyboard ring buffer unexpectedly overflowed")

            print("PASS: Phase 7 APIC timer")
            print("  frequency: 100 Hz")
            print(f"  self-test ticks: {timer_ticks}")
            print("PASS: Phase 7 PS/2 keyboard")
            print(f"  IRQs:       {irqs}")
            print(f"  scancodes:  {scancodes}")
            print(f"  characters: {characters}")
            print(f"  dropped:    {dropped}")
            print("  shell command: kbdstats")
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
    test_phase7()
    print("PASS: all Phase 7 timer/keyboard tests.")
