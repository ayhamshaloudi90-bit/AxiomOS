#!/usr/bin/env python3
"""Validate Phase-19 libaxiom, anonymous mmap, and libc-linked Ring-3 programs."""
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
            tail = "\n".join(text.splitlines()[-180:])
            raise AssertionError(f"phase19 kernel panicked\n--- serial tail ---\n{tail}")
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-180:])
            raise AssertionError(f"phase19 timed out waiting for {marker!r}\n--- serial tail ---\n{tail}")
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
            require(character in mapping, f"phase19: no HMP key mapping for {character!r}")
            keys.append(mapping[character])
    keys.append("ret")
    send_hmp_keys(monitor_path, keys)


def test_phase19():
    build = ROOT / "build"
    build.mkdir(exist_ok=True)
    with (build / "phase19-build.log").open("w") as output:
        subprocess.run(["make", "MODE=normal", "all"], cwd=ROOT, stdout=output, stderr=subprocess.STDOUT, check=True)

    serial = build / "phase19-serial.log"
    monitor = build / "phase19-monitor.sock"
    qemu_log = build / "phase19-qemu.log"
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
            text = wait_for_text(process, serial, "AxiomOS shell ready. Type 'help' for commands.", time.monotonic() + 240)
            require("AxiomOS Phase 19 userspace libc available." in text, "phase19: boot banner missing")
            require("Phase 19 userspace library initialization complete." in text, "phase19: libc init marker missing")
            send_command(monitor, "phase19-demo")
            text = wait_for_text(process, serial, "Phase 19 libc demo complete:", time.monotonic() + 90)
            text = wait_for_text(process, serial, "[process ", time.monotonic() + 20)
            final = serial_text(serial)
            for marker in [
                "Phase 19 stdio formatting: OK",
                "Phase 19 string/memory routines: OK",
                "Phase 19 ctype/numeric conversion: OK",
                "Phase 19 malloc/calloc/realloc/free: OK",
                "Phase 19 malloc-backed fork isolation: OK",
                "Phase 19 mmap-backed exec cleanup: OK",
                "Phase 19 unistd/file wrappers: OK",
                "Phase 19 process wrappers: OK",
                "Phase 19 network syscall wrapper: OK",
            ]:
                require(marker in final, f"phase19: missing {marker!r}")
            require("exited 19]" in final, "phase19: demo did not exit with status 19")
            require("FAILED" not in "\n".join(line for line in final.splitlines() if line.startswith("Phase 19")), "phase19: demo reported failure")
            require("KERNEL PANIC" not in final, "phase19: kernel panicked")
            print("PASS: Phase 19 libaxiom stdio/string/ctype/stdlib")
            print("PASS: Phase 19 anonymous mmap-backed malloc/calloc/realloc/free")
            print("PASS: Phase 19 malloc pages preserve fork isolation")
            print("PASS: Phase 19 exec discards inherited anonymous mappings safely")
            print("PASS: Phase 19 unistd/file/process/network syscall wrappers")
            print("PASS: Phase 19 existing shell remains interactive with libc linkage")
        finally:
            if process.poll() is None:
                process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill(); process.wait()
            monitor.unlink(missing_ok=True)


if __name__ == "__main__":
    test_phase19()
    print("PASS: all Phase 19 userspace libc tests.")
