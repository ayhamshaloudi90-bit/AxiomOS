#!/usr/bin/env python3
"""Validate AxiomOS Phase-21 priority/aging scheduler and affinity ABI."""
from pathlib import Path
import socket
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
        if "KERNEL PANIC" in text:
            tail = "\n".join(text.splitlines()[-240:])
            raise AssertionError(f"phase21 kernel panicked\n--- serial tail ---\n{tail}")
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-240:])
            raise AssertionError(
                f"phase21 timed out waiting for {marker!r}\n--- serial tail ---\n{tail}"
            )
        time.sleep(0.05)


def send_hmp_keys(monitor_path, keys):
    deadline = time.monotonic() + 5
    while not monitor_path.exists():
        require(time.monotonic() < deadline, "QEMU monitor socket was not created")
        time.sleep(0.05)
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
        connection.settimeout(2)
        connection.connect(str(monitor_path))
        try:
            connection.recv(4096)
        except socket.timeout:
            pass
        for key in keys:
            connection.sendall(f"sendkey {key} 20\n".encode("ascii"))
            time.sleep(0.035)


def send_command(monitor_path, command):
    mapping = {" ": "spc", "/": "slash", ".": "dot", "-": "minus", "_": "shift-minus"}
    keys = []
    for character in command:
        if "a" <= character <= "z" or "0" <= character <= "9":
            keys.append(character)
        else:
            require(character in mapping, f"phase21: no HMP key mapping for {character!r}")
            keys.append(mapping[character])
    keys.append("ret")
    send_hmp_keys(monitor_path, keys)


def test_phase21():
    build = ROOT / "build"
    build.mkdir(exist_ok=True)
    with (build / "phase21-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = build / "phase21-serial.log"
    monitor = build / "phase21-monitor.sock"
    qemu_log = build / "phase21-qemu.log"
    serial.write_text("")
    monitor.unlink(missing_ok=True)

    command = [
        "qemu-system-x86_64", "-machine", "q35", "-accel", "tcg", "-smp", "4", "-m", "256M",
        "-cdrom", str(build / "AxiomOS.iso"), "-boot", "d",
        "-netdev", "user,id=net0,ipv6=off",
        "-device", "e1000,netdev=net0,mac=52:54:00:12:34:56",
        "-display", "none", "-serial", f"file:{serial}",
        "-monitor", f"unix:{monitor},server=on,wait=off", "-no-reboot", "-no-shutdown",
    ]

    with qemu_log.open("w") as output:
        process = subprocess.Popen(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)
        try:
            text = wait_for_text(
                process,
                serial,
                "AxiomOS shell ready. Type 'help' for commands.",
                time.monotonic() + 280,
            )
            boot_markers = [
                "AxiomOS Phase 21 advanced scheduler online.",
                "Phase 21 policy: priority + aging",
                "Phase 21 priority scheduling: OK",
                "Phase 21 starvation-prevention aging: OK",
                "Phase 21 CPU affinity groundwork: OK",
                "Phase 21 scheduler benchmark comparison: OK",
                "Phase 21 advanced scheduler initialization complete.",
            ]
            for marker in boot_markers:
                require(marker in text, f"phase21: boot marker missing: {marker!r}")

            send_command(monitor, "schedstats")
            wait_for_text(process, serial, "Scheduler policy: priority-aging", time.monotonic() + 20)

            send_command(monitor, "schedbench")
            final = wait_for_text(
                process,
                serial,
                "Phase 21 scheduler benchmark complete.",
                time.monotonic() + 60,
            )

            markers = [
                "Round-robin selections: 32/32/32",
                "Priority+aging selections: 3/7/86",
                "Live scheduler policy: priority-aging",
                "Phase 21 priority API: OK",
                "Phase 21 affinity API: OK",
                "Phase 21 benchmark comparison: OK",
            ]
            for marker in markers:
                require(marker in final, f"phase21: missing {marker!r}")

            phase21_lines = "\n".join(
                line for line in final.splitlines() if "Phase 21" in line
            )
            require("FAILED" not in phase21_lines, "phase21: scheduler reported failure")
            require("KERNEL PANIC" not in final, "phase21: kernel panicked")

            print("PASS: Phase 21 priority scheduler policy")
            print("PASS: Phase 21 starvation prevention through aging")
            print("PASS: Phase 21 per-task priority syscall API")
            print("PASS: Phase 21 BSP affinity-mask groundwork")
            print("PASS: Phase 21 round-robin vs priority-aging benchmark")
            print("PASS: Phase 21 scheduler statistics command")
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
    test_phase21()
    print("PASS: all Phase 21 advanced scheduler tests.")
