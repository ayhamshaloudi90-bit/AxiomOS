#!/usr/bin/env python3
"""Validate the Phase-14 interactive Ring-3 shell and command execution."""

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
            tail = "\n".join(text.splitlines()[-120:])
            raise AssertionError(
                f"phase14 timed out waiting for {marker!r}; see {serial}\n"
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
            tail = "\n".join(text.splitlines()[-100:])
            raise AssertionError(
                f"phase14 timed out waiting for prompt #{count}\n--- serial tail ---\n{tail}"
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
    require(character in mapping, f"phase14: no HMP key mapping for {character!r}")
    return mapping[character]


def send_command(monitor_path, command):
    keys = [key_for_character(character) for character in command]
    keys.append("ret")
    send_hmp_keys(monitor_path, keys)


def test_phase14():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase14-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    shell_elf = directory / "userspace" / "axiomsh.elf"
    require(shell_elf.exists(), "phase14: shell ELF was not built")

    disk = directory / "phase14-test-disk.img"
    with disk.open("wb") as handle:
        handle.truncate(16 * 1024 * 1024)

    serial = directory / "phase14-serial.log"
    qemu_log = directory / "phase14-qemu.log"
    monitor = directory / "phase14-monitor.sock"
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
                time.monotonic() + 140,
            )
            require("AxiomOS Phase 14 interactive shell online." in text,
                    "phase14: kernel did not launch shell")
            require("Shell executable: /bin/axiomsh" in text,
                    "phase14: wrong shell executable")
            require("Shell privilege: Ring 3" in text,
                    "phase14: shell is not reported as Ring 3")
            require("KERNEL PANIC" not in text, "phase14: kernel panicked before prompt")

            prompt = text.count("axiom> ")
            require(prompt >= 1, "phase14: initial prompt missing")

            cases = [
                ("help", "AxiomOS shell commands:"),
                ("pwd", "\n/\n"),
                ("ls /", "bin/"),
                ("cat /etc/motd", "Welcome to the AxiomOS virtual filesystem!"),
                ("write /tmp/shell.txt shell-data", None),
                ("cat /tmp/shell.txt", "shell-data"),
                ("mkdir /home/demo", None),
                ("ls /home", "demo/"),
                ("stat /etc/motd", "type: file"),
                ("cat /disk/persist.txt", "Persistent storage works across AxiomOS reboots!"),
                ("phase11-demo", "Hello from an ELF64 executable in AxiomOS!"),
                ("kbdstats", "Keyboard IRQs/scancodes/chars/dropped:"),
                ("echo phase14-ok", "phase14-ok"),
            ]

            for shell_command, marker in cases:
                before = serial_text(serial)
                prompt = before.count("axiom> ")
                send_command(monitor, shell_command)
                if marker is not None:
                    wait_for_text(process, serial, marker, time.monotonic() + 20)
                wait_for_prompt_count(process, serial, prompt + 1, time.monotonic() + 20)

            final = serial_text(serial)
            require("shell: command not found" not in final,
                    "phase14: a tested command was not recognized")
            require("Phase 14 shell task creation: FAILED" not in final,
                    "phase14: shell task creation failed")
            require("KERNEL PANIC" not in final, "phase14: kernel panicked during shell test")
            require(re.search(r"\[process \d+ exited 11\]", final) is not None,
                    "phase14: spawned ELF did not return exit status 11")

            stats = re.search(
                r"Keyboard IRQs/scancodes/chars/dropped: (\d+)/(\d+)/(\d+)/(\d+)",
                final,
            )
            require(stats is not None, "phase14: kbdstats output missing")
            require(int(stats.group(1)) > 0 and int(stats.group(3)) > 0,
                    "phase14: keyboard counters did not advance")
            require(int(stats.group(4)) == 0,
                    "phase14: keyboard input overflowed during shell test")

            print("PASS: Phase 14 Ring-3 interactive shell")
            print("PASS: Phase 14 command parser and built-ins")
            print("PASS: Phase 14 VFS file/directory commands")
            print("PASS: Phase 14 persistent /disk access")
            print("PASS: Phase 14 spawn/waitpid ELF execution")
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
    test_phase14()
    print("PASS: all Phase 14 shell tests.")
