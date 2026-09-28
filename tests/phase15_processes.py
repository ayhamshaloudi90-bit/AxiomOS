#!/usr/bin/env python3
"""Validate Phase-15 fork/wait/process-table/kill lifecycle semantics."""

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


def wait_for_text(process, serial, marker, deadline):
    while True:
        text = serial_text(serial)
        if marker in text:
            return text
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-140:])
            raise AssertionError(
                f"phase15 timed out waiting for {marker!r}; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )
        time.sleep(0.05)


def wait_for_prompt_count(process, serial, count, deadline):
    while True:
        text = serial_text(serial)
        if text.count("axiom> ") >= count:
            return text
        require(process.poll() is None, "QEMU exited while waiting for shell prompt")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-120:])
            raise AssertionError(
                f"phase15 timed out waiting for prompt #{count}\n--- serial tail ---\n{tail}"
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


def key_for_character(character):
    if "a" <= character <= "z" or "0" <= character <= "9":
        return character
    mapping = {
        " ": "spc",
        "/": "slash",
        ".": "dot",
        "-": "minus",
        "_": "shift-minus",
    }
    require(character in mapping, f"phase15: no HMP key mapping for {character!r}")
    return mapping[character]


def send_command(monitor_path, command):
    send_hmp_keys(
        monitor_path,
        [key_for_character(character) for character in command] + ["ret"],
    )


def run_command(process, serial, monitor, command, marker=None, timeout=25):
    before = serial_text(serial)
    prompt_count = before.count("axiom> ")
    send_command(monitor, command)
    if marker is not None:
        wait_for_text(process, serial, marker, time.monotonic() + timeout)
    return wait_for_prompt_count(
        process, serial, prompt_count + 1, time.monotonic() + timeout
    )


def test_phase15():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase15-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    for filename in ("phase15_demo.elf", "phase15_sleeper.elf"):
        require((directory / "userspace" / filename).exists(),
                f"phase15: {filename} was not built")

    disk = directory / "phase15-test-disk.img"
    with disk.open("wb") as handle:
        handle.truncate(16 * 1024 * 1024)

    serial = directory / "phase15-serial.log"
    qemu_log = directory / "phase15-qemu.log"
    monitor = directory / "phase15-monitor.sock"
    serial.write_text("")
    monitor.unlink(missing_ok=True)

    command = [
        "qemu-system-x86_64",
        "-machine", "q35",
        "-accel", "tcg",
        "-m", "256M",
        "-cdrom", str(directory / "AxiomOS.iso"),
        "-boot", "d",
        "-drive", f"file={disk},format=raw,if=none,id=axiomdisk",
        "-device", "ide-hd,drive=axiomdisk,bus=ide.0",
        "-display", "none",
        "-serial", f"file:{serial}",
        "-monitor", f"unix:{monitor},server=on,wait=off",
        "-no-reboot",
        "-no-shutdown",
    ]

    with qemu_log.open("w") as output:
        process = subprocess.Popen(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)
        try:
            text = wait_for_text(
                process,
                serial,
                "AxiomOS shell ready. Type 'help' for commands.",
                time.monotonic() + 150,
            )
            require("AxiomOS Phase 15 process management online." in text,
                    "phase15: process-management banner missing")
            require("fork policy: eager private-page copy" in text,
                    "phase15: fork policy banner missing")
            require("KERNEL PANIC" not in text, "phase15: panic before shell")

            text = run_command(
                process, serial, monitor, "phase15-demo",
                "Phase 15 process management demo complete.", 35,
            )
            for marker in (
                "Phase 15 fork/exec/wait: OK",
                "Phase 15 blocked wait/sleep: OK",
                "Phase 15 fork memory isolation: OK",
            ):
                require(marker in text, f"phase15: missing {marker!r}")
            require(re.search(r"\[process \d+ exited 15\]", text) is not None,
                    "phase15: demo process exit status was not collected")

            text = run_command(process, serial, monitor, "ps", "PID  PPID  STATE")
            require("axiomsh" in text, "phase15: ps does not show the shell")
            require("RUNNING" in text or "READY" in text,
                    "phase15: ps does not expose scheduler state")

            text = run_command(
                process, serial, monitor, "spawn phase15-sleeper",
                "Phase 15 sleeper started; waiting for SIGTERM.", 25,
            )
            matches = re.findall(r"\[started pid (\d+)\]", text)
            require(matches, "phase15: background spawn did not report a pid")
            sleeper_pid = int(matches[-1])

            text = run_command(process, serial, monitor, "ps", "PID  PPID  STATE")
            require(re.search(rf"\b{sleeper_pid}\b.*SLEEPING.*spawned-elf", text) is not None,
                    "phase15: sleeping background child not visible in ps")

            run_command(
                process, serial, monitor, f"kill {sleeper_pid}",
                f"sent SIGTERM to {sleeper_pid}", 20,
            )
            text = run_command(
                process, serial, monitor, f"wait {sleeper_pid}",
                f"[process {sleeper_pid} exited 143]", 20,
            )
            require("error" not in text.split(f"wait {sleeper_pid}")[-1],
                    "phase15: wait after SIGTERM returned an error")

            # Reaping the killed child must make its slot reusable.
            text = run_command(
                process, serial, monitor, "phase11-demo",
                "Hello from an ELF64 executable in AxiomOS!", 25,
            )
            require(re.search(r"\[process \d+ exited 11\]", text) is not None,
                    "phase15: task slot was not reusable after wait/reap")

            final = serial_text(serial)
            require("KERNEL PANIC" not in final, "phase15: kernel panicked")
            require("Phase 15 fork: FAILED" not in final, "phase15: fork failed")
            require("Phase 15 fork/exec/wait: FAILED" not in final,
                    "phase15: fork/exec/wait failed")
            require("Phase 15 fork memory isolation: FAILED" not in final,
                    "phase15: fork memory was shared unexpectedly")

            print("PASS: Phase 15 eager-copy fork semantics")
            print("PASS: Phase 15 exec + blocking waitpid + zombie reap")
            print("PASS: Phase 15 process table / ps")
            print("PASS: Phase 15 simplified SIGTERM kill")
            print("PASS: Phase 15 scheduler blocking/sleep wakeups")
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
    test_phase15()
    print("PASS: all Phase 15 process-management tests.")
