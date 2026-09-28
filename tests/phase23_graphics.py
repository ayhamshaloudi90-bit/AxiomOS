#!/usr/bin/env python3
"""Validate AxiomOS Phase-23 framebuffer graphics primitives and userspace API."""
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
            tail = "\n".join(text.splitlines()[-280:])
            raise AssertionError(f"phase23 kernel panicked\n--- serial tail ---\n{tail}")
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-280:])
            raise AssertionError(
                f"phase23 timed out waiting for {marker!r}\n--- serial tail ---\n{tail}"
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
            require(character in mapping, f"phase23: no HMP key mapping for {character!r}")
            keys.append(mapping[character])
    keys.append("ret")
    send_hmp_keys(monitor_path, keys)


def test_phase23():
    build = ROOT / "build"
    build.mkdir(exist_ok=True)
    with (build / "phase23-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = build / "phase23-serial.log"
    monitor = build / "phase23-monitor.sock"
    qemu_log = build / "phase23-qemu.log"
    serial.write_text("")
    monitor.unlink(missing_ok=True)

    command = [
        "qemu-system-x86_64", "-machine", "q35", "-accel", "tcg", "-smp", "4", "-m", "256M",
        "-cdrom", str(build / "AxiomOS.iso"), "-boot", "d",
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
                "AxiomOS Phase 23 graphical framebuffer online.",
                "Phase 23 primitives: pixel + line + rectangle + bitmap + text",
                "Phase 23 userspace graphics ABI: syscall-mediated framebuffer access",
                "Phase 23 framebuffer read/write self-test: OK",
                "Phase 23 clipping protection: OK",
                "Phase 23 graphics library initialization complete.",
            ]
            for marker in boot_markers:
                require(marker in text, f"phase23: boot marker missing: {marker!r}")

            send_command(monitor, "gfxinfo")
            text = wait_for_text(process, serial, "Graphics calls clear/line/rect/bitmap/text:", time.monotonic() + 20)
            require("Framebuffer:" in text and "bpp=32" in text, "phase23: gfxinfo mode missing")

            send_command(monitor, "gfxdemo")
            final = wait_for_text(
                process,
                serial,
                "Phase 23 framebuffer graphics demo complete.",
                time.monotonic() + 40,
            )
            markers = [
                "Phase 23 draw_pixel: OK",
                "Phase 23 draw_line: OK",
                "Phase 23 draw_rectangle: OK",
                "Phase 23 draw_bitmap: OK",
                "Phase 23 draw_text: OK",
                "Phase 23 userspace graphics library: OK",
            ]
            for marker in markers:
                require(marker in final, f"phase23: missing {marker!r}")

            send_command(monitor, "gfxinfo")
            final = wait_for_text(process, serial, "Pixels/clipped:", time.monotonic() + 20)
            phase23_lines = "\n".join(line for line in final.splitlines() if "Phase 23" in line)
            require("FAILED" not in phase23_lines, "phase23: graphics reported failure")
            require("KERNEL PANIC" not in final, "phase23: kernel panicked")

            print("PASS: Phase 23 framebuffer discovery + mode reporting")
            print("PASS: Phase 23 draw_pixel/draw_line/draw_rectangle")
            print("PASS: Phase 23 draw_bitmap/draw_text")
            print("PASS: Phase 23 Ring-3 graphics library + syscall boundary")
            print("PASS: Phase 23 clipping + graphics statistics")
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
    test_phase23()
    print("PASS: all Phase 23 graphical framebuffer tests.")
