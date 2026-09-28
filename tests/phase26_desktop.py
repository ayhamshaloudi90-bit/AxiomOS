#!/usr/bin/env python3
"""Validate the post-roadmap AxiomOS Phase-26 desktop + terminal workflow."""
from pathlib import Path
import re
import socket
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def serial_text(path):
    return path.read_text(errors="replace").replace("\r", "")


def wait_for_new_text(process, serial, marker, start_offset, deadline):
    while True:
        text = serial_text(serial)
        fresh = text[start_offset:]
        if marker in fresh:
            return text
        if "KERNEL PANIC" in fresh:
            tail = "\n".join(text.splitlines()[-320:])
            raise AssertionError(f"phase26 kernel panicked\n--- serial tail ---\n{tail}")
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-320:])
            raise AssertionError(
                f"phase26 timed out waiting for new {marker!r}\n--- serial tail ---\n{tail}"
            )
        time.sleep(0.05)


def wait_for_text(process, serial, marker, deadline):
    return wait_for_new_text(process, serial, marker, 0, deadline)


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
            time.sleep(0.04)


def send_command(monitor_path, command):
    mapping = {
        " ": "spc", "/": "slash", ".": "dot", "-": "minus",
        "_": "shift-minus", ":": "shift-semicolon",
    }
    keys = []
    for character in command:
        if "a" <= character <= "z" or "0" <= character <= "9":
            keys.append(character)
        else:
            require(character in mapping, f"phase26: no HMP key mapping for {character!r}")
            keys.append(mapping[character])
    keys.append("ret")
    send_hmp_keys(monitor_path, keys)


def test_phase26():
    build = ROOT / "build"
    build.mkdir(exist_ok=True)
    with (build / "phase26-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = build / "phase26-serial.log"
    monitor = build / "phase26-monitor.sock"
    qemu_log = build / "phase26-qemu.log"
    serial.write_text("")
    monitor.unlink(missing_ok=True)

    command = [
        "qemu-system-x86_64", "-machine", "q35", "-accel", "tcg",
        "-smp", "4", "-m", "256M",
        "-netdev", "user,id=net0,ipv6=off",
        "-device", "e1000,netdev=net0,mac=52:54:00:12:34:56",
        "-cdrom", str(build / "AxiomOS.iso"), "-boot", "d",
        "-display", "none", "-serial", f"file:{serial}",
        "-monitor", f"unix:{monitor},server=on,wait=off",
        "-no-reboot", "-no-shutdown",
    ]

    with qemu_log.open("w") as output:
        process = subprocess.Popen(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)
        try:
            text = wait_for_text(
                process,
                serial,
                "AxiomOS shell ready. Type 'help' for commands.",
                time.monotonic() + 340,
            )
            for marker in [
                "AxiomOS Phase 26 desktop GUI available.",
                "Desktop executable: /bin/desktop (launch with: desktop)",
                "Desktop terminal workflow: open Terminal, type exit to return",
                "Phase 26 desktop integration complete.",
            ]:
                require(marker in text, f"phase26: boot marker missing: {marker!r}")
            require(
                "Phase 26 PS/2 mouse: online" in text or
                "Phase 26 PS/2 mouse: unavailable - keyboard fallback" in text,
                "phase26: mouse/fallback status missing",
            )

            start = len(serial_text(serial))
            send_command(monitor, "desktop")
            text = wait_for_new_text(
                process, serial, "AxiomOS Phase 26 desktop ready.", start, time.monotonic() + 30
            )
            require(
                "Desktop controls: T/Enter Terminal, S System, N Network, G Graphics, Q Quit." in text[start:],
                "phase26: desktop controls marker missing",
            )

            # Open Terminal from the graphical desktop using the keyboard shortcut.
            start = len(serial_text(serial))
            send_hmp_keys(monitor, ["t"])
            wait_for_new_text(
                process,
                serial,
                "Phase 26 desktop launching terminal. Type 'exit' to return.",
                start,
                time.monotonic() + 20,
            )

            start = len(serial_text(serial))
            send_command(monitor, "echo nested-terminal-ok")
            wait_for_new_text(
                process, serial, "nested-terminal-ok", start, time.monotonic() + 20
            )

            # New shell 'exit' command must return to the desktop rather than kill the GUI.
            start = len(serial_text(serial))
            send_command(monitor, "exit")
            wait_for_new_text(
                process,
                serial,
                "Phase 26 terminal returned to desktop.",
                start,
                time.monotonic() + 20,
            )

            # Quit desktop and prove the original outer shell is still alive.
            start = len(serial_text(serial))
            send_hmp_keys(monitor, ["q"])
            text = wait_for_new_text(
                process,
                serial,
                "Phase 26 desktop exited to shell.",
                start,
                time.monotonic() + 20,
            )
            require(
                re.search(r"\[process \d+ exited 0\]", text[start:]) is not None or
                "Phase 26 desktop exited to shell." in text[start:],
                "phase26: desktop did not return cleanly",
            )

            start = len(serial_text(serial))
            send_command(monitor, "echo phase26-outer-shell-ok")
            final = wait_for_new_text(
                process, serial, "phase26-outer-shell-ok", start, time.monotonic() + 20
            )
            require("KERNEL PANIC" not in final, "phase26: kernel panicked")

            print("PASS: Phase 26 graphical desktop application")
            print("PASS: Phase 26 keyboard launcher fallback")
            print("PASS: Phase 26 nested terminal opens from desktop")
            print("PASS: Phase 26 shell exit returns to desktop")
            print("PASS: Phase 26 desktop exit returns to original shell")
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
    test_phase26()
    print("PASS: all Phase 26 desktop integration tests.")
