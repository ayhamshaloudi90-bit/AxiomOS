#!/usr/bin/env python3
"""Boot AxiomOS and prove Phase-8 timer-driven preemptive multitasking."""

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
            tail = "\n".join(text.replace("\r", "").splitlines()[-40:])
            raise AssertionError(
                f"phase8 timed out waiting for {marker!r}; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )

        time.sleep(0.05)


def parse_u64(text, label):
    match = re.search(rf"^{re.escape(label)}: (\d+)$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


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


def test_phase8():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase8-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = directory / "phase8-serial.log"
    qemu_log = directory / "phase8-qemu.log"
    monitor = directory / "phase8-monitor.sock"

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
                "Phase 8 multitasking complete.",
                time.monotonic() + 45,
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
                "Phase 7 timer + keyboard drivers complete.",
                "AxiomOS Phase 8 preemptive scheduler online.",
                "Scheduler policy: preemptive round-robin",
                "Scheduler quantum: 5 timer ticks",
                "Phase 8 preemptive multitasking test: OK",
                "Phase 8 multitasking complete.",
            ]:
                require(expected in text, f"phase8: missing output: {expected}")

            require("KERNEL PANIC" not in text, "phase8: kernel panicked")
            require("Phase 8 preemptive multitasking test: FAILED" not in text,
                    "phase8: kernel self-test reported failure")

            task_count = parse_u64(text, "Phase 8 task count")
            worker_a = parse_u64(text, "Phase 8 worker A count")
            worker_b = parse_u64(text, "Phase 8 worker B count")
            switches = parse_u64(text, "Phase 8 context switches")
            preemptions = parse_u64(text, "Phase 8 timer preemptions")

            require(task_count == 3,
                    f"phase8: expected 3 schedulable tasks, got {task_count}")
            require(worker_a >= 1000,
                    "phase8: worker A did not make independent progress")
            require(worker_b >= 1000,
                    "phase8: worker B did not make independent progress")
            require(switches >= 3,
                    "phase8: too few context switches observed")
            require(preemptions >= 3,
                    "phase8: scheduler was not timer-preemptive")

            # The scheduler must coexist with the Phase-7 IRQ-driven keyboard.
            send_keys(monitor, ["k", "b", "d", "s", "t", "a", "t", "s", "ret"])
            wait_for_text(
                process,
                serial,
                "Keyboard IRQs/scancodes/chars/dropped:",
                time.monotonic() + 10,
            )

            final_text = serial.read_text(errors="replace").replace("\r", "")
            require("Keyboard IRQs/scancodes/chars/dropped:" in final_text,
                    "phase8: keyboard input broke after scheduler start")
            require("KERNEL PANIC" not in final_text,
                    "phase8: kernel panicked during scheduler/keyboard coexistence test")

            print("PASS: Phase 8 preemptive round-robin scheduler")
            print(f"  tasks:            {task_count}")
            print(f"  worker A count:   {worker_a}")
            print(f"  worker B count:   {worker_b}")
            print(f"  context switches: {switches}")
            print(f"  preemptions:      {preemptions}")
            print("PASS: Phase 8 scheduler + keyboard coexistence")
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
    test_phase8()
    print("PASS: all Phase 8 multitasking tests.")
