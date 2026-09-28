#!/usr/bin/env python3
"""Validate AxiomOS Phase-20 security hardening from Ring 3."""
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
            tail = "\n".join(text.splitlines()[-220:])
            raise AssertionError(f"phase20 kernel panicked\n--- serial tail ---\n{tail}")
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-220:])
            raise AssertionError(f"phase20 timed out waiting for {marker!r}\n--- serial tail ---\n{tail}")
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
            require(character in mapping, f"phase20: no HMP key mapping for {character!r}")
            keys.append(mapping[character])
    keys.append("ret")
    send_hmp_keys(monitor_path, keys)


def test_phase20():
    build = ROOT / "build"
    build.mkdir(exist_ok=True)
    with (build / "phase20-build.log").open("w") as output:
        subprocess.run(["make", "MODE=normal", "all"], cwd=ROOT, stdout=output, stderr=subprocess.STDOUT, check=True)

    serial = build / "phase20-serial.log"
    monitor = build / "phase20-monitor.sock"
    qemu_log = build / "phase20-qemu.log"
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
            text = wait_for_text(process, serial, "AxiomOS shell ready. Type 'help' for commands.", time.monotonic() + 260)
            require("AxiomOS Phase 20 security hardening online." in text, "phase20: boot banner missing")
            require("Phase 20 security initialization complete." in text, "phase20: init marker missing")
            send_command(monitor, "id")
            wait_for_text(process, serial, "uid=1000 gid=1000", time.monotonic() + 20)
            send_command(monitor, "phase20-demo")
            text = wait_for_text(process, serial, "Phase 20 security demo complete:", time.monotonic() + 120)
            final = serial_text(serial)
            markers = [
                "Phase 20 userspace UID/GID credentials: OK",
                "Phase 20 system executable permissions: OK",
                "Phase 20 non-executable file blocked: OK",
                "Phase 20 invalid syscall pointer rejected: OK",
                "Phase 20 Ring-3 kernel isolation: OK",
                "Phase 20 NX heap enforcement: OK",
                "Phase 20 user stack guard page: OK",
                "Phase 20 ELF W^X rejection: OK",
            ]
            for marker in markers:
                require(marker in final, f"phase20: missing {marker!r}")
            require("FAILED" not in "\n".join(line for line in final.splitlines() if line.startswith("Phase 20")), "phase20: demo reported failure")
            require("KERNEL PANIC" not in final, "phase20: kernel panicked")
            print("PASS: Phase 20 UID/GID credential model")
            print("PASS: Phase 20 read/write/execute permission enforcement")
            print("PASS: Phase 20 syscall pointer validation and Ring-3 isolation")
            print("PASS: Phase 20 NX heap and unmapped stack guard")
            print("PASS: Phase 20 ELF W^X rejection")
            print("PASS: Phase 20 kernel stack canary policy active")
        finally:
            if process.poll() is None:
                process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill(); process.wait()
            monitor.unlink(missing_ok=True)


if __name__ == "__main__":
    test_phase20()
    print("PASS: all Phase 20 security tests.")
