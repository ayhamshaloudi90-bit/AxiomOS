#!/usr/bin/env python3
"""Validate AxiomOS Phase-22 pipes, shared memory, and message queues."""
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
            tail = "\n".join(text.splitlines()[-260:])
            raise AssertionError(f"phase22 kernel panicked\n--- serial tail ---\n{tail}")
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-260:])
            raise AssertionError(
                f"phase22 timed out waiting for {marker!r}\n--- serial tail ---\n{tail}"
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
            require(character in mapping, f"phase22: no HMP key mapping for {character!r}")
            keys.append(mapping[character])
    keys.append("ret")
    send_hmp_keys(monitor_path, keys)


def test_phase22():
    build = ROOT / "build"
    build.mkdir(exist_ok=True)
    with (build / "phase22-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = build / "phase22-serial.log"
    monitor = build / "phase22-monitor.sock"
    qemu_log = build / "phase22-qemu.log"
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
                time.monotonic() + 300,
            )
            boot_markers = [
                "AxiomOS Phase 22 inter-process communication online.",
                "Phase 22 IPC mechanisms: pipes + shared memory + message queues",
                "Phase 22 pipe kernel self-test: OK",
                "Phase 22 message-queue kernel self-test: OK",
                "Phase 22 shared-memory mapper: READY",
                "Phase 22 IPC initialization complete.",
            ]
            for marker in boot_markers:
                require(marker in text, f"phase22: boot marker missing: {marker!r}")

            send_command(monitor, "ipcdemo")
            final = wait_for_text(
                process,
                serial,
                "Phase 22 IPC userspace demo complete.",
                time.monotonic() + 90,
            )
            markers = [
                "Phase 22 pipe process communication: OK",
                "Phase 22 peer cross-program IPC: OK",
                "Phase 22 shared memory communication: OK",
                "Phase 22 message queue communication: OK",
                "Phase 22 two-program IPC demonstration: OK",
                "Phase 22 IPC statistics: OK",
            ]
            for marker in markers:
                require(marker in final, f"phase22: missing {marker!r}")

            send_command(monitor, "ipcstats")
            final = wait_for_text(process, serial, "Messages sent/received:", time.monotonic() + 20)
            phase22_lines = "\n".join(line for line in final.splitlines() if "Phase 22" in line)
            require("FAILED" not in phase22_lines, "phase22: IPC reported failure")
            require("KERNEL PANIC" not in final, "phase22: kernel panicked")

            print("PASS: Phase 22 byte-stream pipe communication")
            print("PASS: Phase 22 page-backed shared memory")
            print("PASS: Phase 22 bounded message queues")
            print("PASS: Phase 22 two-program userspace IPC")
            print("PASS: Phase 22 IPC lifecycle + statistics")
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
    test_phase22()
    print("PASS: all Phase 22 IPC tests.")
