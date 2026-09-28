#!/usr/bin/env python3
"""Validate Phase-16 synchronization primitives and deliberate race demo."""

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
        if "Phase 16 synchronization self-test: FAILED" in text:
            match = re.search(
                r"Phase 16 synchronization failure stage: (\d+)", text
            )
            stage = match.group(1) if match is not None else "unknown"
            tail = "\n".join(text.splitlines()[-160:])
            raise AssertionError(
                f"phase16 kernel self-test failed at stage {stage}; "
                f"see {serial}\n--- serial tail ---\n{tail}"
            )
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-160:])
            raise AssertionError(
                f"phase16 timed out waiting for {marker!r}; see {serial}\n"
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
                f"phase16 timed out waiting for prompt #{count}\n"
                f"--- serial tail ---\n{tail}"
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
        "-": "minus",
    }
    require(character in mapping, f"phase16: no HMP key mapping for {character!r}")
    return mapping[character]


def send_command(monitor_path, command):
    send_hmp_keys(
        monitor_path,
        [key_for_character(character) for character in command] + ["ret"],
    )


def test_phase16():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase16-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    disk = directory / "phase16-test-disk.img"
    with disk.open("wb") as handle:
        handle.truncate(16 * 1024 * 1024)

    serial = directory / "phase16-serial.log"
    qemu_log = directory / "phase16-qemu.log"
    monitor = directory / "phase16-monitor.sock"
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
        process = subprocess.Popen(
            command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT
        )
        try:
            text = wait_for_text(
                process,
                serial,
                "AxiomOS shell ready. Type 'help' for commands.",
                time.monotonic() + 170,
            )

            require("AxiomOS Phase 16 synchronization online." in text,
                    "phase16: synchronization banner missing")
            require("Phase 16 race condition demonstrated: OK" in text,
                    "phase16: deliberate race was not demonstrated")
            require("Phase 16 spinlock protected counter: OK" in text,
                    "phase16: spinlock counter test missing")
            require("Phase 16 mutex protected counter: OK" in text,
                    "phase16: mutex counter test missing")
            require("Phase 16 semaphore limit: OK" in text,
                    "phase16: semaphore limit test missing")
            require("Phase 16 wait queue wakeups: OK" in text,
                    "phase16: wait-queue test missing")
            require("Phase 16 synchronization self-test: OK" in text,
                    "phase16: self-test did not complete")
            require("KERNEL PANIC" not in text, "phase16: kernel panicked")

            race = re.search(
                r"Phase 16 race expected/observed: (\d+)/(\d+)", text
            )
            require(race is not None, "phase16: race counts missing")
            require(int(race.group(1)) == 128,
                    "phase16: unexpected race-test expected count")
            require(int(race.group(2)) < int(race.group(1)),
                    "phase16: deliberate lost update did not occur")

            spin = re.search(
                r"Phase 16 spinlock counter expected/actual: (\d+)/(\d+)", text
            )
            require(spin is not None and spin.group(1) == spin.group(2),
                    "phase16: spinlock did not preserve the counter")

            mutex = re.search(
                r"Phase 16 mutex blocks/wakeups: (\d+)/(\d+)", text
            )
            require(mutex is not None, "phase16: mutex contention stats missing")
            require(int(mutex.group(1)) > 0 and int(mutex.group(2)) > 0,
                    "phase16: mutex never blocked/woke a task")

            semaphore = re.search(
                r"Phase 16 semaphore limit/peak: (\d+)/(\d+)", text
            )
            require(semaphore is not None,
                    "phase16: semaphore limit/peak missing")
            require(semaphore.group(1) == "2" and semaphore.group(2) == "2",
                    "phase16: semaphore did not enforce a limit of two")

            before_prompts = text.count("axiom> ")
            send_command(monitor, "echo phase16-ok")
            text = wait_for_text(
                process, serial, "phase16-ok", time.monotonic() + 20
            )
            text = wait_for_prompt_count(
                process, serial, before_prompts + 1, time.monotonic() + 20
            )
            require("KERNEL PANIC" not in text,
                    "phase16: shell integration panicked after synchronization tests")

            print("PASS: Phase 16 deliberate lost-update race demonstrated")
            print("PASS: Phase 16 IRQ-save spinlock protected critical section")
            print("PASS: Phase 16 blocking mutex + FIFO wait queue")
            print("PASS: Phase 16 counting semaphore concurrency limit")
            print("PASS: Phase 16 scheduler BLOCKED/READY wakeup integration")
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
    test_phase16()
    print("PASS: all Phase 16 synchronization tests.")
