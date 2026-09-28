#!/usr/bin/env python3
"""Validate AxiomOS Phase-25 procfs observability and sysinfo tooling."""
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
            tail = "\n".join(text.splitlines()[-300:])
            raise AssertionError(f"phase25 kernel panicked\n--- serial tail ---\n{tail}")
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-300:])
            raise AssertionError(
                f"phase25 timed out waiting for {marker!r}\n--- serial tail ---\n{tail}"
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
    mapping = {
        " ": "spc",
        "/": "slash",
        ".": "dot",
        "-": "minus",
        "_": "shift-minus",
    }
    keys = []
    for character in command:
        if "a" <= character <= "z" or "0" <= character <= "9":
            keys.append(character)
        else:
            require(character in mapping, f"phase25: no HMP key mapping for {character!r}")
            keys.append(mapping[character])
    keys.append("ret")
    send_hmp_keys(monitor_path, keys)


def test_phase25():
    build = ROOT / "build"
    build.mkdir(exist_ok=True)
    with (build / "phase25-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = build / "phase25-serial.log"
    monitor = build / "phase25-monitor.sock"
    qemu_log = build / "phase25-qemu.log"
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
                time.monotonic() + 320,
            )
            for marker in [
                "AxiomOS Phase 25 developer tooling online.",
                "Phase 25 /proc files: meminfo processes interrupts files net scheduler cpuinfo pagemap",
                "Phase 25 userspace tool: /bin/sysinfo",
                "Phase 25 procfs read-only self-test: OK",
                "Phase 25 developer tooling initialization complete.",
                "AxiomOS original Phase 0-25 roadmap complete.",
            ]:
                require(marker in text, f"phase25: boot marker missing: {marker!r}")

            send_command(monitor, "ls /proc")
            text = wait_for_text(process, serial, "pagemap", time.monotonic() + 20)
            for name in [
                "meminfo", "processes", "interrupts", "files",
                "net", "scheduler", "cpuinfo", "pagemap",
            ]:
                require(name in text, f"phase25: /proc missing {name}")

            send_command(monitor, "sysinfo")
            text = wait_for_text(
                process,
                serial,
                "Phase 25 developer tooling demo complete.",
                time.monotonic() + 50,
            )
            for marker in [
                "===== /proc/cpuinfo =====",
                "CPUsOnline:",
                "===== /proc/meminfo =====",
                "PhysicalFreePages:",
                "===== /proc/processes =====",
                "axiomsh",
                "===== /proc/scheduler =====",
                "ContextSwitches:",
                "===== /proc/interrupts =====",
                "TimerVector32:",
                "===== /proc/files =====",
                "===== /proc/net =====",
                "IPv4: 10.0.2.15",
                "===== /proc/pagemap =====",
                "CR3: 0x",
                "Phase 25 /proc observability: OK",
            ]:
                require(marker in text, f"phase25: sysinfo marker missing: {marker!r}")

            send_command(monitor, "write /proc/meminfo nope")
            final = wait_for_text(process, serial, "write: error -13", time.monotonic() + 20)
            require("KERNEL PANIC" not in final, "phase25: kernel panicked")

            print("PASS: Phase 25 read-only /proc pseudo-filesystem")
            print("PASS: Phase 25 memory/process/page-map observability")
            print("PASS: Phase 25 interrupt/open-file/network statistics")
            print("PASS: Phase 25 scheduler/CPU diagnostics")
            print("PASS: Phase 25 /bin/sysinfo Ring-3 developer dashboard")
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
    test_phase25()
    print("PASS: all Phase 25 developer-tooling tests.")
